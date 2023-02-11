file(GLOB SOURCES *.cc *.h)
add_style_check_target(nrai_lint "${SOURCES}")
set(PYBIND11_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/pybind11/include)

include_directories(${PYBIND11_INCLUDE_DIR})

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

set(pybind11_DIR ${CMAKE_SOURCE_DIR}/third_party/pybind11/share/cmake/pybind11)
find_package(pybind11 REQUIRED)
pybind11_add_module(_nrai ${CMAKE_CURRENT_SOURCE_DIR}/pybind11_module.cc)

add_executable(nrai_main ${NRAI_MAIN})
target_link_libraries(nrai_main nrai)
