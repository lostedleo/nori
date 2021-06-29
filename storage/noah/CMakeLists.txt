project(noah C CXX)

option(PROJECT_LINK_SO "Whether project are linked dynamically" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(node)
add_subdirectory(meta)
add_subdirectory(test)
add_subdirectory(tools)
add_subdirectory(test_client)
