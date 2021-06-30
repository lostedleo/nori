file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/net/tools/rpc_press/*.cpp")

add_executable(rpc_press ${SOURCES})
if (BUILD_SHARED_LIBS)
  target_link_libraries(rpc_press brpc-shared)
else ()
  target_link_libraries(rpc_press brpc-static)
endif ()
