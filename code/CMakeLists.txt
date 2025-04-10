cmake_minimum_required(VERSION 3.10)
project(solver)

# specify C++ 26 standard
set(CMAKE_CXX_STANDARD 26)
message("-- C++ version: ${CMAKE_CXX_STANDARD}")

# Set compiler flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -m64")
if ("${CMAKE_BUILD_TYPE}" MATCHES release OR "${CMAKE_BUILD_TYPE}" MATCHES Release)
    message("-- Compiling in Release Mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
elseif ("${CMAKE_BUILD_TYPE}" MATCHES debug OR "${CMAKE_BUILD_TYPE}" MATCHES Debug)
    message("-- Compiling in Debug Mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og -Wall -Wextra")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # used to generate compile_commands.json for VSCode intellisense/warnings

# Add include directory.
include_directories("include/")

# Including GOC.
add_subdirectory(goc)
include_directories(goc/include)

# Create library with source codes.
add_library(
    solver 
    src/instance/vrp_instance.cpp 
    src/instance/load_igp.cpp 
    src/preprocess/preprocess_utils.cpp
    src/preprocess/preprocess_capacity.cpp 
    src/preprocess/preprocess_time_windows.cpp 
    src/preprocess/preprocess_service_waiting.cpp 
    src/preprocess/preprocess_travel_times.cpp 
    src/preprocess/preprocess_triangle_depot.cpp 
    src/preprocess/preprocess_ng_neighborhoods.cpp 
    src/preprocess/preprocess_validity.cpp 
    src/labeling/label.cpp 
    src/labeling/monodirectional_labeling.cpp 
    src/labeling/lazy_label.cpp 
    src/labeling/pwl_domination_function.cpp 
    src/labeling/bidirectional_labeling.cpp 
    src/labeling/ng_neighborhoods.cpp 
    src/bcp/pricing_problem.cpp 
    src/bcp/spf.cpp 
    src/bcp/bcp.cpp
    src/heuristics/greedy_makespan.cpp
)
target_link_libraries(solver goc)

# Create binaries.
# add_executable(pricing src/main_pricing.cpp)
# target_link_libraries(pricing solver)

add_executable(bp src/main_bp.cpp)
target_link_libraries(bp solver)