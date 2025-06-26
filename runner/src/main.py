import os, sys, time

from params.args import parse_program_args
from params.constants import PROJECT_ROOT_DIR

PROG_START_TIME = time.time()
print("Starting program [datetime: {}]".format(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(PROG_START_TIME))))

# Get Kairos-TDVRPTW directory and load the lib.
"""
Load the Kairos library based on the provided arguments.
"""
args = parse_program_args()
KAIROS_BUILD_TYPE =  args["build_type"].lower()
KAIROS_LIB_DIR = os.path.join(
    PROJECT_ROOT_DIR, "build", KAIROS_BUILD_TYPE, 
)
if not os.path.exists(KAIROS_LIB_DIR):
    raise FileNotFoundError(f"Kairos-TDVRPTW library not found at {KAIROS_LIB_DIR}")
sys.path.append(KAIROS_LIB_DIR)  # or wherever the .so is
import kairos_tdvrptw as ks


def main():

    ks.nyr.test_interval_vector_intersects()
    ks.nyr.test_interval_vector_includes()

    # Create intervals
    interval = ks.goc.Interval(0.0, 10.0)
    print("Interval:", interval)

    # # Create PWL functions
    breakpoints = [0.0, 1.0, 2.0, 3.0]
    values = [0.0, 2.0, 4.5, 10.0]
    test_ndcpwlf = ks.nyr.NDCPWLF(breakpoints, values)
    print("NDCPWLF:", test_ndcpwlf)

if __name__ == "__main__":
    main()
