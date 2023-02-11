# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# URLs for each downloaded or bundled third-party dependency.
#############################################################
project(net C CXX)

# In-tree dependencies.
#######################

# get third_party property
# get protobuf include and protoc path
get_target_property(PROTOBUF_INCLUDE_DIR protobuf INTERFACE_INCLUDE_DIRECTORIES)

option(BRPC_WITH_GLOG "With glog" ON)
option(DEBUG "Print debug logs" OFF)
option(WITH_THRIFT "With thrift framed protocol supported" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)
option(BUILD_UNIT_TESTS "Whether to build unit tests" OFF)
option(BUILD_EXAMPLES "Whether to build examples" ON)

set(WITH_GLOG_VAL "0")
if(BRPC_WITH_GLOG)
  set(WITH_GLOG_VAL "1")
endif()

if(WITH_DEBUG_SYMBOLS)
  set(DEBUG_SYMBOL "-g")
endif()

if(WITH_THRIFT)
    set(THRIFT_CPP_FLAG "-DENABLE_THRIFT_FRAMED_PROTOCOL")
    set(THRIFT_LIB "thrift")
endif()

include(FindThreads)
include(GNUInstallDirs)
configure_file(${CMAKE_SOURCE_DIR}/build_tools/config.h.in ${CMAKE_SOURCE_DIR}/net/butil/config.h @ONLY)

execute_process(
  COMMAND bash -c "git rev-parse --short HEAD | tr -d '\n'"
  OUTPUT_VARIABLE BRPC_REVISION
)


set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBRPC_WITH_GLOG=${WITH_GLOG_VAL}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DBRPC_REVISION=\\\"${BRPC_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} ${THRIFT_CPP_FLAG}")
set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS}")

find_package(OpenSSL)

include_directories(
  ${CMAKE_SOURCE_DIR}/net
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

set(DYNAMIC_LIB
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${OPENSSL_SSL_LIBRARY}
  ${THRIFT_LIB}
  protobuf
  gflags
  libprotoc
  leveldb
  dl
  z
)

if(BRPC_WITH_GLOG)
  set(DYNAMIC_LIB ${DYNAMIC_LIB} glog)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(DYNAMIC_LIB ${DYNAMIC_LIB} rt snappy)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(DYNAMIC_LIB ${DYNAMIC_LIB}
      pthread
      "-framework CoreFoundation"
      "-framework CoreGraphics"
      "-framework CoreData"
      "-framework CoreText"
      "-framework Security"
      "-framework Foundation"
      "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
      "-Wl,-U,_ProfilerStart"
      "-Wl,-U,_ProfilerStop")
endif()

## list all source files
set(BUTIL_SOURCES
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/dmg_fp/g_fmt.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/dmg_fp/dtoa_wrapper.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/dynamic_annotations/dynamic_annotations.c
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/icu/icu_utf.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/superfasthash/superfasthash.c
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/modp_b64/modp_b64.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/symbolize/demangle.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/symbolize/symbolize.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/snappy/snappy-sinksource.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/snappy/snappy-stubs-internal.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/snappy/snappy.cc
  ${CMAKE_SOURCE_DIR}/net/butil/third_party/murmurhash3/murmurhash3.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/arena.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/at_exit.cc
  ${CMAKE_SOURCE_DIR}/net/butil/atomicops_internals_x86_gcc.cc
  ${CMAKE_SOURCE_DIR}/net/butil/base64.cc
  ${CMAKE_SOURCE_DIR}/net/butil/big_endian.cc
  ${CMAKE_SOURCE_DIR}/net/butil/cpu.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/alias.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/asan_invalid_access.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/crash_logging.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/debugger.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/debugger_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/dump_without_crashing.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/proc_maps_linux.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/stack_trace.cc
  ${CMAKE_SOURCE_DIR}/net/butil/debug/stack_trace_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/environment.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_enumerator.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_enumerator_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_path.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_path_constants.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/memory_mapped_file.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/memory_mapped_file_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/scoped_file.cc
  ${CMAKE_SOURCE_DIR}/net/butil/files/scoped_temp_dir.cc
  ${CMAKE_SOURCE_DIR}/net/butil/file_util.cc
  ${CMAKE_SOURCE_DIR}/net/butil/file_util_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/guid.cc
  ${CMAKE_SOURCE_DIR}/net/butil/guid_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/hash.cc
  ${CMAKE_SOURCE_DIR}/net/butil/lazy_instance.cc
  ${CMAKE_SOURCE_DIR}/net/butil/location.cc
  ${CMAKE_SOURCE_DIR}/net/butil/memory/aligned_memory.cc
  ${CMAKE_SOURCE_DIR}/net/butil/memory/ref_counted.cc
  ${CMAKE_SOURCE_DIR}/net/butil/memory/ref_counted_memory.cc
  ${CMAKE_SOURCE_DIR}/net/butil/memory/singleton.cc
  ${CMAKE_SOURCE_DIR}/net/butil/memory/weak_ptr.cc
  ${CMAKE_SOURCE_DIR}/net/butil/posix/file_descriptor_shuffle.cc
  ${CMAKE_SOURCE_DIR}/net/butil/posix/global_descriptors.cc
  ${CMAKE_SOURCE_DIR}/net/butil/process_util.cc
  ${CMAKE_SOURCE_DIR}/net/butil/rand_util.cc
  ${CMAKE_SOURCE_DIR}/net/butil/rand_util_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/fast_rand.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/safe_strerror_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/sha1_portable.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/latin1_string_conversions.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/nullable_string16.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/safe_sprintf.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string16.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string_number_conversions.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string_split.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string_piece.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string_util.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/string_util_constants.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/stringprintf.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/utf_offset_string_conversions.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/utf_string_conversion_utils.cc
  ${CMAKE_SOURCE_DIR}/net/butil/strings/utf_string_conversions.cc
  ${CMAKE_SOURCE_DIR}/net/butil/synchronization/cancellation_flag.cc
  ${CMAKE_SOURCE_DIR}/net/butil/synchronization/rw_mutex.cc
  ${CMAKE_SOURCE_DIR}/net/butil/synchronization/condition_variable_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/synchronization/waitable_event_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/non_thread_safe_impl.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/platform_thread_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/simple_thread.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_checker_impl.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_collision_warner.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_id_name_manager.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_local_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_local_storage.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_local_storage_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/thread_restrictions.cc
  ${CMAKE_SOURCE_DIR}/net/butil/threading/watchdog.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/clock.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/default_clock.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/default_tick_clock.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/tick_clock.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/time.cc
  ${CMAKE_SOURCE_DIR}/net/butil/time/time_posix.cc
  ${CMAKE_SOURCE_DIR}/net/butil/version.cc
  ${CMAKE_SOURCE_DIR}/net/butil/logging.cc
  ${CMAKE_SOURCE_DIR}/net/butil/class_name.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/errno.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/find_cstr.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/status.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/string_printf.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/thread_local.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/unix_socket.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/endpoint.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/fd_utility.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/files/temp_file.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/files/file_watcher.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/time.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/zero_copy_stream_as_streambuf.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/crc32c.cc
  ${CMAKE_SOURCE_DIR}/net/butil/containers/case_ignored_flat_map.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/iobuf.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/binary_printer.cpp
  ${CMAKE_SOURCE_DIR}/net/butil/recordio.cc
  ${CMAKE_SOURCE_DIR}/net/butil/popen.cpp
  )

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUTIL_SOURCES ${BUTIL_SOURCES}
      ${CMAKE_SOURCE_DIR}/net/butil/file_util_linux.cc
      ${CMAKE_SOURCE_DIR}/net/butil/threading/platform_thread_linux.cc
      ${CMAKE_SOURCE_DIR}/net/butil/strings/sys_string_conversions_posix.cc)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(BUTIL_SOURCES ${BUTIL_SOURCES}
      ${CMAKE_SOURCE_DIR}/net/butil/mac/bundle_locations.mm
      ${CMAKE_SOURCE_DIR}/net/butil/mac/foundation_util.mm
      ${CMAKE_SOURCE_DIR}/net/butil/file_util_mac.mm
      ${CMAKE_SOURCE_DIR}/net/butil/threading/platform_thread_mac.mm
      ${CMAKE_SOURCE_DIR}/net/butil/strings/sys_string_conversions_mac.mm
      ${CMAKE_SOURCE_DIR}/net/butil/time/time_mac.cc
      ${CMAKE_SOURCE_DIR}/net/butil/mac/scoped_mach_port.cc)
  if((CMAKE_SYSTEM_PROCESSOR MATCHES "arm64"))
  endif()
endif()

file(GLOB_RECURSE BVAR_SOURCES "${CMAKE_SOURCE_DIR}/net/bvar/*.cpp")
file(GLOB_RECURSE BTHREAD_SOURCES "${CMAKE_SOURCE_DIR}/net/bthread/*.cpp")
file(GLOB_RECURSE JSON2PB_SOURCES "${CMAKE_SOURCE_DIR}/net/json2pb/*.cpp")
file(GLOB_RECURSE BRPC_SOURCES "${CMAKE_SOURCE_DIR}/net/brpc/*.cpp")
file(GLOB_RECURSE THRIFT_SOURCES "${CMAKE_SOURCE_DIR}/net/brpc/thrift*.cpp")

if(WITH_THRIFT)
  message("brpc compile with thrift protocol")
else()
  # Remove thrift sources
  foreach(v ${THRIFT_SOURCES})
    list(REMOVE_ITEM BRPC_SOURCES ${v})
  endforeach()
  set(THRIFT_SOURCES "")
endif()

set(MCPACK2PB_SOURCES
  ${CMAKE_SOURCE_DIR}/net/mcpack2pb/field_type.cpp
  ${CMAKE_SOURCE_DIR}/net/mcpack2pb/mcpack2pb.cpp
  ${CMAKE_SOURCE_DIR}/net/mcpack2pb/parser.cpp
  ${CMAKE_SOURCE_DIR}/net/mcpack2pb/serializer.cpp
  )

set(BRPC_OUTPUT ${CMAKE_BINARY_DIR}/output_brpc)
set(BRPC_OUTPUT_INCLUDE ${BRPC_OUTPUT}/include)
set(BRPC_OUTPUT_LIB ${BRPC_OUTPUT}/lib)
set(BRPC_OUTPUT_BIN ${BRPC_OUTPUT}/bin)
include_directories(${BRPC_OUTPUT_INCLUDE})

file(GLOB PROTOS "*.proto")
list(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_BINARY_DIR})
foreach(PROTO ${PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRCS "${BRPC_OUTPUT_INCLUDE}/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${BRPC_OUTPUT_INCLUDE}/${PROTO_WE}.pb.h" "${BRPC_OUTPUT_INCLUDE}/${PROTO_WE}.pb.cc"
    COMMAND protoc ${PROTO_FLAGS} --cpp_out=${BRPC_OUTPUT_INCLUDE} --proto_path=${PROTOBUF_INCLUDE_DIR} --proto_path=${CMAKE_SOURCE_DIR}/net ${PROTO}
    DEPENDS ${PROTO} protoc
)
endforeach()

file(GLOB BRPC_PROTOS "brpc/*.proto")
foreach(PROTO ${BRPC_PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRCS "${BRPC_OUTPUT_INCLUDE}/brpc/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${BRPC_OUTPUT_INCLUDE}/brpc/${PROTO_WE}.pb.h" "${BRPC_OUTPUT_INCLUDE}/brpc/${PROTO_WE}.pb.cc"
    COMMAND protoc ${PROTO_FLAGS} --cpp_out=${BRPC_OUTPUT_INCLUDE} --proto_path=${PROTOBUF_INCLUDE_DIR} --proto_path=${CMAKE_SOURCE_DIR}/net --proto_path=${CMAKE_SOURCE_DIR}/net/brpc/ ${PROTO}
    DEPENDS ${PROTO} protoc
  )
endforeach()

file(GLOB BRPC_POLICY_PROTOS "brpc/policy/*.proto")
foreach(PROTO ${BRPC_POLICY_PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRCS "${BRPC_OUTPUT_INCLUDE}/brpc/policy/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${BRPC_OUTPUT_INCLUDE}/brpc/policy/${PROTO_WE}.pb.h" "${BRPC_OUTPUT_INCLUDE}/brpc/policy/${PROTO_WE}.pb.cc"
    COMMAND protoc ${PROTO_FLAGS} --cpp_out=${BRPC_OUTPUT_INCLUDE} --proto_path=${PROTOBUF_INCLUDE_DIR} --proto_path=${CMAKE_SOURCE_DIR}/net --proto_path=${CMAKE_SOURCE_DIR}/net/brpc/policy ${PROTO}
    DEPENDS ${PROTO} protoc
  )
endforeach()

set(SOURCES
  ${BVAR_SOURCES}
  ${BTHREAD_SOURCES}
  ${JSON2PB_SOURCES}
  ${MCPACK2PB_SOURCES}
  ${BRPC_SOURCES}
  ${PROTO_SRCS}
  ${THRIFT_SOURCES}
)

if(BUILD_UNIT_TESTS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNIT_TEST -DBVAR_NOT_LINK_DEFAULT_VARIABLES")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUNIT_TEST")
elseif(NOT DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
endif()

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BRPC_OUTPUT_LIB})
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BRPC_OUTPUT_LIB})

add_library(BUTIL_LIB OBJECT ${BUTIL_SOURCES})
target_link_libraries(BUTIL_LIB ${DYNAMIC_LIB})

add_library(butil-shared SHARED $<TARGET_OBJECTS:BUTIL_LIB>)
add_library(butil-static STATIC $<TARGET_OBJECTS:BUTIL_LIB>)
SET_TARGET_PROPERTIES(butil-static PROPERTIES OUTPUT_NAME butil CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRPC_OUTPUT_INCLUDE}
)
SET_TARGET_PROPERTIES(butil-shared PROPERTIES OUTPUT_NAME butil CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRPC_OUTPUT_INCLUDE}
)
target_link_libraries(butil-static ${DYNAMIC_LIB})
target_link_libraries(butil-shared ${DYNAMIC_LIB})

add_library(BRPC_LIB OBJECT ${SOURCES})
target_link_libraries(BRPC_LIB ${DYNAMIC_LIB})
add_library(brpc-shared SHARED $<TARGET_OBJECTS:BRPC_LIB>)
add_library(brpc-static STATIC $<TARGET_OBJECTS:BRPC_LIB>)
SET_TARGET_PROPERTIES(brpc-static PROPERTIES OUTPUT_NAME brpc CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRPC_OUTPUT_INCLUDE}
)
SET_TARGET_PROPERTIES(brpc-shared PROPERTIES OUTPUT_NAME brpc CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRPC_OUTPUT_INCLUDE}
)

target_link_libraries(brpc-static ${DYNAMIC_LIB} butil-static)
target_link_libraries(brpc-shared ${DYNAMIC_LIB} butil-shared)

file(COPY ${CMAKE_SOURCE_DIR}/net/
  DESTINATION ${BRPC_OUTPUT_INCLUDE}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "test" EXCLUDE
  PATTERN "tools" EXCLUDE
  PATTERN "example" EXCLUDE
  PATTERN "docs" EXCLUDE
)

install(DIRECTORY ${BRPC_OUTPUT_INCLUDE}
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
)
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
  set(LIBSUFFIX 64)
else()
  set(LIBSUFFIX "")
endif()

#install directory
# cmake -DCMAKE_INSTALL_PREFIX=/usr
install(TARGETS brpc-shared
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIBSUFFIX}
  ARCHIVE DESTINATION lib${LIBSUFFIX}
)

install(TARGETS brpc-static
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIBSUFFIX}
  ARCHIVE DESTINATION lib${LIBSUFFIX}
)

add_subdirectory(mcpack2pb)
add_subdirectory(tools)
if(BUILD_UNIT_TESTS)
  add_subdirectory(test)
endif()
if(BUILD_EXAMPLES)
  add_subdirectory(example)
endif()
