import json, os
from typing import Any, Optional

from params.constants import INSTANCES_DIR, OBJ_DIR
from .run import run_program
from utils.utils import read_json_from_file

def tags_selection_function(
        instance_tags: list[str], 
        tag_sets: list[list[str]]
    ) -> bool:
    """
    Function that, given sets of tags, checks if the instance tags
    match any of the sets of tags.
    """
    # check tagset per tagset. If any tagset is matched, return True.
    for tag_set in tag_sets:
        instance_tags_ok = True
        for tag in tag_set:
            if tag.startswith("!"):
                exclusion_tag = tag[1:]
                if exclusion_tag in instance_tags:
                    instance_tags_ok = False
                    break
            else:
                if tag not in instance_tags:
                    instance_tags_ok = False
                    break
        if instance_tags_ok:
            # instance tags are ok for this tagset.
            return True            

    return False

def get_entries_from_index_file(
        instance_dir: str
    ) -> list[dict[str, Any]]|None:
    """
    Get the entries from the index file of the instance directory.
    If no index file is found, return None.
    """
    index_file_path = F"{instance_dir}/index.json"
    if not os.path.isfile(index_file_path):
        return None
    return read_json_from_file(index_file_path)

def get_instance_entry(
        list_of_entries: list[dict[str, Any]],
        instance_filename: str
    ) -> dict[str, Any]:
    """
    Get the entry from the list of entries that matches the instance name.
    """
    for entry in list_of_entries:
        if entry["instance_filename"] == instance_filename:
            return entry
    return None

def instances_for_experiment(
        experiment_file_json: dict[str, Any],
        selected_instances: Optional[list[str]]
    ):
    """
    Lera-Romero's style benchmark instances selection.

    Get the instances that are specified in the experiment file,
    from instance directories or dataset directories.
    Apply some filters to the instances, such as SELECT filters.

    Returns: the instances that are specified in the experiment file.
    They can be from many datasets and the datasets may have SELECT filters.
    Example: {"name":"ejor2019", "select":"!TAG1|TAG2 TAG3"}
    Meaning: select all instances from dataset ejor2019 that 
    either not containts TAG1 or contains both TAG2 and TAG3.
    """
    # VRP-benchmarks style instances selection.
    instances: list[str] = []
    for dataset in experiment_file_json["datasets"]:

        # get all subdirectories of the dataset directory
        dataset_dir = f"{INSTANCES_DIR}/{dataset['name']}"
        dataset_subdirs = os.listdir(dataset_dir)
        instance_dirs = []

        # check that dataset_subdirs contains only directories
        if all(os.path.isdir(f"{dataset_dir}/{subdir}") for subdir in dataset_subdirs):
            if "n" in dataset:
                # filter out directories that do not match the n values
                dataset_subdirs_filtered = []
                for subdir in dataset_subdirs:
                    n_subdir = int(subdir.split("=")[1])
                    if n_subdir in dataset["n"]:
                        dataset_subdirs_filtered.append(subdir)
                
                for subdir in dataset_subdirs_filtered:
                    instance_dirs.append(f"{dataset_dir}/{subdir}")
        else:
            # the dataset_dir itself is the instance directory
            instance_dirs.append(dataset_dir)

        if "select" in dataset:
            tag_sets = dataset["select"].split("|")
            tag_sets = [s.split(" ") for s in tag_sets]
            tag_sets = [[t.strip() for t in s if t.strip()] for s in tag_sets if any(t.strip() for t in s)]

        # actual instance selection
        for instance_dir in instance_dirs:
            entries = get_entries_from_index_file(instance_dir)

            for instance_filename in os.listdir(instance_dir):
                if instance_filename.endswith(".json"):
                    # determine if the instance should be selected
                    if instance_filename == "index.json":
                        continue
                    if selected_instances != None and instance_filename not in selected_instances: 
                        continue # not a selected instance
                    if "select" in dataset and entries != None:
                        entry = get_instance_entry(entries, instance_filename)
                        if not tags_selection_function(entry["tags"], tag_sets): 
                            continue # Filter out instances that do not match selection criteria.

                    # selected instance
                    instance = {
                        "dataset_name": dataset["name"],
                        "instance_dirpath": instance_dir,
                        "instance_filename": instance_filename,
                    }
                    instances.append(instance)

    return instances

def run_experiment(
        args: dict[str, Any],
        experiment, 
        instance, 
        solutions
    ):
    """
    Run the executable file from the experiment and passes 
    it three arguments to STDIN in the following order:
    experiment: experiment to run.
    
    Args:
    instance: instance JSON to use.
    solutions: instance known solutions of the solutions.json file of the dataset.
    Returns: a JSON object with the output of the execution.
    
    If success == true: it also has the attributes {"execution_log":json, "solution":json}
    If success == false: it also has the attributes {"exit_code":number, ""}
    """
    build_type = args["build_type"]
    use_callgrind = args["callgrind"]
    use_valgrind = args["valgrind"]
    use_heaptrack = args["heaptrack"]
    silent = args["silent"]
    memlimit_gb = args["memlimit"]
    
    # Get executable command depending on whether valgrind or callgrind are enabled.
    executable_path = F"{OBJ_DIR}/{build_type}/{experiment['executable']}"
    executable = [executable_path]
    if use_valgrind: executable = ["valgrind", "--track-origins=yes", executable_path]
    elif use_callgrind: executable = ["valgrind", "--tool=callgrind", executable_path]
    elif use_heaptrack: executable = ["heaptrack", executable_path]

    # Execute experiment.
    result = run_program(
        executable, 
        f"{json.dumps(experiment)}{json.dumps(instance)}{json.dumps(solutions)}", 
        memlimit_gb, 
        silent
    )

    # Try to parse STDOUT as JSON, otherwise leave it as string.
    stdout_json = ""
    try: 
        stdout_json = json.loads(result["stdout"])
    except:
        stdout_json = result["stdout"]

    # If experiment finished successfuly, return the observation and the metadata.
    return {
        "dataset_name": instance["dataset_name"], 
        "instance_dirpath": instance["instance_dirpath"],
        "instance_filename": instance["instance_filename"], 
        "experiment_name": experiment["name"], 
        "stderr": result["stderr"], 
        "stdout": stdout_json, 
        "exit_code": result["exit_code"],
        "time": result["time"]
    }
