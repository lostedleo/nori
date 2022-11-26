include_directories(
  ${CMAKE_SOURCE_DIR}/ai
)

set(GTEST_LIB gtest)
set(GTEST_MAIN_LIB gtest_main)

file(GLOB UNITTESTS "*_unittest.cc")
foreach(UT ${UNITTESTS})
  get_filename_component(UT_WE ${UT} NAME_WE)
  add_executable(${UT_WE} ${UT})
  target_link_libraries(${UT_WE}
    ${GTEST_MAIN_LIB}
    ${GTEST_LIB}
    nrai
  )
  add_test(NAME ${UT_WE} COMMAND ${UT_WE})
endforeach()
