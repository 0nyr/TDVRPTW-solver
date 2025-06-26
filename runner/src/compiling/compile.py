import os,subprocess, datetime

from typing import Any
from utils.terminal import purple, green, red
from utils.utils import create_dir
from params.constants import CMAKELISTS_DIR, OBJ_DIR, PROJECT_ROOT_DIR

def clean_build_dir(
        build_type: str
    ):
    """
    Deletes the build directory.
	"""
    print(purple("Cleaning build directory"), flush=True)
    build_dirpath = os.path.join(OBJ_DIR, build_type)
    if os.path.isdir(build_dirpath):
        # ensure build_dirpath is within the current repo
        if not build_dirpath.startswith(PROJECT_ROOT_DIR):
            raise ValueError(F"build_dirpath: {build_dirpath} is not within the current repository: {PROJECT_ROOT_DIR}")
        subprocess.call(["rm", "-rf", build_dirpath])

def compile(
        args: dict[str, Any]
    ) -> bool:
    """
    Compiles the CMakeLists.txt in CMAKELISTS_DIR specified in the config.
    Saves the compilation files in OBJ_DIR.
    Returns: if the compilation process was successful.
    """
    build_type: str = args["build_type"]
    clean_build: bool = args["clean_build"]

    # Clean build directory.
    if clean_build: clean_build_dir(build_type)

    # Create /obj directory.
    create_dir(OBJ_DIR)
    create_dir(F"{OBJ_DIR}/{build_type}")

    # Get number of processors.
    num_processors = os.cpu_count()
    if num_processors is None:
        raise ValueError("Could not get number of processors.")

    # Compile project using cmake.
    cmake_cmd = [
        "cmake", 
        F"{CMAKELISTS_DIR}", 
        F"-DCMAKE_BUILD_TYPE={build_type.capitalize()}", 
        # F"-DRUNNER=ON",
    ]
    make_cmd = [
        "make",
        F"-j{num_processors}",
    ]
    print(purple("Compiling code with: "), flush=True)
    print(purple(" "*4 + " ".join(cmake_cmd)), flush=True)
    print(purple(" "*4 + " ".join(make_cmd)), flush=True)

    t0 = datetime.datetime.now()
    os.chdir(F"{OBJ_DIR}/{build_type}")
    exit_code = subprocess.call(cmake_cmd)
    if exit_code == 0: exit_code = subprocess.call(make_cmd)
    os.chdir(PROJECT_ROOT_DIR)
    if exit_code == 0:
        print(green(F"Finished compiling - Time: {(datetime.datetime.now() - t0).total_seconds()} sec."), flush=True)
    else:
        print(red(F"Compilation failed - Time: {(datetime.datetime.now() - t0).total_seconds()} sec."), flush=True)
    return exit_code == 0
