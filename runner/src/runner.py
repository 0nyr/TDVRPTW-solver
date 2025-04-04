import os, json, datetime, os

from utils.terminal import purple
from utils.utils import read_json_from_file, save_json_to_file, save_csv_to_file, load_csv_from_file, get_filename_from_path
from utils.formatting import format_date_for_filepath
from params.constants import OUTPUT_DIR, INSTANCES_DIR, RUNNER_START_TIME
from compiling.compile import compile
from running.experiment import run_experiment, instances_for_experiment
from params.args import parse_program_args
from output.csv_output import get_csv_res
from tqdm import tqdm

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
		TSave = 30
		experiment_filename = os.path.basename(experiment_file).replace(".json", "")
		output_file_name = F"{format_date_for_filepath(RUNNER_START_TIME)}-{experiment_filename}.json"
		csv_output_filepath = F"{OUTPUT_DIR}/csv/{output_file_name.replace('.json', '.csv')}"
		if args["carry_on"] is not None:
			csv_output_filepath = args["carry_on"]
		TInit = datetime.datetime.now() # TInit = "timestamp when the experimentation started".
		TLast = datetime.datetime.now() # TLast = "last time the output was saved".

		# For each instances specified in the experiment file.
		instances = instances_for_experiment(
			experiment_file_json, 
			selected_instances
		)

		# prepare experiment runs
		experiment_runs:list[tuple] = []

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
				#print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]} ({datetime.datetime.now()})"), flush=True)
				
				experiment_runs.append((experiment, instance, solutions))
		
		# If carry-on, filter out runs (instance & experiment) that have already been run.
		if args["carry_on"] is not None:
			print("Original number of runs:", len(experiment_runs))
			carry_on_data = load_csv_from_file(args["carry_on"])
			indexes_to_remove = []
			for idx, run in enumerate(experiment_runs):
				experiment_to_remove = run[0]["name"]
				instance_to_remove = run[1]["instance_filename"]
				if any([experiment_to_remove == cod["experiment_name"] and instance_to_remove == get_filename_from_path(cod["instance_filepath"]) for cod in carry_on_data]):
					print("Marking for removal", experiment_to_remove, instance_to_remove)
					indexes_to_remove.append(idx)
			# Remove the runs in reverse order to avoid index issues.
			for idx in sorted(indexes_to_remove, reverse=True):
				del experiment_runs[idx]
			print("Removed", len(indexes_to_remove), "runs from the list of runs to execute.")

		# Print the number of runs to be executed.
		if args["carry_on"] is not None:
			print("Remaining number of runs:", len(experiment_runs))
		else:
			print("Total number of runs:", len(experiment_runs))
		# Print each run.
		for experiment, instance, solutions in experiment_runs:
			print(purple(F"[{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]}"), flush=True)

		if args["dry_run"]: return
		for experiment, instance, solutions in tqdm(experiment_runs, desc="Running experiments"):
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