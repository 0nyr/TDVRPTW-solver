import os,subprocess, datetime

from utils.terminal import purple, green, red
from utils.utils import create_dir
from params.constants import CMAKELISTS_DIR, OBJ_DIR, PROJECT_ROOT_DIR

def compile(
        build_type: str
    ) -> bool:
	"""
	Compiles the CMakeLists.txt in CMAKELISTS_DIR specified in the config.
    Saves the compilation files in OBJ_DIR.
    Returns: if the compilation process was successful.
    """
	# Create /obj directory.
	create_dir(OBJ_DIR)
	create_dir(F"{OBJ_DIR}/{build_type}")

	# Compile project using cmake.
	print(purple("Compiling code"), flush=True)
	t0 = datetime.datetime.now()
	os.chdir(F"{OBJ_DIR}/{build_type}")
	exit_code = subprocess.call(["cmake", F"{CMAKELISTS_DIR}", F"-DCMAKE_BUILD_TYPE={build_type}", F"-DRUNNER=ON"])
	if exit_code == 0: exit_code = subprocess.call(["make"])
	os.chdir(PROJECT_ROOT_DIR)
	if exit_code == 0:
		print(green(F"Finished compiling - Time: {(datetime.datetime.now() - t0).total_seconds()} sec."), flush=True)
	else:
		print(red(F"Compilation failed - Time: {(datetime.datetime.now() - t0).total_seconds()} sec."), flush=True)
	return exit_code == 0
