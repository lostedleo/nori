file(GLOB SOURCES *.cc *.h)
add_style_check_target(noah_client_lint "${SOURCES}")

set(CLIENT_TOOL
  ${CMAKE_CURRENT_SOURCE_DIR}/client_tool.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_read.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_read.h
)

list(REMOVE_ITEM SOURCES ${CLIENT_TOOL})

add_library(noah_client ${SOURCES})
target_link_libraries(noah_client noah_common node_proto)
set_target_properties(noah_client PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(client_tool ${CLIENT_TOOL})
target_link_libraries(client_tool noah_client)
