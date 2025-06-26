cmake_minimum_required(VERSION 3.30)
project(nyr)

# Inherit flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD 26)

# Include magic_enum
include_directories(${CMAKE_SOURCE_DIR}/magic_enum/include)

# Include magic_enum
include_directories(${CMAKE_SOURCE_DIR}/magic_enum/include)

# Include GOC (assumes the parent scope provides it)
include_directories(${CMAKE_SOURCE_DIR}/goc/include)

# Add nyr includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find Boost and add includes
set(Boost_USE_STATIC_LIBS OFF)  # dynamic or static linking. Default on NixOs is dynamic.
set(Boost_USE_MULTITHREADED ON)
set(Boost_NO_SYSTEM_PATHS ON) # Prevents CMake from searching system paths
set(BOOST_ROOT "$ENV{BOOST_BIN}")
set(BOOST_INCLUDE_DIRS "$ENV{BOOST_INCLUDE}")
set(Boost_LIBRARY_DIR "$ENV{BOOST_BIN}")
find_package(Boost REQUIRED COMPONENTS graph system filesystem)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found! Make sure BOOST_INCLUDE and BOOST_BIN are correctly set.")
endif()
include_directories(${Boost_INCLUDE_DIRS}) # Add Boost headers

# Create nyr library
add_library(
    nyr 
    src/time/time.cpp
    src/math/interval.cpp
    src/math/ndcpwlf.cpp
    src/vrp/instance.cpp
    src/solutions/objectives.cpp
    src/solutions/route.cpp
    src/solutions/conversions.cpp
)

# Link against goc, Boost, and other dependencies
target_link_libraries(nyr PUBLIC goc ${Boost_LIBRARIES})
