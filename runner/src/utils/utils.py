import json, os

def read_json_from_file(file_path: str):
    """
    Load JSON content from a file.
    """
    with open(file_path, "r") as f:
        return json.loads(f.read())

def save_json_to_file(file_path, json_object):
    """
    Save JSON content to a file.
    """
    with open(file_path, "w") as f:
        f.write(json.dumps(json_object, indent=4))

def create_dir(dir_path):
    """
    Creates the directory at the specified path if it does not exist.
    """
    if not os.path.isdir(dir_path): 
        os.mkdir(dir_path)

def get_a_parent_dir(
        current_dir: str, # the current directory
        parenting_level: int, # the number of parent directories to go up
    ) -> str:
    """
    Returns the path of the parent directory of the current directory.
    """
    parent_dir = current_dir
    if parenting_level > 0:
        for _ in range(parenting_level):
            parent_dir = os.path.abspath(os.path.join(parent_dir, os.pardir))
    return parent_dir

def check_file_exists(filepath: str):
    """
    Check if a file exists.
    """
    if not os.path.isfile(filepath):
        raise FileNotFoundError(f"File: {filepath} does not exist.")

def check_files_exist(filepaths: list[str]):
    """
    Check if a list of files exist.
    """
    for filepath in filepaths:
        check_file_exists(filepath)

def save_csv_to_file(file_path: str, csv_content: dict):
    """
    Save a CSV content to a file.
    If the file doesn't exists, it will be created,
    with the header as the first line.
    """
    parent_dir = os.path.dirname(file_path)
    create_dir(parent_dir)

    if not os.path.isfile(file_path):
        with open(file_path, "w") as f:
            f.write(f"{';'.join(csv_content.keys())}\n")
    
    with open(file_path, "a") as f:
        f.write(f"{';'.join(map(str, csv_content.values()))}\n")