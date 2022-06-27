# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("Waterbase")
# set(CMAKE_CXX_STANDARD 20)

include_directories(.)
include_directories("build/debug/")

#Include externals
add_subdirectory ("idl")


# Include sub-projects.
add_subdirectory ("Waterbase")
add_subdirectory ("WaterbaseClient")

