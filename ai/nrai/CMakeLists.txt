file(GLOB SOURCES *.cc *.h)
add_style_check_target(nrai_lint "${SOURCES}")

set(NRAI
  ${CMAKE_CURRENT_SOURCE_DIR}/core.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sense.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/logic.cc
)

set(NRAI_MAIN
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
)

add_library(nrai ${NRAI})
target_link_libraries(nrai)

add_executable(nrai_main ${NRAI_MAIN})
target_link_libraries(nrai_main nrai)
