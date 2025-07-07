from typing import Any
import os, json, datetime

from utils.utils import read_json_from_file
from running.experiment import instances_for_experiment
from utils.formatting import format_date_for_filepath
from utils.utils import read_json_from_file, join_paths, check_key_series_in_dict
from params.constants import OUTPUT_DIR, RUNNER_START_TIME, OPTIMIZATION_OBJECTIVE
from utils.terminal import purple, green

import kairos_tdvrptw as ks

def load_instance(instance: dict[str, Any]):
    """
    Loads an instance JSON file and updates it with the filename.

    Args:
        instance (dict): Dictionary with 'instance_dirpath' and 'instance_filename' keys.

    Returns:
        dict: Loaded instance data with 'instance_filename' key updated.
    """
    instance_filepath = f"{instance['instance_dirpath']}/{instance['instance_filename']}"
    instance_json_data = read_json_from_file(instance_filepath)
    instance_json_data["instance_filename"] = instance["instance_filename"]
    return instance_json_data

def load_instance_to_tdvrptw_instance(instance: dict[str, Any]):
    """
    Loads an instance JSON file and converts it to a TDVRPTW instance.

    Args:
        instance (dict): Dictionary with 'instance_dirpath' and 'instance_filename' keys.

    Returns:
        ks.nyr.VRPInstance: Loaded TDVRPTW instance.
    """
    instance_json_data = load_instance(instance)
    tdvrptw_instance: ks.nyr.VRPInstance = ks.load_instance_from_json(instance_json_data)
    # print(tdvrptw_instance)
    artfs = ks.nyr.make_artfs(tdvrptw_instance)
    # print("ARTFs:", artfs)
    return tdvrptw_instance, artfs

def load_instances_with_solutions(
    experiment: dict[str, Any],
    selected_instances: list[str] | None = None
):
    """
    Load instances specified in the experiment and their solutions (BKS).
    """
    # For each instances specified in the experiment file.
    instances = instances_for_experiment(
        experiment, 
        selected_instances
    )

    for instance in instances:
        # Get instance solution (BKS) from the dataset directory.
        solution = None
        solutions_filepath = f"{instance["instance_dirpath"]}/solutions.json"
        if os.path.isfile(solutions_filepath):
            solution_data = read_json_from_file(solutions_filepath)
            if check_key_series_in_dict(solution_data, [OPTIMIZATION_OBJECTIVE, instance["instance_filename"]]):
                solution = solution_data[OPTIMIZATION_OBJECTIVE][instance["instance_filename"]]

        yield (instance, solution)

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

        for instance, solution in load_instances_with_solutions(experiment_file_json, selected_instances):
            # For each experiment defined in the experiment file.
            for experiment in experiment_file_json["experiments"]:
                # Check if the experiment was selected in the exps argument.
                if selected_experiments is not None and experiment["name"] not in selected_experiments:
                    continue
                experiment_runs.append((experiment, instance, solution))

        print("Total number of runs:", len(experiment_runs))
        
        # Print each run.
        for experiment, instance, solution in experiment_runs:
            print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]}"), flush=True)
        print()

    return experiment_runs

def get_all_datasets():
    """
    Returns a basic experiment object with all datasets.
    """
    return {"datasets": [
        {"name": "Dabia2013",},
        {"name": "Ari2018",},
        {"name": "Rifki2020",},
        {"name": "Solomon1987",},
        {"name": "Vu2020",}
    ]}

def load_all_instances_and_solutions():
    """
    Load all instances and their solutions (BKS) from the datasets.
    """
    all_instance_solution_pairs = []
    all_datasets_exp = get_all_datasets()
    
    print(purple(f"Loading all instances and their solutions (BKS) from the datasets: "))
    for dataset_name in all_datasets_exp["datasets"]:
        print(purple(f"    + {dataset_name['name']}"))

    for instance, solution in load_instances_with_solutions(all_datasets_exp):
        all_instance_solution_pairs.append((instance, solution))
    
    print(green(f"Total number of instances loaded: {len(all_instance_solution_pairs)}"))

    return all_instance_solution_pairs
