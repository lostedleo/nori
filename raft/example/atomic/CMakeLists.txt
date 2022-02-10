project(atomic C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)
option(LINK_TCMALLOC "Link tcmalloc if possible" ON)

nori_protobuf_generate(PROTO_SRC PROTO_HEADER atomic.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(EXAMPLE_LINK_SO)
  set(BRAFT_LIB braft-shared)
else()
  set(BRAFT_LIB braft-static)
endif()

if (LINK_TCMALLOC)
  set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")
  set(GPERFTOOLS_LIBRARIES gperftools)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format")

add_executable(atomic_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(atomic_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(atomic_test test.cpp ${PROTO_SRC} ${PROTO_HEADER})

set(DYNAMIC_LIB
  ${GPERFTOOLS_LIBRARIES}
  ${BRAFT_LIB}
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  FILES_MATCHING
  PATTERN "*.sh"
)

target_link_libraries(atomic_client
  ${DYNAMIC_LIB}
)
target_link_libraries(atomic_server
  ${DYNAMIC_LIB}
)
target_link_libraries(atomic_test
  ${DYNAMIC_LIB}
)
