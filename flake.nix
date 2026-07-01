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
        llvm = pkgs.llvmPackages;
      in {
        devShells.default = pkgs.mkShell {
          packages = [
            pkgs.cmake
            pkgs.pkg-config
            llvm.llvm
            llvm.clang-unwrapped
            llvm.clang-tools
            pkgs.check
            pkgs.expect
            pkgs.cpplint
            pkgs.doxygen
            pkgs.graphviz
            pkgs.python3
            pkgs.shfmt
            pkgs.shellcheck
          ];
        };
      });
}
