project(noah C CXX)

option(PROJECT_LINK_SO "Whether project are linked dynamically" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)

if(WITH_DEBUG_SYMBOLS)
  set(DEBUG_SYMBOL "-g")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-sign-compare -Wno-deprecated-declarations -Wno-inconsistent-missing-override")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-variable -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_SYMBOL} -DBRPC_ENABLE_CPU_PROFILER")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(DYNAMIC_LIB "")
if(NOT BUILD_SHARED_LIBS)
  set(DYNAMIC_LIB pthread z)
endif()

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(node)
add_subdirectory(meta)
add_subdirectory(test)
add_subdirectory(tools)
add_subdirectory(test_client)
