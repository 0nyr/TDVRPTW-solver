import os, json, datetime, os

from utils.terminal import purple
from utils.utils import read_json_from_file, save_json_to_file, save_csv_to_file
from utils.formatting import format_date_for_filepath
from params.constants import OUTPUT_DIR, INSTANCES_DIR, RUNNER_START_TIME
from compiling.compile import compile
from running.experiment import run_experiment, instances_for_experiment
from params.args import parse_program_args
from output.csv_output import get_csv_res

def main():
	args = parse_program_args()
	experiment_files: list[str] = args["experiments"] # List of .json experiment files.
	selected_instances = args["instances"]
	selected_experiments = args["exps"]

	# Compile project.
	if not compile(args): exit(0)

	# Run experiment files.
	for experiment_file in experiment_files:
		experiment_file_json = json.load(open(experiment_file))

		# Outputs of the experiments will be stored in this object.
		print("experiment_file", experiment_file)
		print("type of experiment_file", type(experiment_file))
		output = {
			"date": str(datetime.date.today()), 
			"experiment_file": os.path.abspath(experiment_file), 
			"outputs": []
		}

		# Periodically, every TSave seconds the output will be saved to the output folder with the name "<date>-<experiment_file_name>.json".
		TSave = 5
		experiment_filename = os.path.basename(experiment_file).replace(".json", "")
		output_file_name = F"{format_date_for_filepath(RUNNER_START_TIME)}-{experiment_filename}.json"
		csv_output_filepath = F"{OUTPUT_DIR}/csv/{output_file_name.replace('.json', '.csv')}"
		TInit = datetime.datetime.now() # TInit = "timestamp when the experimentation started".
		TLast = datetime.datetime.now() # TLast = "last time the output was saved".

		# For each instances specified in the experiment file.
		instances = instances_for_experiment(
			experiment_file_json, 
			selected_instances
		)
		for instance in instances:
			# Get instance solutions from the dataset directory.
			solutions = []
			if os.path.isfile(f"{instance["instance_dirpath"]}/solutions.json"):
				solutions = read_json_from_file(F"{INSTANCES_DIR}/{instance["dataset_name"]}/solutions.json")
				solutions = [s for s in solutions if s["instance_name"] == instance["instance_name"]]

			# For each experiment defined in the experiment file.
			for experiment in experiment_file_json["experiments"]:
				# Check if the experiment was selected in the exps argument.
				if selected_experiments != None and experiment["name"] not in selected_experiments: continue

				# Run the experiment.
				print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]} ({datetime.datetime.now()})"), flush=True)
				res = run_experiment(args, experiment, instance, solutions)
				output["outputs"].append(res)

				# Save the CSV output.
				save_csv_to_file(csv_output_filepath, get_csv_res(res))
				
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