add_executable(parallel_http parallel_http.cpp)

if (BUILD_SHARED_LIBS)
  target_link_libraries(parallel_http brpc-shared)
else ()
  target_link_libraries(parallel_http brpc-static)
endif ()
