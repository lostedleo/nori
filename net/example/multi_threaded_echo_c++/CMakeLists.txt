cmake_minimum_required(VERSION 2.8.10)
project(multi_threaded_echo_c++ C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)

include(FindThreads)
nori_protobuf_generate(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(EXAMPLE_LINK_SO)
    set(BRPC_LIB brpc-shared)
else()
    set(BRPC_LIB brpc-static)
endif()
set(GPERFTOOLS_LIBRARIES gperftools)
 
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

add_executable(multi_threaded_echo_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(multi_threaded_echo_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})

if(EXAMPLE_LINK_SO)
    target_link_libraries(multi_threaded_echo_client ${BRPC_LIB} ${GPERFTOOLS_LIBRARIES})
    target_link_libraries(multi_threaded_echo_server ${BRPC_LIB} ${GPERFTOOLS_LIBRARIES})
else()
    target_link_libraries(multi_threaded_echo_client ${BRPC_LIB} ${GPERFTOOLS_LIBRARIES})
    target_link_libraries(multi_threaded_echo_server ${BRPC_LIB} ${GPERFTOOLS_LIBRARIES})
endif()
