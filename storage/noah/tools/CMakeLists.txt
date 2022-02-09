file(GLOB SOURCES *.cc *.h)
add_style_check_target(noah_tools_lint "${SOURCES}")

set(NOAH_TOOL
  ${CMAKE_CURRENT_SOURCE_DIR}/noah_client.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/noah_tool.cc
)

set(NOAH_ANALYSIS
  ${CMAKE_CURRENT_SOURCE_DIR}/noah_client.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/noah_analysis.cc
)

set(IMPORT_TOOL_TOKEN
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_token.cc
)
set(IMPORT_TOOL_REAL_LOC
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_real_loc.cc
)
set(IMPORT_TOOL_FEEDBACK
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_feedback.cc
)
set(IMPORT_TOOL_SWITCH
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_switch.cc
)
set(IMPORT_TOOL_ACCOUNT
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_account.cc
)
set(IMPORT_TOOL_USER_ATTR
  ${CMAKE_CURRENT_SOURCE_DIR}/import_tool_user_attr.cc
)

set(GET_TOOL_TOKEN
  ${CMAKE_CURRENT_SOURCE_DIR}/get_tool_token.cc
)
set(TEST_CLIENT
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client.cc
)
add_executable(noah_tool ${NOAH_TOOL})
target_link_libraries(noah_tool noah_client readline ${DYNAMIC_LIB})

add_executable(noah_analysis ${NOAH_ANALYSIS})
target_link_libraries(noah_analysis noah_client readline ${DYNAMIC_LIB})

add_executable(import_tool_token ${IMPORT_TOOL_TOKEN})
target_link_libraries(import_tool_token noah_client ${DYNAMIC_LIB})

add_executable(import_tool_real_loc ${IMPORT_TOOL_REAL_LOC})
target_link_libraries(import_tool_real_loc noah_client ${DYNAMIC_LIB})

add_executable(import_tool_feedback ${IMPORT_TOOL_FEEDBACK})
target_link_libraries(import_tool_feedback noah_client ${DYNAMIC_LIB})

add_executable(import_tool_account ${IMPORT_TOOL_ACCOUNT})
target_link_libraries(import_tool_account noah_client ${DYNAMIC_LIB})

add_executable(import_tool_user_attr ${IMPORT_TOOL_USER_ATTR})
target_link_libraries(import_tool_user_attr noah_client ${DYNAMIC_LIB})

add_executable(get_tool_token ${GET_TOOL_TOKEN})
target_link_libraries(get_tool_token noah_client ${DYNAMIC_LIB})

add_executable(test_client ${TEST_CLIENT})
target_link_libraries(test_client noah_client noah_test_client ${DYNAMIC_LIB})

add_executable(import_tool_switch ${IMPORT_TOOL_SWITCH})
target_link_libraries(import_tool_switch noah_client ${DYNAMIC_LIB})
