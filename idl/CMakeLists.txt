
cmake_minimum_required (VERSION 3.8)

project(idl)

find_package(CapnProto CONFIG REQUIRED)


capnp_generate_cpp(CAPNP_SOURCES CAPNP_HEADERS idl.capnp)

add_library(idl ${CAPNP_SOURCES} ${CAPNP_HEADERS})

