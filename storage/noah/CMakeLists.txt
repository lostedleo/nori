project(noah C CXX)

option(PROJECT_LINK_SO "Whether project are linked dynamically" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)

if(WITH_DEBUG_SYMBOLS)
  set(DEBUG_SYMBOL "-g")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} -DBRPC_ENABLE_CPU_PROFILER")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field -Wno-format-security")
  if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
