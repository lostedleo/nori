cmake_minimum_required(VERSION 2.8.10)
project(block C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)
option(LINK_TCMALLOC "Link tcmalloc if possible" ON)

nori_protobuf_generate(PROTO_SRC PROTO_HEADER block.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(EXAMPLE_LINK_SO)
  set(BRAFT_LIB braft-shared)
else()
  set(BRAFT_LIB braft-static)
endif()

if (LINK_TCMALLOC)
  set(CMAKE_CXX_FLAGS "-DBRPC_ENABLE_CPU_PROFILER")
  set(GPERFTOOLS_LIBRARIES gperftools)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")

add_executable(block_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(block_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  FILES_MATCHING
  PATTERN "*.sh"
)

set(DYNAMIC_LIB
  ${GPERFTOOLS_LIBRARIES}
  ${BRAFT_LIB}
)

target_link_libraries(block_client
  ${DYNAMIC_LIB}
)
target_link_libraries(block_server
  ${DYNAMIC_LIB}
)
