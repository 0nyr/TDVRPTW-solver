{
  description = "Nix environment";

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
            pythonPackages.pandas
            pythonPackages.tabulate
            pythonPackages.jinja2
            pythonPackages.seaborn
            pythonPackages.matplotlib
            
            # C/C++ development
            pkgs.gcc
            pkgs.gnumake
            pkgs.gdb
            pkgs.valgrind
            pkgs.cmake
            pkgs.boost

            # pybind11 for C++/Python bindings
            pythonPackages.pybind11
          ];

          shellHook = ''
            # CPLEX environment variables
            export CPLEX_HOME=~/cplex2210/CPLEX_Studio221
            export CPLEX_INCLUDE=~/cplex2210/CPLEX_Studio221/cplex/include/
            export CPLEX_BIN=~/cplex2210/CPLEX_Studio221/cplex/lib/x86-64_linux/static_pic/libcplex.a

            # BOOST environment variables
            export BOOST_INCLUDE=${boostDev}/include
            export BOOST_BIN=${boostOut}/lib

            unset NIX_ENFORCE_NO_NATIVE
            echo "WARNING: This shell is for development purposes only. Disable NIX_ENFORCE_NO_NATIVE to use native code generation."

            echo "BOOST_INCLUDE: $BOOST_INCLUDE"
            echo "BOOST_BIN: $BOOST_BIN"
            echo "pybind11 include: ${pythonPackages.pybind11}/include"
            echo "Nix shell loaded."
          '';
        };
      }
    );
}