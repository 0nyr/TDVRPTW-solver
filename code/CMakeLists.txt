cmake_minimum_required(VERSION 3.10)
project(solver)

# specify C++ 26 standard
set(CMAKE_CXX_STANDARD 26)
message("-- C++ version: ${CMAKE_CXX_STANDARD}")

# allow three build‐types: Release, Debug, Fastdebug
set(VALID_BUILD_TYPES Release Debug Fastdebug)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Fastdebug CACHE STRING "Choose build type: ${VALID_BUILD_TYPES}" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# common flags
set(COMMON_FLAGS "-pthread -m64")
set(ARCH_OPTI_FLAGS "-march=native -mtune=native")
string(APPEND CMAKE_CXX_FLAGS        " ${COMMON_FLAGS}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${COMMON_FLAGS}")

# sanitizer flags for plain Debug only
set(UBSAN_FLAGS "-fsanitize=float-divide-by-zero,float-cast-overflow")

# set flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "-- Compiling in Release Mode")
  string(APPEND CMAKE_CXX_FLAGS " -O3 -DNDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "-- Compiling in Debug Mode (UBSan & logging)")
  string(APPEND CMAKE_CXX_FLAGS        " -g -Og -Wall -Wextra ${UBSAN_FLAGS}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${UBSAN_FLAGS}")
  # preserve full logging (no extra defines)
elseif(CMAKE_BUILD_TYPE STREQUAL "Fastdebug")
  message(STATUS "-- Compiling in Fastdebug Mode (UBSan, no logging, quick iter)")
  # a light optimization level to keep compile+link cheap
  string(APPEND CMAKE_CXX_FLAGS " -g -O1 -Wall -Wextra ${UBSAN_FLAGS}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${UBSAN_FLAGS}")
else()
  message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. Valid types: ${VALID_BUILD_TYPES}")
endif()

# used to generate compile_commands.json for VSCode intellisense/warnings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# Tell GCC to use the plugin-based linker and allow real parallel LTO
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}
  -fuse-linker-plugin
  -fuse-ld=gold"    # or -fuse-ld=lld if you prefer LLVM’s linker
)

# Add main include directory.
include_directories("main/include/")

# Include magic_enum.
include_directories(magic_enum/include)

# Including GOC.
add_subdirectory(goc)
include_directories(goc/include)

# Include NYR.
add_subdirectory(nyr)
include_directories(nyr/include)

# Create library with source codes.
add_library(
    solver 
    main/src/preprocess/load_igp.cpp 
    main/src/preprocess/preprocess_utils.cpp
    main/src/preprocess/preprocess_capacity.cpp 
    main/src/preprocess/preprocess_time_windows.cpp 
    main/src/preprocess/preprocess_service_waiting.cpp 
    main/src/preprocess/preprocess_travel_times.cpp 
    main/src/preprocess/preprocess_triangle_depot.cpp 
    main/src/preprocess/preprocess_ng_neighborhoods.cpp 
    main/src/preprocess/preprocess_validity.cpp 
    # src/labeling/label.cpp 
    # src/labeling/monodirectional_labeling.cpp 
    # src/labeling/lazy_label.cpp 
    # src/labeling/pwl_domination_function.cpp 
    # src/labeling/bidirectional_labeling.cpp 
    # src/labeling/ng_neighborhoods.cpp 
    # src/bcp/pricing_problem.cpp 
    # src/bcp/spf.cpp 
    # src/bcp/bcp.cpp
    main/src/heuristics/greedy_makespan.cpp
    main/src/heuristics/aco.cpp
    main/src/heuristics/greedy_duration.cpp
)
target_link_libraries(solver goc nyr) # add compiled libs

# Find and include pybind11.
set(PYBIND11_FINDPYTHON ON)
find_package(Python  REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
pybind11_add_module(
    kairos_tdvrptw 
    bindings/src/bindings.cpp
)
# make sure we link in the goc/solver code that defines LinearFunction, etc.
target_link_libraries(kairos_tdvrptw
    PRIVATE
      goc    # GOC library by Lera-Romero
      nyr    # NYR library, on top of GOC, by 0nyr     
      solver # main solver library
)
target_include_directories(kairos_tdvrptw 
  PRIVATE 
  ${CMAKE_SOURCE_DIR}/bindings/include
)

# Create binaries.
# add_executable(pricing src/main_pricing.cpp)
# target_link_libraries(pricing solver)

# Test binary.
# add_executable(test_pwlf main/src/main_test_pwlf.cpp)
# target_link_libraries(test_pwlf solver)

add_executable(bp main/src/main_bp.cpp)
target_link_libraries(bp solver)