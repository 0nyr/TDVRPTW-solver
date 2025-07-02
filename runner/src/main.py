# --- Command recap:
# Compile: python runner/src/runner.py --just-compile -b debug
# Run: python runner/src/main.py -b debug &> logs/test.log

import os, sys

from params.args import parse_program_args
args = parse_program_args()

from utils.terminal import purple, green
from params.constants import PROJECT_ROOT_DIR, RUNNER_START_TIME
from utils.formatting import format_date_for_console
print(purple(f"Starting program [datetime: {format_date_for_console(RUNNER_START_TIME)}]..."))

# Start by compiling the project if needed.
from compiling.compile import compile
if not compile(args): exit(0)
if args["just_compile"]:
    print(purple("Compilation finished. Exiting (--just_compile)..."))
    exit(0)

# Get Kairos-TDVRPTW directory and load the lib.
"""
Load the Kairos library based on the provided arguments.
"""
KAIROS_BUILD_TYPE =  args["build_type"].lower()
KAIROS_LIB_DIR = os.path.join(
    PROJECT_ROOT_DIR, "build", KAIROS_BUILD_TYPE, 
)
if not os.path.exists(KAIROS_LIB_DIR):
    raise FileNotFoundError(f"Kairos-TDVRPTW library not found at {KAIROS_LIB_DIR}")
sys.path.append(KAIROS_LIB_DIR)  # or wherever the .so is
import kairos_tdvrptw as ks

from utils.utils import read_json_from_file, save_json_to_file, save_csv_to_file, load_csv_from_file, get_filename_from_path, join_paths
from utils.formatting import format_date_for_filepath, format_date_for_console
from params.constants import OUTPUT_DIR, INSTANCES_DIR, RUNNER_START_TIME
from running.experiment import run_experiment, instances_for_experiment
from params.args import parse_program_args
from utils.math import percentage_difference
from tqdm import tqdm
from typing import Any
import json, datetime, time
import random
import pandas as pd

def get_runs(args: dict[str, Any]):
    """
    Load the experiment file and return the runs.
    """
    experiment_files: list[str] = args["experiments"] # List of .json experiment files.
    selected_instances = args["instances"]
    selected_experiments = args["exps"]
    
    # prepare experiment runs
    experiment_runs:list[tuple] = []

    # Run experiment files.
    for experiment_file in experiment_files:
        experiment_file_json = json.load(open(experiment_file))
        experiment_filename = os.path.basename(experiment_file).replace(".json", "")

        # Outputs of the experiments will be stored in this object.
        print("experiment_file", experiment_file)
        print("type of experiment_file", type(experiment_file))
        
        output_keyname = f"{format_date_for_filepath(RUNNER_START_TIME)}-{experiment_filename}"
        annotated_experiment_output_dirpath =  join_paths(OUTPUT_DIR, output_keyname)
        annotated_experiment_filepath = join_paths(annotated_experiment_output_dirpath, "annotated_experiment.json")
        csv_output_filepath =  join_paths(OUTPUT_DIR, f"csv/{output_keyname}.csv")
        json_output_dirpath = join_paths(annotated_experiment_output_dirpath, "outputs")
        log_output_dirpath = join_paths(annotated_experiment_output_dirpath, "logs")

        annotated_experiment = {
            "date": str(datetime.date.today()), 
            "experiment_file": os.path.abspath(experiment_file),
            "experiment_params": experiment_file_json,
            "annotated_experiment_output_dirpath": annotated_experiment_output_dirpath,
            "csv_output_filepath": csv_output_filepath
        }

        # For each instances specified in the experiment file.
        instances = instances_for_experiment(
            experiment_file_json, 
            selected_instances
        )

        for instance in instances:
            # Get instance solutions from the dataset directory.
            solutions = []
            solutions_filepath = f"{instance["instance_dirpath"]}/solutions.json"
            # TODO: the day I need solutions in the solver, edit
            # if os.path.isfile(f"{instance["instance_dirpath"]}/solutions.json"):
            #     solutions = read_json_from_file(F"{INSTANCES_DIR}/{instance["dataset_name"]}/solutions.json")
            #     solutions = [s for s in solutions if s["instance_name"] == instance["instance_name"]]

            # For each experiment defined in the experiment file.
            for experiment in experiment_file_json["experiments"]:
                # Check if the experiment was selected in the exps argument.
                if selected_experiments != None and experiment["name"] not in selected_experiments: continue

                # Run the experiment.
                #print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]} ({datetime.datetime.now()})"), flush=True)
                
                experiment_runs.append((experiment, instance, solutions))

        print("Total number of runs:", len(experiment_runs))
        
        # Print each run.
        for experiment, instance, solutions in experiment_runs:
            print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]}"), flush=True)
        print()

    return experiment_runs

def main():
    # Create intervals
    interval = ks.goc.Interval(0.0, 10.0)
    print("Interval:", interval)

    # Create PWL functions
    breakpoints = [0.0, 1.0, 2.0, 3.0]
    values = [0.0, 2.0, 4.5, 10.0]
    test_ndcpwlf = ks.nyr.NDCPWLF(breakpoints, values)
    print("NDCPWLF:", test_ndcpwlf)

    if args["dry_run"]: return
    experiment_runs = get_runs(args)

    df = pd.DataFrame()
    for experiment, instance, solutions in experiment_runs:
        df_res_stats = run_experiment_on_instance(
            args,
            experiment,
            instance,
            solutions,
            df
        )
        df = pd.concat(
            [df, df_res_stats],
            ignore_index=True
        )
        # # TODO: remove. Early exit (just for testing)
        # break
    
    # Print the full results
    print(f"\nFull benchmark results: [build type: {KAIROS_BUILD_TYPE}]\n")
    print(df.to_markdown(index=False))

    # Print total time taken for the program.
    total_time = datetime.datetime.now() - RUNNER_START_TIME
    print(purple(F"Total time taken: {total_time}"))


def legacy_test_route_duration_calculation(
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
):
    """
    legacy manual test function.
    Test the route duration calculation using ONYR and LERA methods.
    You need to ensure the route selected is valid and feasible for the instance.
    """

    # Create some random routes for testing.
    random_routes: list[list[int]] = []
    # for i in range(5):
    #     random_route_length = 10
    #     #random_route_length = random.randint(1, tdvrptw_instance.nb_clients())
    #     random_clients = random.sample(range(1, tdvrptw_instance.nb_clients() + 1), random_route_length)
    #     random_route = [tdvrptw_instance.o] + random_clients + [tdvrptw_instance.d]
    #     random_routes.append(random_route)
    random_routes.append([
        0,
        37,
        14,
        44,
        86,
        6,
        101
    ])
    
    for route in random_routes:
        print(green(f"Random route: {route}"))
        # Evaluate the route.
        delta_route: ks.nyr.NDCPWLF = ks.nyr.perform_tree_chain_composition(
            tdvrptw_instance, 
            artfs, 
            route
        )
        print(green(f"RRTF (tree-chain): {delta_route}"))
        print(green(f"RRTF (tree-chain) duration: {ks.nyr.compute_optimal_departure_time_and_duration(delta_route)}"))
        delta_route_sequential: ks.nyr.NDCPWLF = ks.nyr.perform_sequential_chain_composition(
            tdvrptw_instance, 
            artfs, 
            route
        )
        print(green(f"RRTF (sequential): {delta_route_sequential}"))
        print(green(f"RRTF (sequential) duration: {ks.nyr.compute_optimal_departure_time_and_duration(delta_route_sequential)}"))

        route_duration_onyr = ks.nyr.compute_RouteDuration_from_delta_path(
            delta_route,
            route
        )
        print(green(f"Route duration (ONYR): {route_duration_onyr}"))
        route_duration_lera = ks.nyr.compute_RouteDuration_lera(
            tdvrptw_instance,
            route
        )
        print(green(f"Route duration (LERA): {route_duration_lera}"))
        print("is equal:", route_duration_onyr == route_duration_lera)

def run_experiment_on_instance(
    args: dict[str, Any],
    experiment,
    instance,
    solutions,
    df: pd.DataFrame # for storing results
):
    start_time = datetime.datetime.now()
    print(purple(F"Running [{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]}"), flush=True)

    # Load the instance
    instance_filepath = F"{instance['instance_dirpath']}/{instance['instance_filename']}"
    instance_json_data = read_json_from_file(instance_filepath)
    instance_json_data["instance_filename"] = instance["instance_filename"]

    tdvrptw_instance: ks.nyr.VRPInstance = ks.load_instance_from_json(instance_json_data)
    # print(tdvrptw_instance)
    artfs = ks.nyr.make_artfs(tdvrptw_instance)
    # print("ARTFs:", artfs)

    # Run heuristics and measure time
    heuristics = [
        {
            "name": "GNN-Makespan",
            "func": lambda: ks.greedy_nearest_neighbor_makespan(tdvrptw_instance),
            "args": (),
            "kwargs": {},
        },
        {
            "name": "GNN-Duration",
            "func": lambda: ks.greedy_nearest_neighbor_duration(tdvrptw_instance, artfs),
            "args": (),
            "kwargs": {},
        },
        {
            "name": "Regret-Insertion",
            "func": lambda: ks.regret_insertion_duration(tdvrptw_instance, artfs),
            "args": (),
            "kwargs": {},
        },
    ]

    heuristic_results = []
    for heuristic in heuristics:
        time_start = time.time()
        solution = heuristic["func"](*heuristic["args"], **heuristic["kwargs"])
        time_taken = time.time() - time_start
        heuristic_results.append({
            "name": heuristic["name"],
            "solution": solution,
            "duration": solution.value,
            "time_taken": time_taken,
        })

    # Find best approach (lowest duration)
    best_result = min(heuristic_results, key=lambda x: x["duration"])
    print(green(f"Best approach: {best_result['name']} ({best_result['duration']})"))

    # Print all approaches and percentage differences
    for res in heuristic_results:
        print(green(f"{res['name']}: {res['duration']} (time: {res['time_taken']:.4f}s)"))
    for i in range(len(heuristic_results)):
        for j in range(i + 1, len(heuristic_results)):
            diff = percentage_difference(heuristic_results[i]["duration"], heuristic_results[j]["duration"])
            print(green(f"Percentage difference between {heuristic_results[i]['name']} and {heuristic_results[j]['name']}: {diff}%"))

    # Prepare stats for DataFrame
    stats = {
        "instance_name": instance["instance_filename"],
        "dataset_name": instance["dataset_name"],
        "best_approach": best_result["name"],
        "best_duration": best_result["duration"],
        "total_time_taken": (datetime.datetime.now() - start_time).total_seconds(),
    }
    # Add each heuristic's duration and time_taken to stats
    for res in heuristic_results:
        stats[f"duration_{res['name'].lower().replace('-', '_')}"] = res["duration"]
        stats[f"time_{res['name'].lower().replace('-', '_')}"] = res["time_taken"]

    df_current_stats = pd.DataFrame(stats, index=[0])
    return df_current_stats
    

if __name__ == "__main__":
    main()
