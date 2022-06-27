cmake_minimum_required (VERSION 3.8)

project(WaterbaseClient)
find_package(CapnProto CONFIG REQUIRED)



target_link_libraries(idl)


add_executable (client "WaterbaseClient.cpp" "WaterbaseClient.h")
target_link_libraries(client CapnProto::capnp)


