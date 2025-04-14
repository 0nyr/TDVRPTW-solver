import os, json

def complete_res_json(
        res: dict,
    ):
    """
    Add additional fields to the JSON result:
        + the commit of the current solver repo
        + machine information(host name, CPU, system, etc.)
    """
    # Get current commit hash
    commit_hash = os.popen("git rev-parse HEAD").read().strip()

    # Get machine information
    machine_info = os.popen("uname -a").read().strip()
    machine_info = machine_info.split(" ")
    machine_info = {
        "host_name": machine_info[1],
        "system": machine_info[0],
        "kernel_version": machine_info[2],
        "architecture": machine_info[3]
    }

    # Add commit hash and machine info to the result
    res["commit_hash"] = commit_hash
    res["machine_info"] = machine_info
    return res

