file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/net/tools/rpc_replay/*.cpp")
add_executable(rpc_replay ${SOURCES})

if (BUILD_SHARED_LIBS)
  target_link_libraries(rpc_replay brpc-shared)
else ()
  target_link_libraries(rpc_replay brpc-static)
endif ()
