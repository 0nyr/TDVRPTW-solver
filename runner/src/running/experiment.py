import json
from typing import Any, Optional

from params.constants import INSTANCES_DIR, OBJ_DIR
from .run import run_program
from utils.utils import read_json_from_file

def instances_for_experiment_file(
		experiment_file_json: dict[str, Any],
		selected_instances: Optional[list[str]]
    ):
	"""
    Returns: the instances that are specified in the experiment file.
    They can be from many datasets and the datasets may have SELECT filters.
    Example: {"name":"ejor2019", "select":"!TAG1|TAG2 TAG3"}
    Meaning: select all instances from dataset ejor2019 that either not containts TAG1 or contains both TAG2 and TAG3.
    """
	instances = []
	for dataset in experiment_file_json["datasets"]:
		dataset_dir = F"{INSTANCES_DIR}/{dataset['name']}"
		dataset_index = read_json_from_file(F"{dataset_dir}/index.json")
		tags_selection_function = lambda tags: True
		
		if "select" in dataset:
			tag_sets = dataset["select"].split("|")
			tag_sets = [s.split(" ") for s in tag_sets]
			tag_sets = [[t.strip() for t in s if t.strip()] for s in tag_sets if any(t.strip() for t in s)]
			tags_selection_function = lambda tags: any([all([(tag.replace("!", "") in tags) is not ("!" in tag) for tag in tag_set]) for tag_set in tag_sets])
			
		for entry in dataset_index:
			if selected_instances != None and entry["instance_name"] not in selected_instances: continue # Only leave instances with names matching the filter (if filter is * then all).
			if not tags_selection_function(entry["tags"]): continue # Filter out instances that do not match selection criteria.
			instance = read_json_from_file(F"{dataset_dir}/{entry['file_name']}")
			instance["instance_name"] = entry["instance_name"]
			instance["dataset_name"] = dataset["name"]
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
    
	The base format of the JSON object is {"dataset_name":string, "instance_name":string, "experiment_name":string, "success":bool, "stderr":string}.
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
		"dataset_name":instance["dataset_name"], 
		"instance_name":instance["instance_name"], 
		"experiment_name":experiment["name"], 
		"stderr": result["stderr"], 
		"stdout": stdout_json, 
		"exit_code": result["exit_code"],
		"time": result["time"]
	}
