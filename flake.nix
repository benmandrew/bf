{
  description = "bf devShell";

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
      in {
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
