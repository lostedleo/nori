file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/net/tools/rpc_press/*.cpp")
add_executable(rpc_press ${SOURCES})
target_link_libraries(rpc_press brpc-static ${DYNAMIC_LIB})
