set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${BRAFT_OUTPUT_BIN})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(braft_cli braft_cli.cpp)
target_link_libraries(braft_cli
                      braft-static
                      )
