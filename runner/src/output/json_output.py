import os, json, datetime

from params.constants import RUNNER_START_TIME
from utils.formatting import format_date_for_console

def complete_res_json(
        res: dict,
        start_time: datetime.datetime,
        end_time: datetime.datetime,
    ):
    """
    Add additional fields to the JSON result:
        + the commit of the current solver repo
        + machine information(host name, CPU, system, etc.)
    """
    # Get current commit hash
    commit_hash = os.popen("git rev-parse HEAD").read().strip()

    # Add timestamps
    runner_timestamps = {
        "runner_start_time": format_date_for_console(RUNNER_START_TIME),
        "instexp_start_time": format_date_for_console(start_time),
        "instexp_end_time": format_date_for_console(end_time),
        "instexp_duration": (end_time - start_time).total_seconds()
    }
    res["runner_timestamps"] = runner_timestamps

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

