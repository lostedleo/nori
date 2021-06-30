add_executable(trackme_server trackme_server.cpp)

if (BUILD_SHARED_LIBS)
  target_link_libraries(trackme_server brpc-shared)
else ()
  target_link_libraries(trackme_server brpc-static)
endif ()
