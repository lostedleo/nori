project(echo_c++ C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

nori_protobuf_generate(PROTO_SRC PROTO_HEADER echo.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(echo_client client.cpp ${PROTO_SRC})
add_executable(echo_server server.cpp ${PROTO_SRC})

if(EXAMPLE_LINK_SO)
    target_link_libraries(echo_client brpc-shared gperftools)
    target_link_libraries(echo_server brpc-shared gperftools)
else()
    target_link_libraries(echo_client brpc-static gperftools)
    target_link_libraries(echo_server brpc-static gperftools)
endif()
