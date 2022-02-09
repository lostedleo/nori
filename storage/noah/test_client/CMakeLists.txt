file(GLOB SOURCES *.cc *.h)
add_style_check_target(noah_test_client_lint "${SOURCES}")

set(TEST_CLIENT_TOOL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_tool.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_read.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_read.h
)

list(REMOVE_ITEM SOURCES ${TEST_CLIENT_TOOL})

add_library(noah_test_client ${SOURCES})
target_link_libraries(noah_test_client noah_common node_proto)
set_target_properties(noah_test_client PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(test_client_tool ${TEST_CLIENT_TOOL})
target_link_libraries(test_client_tool noah_test_client ${DYNAMIC_LIB})
