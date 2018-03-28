set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -D__STRICT_ANSI__ -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(EXECUTABLE_OUTPUT_PATH ${BRPC_OUTPUT_BIN})

add_subdirectory(parallel_http)
add_subdirectory(rpc_press)
add_subdirectory(rpc_replay)
add_subdirectory(rpc_view)
add_subdirectory(trackme_server)
