# --- Command recap:
# Compile: python runner/src/runner.py --just-compile -b debug
# Run: python runner/src/main.py -b debug &> logs/test.log

import os, sys

from params.args import parse_program_args
from params.constants import PROJECT_ROOT_DIR, RUNNER_START_TIME
from utils.formatting import format_date_for_console

print(f"Starting program [datetime: {format_date_for_console(RUNNER_START_TIME)}]...")

# Get Kairos-TDVRPTW directory and load the lib.
"""
Load the Kairos library based on the provided arguments.
"""
args = parse_program_args()
KAIROS_BUILD_TYPE =  args["build_type"].lower()
KAIROS_LIB_DIR = os.path.join(
    PROJECT_ROOT_DIR, "build", KAIROS_BUILD_TYPE, 
)
if not os.path.exists(KAIROS_LIB_DIR):
    raise FileNotFoundError(f"Kairos-TDVRPTW library not found at {KAIROS_LIB_DIR}")
sys.path.append(KAIROS_LIB_DIR)  # or wherever the .so is
import kairos_tdvrptw as ks

from utils.terminal import purple, green
from utils.utils import read_json_from_file, save_json_to_file, save_csv_to_file, load_csv_from_file, get_filename_from_path, join_paths
from utils.formatting import format_date_for_filepath, format_date_for_console
from params.constants import OUTPUT_DIR, INSTANCES_DIR, RUNNER_START_TIME
from compiling.compile import compile
from running.experiment import run_experiment, instances_for_experiment
from params.args import parse_program_args
from output.csv_output import get_csv_res
from output.json_output import complete_res_json
from tqdm import tqdm
from typing import Any
import json, datetime
import random

def get_runs(args: dict[str, Any]):
    """
    Load the experiment file and return the runs.
    """
    experiment_files: list[str] = args["experiments"] # List of .json experiment files.
    selected_instances = args["instances"]
    selected_experiments = args["exps"]

    # Compile project.
    if not compile(args): exit(0)
    if args["just_compile"]:
        print(purple("Compilation finished. Exiting (--just_compile)..."))
        exit(0)
    
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

    for experiment, instance, solutions in experiment_runs:
        run_experiment_on_instance(
            args,
            experiment,
            instance,
            solutions
        )
        # TODO: remove. Early exit (just for testing)
        break

    # Print total time taken for the program.
    total_time = datetime.datetime.now() - RUNNER_START_TIME
    print(purple(F"Total time taken: {total_time}"))

def run_experiment_on_instance(
    args: dict[str, Any],
    experiment,
    instance,
    solutions
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

    # Create some random routes for testing.
    random_routes = []
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
        delta_route = ks.nyr.perform_tree_chain_composition(
            tdvrptw_instance, 
            artfs, 
            route
        )
        print(green(f"RRTF: {delta_route}"))


if __name__ == "__main__":
    main()
