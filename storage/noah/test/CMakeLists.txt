set(GTEST_LIB gtest)
set(GTEST_MAIN_LIB gtest_main)
set(GPERFTOOLS_LIBRARY gperftools)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DUNIT_TEST -g -Dprotected=public -D__STRICT_ANSI__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} -O2 -g -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer -Wno-unused-result")

file(GLOB TEST_SRCS "*_test.cc")
add_style_check_target(noah_test_lint "${TEST_SRCS}")

foreach(NOAH_UT ${TEST_SRCS})
  get_filename_component(NOAH_UT_WE ${NOAH_UT} NAME_WE)
  add_executable(${NOAH_UT_WE} ${NOAH_UT})
  target_link_libraries(${NOAH_UT_WE}
                        ${GTEST_MAIN_LIB}
                        ${GTEST_LIB}
                        ${GPERFTOOLS_LIBRARY}
                        meta
                        node
                        noah_client
  )
endforeach()
