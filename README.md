# TDVRPTW-solver

TDVRPTW-solver is a research project developed by Florian Rascoussier during his PhD on vehicle routing and optimization. It extends the solver of Gonzalo Lera-Romero, Juan José Miranda-Bront and Francisco Soulignac for the Time-Dependent Vehicle Routing Problem with Time Windows (TDVRPTW). The original implementation accompanies their article, [*Linear edge costs and labeling algorithms: The case of the time-dependent vehicle routing problem with time windows*](https://doi.org/10.1002/net.21937), published in *Networks* in 2020, and is available in [gleraromero/networks2020](https://github.com/gleraromero/networks2020).

This fork served as direct groundwork for the development of [KAYROS](https://github.com/0nyr/kayros). The PhD work in this repository includes corrections to the piecewise-linear function machinery, C++/Python bindings, build and experiment-runner refactoring, and heuristic development. These investigations provided implementation experience and numerical validation work used in developing KAYROS, which is maintained in its own repository.

The current code includes these subsequent extensions and should be distinguished from the version used for the original article. For the original implementation and its reproduction instructions, consult the upstream repository. For the software developed here, use this repository's revision history and the citation metadata below.

## Citation

If you use this fork, please cite the software using [CITATION.cff](CITATION.cff), which lists the 3 original authors followed by Florian Rascoussier as the author of the PhD extensions. Please also cite the original article when discussing or using its algorithms:

Gonzalo Lera-Romero, Juan J. Miranda-Bront and Francisco J. Soulignac (2020). *Linear edge costs and labeling algorithms: The case of the time-dependent vehicle routing problem with time windows*. Networks, 76(1), 24–53. [doi:10.1002/net.21937](https://doi.org/10.1002/net.21937).

## Commands

- `python runner/src/runner.py experiments/bp_test.json`: run tests on small instances.
- `python runner/src/runner.py --just-compile --clean-build`: perform a clean full build.
- `python runner/src/runner.py experiments/bp_all.json --carry-on out/csv/2025-03-22-00-20-24-bp_all.csv --dry-run`: inspect the remaining runs before resuming an experiment series. Replace the CSV path with your previous run.
- `python runner/src/runner.py experiments/bp_ng_EXPS-835ae8.json > out/tmp/out23.txt`: run an experiment and redirect standard output to a file. Create `out/tmp/` first if needed.

## Original article abstract

The following abstract describes the original article by Lera-Romero et al. (2020).

In this paper we implement a branch‐price and cut algorithm for a time dependent vehicle routing problem with time windows in which the goal is to minimize the total route duration. The travel time between two customers is given by a piecewise linear function on the departure time and, thus, it need not remain fixed along the planning horizon. We discuss different alternatives for the implementation of these linear functions within the labeling algorithm applied to solve the pricing problem. We also provide a tailored implementation for one of these alternatives, relying on efficient data structures for storing the labels, and show several strategies to accelerate the algorithm. Computational results show that the proposed techniques are effective and improve the column generation step, solving all instances with 25 customers, 49 of 56 with 50 customers, and many instances with 100 customers. Furthermore, heuristic adaptations are able to find good quality solutions in reasonable computation times.

## Getting started
The repository retains the original experiment workflow alongside the PhD extensions. The commands above use the current runner layout. A Nix development shell is provided in `flake.nix`, with local CPLEX paths configured there.

### Prerequisites

- Python 3.13 (the version provided by the Nix shell) [(more info)](https://www.python.org/)
- CPLEX >= 12.8 [(more info)](https://www.ibm.com/products/ilog-cplex-optimization-studio)
- Boost Graph Library >=1.66 [(more info)](https://www.boost.org/doc/libs/1_66_0/libs/graph/doc/index.html)
    - On Linux: ```sudo apt-get install libboost-all-dev```
- CMake >= 3.10 [(more info)](https://cmake.org/)
    - On Linux: ```sudo apt-get install cmake```
- A compiler supporting the C++26 mode requested by [code/CMakeLists.txt](code/CMakeLists.txt)

### Built with

- Kaleidoscope: A tool to visualize the outputs of Optimization Problems [(more info)](https://github.com/gleraromero/kaleidoscope)
- Runner: A script to ease the process of running experiments [(more info)](https://github.com/gleraromero/runner)
- GOC lib: A library that includes interfaces for using (Mixed Integer) Linear Programming solvers, and some useful resources [(more info)](https://github.com/gleraromero/goc).

### Running the experiments.

1. Add environment variables with the paths to the libraries.
    1. Add two environment variables to bash with CPLEX include and library paths.
        1. ```export CPLEX_INCLUDE=<path_to_cplex_include_dir>```
            - Usually on Linux: _/opt/ibm/ILOG/CPLEX_Studio\<VERSION\>/cplex/include_
        1. ```export CPLEX_BIN=<path_to_cplex_lib_binary_file>```
            - Usually on Linux: _/opt/ibm/ILOG/CPLEX_Studio\<VERSION\>/cplex/lib/x86-64_linux/static_pic/libcplex.a_
    1. Add two environment variables to bash with BOOST Graph Library include and library paths.
        1. ```export BOOST_INCLUDE=<path_to_boost_include_dir>```
            - Usually on Linux: _/usr/include_
        1. ```export BOOST_BIN=<path_to_boost_lib_binary_file>```
            - Usually on Linux: _/usr/lib/x86_64-linux-gnu/libboost_graph.a_
2. Go to the repository root directory.
3. Execute ```python3 runner/src/runner.py <experiment_file>```, example: `python runner/src/runner.py experiments/bp.json`
4. The execution output will be continually saved to the output folder.

> Experiment files are located in the _experiments_ folder. For more information see Section [Experiments](#Experiments)

### Experiments

The original article organizes its experiments as follows. This fork also includes additional experiment configurations in `experiments/`.

* _Section 7.1_: pricing.json
* _Section 7.2_: bp.json
* _Section 7.4_: bp_heur.json

### Visualizing the experiment results.

1. Go to https://gleraromero.github.io/kaleidoscope/networks2020
1. Add the output file.
1. Select the experiments.
1. Add some attributes to visualize.
1. Click on Refresh.
1. If more details on an experiment are desired click on the + icon in a specific row.

### Checker
We include a checker program to validate that algorithms produce **valid** routes. To run the checker execute:
```python3 checker/checker.py output/<output_file.json>```

The checker will go through each instance and validate:

- That the exact solution route is feasible (with respect to all resources).
- That the reported duration of the route is correct.
- If Optimum status is reported, then it should be better or equal than any solution in the _solutions.json_ file of its dataset.

## Built With

* [JSON for Modern C++](https://github.com/nlohmann/json)
* [Boost Graph Library](https://www.boost.org/doc/libs/1_66_0/libs/graph/doc/index.html)

## Authors

- Gonzalo Lera-Romero: original solver and article.
- Juan José Miranda-Bront: original solver and article.
- Francisco Soulignac: original solver and article.
- Florian Rascoussier: PhD research extensions and groundwork for KAYROS.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
