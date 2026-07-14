{
  description = "bf devShell and backend container image";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_22;
        # cpplint 2.0.2's own test suite fails to build in this nixpkgs
        # rev; its self-tests are irrelevant to us, so skip them. It is a
        # Python app, so the check phase is gated by overridePythonAttrs
        # (overrideAttrs does not reach it).
        cpplint = pkgs.cpplint.overridePythonAttrs (_: { doCheck = false; });

        # bfc, built via nix so the container shares one LLVM with the dev
        # shell. The source is filtered to what the build reads (mirroring
        # the old Docker build context) to avoid rebuilds on doc/test churn.
        bfc = llvm.stdenv.mkDerivation {
          pname = "bfc";
          version = "1.0.0";
          src = pkgs.lib.fileset.toSource {
            root = ./.;
            fileset = pkgs.lib.fileset.unions [
              ./CMakeLists.txt
              ./cmake
              ./src
            ];
          };
          nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
          buildInputs = [ llvm.llvm ];
          # DOCKER_BUILD=ON drops the test/docs subdirectories, which are
          # not in the filtered source.
          cmakeFlags = [ "-DDOCKER_BUILD=ON" ];
          ninjaFlags = [ "bfc" ];
          installPhase = ''
            runHook preInstall
            install -Dm755 bfc "$out/bin/bfc"
            runHook postInstall
          '';
        };

        # The web server: one stdlib-only Go file, so a bare offline
        # `go build` with no module resolution suffices.
        bfServer = pkgs.stdenv.mkDerivation {
          pname = "bf-server";
          version = "1.0.0";
          src = pkgs.lib.fileset.toSource {
            root = ./web;
            fileset = ./web/server.go;
          };
          nativeBuildInputs = [ pkgs.go ];
          buildPhase = ''
            runHook preBuild
            export HOME="$TMPDIR" GOCACHE="$TMPDIR/go-cache"
            export GOPROXY=off GO111MODULE=off CGO_ENABLED=0
            go build -o server server.go
            runHook postBuild
          '';
          installPhase = ''
            runHook preInstall
            install -Dm755 server "$out/bin/server"
            runHook postInstall
          '';
        };

        highlightPy = pkgs.writeText "highlight.py"
          (builtins.readFile ./scripts/highlight.py);

        # Slimmer runtimes for the image than the dev shell's: highlight.py
        # is stdlib-only so python3Minimal suffices, and the graph is only
        # ever emitted as SVG, so graphviz-nox (no X11) is enough.
        runtimePython = pkgs.python3Minimal;
        runtimeGraphviz = pkgs.graphviz-nox;

        # The bfc backend image, assembled from the same pinned Graphviz and
        # bfc the dev shell builds against, so the CFG renders identically in
        # production and in `nix develop`. Debian's Graphviz (2.42) cannot
        # parse the instruction-level HTML labels the renderer emits.
        bfcImage = pkgs.dockerTools.buildLayeredImage {
          name = "benmandrew/bf";
          tag = "bfc";
          contents = [ bfc bfServer runtimeGraphviz runtimePython ];
          config = {
            Entrypoint = [
              "${bfServer}/bin/server"
              "${bfc}/bin/bfc"
              "${highlightPy}"
            ];
            Env = [
              "PATH=${pkgs.lib.makeBinPath [ runtimeGraphviz runtimePython ]}"
            ];
            ExposedPorts = { "8000/tcp" = { }; };
          };
        };
      in {
        packages = {
          inherit bfc bfcImage;
          server = bfServer;
        };

        # Build with LLVM 22's wrapped clang stdenv, not clang-unwrapped.
        # bfc/bfi pick up their sanitizer runtime from whatever compiler
        # links them, and LLVM 21's ASan hangs at startup on macOS 26
        # (spinning in get_dyld_hdr during shadow-memory init) whereas 22's
        # runs. The wrapper is also what supplies the libc++ include and
        # runtime paths that cfg_dot.cpp, the one C++ TU, needs to compile
        # and link. clang-tools still provides clang-format/tidy.
        devShells.default = (pkgs.mkShell.override { stdenv = llvm.stdenv; }) {
          packages = [
            pkgs.cmake
            pkgs.pkg-config
            llvm.llvm
            llvm.clang-tools
            pkgs.check
            pkgs.expect
            cpplint
            pkgs.doxygen
            pkgs.graphviz
            pkgs.python3
            pkgs.ruff
            pkgs.shfmt
            pkgs.shellcheck
          ];
        };
      });
}
