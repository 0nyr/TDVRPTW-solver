cmake_minimum_required(VERSION 3.30)
project(nyr)

# Inherit flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD 26)

# Include GOC (assumes the parent scope provides it)
include_directories(${CMAKE_SOURCE_DIR}/goc/include)

# Add nyr includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create nyr library
add_library(
    nyr 
    src/time/time.cpp
)

# Link against goc
target_link_libraries(nyr PUBLIC goc)
