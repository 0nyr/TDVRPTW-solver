import argparse, os

from .constants import *
from utils.terminal import *
from utils.utils import check_files_exist

def parse_program_args():
    """
    Define and parse the command line arguments for the program.
    """

    # Set command line parameters.
    arg_parser = argparse.ArgumentParser(description="Runs the experiment file(s) specified.")
    arg_parser.add_argument("experiments", metavar="EXP_FILE", help="JSON experiment file(s) with the experiments to run.", type=str, nargs='*')
    arg_parser.add_argument("--instances", "-I", nargs="*", help="Only execute experiment(s) on selected instances (with these names).")
    arg_parser.add_argument("--exps", "-E", nargs="*", help="Only execute selected experiment(s) (with these names).")
    arg_parser.add_argument("--carry-on", "-CO", help="Carry on the experiment from provided .csv output file.", type=str)
    arg_parser.add_argument("--callgrind", "-C", help="Runs the experiment(s) using callgrind.", action="store_true")
    arg_parser.add_argument("--valgrind", "-V", help="Runs the experiment(s) using valgrind.", action="store_true")
    arg_parser.add_argument("--heaptrack", "-H", help="Runs the experiment(s) using heaptrack.", action="store_true")
    arg_parser.add_argument("--memlimit", "-M", help="Sets a memory limit in GB (default 15GB).", default=15)
    arg_parser.add_argument("--silent", "-S", help="Do not print the stderr stream of the experiments to the screen.", action="store_true")
    arg_parser.add_argument("--clean-build", help="Clean the obj/ directory before compiling.", action="store_true")
    arg_parser.add_argument("--dry-run", help="Do not run the experiments, only compile the code and load instances and experiments.", action="store_true")
    arg_parser.add_argument("--just-compile", "-c", help="Only compile the code and exit.", action="store_true")
    arg_parser.add_argument("--save-bks", "-sbks", help="Save new found BKS.", action="store_true")
    arg_parser.add_argument(
        "--build-type","-b",
        choices=("debug","release","fastdebug","all"),
        default="all",
        help="Which configuration to build"
    )

    # Read command line parameters.
    args = vars(arg_parser.parse_args())
    experiment_files = args["experiments"]
    selected_instances = args["instances"]
    selected_experiments = args["exps"]
    use_callgrind = args["callgrind"]
    use_valgrind = args["valgrind"]
    use_heaptrack = args["heaptrack"]
    memlimit_gb = args["memlimit"]
    silent = args["silent"]

    # The build type when running callgrind or valgrind must not be release.
    if use_callgrind or use_valgrind or use_heaptrack:
        if args["build_type"] == "release":
            print(red("Cannot run callgrind or valgrind with release build type."))
            print(red("Please use debug, or fastdebug build type."))
            exit(1)

    # file checks
    check_files_exist(experiment_files)
    if args["carry_on"] is not None:
        check_files_exist([args["carry_on"]])

    print("Program arguments:")
    for key, value in args.items():
        msg = "    + " + str(key) + ": "
        if type(value) == list and len(value) > 0:
            msg += "[\n"
            for v in value:
                msg += " "*8 + str(v) + "\n"
            msg += "    ]"
        else:
            msg += str(value)
        print(blue(msg))
    print()

    return args
