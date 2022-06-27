# CMakeList.txt : CMake project for Waterbase, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(WaterbaseServer)
find_package(CapnProto CONFIG REQUIRED)



target_link_libraries(idl)


add_executable (server "Waterbase.cpp" "Waterbase.h")
target_link_libraries(server CapnProto::capnp)



# Add source to this project's executable.

# TODO: Add tests and install targets if needed.
