include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(gflags_test gflags_test.cc)
target_link_libraries(gflags_test gflags pthread)

add_executable(leveldb_test leveldb_test.cc)
target_link_libraries(leveldb_test leveldb)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(leveldb_test snappy pthread)
endif()

add_executable(gperf_test gperf_test.cc)
target_link_libraries(gperf_test gperftools pthread)
