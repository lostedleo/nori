file(GLOB SOURCES *.cc *.h)
add_style_check_target(life_game_lint "${SOURCES}")

set(LIFE_GAME
  ${CMAKE_CURRENT_SOURCE_DIR}/life_game.cc
)

set(LIFE_GAME_MAIN
  ${CMAKE_CURRENT_SOURCE_DIR}/life_main.cc
)

add_library(life_game ${LIFE_GAME})
target_link_libraries(life_game brpc-static)

add_executable(nori_main ${LIFE_GAME_MAIN})
target_link_libraries(nori_main life_game)
