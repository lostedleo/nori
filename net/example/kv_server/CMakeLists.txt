cmake_minimum_required(VERSION 2.8.10)
project(kv_server C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

nori_protobuf_generate(PROTO_SRC PROTO_HEADER cache.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(CACHE_PROTO OBJECT ${PROTO_SRC})
add_executable(cache_client client.cpp file_read.cpp $<TARGET_OBJECTS:CACHE_PROTO>)
add_executable(cache_server server.cpp cache.cpp $<TARGET_OBJECTS:CACHE_PROTO>)

if(EXAMPLE_LINK_SO)
    target_link_libraries(cache_client brpc-shared gperftools)
    target_link_libraries(cache_server brpc-shared gperftools)
else()
    target_link_libraries(cache_client brpc-static gperftools)
    target_link_libraries(cache_server brpc-static gperftools)
endif()
