set(GTEST_LIB gtest)
set(GTEST_MAIN_LIB gtest_main)
set(GPERFTOOLS_LIBRARY gperftools)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DUNIT_TEST -g -Dprivate=public -Dprotected=public -D__STRICT_ANSI__ -include sstream_workaround.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer -Wno-unused-result")

file(GLOB TEST_BRAFT_SRCS "test_*.cpp")
foreach(BRAFT_UT ${TEST_BRAFT_SRCS})
  get_filename_component(BRAFT_UT_WE ${BRAFT_UT} NAME_WE)
  add_executable(${BRAFT_UT_WE} ${BRAFT_UT})
  #$<TARGET_OBJECTS:BRAFT_OBJ>)
  target_link_libraries(${BRAFT_UT_WE}
                        ${GTEST_MAIN_LIB}
                        ${GTEST_LIB}
                        ${GPERFTOOLS_LIBRARY}
                        braft-static
  )
endforeach()
