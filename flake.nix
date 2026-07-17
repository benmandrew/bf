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

      in {
        # The web demo compiles client-side: bfc is built to WebAssembly (see
        # scripts/build-wasm.sh) and runs in the browser. There is no backend
        # and no server image -- the `site` CMake target stages a static bundle
        # (build/site) that any static host can serve. See web/README.md.
        packages = {
          inherit bfc;
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
            pkgs.ninja
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
            # Emscripten for the client-side wasm build (scripts/build-wasm*.sh):
            # emcc/emcmake, replacing a separately-installed ~/emsdk. ninja above
            # drives the LLVM wasm cross-build.
            pkgs.emscripten
          ];

          # emscripten's cache lives in the read-only Nix store, but emcc writes
          # the wasm sysroot libraries (libc, libc++, ...) into it on first link.
          # Point EM_CACHE at a writable, gitignored dir (under build-wasm/, which
          # already holds the wasm scratch) so the build works and the generated
          # libs persist across shells -- the ~25s populate happens only once.
          shellHook = ''
            export EM_CACHE="''${EM_CACHE:-$PWD/build-wasm/emcache}"
          '';
        };
      });
}
