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

from loading.load import get_runs
from utils.formatting import format_date_for_console
from params.constants import RUNNER_START_TIME, PROJECT_COMMIT_HASH, PROGRAM_SHORT_NAME
from params.args import parse_program_args
from utils.math import percentage_difference
from benchmarks.bks import check_bks, BKSCheckStats, SolutionStatus, save_new_bks_in_storage
from loading.load import load_instance_to_tdvrptw_instance

from typing import Any
import json, datetime, time
import pandas as pd


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
    for experiment, instance, solution in experiment_runs:
        (tdvrptw_instance, artfs) = load_instance_to_tdvrptw_instance(instance)
        df_res_stats = run_experiment_on_instance(
            args,
            experiment,
            instance,
            solution,
            df,
            tdvrptw_instance,
            artfs
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



def run_experiment_on_instance(
    args: dict[str, Any],
    experiment,
    instance,
    solution,
    df: pd.DataFrame, # for storing results
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
):
    start_time = datetime.datetime.now()

    instance_filename = instance["instance_filename"]
    dataset_name = instance["dataset_name"]
    print(purple(F"Running [{dataset_name}] {instance_filename} - {experiment["name"]}"), flush=True)

    bks_stats = check_bks(
        instance_filename,
        dataset_name,
        solution,
        tdvrptw_instance,
        artfs
    )

    # Run heuristics and measure time
    heuristics = [
        {
            "name": "NNH-Makespan",
            "func": lambda: ks.greedy_nearest_neighbor_makespan(tdvrptw_instance),
            "args": (),
            "kwargs": {},
        },
        {
            "name": "NNH-Duration",
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
        {
            "name": "Regret-1-Insert",
            "func": lambda: ks.regret_k_insertion_duration(tdvrptw_instance, artfs, 1),
            "args": (),
            "kwargs": {},
        },
        {
            "name": "Regret-2-Insert",
            "func": lambda: ks.regret_k_insertion_duration(tdvrptw_instance, artfs, 2),
            "args": (),
            "kwargs": {},
        },
        {
            "name": "Regret-3-Insert",
            "func": lambda: ks.regret_k_insertion_duration(tdvrptw_instance, artfs, 3),
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
        # solution_as_str = json.dumps(json.loads(str(solution)), indent=4)
        # print(green(f"({heuristic['name']}) - Sol. Duration: {solution.value}, : \n{solution_as_str}"))

    # Find best approach (lowest duration)
    best_result = min(heuristic_results, key=lambda x: x["duration"])
    # print(green(f"Best approach: {best_result['name']} ({best_result['duration']})"))

    # # Print all approaches and percentage differences
    # for res in heuristic_results:
    #     print(green(f"{res['name']}: {res['duration']} (time: {res['time_taken']:.4f}s)"))
    # for i in range(len(heuristic_results)):
    #     for j in range(i + 1, len(heuristic_results)):
    #         diff = percentage_difference(heuristic_results[i]["duration"], heuristic_results[j]["duration"])
    #         print(green(f"Percentage difference between {heuristic_results[i]['name']} and {heuristic_results[j]['name']}: {diff}%"))

    # Save new solution is it is better than the current BKS
    if best_result["duration"] < bks_stats.bks_recomp_dur_onyr:
        print(green(f"New best solution found: {best_result['name']} with duration {best_result['duration']}"))
        
        solution_routes_as_dict = best_result["solution"].to_json()
        del solution_routes_as_dict["objective"]
        new_solution = {
            "value": best_result["duration"],
            "solution": solution_routes_as_dict,
            "status": str(SolutionStatus.HEURISTIC),
            "metadata": {
                "authors": "0nyr (Florian Rascoussier)",
                "time": best_result["time_taken"],
                "program": PROGRAM_SHORT_NAME,
                "origin": best_result["name"],
                "commit_hash": PROJECT_COMMIT_HASH,
                "instexp_start_time": format_date_for_console(RUNNER_START_TIME),
            }
        }
        save_new_bks_in_storage(
            instance["instance_dirpath"],
            instance["instance_filename"],
            new_solution,
            throw_on_failed_check=False
        )

    # Prepare stats for DataFrame
    stats = {
        "instance_name": instance["instance_filename"],
        "dataset_name": instance["dataset_name"],
        "best_approach": best_result["name"],
        "best_duration": best_result["duration"],
        "total_time_taken": (datetime.datetime.now() - start_time).total_seconds(),
        "bks_stored_duration": bks_stats.bks_stored_duration,
        "bks_recomp_dur_onyr": bks_stats.bks_recomp_dur_onyr,
        "bks_recomp_dur_lera": bks_stats.bks_recomp_dur_lera,
        "best_duration_per-diff_bks": percentage_difference(
            best_result["duration"], 
            bks_stats.bks_recomp_dur_onyr
        ),
    }
    # Add each heuristic's duration and time_taken to stats
    for res in heuristic_results:
        stats[f"duration_{res['name'].lower().replace('-', '_')}"] = res["duration"]
        stats[f"time_{res['name'].lower().replace('-', '_')}"] = res["time_taken"]

    df_current_stats = pd.DataFrame(stats, index=[0])
    return df_current_stats


if __name__ == "__main__":
    main()

    # from benchmarks.bks import check_all_bks_duration
    # check_all_bks_duration()

    # from benchmarks.bks import check_all_and_remove_incorrect_bks
    # check_all_and_remove_incorrect_bks()

    # from benchmarks.bks import check_all_lera_bks
    # check_all_lera_bks()
