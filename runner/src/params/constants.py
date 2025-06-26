import os, datetime

from utils.terminal import blue
from utils.utils import get_a_parent_dir, read_json_from_file

# Directories.
RUNNER_DIR = get_a_parent_dir(os.path.dirname(__file__), 2) # Where Python runner files are located.
PROJECT_ROOT_DIR = get_a_parent_dir(RUNNER_DIR, 1) # Root of the repository.

# Load runner config file where configurations are stored.
CONFIG = read_json_from_file(F"{RUNNER_DIR}/src/config.json")

# Set directories.
OUTPUT_DIR =  os.path.abspath(os.path.join(RUNNER_DIR, CONFIG["output_dir"])) # Directory where output files should be saved.
CMAKELISTS_DIR = os.path.abspath(os.path.join(RUNNER_DIR, CONFIG["cmakelists_dir"])) # Directory that contains the root CMakeLists.txt file to compile the project.
INSTANCES_DIR = os.path.abspath(os.path.join(RUNNER_DIR, CONFIG["instances_dir"])) # Directory where datasets are stored.
OBJ_DIR = F"{PROJECT_ROOT_DIR}/build" # Directory where the object files will be created.

# date formats
FILEPATH_DATE_FORMAT = "%Y-%m-%d-%H-%M-%S"
CONSOLE_DATE_FORMAT = "%Y-%m-%d %H:%M:%S"
RUNNER_START_TIME = datetime.datetime.now()

# Print constants.
print("Constants:")
print(blue(f"    + RUNNER_DIR: {RUNNER_DIR}"))
print(blue(f"    + PROJECT_ROOT_DIR: {PROJECT_ROOT_DIR}"))
print(blue(f"    + OUTPUT_DIR: {OUTPUT_DIR}"))
print(blue(f"    + CMAKELISTS_DIR: {CMAKELISTS_DIR}"))
print(blue(f"    + INSTANCES_DIR: {INSTANCES_DIR}"))
print(blue(f"    + OBJ_DIR: {OBJ_DIR}"))
print()