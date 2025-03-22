import os, json

def get_csv_res(
        res: dict,
    ):
    """
    Get the CSV result from the experiment result.
    """

    # get current commit hash
    commit_hash = os.popen("git rev-parse HEAD").read().strip()

    # JSON stdout
    objective_value = float("inf")
    stdout_data = res["stdout"]
    status = "Unknown"
    lp_time = 0
    cut_time = 0
    pricing_time = 0
    nodes_closed = 0
    final_constraint_count = 0
    final_variable_count = 0
    if isinstance(stdout_data, dict):
        timed_solutions: list[dict] = stdout_data["timed_solutions"]
        # if empty, set to Infinity
        
        if len(timed_solutions) > 0:
            objective_value = timed_solutions[-1]["solution"]["value"]

        # check key "Exact" in stdout_data
        if "Exact" in stdout_data:
            exact = stdout_data["Exact"]
            status = exact["status"]
            lp_time = exact["lp_time"]
            cut_time = exact["cut_time"]
            pricing_time = exact["pricing_time"]
            nodes_closed = exact["nodes_closed"]
            final_constraint_count = exact["final_constraint_count"]
            final_variable_count = exact["final_variable_count"]

    return {
        "dataset_name": res["dataset_name"],
        "instance_filepath": os.path.join(res["instance_dirpath"], res["instance_filename"]),
        "experiment_name": res["experiment_name"],
        "exit_code": res["exit_code"],
        "time": res["time"],
        "status": status,
        "objective_value": objective_value,
        "commit_hash": commit_hash,
        "lp_time": lp_time,
        "cut_time": cut_time,
        "pricing_time": pricing_time,
        "nodes_closed": nodes_closed,
        "final_constraint_count": final_constraint_count,
        "final_variable_count": final_variable_count
    }