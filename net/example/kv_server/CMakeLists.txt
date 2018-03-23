cmake_minimum_required(VERSION 2.8.10)
project(multi_threaded_echo_c++ C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -g -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

file(GLOB PROTOS "*.proto")
list(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_BINARY_DIR})
foreach(PROTO ${PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${PROTO_WE}.pb.h" "${PROTO_WE}.pb.cc"
    COMMAND protoc ${PROTO_FLAGS} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${PROTO}
    DEPENDS ${PROTO}
    )
endforeach()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(cache_client client.cpp file_read.cpp ${PROTO_SRC})
add_executable(cache_server server.cpp cache.cpp ${PROTO_SRC} ${PROTO_HEADER})

if(EXAMPLE_LINK_SO)
    target_link_libraries(cache_client brpc-shared gperftools)
    target_link_libraries(cache_server brpc-shared gperftools)
else()
    target_link_libraries(cache_client brpc-static gperftools)
    target_link_libraries(cache_server brpc-static gperftools)
endif()
