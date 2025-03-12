import sys, os, json, datetime, os, argparse, subprocess, ntpath, select

from utils.terminal import purple, green, red
from utils.utils import read_json_from_file, save_json_to_file
from params.constants import OUTPUT_DIR, INSTANCES_DIR, CONFIG, RUNNER_DIR
from compiling.compile import compile
from running.experiment import run_experiment, instances_for_experiment_file
from params.args import parse_program_args

def main():
	args = parse_program_args()
	experiment_files = args["experiments"]
	selected_instances = args["instances"]
	selected_experiments = args["exps"]
	build_type = args["build_type"]

	# Compile project.
	if not compile(build_type): exit(0)

	# Run experiment files.
	for experiment_file in experiment_files:
		experiment_file_json = json.loads(experiment_file.read())

		# Outputs of the experiments will be stored in this object.
		experiment_file_name = ntpath.basename(experiment_file.name.replace(".json", ""))
		output = {"date": str(datetime.date.today()), "experiment_file": experiment_file_name, "outputs": []}

		# Periodically, every TSave seconds the output will be saved to the output folder with the name "<date>-<experiment_file_name>.json".
		TSave = 60
		output_file_name = F"{datetime.date.today()}-{experiment_file_name}.json"
		TInit = datetime.datetime.now() # TInit = "timestamp when the experimentation started".
		TLast = datetime.datetime.now() # TLast = "last time the output was saved".

		# For each instances specified in the experiment file.
		instances = instances_for_experiment_file(
			experiment_file_json, 
			selected_instances
		)
		for instance in instances:
			# Get instance solutions from the dataset directory.
			solutions = []
			if os.path.isfile(F"{INSTANCES_DIR}/{instance['dataset_name']}/solutions.json"):
				solutions = read_json_from_file(F"{INSTANCES_DIR}/{instance['dataset_name']}/solutions.json")
				solutions = [s for s in solutions if s["instance_name"] == instance["instance_name"]]

			# For each experiment defined in the experiment file.
			for experiment in experiment_file_json["experiments"]:
				# Check if the experiment was selected in the exps argument.
				if selected_experiments != None and experiment["name"] not in selected_experiments: continue

				# Run the experiment.
				print(purple(F"[{instance['dataset_name']}] {instance['instance_name']} - {experiment['name']} ({datetime.datetime.now()})"), flush=True)
				output["outputs"].append(run_experiment(args, experiment, instance, solutions))
				
				# If TSave seconds have passed since TLast then save output.
				if (datetime.datetime.now() - TLast).total_seconds() >= TSave:
					output["time"] = (datetime.datetime.now() - TInit).total_seconds()
					save_json_to_file(F"{OUTPUT_DIR}/{output_file_name}", output)
					TLast = datetime.datetime.now()
		
		# Having finished all experiments from the experimentation_file, save the final output.
		output["time"] = (datetime.datetime.now() - TInit).total_seconds()
		save_json_to_file(F"{OUTPUT_DIR}/{output_file_name}", output)

if __name__== "__main__":
  main()