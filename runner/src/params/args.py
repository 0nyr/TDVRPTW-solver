import argparse, os

from .constants import *
from utils.terminal import *

def parse_program_args():
    """
    Define and parse the command line arguments for the program.
    """

    # Set command line parameters.
    arg_parser = argparse.ArgumentParser(description="Runs the experiment file(s) specified.")
    arg_parser.add_argument("experiments", metavar="EXP_FILE", help="JSON experiment file(s) with the experiments to run.", type=argparse.FileType('r'), nargs='+')
    arg_parser.add_argument("--instances", "-I", nargs="*", help="Only execute experiment(s) on selected instances (with these names).")
    arg_parser.add_argument("--exps", "-E", nargs="*", help="Only execute selected experiment(s) (with these names).")
    arg_parser.add_argument("--callgrind", "-C", help="Runs the experiment(s) using callgrind.", action="store_true")
    arg_parser.add_argument("--valgrind", "-V", help="Runs the experiment(s) using valgrind.", action="store_true")
    arg_parser.add_argument("--heaptrack", "-H", help="Runs the experiment(s) using heaptrack.", action="store_true")
    arg_parser.add_argument("--memlimit", "-M", help="Sets a memory limit in GB (default 15GB).", default=15)
    arg_parser.add_argument("--silent", "-S", help="Do not print the stderr stream of the experiments to the screen.", action="store_true")

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

    # The build type when running callgrind or valgrind is 'debug' otherwise it is 'release'.
    build_type = "debug" if use_callgrind or use_valgrind else "release"
    args["build_type"] = build_type

    print("Program arguments:")
    for key, value in args.items():
        msg = "    + " + str(key) + ": "
        if type(value) == list:
            msg += "[\n"
            for v in value:
                msg += str(v) + "\n"
            msg += "]"
        else:
            msg += str(value)
        print(blue(msg))
    print()
    
    return args