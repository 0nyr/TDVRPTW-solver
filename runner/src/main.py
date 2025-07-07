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
from params.constants import RUNNER_START_TIME
from params.args import parse_program_args
from utils.math import percentage_difference
from benchmarks.bks import check_bks
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
        df_res_stats = check_bks(
            instance["instance_filename"],
            instance["dataset_name"],
            solution,
            tdvrptw_instance,
            artfs
        )
        # df_res_stats = run_experiment_on_instance(
        #     args,
        #     experiment,
        #     instance,
        #     solution,
        #     df,
        #     tdvrptw_instance,
        #     artfs
        # )
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
    print(purple(F"Running [{instance["dataset_name"]}] {instance["instance_filename"]} - {experiment["name"]}"), flush=True)

    (stored_duration_sum, recomputed_duration_sum_onyr, recomputed_duration_sum_lera) = check_bks(
        instance["instance_filename"],
        instance["dataset_name"],
        solution, tdvrptw_instance, artfs
    )

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
        solution_as_str = json.dumps(json.loads(str(solution)), indent=4)
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

    # Prepare stats for DataFrame
    stats = {
        "instance_name": instance["instance_filename"],
        "dataset_name": instance["dataset_name"],
        "best_approach": best_result["name"],
        "best_duration": best_result["duration"],
        "total_time_taken": (datetime.datetime.now() - start_time).total_seconds(),
        "bks_stored_duration": stored_duration_sum,
        "bks_recomp_dur_onyr": recomputed_duration_sum_onyr,
        "bks_recomp_dur_lera": recomputed_duration_sum_lera,
        "best_duration_per-diff_bks": percentage_difference(best_result["duration"], recomputed_duration_sum_onyr),
    }
    # Add each heuristic's duration and time_taken to stats
    for res in heuristic_results:
        stats[f"duration_{res['name'].lower().replace('-', '_')}"] = res["duration"]
        stats[f"time_{res['name'].lower().replace('-', '_')}"] = res["time_taken"]

    df_current_stats = pd.DataFrame(stats, index=[0])
    return df_current_stats
    

if __name__ == "__main__":
    # main()

    from benchmarks.bks import check_all_bks_duration
    check_all_bks_duration()

    # from benchmarks.bks import check_all_lera_bks
    # check_all_lera_bks()
