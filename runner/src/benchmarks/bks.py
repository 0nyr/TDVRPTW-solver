import pandas as pd
from typing import Any
import os
from enum import Enum

from utils.utils import read_json_from_file, join_paths, check_key_series_in_dict
from utils.terminal import purple, green
from utils.math import percentage_difference
from running.experiment import instances_for_experiment
from params.constants import INSTANCES_DIR, RUNNER_START_TIME, OPTIMIZATION_OBJECTIVE
from loading.load import load_instance_to_tdvrptw_instance, load_all_instances_and_solutions
from output.latex import generate_latex_longtable
from utils.formatting import format_date_for_filepath
from dataclasses import dataclass, asdict

import kairos_tdvrptw as ks


class SolutionStatus(Enum):
    OPTIMAL = "optimal"
    HEURISTIC = "heuristic"
    MISSING = "missing"

    def __str__(self):
        return self.value

@dataclass
class BKSCheckStats:
    """
    Stats for a BKS (Best Known Solution) for a given instance.
    Recomputes the Duration sum for the BKS using both the
    original Lera method and the new ONYR method.
    """
    instance_name: str
    dataset_name: str
    bks_stored_duration: float
    bks_recomp_dur_onyr: float
    bks_recomp_dur_lera: float
    percentage_diff_lera_onyr: float
    percentage_diff_stored_onyr: float
    is_stored_bks_correct: bool
    solution_status: SolutionStatus

    def to_dict(self):
        return asdict(self)

    def to_dataframe(self):
        return pd.DataFrame([self.to_dict()], index=[0])


def load_original_lera_Duration_bks():
    """
    Load the original Lera BKS durations from a JSON file.
    """
    LERA_SOLUTIONS_FILEPATH = os.path.join(
        INSTANCES_DIR,
        "../",
        "benchmarks/tdvrptw/Lera2019/dabia_et_al_2013/solutions.json"
    )
    lera_bks = read_json_from_file(LERA_SOLUTIONS_FILEPATH)
    return lera_bks

def get_solution_status(solution: dict[str, Any]) -> SolutionStatus:
    """
    Check if the solution is marked as optimal.
    The solution is marked as optimal if it has a "tags" key with "OPT" in it.
    """
    if solution is None:
        return SolutionStatus.MISSING

    # Lera's solution format has a "tags" key that is a list.
    # We check if "OPT" is in the tags list.
    if "tags" in solution and isinstance(solution["tags"], list):
        if "OPT" in solution["tags"]:
            return SolutionStatus.OPTIMAL
        else:
            return SolutionStatus.HEURISTIC
    
    # Onyr's solution format has a "status" key that is a string.
    if "status" in solution and isinstance(solution["status"], str):
        if solution["status"].lower() == "optimum":
            return SolutionStatus.OPTIMAL
        else: 
            return SolutionStatus.HEURISTIC
        
    return SolutionStatus.MISSING

def recompute_route_duration(
    route: list[int],
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
):
    recomputed_duration_onyr: ks.nyr.RouteDuration = ks.nyr.compute_RouteDuration_from_scratch(
        artfs, route
    )
    print(green(f"  Recomputed duration for route (onyr) {route}: {recomputed_duration_onyr}"))
    
    recomputed_duration_lera: ks.nyr.RouteDuration = ks.nyr.compute_RouteDuration_lera(
        tdvrptw_instance, route
    )
    print(green(f"  Recomputed duration for route (lera) {route}: {recomputed_duration_lera}"))
    return recomputed_duration_onyr, recomputed_duration_lera

def recompute_bks_duration(
    solution: dict,
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
):
    """
    Given a solution (generally a BKS) and the TDVRPTW instance
    from which it was derived, recompute the solution's route durations.
    This is useful to ensure that the stored durations in the solution
    are correct. 
    """
    # If solution is missing, skip it
    if solution is None:
        return (ks.goc.INFTY, ks.goc.INFTY, ks.goc.INFTY)

    if "solution" in solution:
        solution = solution["solution"]

    if "routes" in solution:
        # Recompute the solution route durations to ensure they are correct.
        solution_routes: list[dict] = solution["routes"]
        recomputed_duration_sum_onyr = 0
        recomputed_duration_sum_lera = 0
        for route_obj in solution_routes:
            route: list[int] = route_obj["path"]
            stored_duration = route_obj["duration"]
            print(purple(f"Recomputing duration for route {route}... (stored: {stored_duration}):"))
            (recomputed_duration_onyr, recomputed_duration_lera) = recompute_route_duration(
                route, tdvrptw_instance, artfs
            )
            recomputed_duration_sum_onyr += recomputed_duration_onyr.value
            recomputed_duration_sum_lera += recomputed_duration_lera.value
            print(green(f"Recomputed duration for route (lera) {route}: {recomputed_duration_lera} (stored: {stored_duration})"))

        stored_duration_sum = solution["value"]
        print(green(f"Total recomputed duration: {recomputed_duration_sum_onyr} (stored: {stored_duration_sum})"))
        
        return (stored_duration_sum, recomputed_duration_sum_onyr, recomputed_duration_sum_lera)
    
    else:
        return (ks.goc.INFTY, ks.goc.INFTY, ks.goc.INFTY)

def check_bks(
    instance_name: str,
    dataset_name: str,
    solution,
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
) -> BKSCheckStats:
    # Recompute the BKS duration in 2 different ways:
    # 1. Using the 0nyr method with `NDCPWLF`s
    # 2. Using the original Lera method using `PWLFunction`s.`
    (
        stored_duration_sum, 
        recomputed_duration_sum_onyr, 
        recomputed_duration_sum_lera
    ) = recompute_bks_duration(
        solution, 
        tdvrptw_instance, 
        artfs
    )
    percentage_diff_lera_onyr = percentage_difference(
        recomputed_duration_sum_lera, recomputed_duration_sum_onyr
    )

    # Determine if the stored BKS is correct
    is_stored_bks_correct = True
    if (stored_duration_sum < ks.goc.INFTY and
        recomputed_duration_sum_onyr >= ks.goc.INFTY):
        print(purple(f"WARNING: Stored BKS duration {stored_duration_sum} is invalid (recomputed duration is INFTY)."))
        is_stored_bks_correct = False
    
    # Determine if the BKS is marked as Optimal
    solution_status = get_solution_status(solution)
    percentage_diff_stored_onyr = percentage_difference(
        stored_duration_sum, recomputed_duration_sum_onyr
    )

    # Prepare stats as a BKSStats dataclass and return
    stats = BKSCheckStats(
        instance_name=instance_name,
        dataset_name=dataset_name,
        bks_stored_duration=stored_duration_sum,
        bks_recomp_dur_onyr=recomputed_duration_sum_onyr,
        bks_recomp_dur_lera=recomputed_duration_sum_lera,
        percentage_diff_lera_onyr=percentage_diff_lera_onyr,
        percentage_diff_stored_onyr=percentage_diff_stored_onyr,
        is_stored_bks_correct=is_stored_bks_correct,
        solution_status=solution_status,
    )
    return stats

def check_all_lera_bks():
    """
    Check all Lera BKS (Duration) by recomputing their durations
    """
    lera_bks: list[dict] = load_original_lera_Duration_bks()
    instances = instances_for_experiment(
        {"datasets": [{"name": "Dabia2013"}]}, None
    )
    lera_bks_to_instance: list[tuple] = []

    def get_instance_from_lera_codename(
            codename: str, instances: list[dict[str, Any]]
        ):
        """
        Get the instance from the Lera codename.
        The codename is in the format "C102_25" where:
        - C102 is the Solomon instance code
        - 25 is the number of clients

        We need to find the matching instance with:
        - instance_filename: {solomon_code}-n={num_clients + 2}-...json
        """
        # Find the instance in the instances list
        solomon_code, num_clients = codename.split("_")
        for instance in instances:
            instance_filename = instance["instance_filename"]
            if instance_filename.startswith(f"{solomon_code}-n={int(num_clients) + 2}-"):
                return instance
        
        raise ValueError(f"Could not find instance for Lera codename: {codename}")
        
    # Pair each Lera BKS with its corresponding instance
    for bks in lera_bks:
        lera_instance_codename = bks["instance_name"] # ex: "C102_25"
        matching_instance = get_instance_from_lera_codename(
            lera_instance_codename, instances
        )
        lera_bks_to_instance.append(
            (lera_instance_codename, matching_instance, bks)
        )
    
    # Check each Lera BKS
    df_bks_stats = pd.DataFrame()
    for lera_instance_codename, instance, bks in lera_bks_to_instance:
        print(purple(f"Checking Lera BKS for {lera_instance_codename}..."))
        (tdvrptw_instance, artfs) = load_instance_to_tdvrptw_instance(instance)

        df_bks_stats = pd.concat(
            [
                df_bks_stats,
                check_bks(
                    lera_instance_codename,
                    instance["dataset_name"],
                    bks,
                    tdvrptw_instance,
                    artfs
                ).to_dataframe()
            ],
            ignore_index=True
        )
    
    # Print the full results
    print(f"\nFull Lera BKS check results:\n")
    print(df_bks_stats.to_markdown(index=False))

    # Print number of BKS that are correct over total
    num_bks_correct = df_bks_stats["is_stored_bks_correct"].sum()
    num_bks_total = len(df_bks_stats)
    print(f"\nNumber of BKS that are correct: {num_bks_correct} / {num_bks_total} ({num_bks_correct / num_bks_total * 100:.2f}%)")
    print(f"Number of BKS that are incorrect: {num_bks_total - num_bks_correct} / {num_bks_total} ({(num_bks_total - num_bks_correct) / num_bks_total * 100:.2f}%)")
    # Print number of incorrect BKS where n=100 (in codename)
    num_incorrect_n100 = df_bks_stats[
        (~df_bks_stats["is_stored_bks_correct"]) &
        (df_bks_stats["instance_name"].str.endswith("_100"))
    ].shape[0]
    print(f"Number of incorrect BKS with n=100: {num_incorrect_n100}")

    print()
    # Remove the "dataset_name" column before generating the LaTeX table
    df_bks_stats = df_bks_stats.drop(columns=["dataset_name"])
    print(generate_latex_longtable(
        df_bks_stats,
        table_caption=r"Lera-Romero BKS check results. The BKS Duration is recomputed using both the original corrected Lera \texttt{PWLFunction} composition method (column \texttt{bks recomp dur lera}) and the new (onyr) \texttt{NDCPWLF} composition method (column \texttt{bks recomp dur onyr}), with the percentage difference between the two given in the following column.",
        table_label="tab:lera_bks_check"
    ))

    # Remove BKS from lera_bks that are not correct
    to_remove = []
    for bks in lera_bks:
        if not df_bks_stats[df_bks_stats["instance_name"] == bks["instance_name"]]["is_stored_bks_correct"].bool():
            print(purple(f"Removing incorrect BKS: {bks['instance_name']}"))
            to_remove.append(bks)
    
    nb_removed = 0
    for bks in to_remove:
        lera_bks.remove(bks)
        nb_removed += 1
    
    print(f"Removed {nb_removed} incorrect BKS from the original Lera BKS list.")

    # Save the correct BKS to a file
    LERA_SOLUTIONS_FILEPATH = os.path.join(
        INSTANCES_DIR,
        "../",
        "benchmarks/tdvrptw/Lera2019/dabia_et_al_2013/lera_bks_checked.json"
    )
    print(f"Saving correct Lera BKS to {LERA_SOLUTIONS_FILEPATH}...")
    with open(LERA_SOLUTIONS_FILEPATH, "w") as f:
        import json
        json.dump(lera_bks, f, indent=4)

def check_all_bks_duration():
    """
    Check all BKS (Duration) for all datasets by recomputing their durations.
    """
    all_instance_solution_pairs = load_all_instances_and_solutions()
    df_bks_stats = pd.DataFrame()
    for instance, solution in all_instance_solution_pairs:
        print(purple(f"Checking BKS for {instance['instance_filename']}..."))
        instance_name = instance["instance_filename"]
        dataset_name = instance["dataset_name"]

        stats = None
        # In case the solution is None (missing)
        if solution is None:
            print(purple(f"WARNING: No solution found for instance {instance_name}."))
            stats = pd.DataFrame(
                {
                    "instance_name": instance_name,
                    "dataset_name": dataset_name,
                    "bks_stored_duration": ks.goc.INFTY,
                    "bks_recomp_dur_onyr": ks.goc.INFTY,
                    "bks_recomp_dur_lera": ks.goc.INFTY,
                    "percentage_diff_lera_onyr": 0,
                    "percentage_diff_stored_onyr": 0,
                    "is_stored_bks_correct": True,
                    "solution_status": SolutionStatus.MISSING
                }, 
                index=[0]
            )
        else:
            (tdvrptw_instance, artfs) = load_instance_to_tdvrptw_instance(instance)
            stats = check_bks(
                instance["instance_filename"],
                instance["dataset_name"],
                solution,
                tdvrptw_instance,
                artfs
            ).to_dataframe()

        df_bks_stats = pd.concat(
            [
                df_bks_stats,
                stats
            ],
            ignore_index=True
        )
    
    # Print the full results
    print(f"\nFull BKS check results:\n")
    print(df_bks_stats.to_markdown(index=False))

    # Save as CSV file
    from params.constants import PROJECT_ROOT_DIR
    bks_check_csv_filepath = join_paths(
        PROJECT_ROOT_DIR,
        "out/res/"
        f"bks_check_results_{format_date_for_filepath(RUNNER_START_TIME)}.csv"
    )
    print(f"Saving BKS check results to {bks_check_csv_filepath}...")
    df_bks_stats.to_csv(bks_check_csv_filepath, index=False)

    return all_instance_solution_pairs, df_bks_stats

def remove_bks_from_storage(
    instance_dirpath: str,
    instance_filename: str,
):
    """
    Remove the BKS file from storage.
    The BKS file is stored in the instance directory with the name "solution.json".
    """
    solutions_filepath = join_paths(
        instance_dirpath,
        "solutions.json"
    )
    
    if os.path.isfile(solutions_filepath):
        solution_data = read_json_from_file(solutions_filepath)
        if check_key_series_in_dict(solution_data, [OPTIMIZATION_OBJECTIVE, instance_filename]):
            # Remove the key [OPTIMIZATION_OBJECTIVE][instance_filename] from the solution_data dict
            del solution_data[OPTIMIZATION_OBJECTIVE][instance_filename]
            # Save the updated solution_data back to the file
            with open(solutions_filepath, "w") as f:
                import json
                json.dump(solution_data, f, indent=4)

            print(purple(f"  > Removed BKS for instance {instance_filename} from storage..."))
            return True
        else:
            print(purple(f"WARNING: BKS marked for removal for instance {instance_filename} not found in storage."))
            return False
    else:
        print(purple(f"WARNING: No BKS file found for instance {instance_filename} in storage."))

    return False

def remove_all_incorrect_bks(
    df_bks_stats: pd.DataFrame,
    all_instance_solution_pairs: list[tuple[dict[str, Any], dict[str, Any]]]
):
    """
    Remove all incorrect BKS from storage.
    """
    nb_removed = 0
    for instance, _ in all_instance_solution_pairs:
        instance_filename = instance["instance_filename"]
        
        # Get the BKS stats for this instance
        bks_stats = df_bks_stats[df_bks_stats["instance_name"] == instance_filename]
        if bks_stats.empty:
            print(purple(f"WARNING: No BKS stats found for instance {instance_filename}."))
            continue
        else:
            is_stored_bks_correct = bks_stats["is_stored_bks_correct"].bool()
            if not is_stored_bks_correct:
                print(purple(f"Removing incorrect BKS for instance {instance_filename}..."))
                is_removed = remove_bks_from_storage(
                    instance["instance_dirpath"],
                    instance_filename
                )
                if is_removed:
                    nb_removed += 1

    print(green(f"Total number of incorrect BKS removed: {nb_removed}"))

def check_all_and_remove_incorrect_bks():
    """
    Check all BKS (Duration) for all datasets by recomputing their durations.
    If the BKS is incorrect, remove it from storage.
    """
    (all_instance_solution_pairs, df_bks_stats) = check_all_bks_duration()
    
    # Remove all incorrect BKS from storage
    remove_all_incorrect_bks(df_bks_stats, all_instance_solution_pairs)
