{
  description = "Julia environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = github:numtide/flake-utils;
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        # Override the Nix package set to allow unfree packages
        pkgs = import nixpkgs {
          system = system; 
          config.allowUnfree = true; 
        };

        pythonPackages = pkgs.python313Packages;

        # Boost paths, see: https://stackoverflow.com/questions/43425262/nix-boost-install-misses-headers
        boostDev = pkgs.boost.dev;  # Headers
        boostOut = pkgs.boost.out;  # Libraries
      in
      {
        # development environment
        devShells.default = pkgs.mkShell {
          packages = [
            # Python
            pythonPackages.python
            pythonPackages.numpy
            pythonPackages.tqdm
            
            # C/C++ development
            pkgs.gcc
            pkgs.gnumake
            pkgs.gdb
            pkgs.valgrind
            pkgs.cmake
            pkgs.boost
          ];

          shellHook = ''
            # CPLEX environment variables
            export CPLEX_HOME=~/cplex2210/CPLEX_Studio221
            export CPLEX_INCLUDE=~/cplex2210/CPLEX_Studio221/cplex/include/
            export CPLEX_BIN=~/cplex2210/CPLEX_Studio221/cplex/lib/x86-64_linux/static_pic/libcplex.a

            # BOOST environment variables
            export BOOST_INCLUDE=${boostDev}/include
            export BOOST_BIN=${boostOut}/lib

            echo "BOOST_INCLUDE: $BOOST_INCLUDE"
            echo "BOOST_BIN: $BOOST_BIN"
            echo "Nix shell loaded."
          '';
        };
      }
    );
}