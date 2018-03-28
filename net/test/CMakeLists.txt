include_directories(${CMAKE_CURRENT_BINARY_DIR})
file(GLOB PROTOS "*.proto")
list(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_BINARY_DIR})
foreach(PROTO ${PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND TEST_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${PROTO_WE}.pb.h" "${PROTO_WE}.pb.cc"
    COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            --proto_path=${PROTOBUF_INCLUDE_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${PROTO}
    DEPENDS ${PROTO} protoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

set(GPERFTOOLS_LIBRARIES gperftools)
set(GTEST_LIB gtest)
set(GTEST_MAIN_LIB gtest_main)

set(CMAKE_CPP_FLAGS "-DBRPC_WITH_GLOG=${WITH_GLOG_VAL} -DFLAGS_NS=${google}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DUNIT_TEST -Dprivate=public -Dprotected=public -DBVAR_NOT_LINK_DEFAULT_VARIABLES -D__STRICT_ANSI__ -include ${CMAKE_CURRENT_SOURCE_DIR}/sstream_workaround.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
use_cxx11()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #required by butil/crc32.cc to boost performance for 10x
  if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
  endif()
  if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
  endif()
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert1.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert2.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert1.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cert2.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/jsonout DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

SET(TEST_BUTIL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/popen_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/at_exit_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/atomicops_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/base64_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/big_endian_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/bits_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/hash_tables_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/mru_cache_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/small_map_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stack_container_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/cpu_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/crash_logging_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/leak_tracker_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/proc_maps_linux_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stack_trace_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/environment_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_util_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/dir_reader_posix_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_path_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_temp_dir_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/guid_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/hash_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/lazy_instance_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/md5_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/aligned_memory_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/linked_ptr_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/ref_counted_memory_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/ref_counted_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_ptr_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_vector_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/singleton_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/weak_ptr_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/observer_list_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/file_descriptor_shuffle_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/rand_util_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/safe_numerics_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_clear_errno_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_generic_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/security_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sha1_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stl_util_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/nullable_string16_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/safe_sprintf_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string16_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stringprintf_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string_number_conversions_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string_piece_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string_split_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string_tokenizer_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/string_util_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stringize_macros_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sys_string_conversions_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/utf_offset_string_conversions_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/utf_string_conversions_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/cancellation_flag_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/condition_variable_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/lock_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/waitable_event_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/type_traits_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/non_thread_safe_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_thread_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_thread_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_checker_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_collision_warner_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_id_name_manager_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_local_storage_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_local_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/watchdog_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/pr_time_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/time_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/version_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/logging_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/cacheline_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/class_name_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/endpoint_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/unique_ptr_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/errno_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fd_guard_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/file_watcher_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/find_cstr_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_lock_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/status_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/string_printf_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/string_splitter_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/synchronous_event_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/temp_file_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/baidu_thread_local_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/baidu_time_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/flat_map_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/crc32c_unittest.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/iobuf_unittest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_switches.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/scoped_locale.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/test_file_util_linux.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/butil_unittest_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/butil_unittest_main.cpp
  )

# -DBVAR_NOT_LINK_DEFAULT_VARIABLES not work for gcc >= 5.0, just remove the file to prevent linking into unit tests
list(REMOVE_ITEM BVAR_SOURCES ${CMAKE_SOURCE_DIR}/net/bvar/default_variables.cpp)

file(GLOB TEST_BVAR_SRCS "bvar_*_unittest.cpp")
add_executable(test_bvar
  ${BVAR_SOURCES}
  ${TEST_BVAR_SRCS})
target_link_libraries(test_bvar ${GTEST_LIB}
  ${GPERFTOOLS_LIBRARIES}
  ${DYNAMIC_LIB}
  butil-static)

add_library(test_proto ${TEST_PROTO_SRCS})
target_link_libraries(test_proto protobuf)
add_executable(test_butil ${TEST_BUTIL_SOURCES})
target_link_libraries(test_butil ${GTEST_LIB} ${GPERFTOOLS_LIBRARIES} ${DYNAMIC_LIB} brpc-static test_proto)

file(GLOB BTHREAD_UNITTESTS "bthread*unittest.cpp")
foreach(BTHREAD_UT ${BTHREAD_UNITTESTS})
  get_filename_component(BTHREAD_UT_WE ${BTHREAD_UT} NAME_WE)
  add_executable(${BTHREAD_UT_WE} ${BTHREAD_UT})
  target_link_libraries(${BTHREAD_UT_WE}
    ${GTEST_MAIN_LIB}
    ${GTEST_LIB}
    ${GPERFTOOLS_LIBRARIES}
    ${DYNAMIC_LIB}
    brpc-static
    test_proto
  )
endforeach()

file(GLOB BRPC_UNITTESTS "brpc_*_unittest.cpp")
foreach(BRPC_UT ${BRPC_UNITTESTS})
  get_filename_component(BRPC_UT_WE ${BRPC_UT} NAME_WE)
  add_executable(${BRPC_UT_WE} ${BRPC_UT})
  target_link_libraries(${BRPC_UT_WE}
    ${GTEST_MAIN_LIB}
    ${GPERFTOOLS_LIBRARIES}
    ${GTEST_LIB}
    ${DYNAMIC_LIB}
    brpc-static
    test_proto
  )
endforeach()
