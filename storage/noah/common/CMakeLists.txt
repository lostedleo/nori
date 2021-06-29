file(GLOB SOURCES *.cc *.h)
add_style_check_target(noah_common_lint "${SOURCES}")

add_library(noah_common ${SOURCES})

if(PROJECT_LINK_SO)
  target_link_libraries(noah_common meta_proto braft-shared gperftools)
else()
  target_link_libraries(noah_common meta_proto braft-static gperftools)
endif()
