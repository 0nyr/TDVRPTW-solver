import pandas as pd
from typing import Any
import os

from utils.utils import read_json_from_file, join_paths, check_key_series_in_dict
from utils.terminal import purple, green
from utils.math import percentage_difference
from running.experiment import instances_for_experiment
from params.constants import INSTANCES_DIR
from loading.load import load_instance_to_tdvrptw_instance

import kairos_tdvrptw as ks

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
):
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
    solution_status = ""
    if isinstance(solution["tags"], list) and len(solution["tags"]) > 0:
        solution_status = "optimal" if "OPT" in solution["tags"] else ""

    # Prepare stats for DataFrame and return
    stats = {
        "instance_name": instance_name,
        "dataset_name": dataset_name,
        "bks_stored_duration": stored_duration_sum,
        "bks_recomp_dur_onyr": recomputed_duration_sum_onyr,
        "bks_recomp_dur_lera": recomputed_duration_sum_lera,
        "percentage_diff_lera_onyr": percentage_diff_lera_onyr,
        "is_stored_bks_correct": is_stored_bks_correct,
        "marked_optimal": solution_status,
    }
    return pd.DataFrame(stats, index=[0])

def check_all_lera_bks():
    """
    Check all Lera BKS (Duration) by recomputing their durations
    """
    lera_bks = load_original_lera_Duration_bks()
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
                )
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