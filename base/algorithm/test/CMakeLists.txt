project(kmeans_test C CXX)

option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)
option(LINK_TCMALLOC "Link tcmalloc if possible" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -g2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(EXAMPLE_LINK_SO)
  set(BASE_LIB base-shared)
else()
  set(BASE_LIB base-static)
endif()

if (LINK_TCMALLOC)
  set(GPERFTOOLS_LIBRARIES gperftools pthread)
endif ()

add_executable(kmeans_test kmeans_test.cpp)
add_executable(gmm_test gmm_test.cpp)
add_executable(hmm_test hmm_test.cpp)

set(DYNAMIC_LIB
  ${GPERFTOOLS_LIBRARIES}
  ${BASE_LIB}
)

target_link_libraries(kmeans_test
  ${DYNAMIC_LIB}
)

target_link_libraries(gmm_test
  ${DYNAMIC_LIB}
)

target_link_libraries(hmm_test
  ${DYNAMIC_LIB}
)
