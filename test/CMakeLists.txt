include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(gflags_test gflags_test.cc)
target_link_libraries(gflags_test gflags)

add_executable(leveldb_test leveldb_test.cc)
target_link_libraries(leveldb_test leveldb)

add_executable(gperf_test gperf_test.cc)
target_link_libraries(gperf_test gperftools)
