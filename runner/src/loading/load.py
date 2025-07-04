from typing import Any
from utils.utils import read_json_from_file

import kairos_tdvrptw as ks

def load_instance(instance: dict[str, Any]):
    """
    Loads an instance JSON file and updates it with the filename.

    Args:
        instance (dict): Dictionary with 'instance_dirpath' and 'instance_filename' keys.

    Returns:
        dict: Loaded instance data with 'instance_filename' key updated.
    """
    instance_filepath = f"{instance['instance_dirpath']}/{instance['instance_filename']}"
    instance_json_data = read_json_from_file(instance_filepath)
    instance_json_data["instance_filename"] = instance["instance_filename"]
    return instance_json_data

def load_instance_to_tdvrptw_instance(instance: dict[str, Any]):
    """
    Loads an instance JSON file and converts it to a TDVRPTW instance.

    Args:
        instance (dict): Dictionary with 'instance_dirpath' and 'instance_filename' keys.

    Returns:
        ks.nyr.VRPInstance: Loaded TDVRPTW instance.
    """
    instance_json_data = load_instance(instance)
    tdvrptw_instance: ks.nyr.VRPInstance = ks.load_instance_from_json(instance_json_data)
    # print(tdvrptw_instance)
    artfs = ks.nyr.make_artfs(tdvrptw_instance)
    # print("ARTFs:", artfs)
    return tdvrptw_instance, artfs