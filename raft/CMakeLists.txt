project(braft C CXX)

set(WITH_GLOG_VAL "0")
if(BRPC_WITH_GLOG)
  set(WITH_GLOG_VAL "1")
endif()

set(GFLAGS_NS google)
set(BRPC_LIB brpc-static)
set(DYNAMIC_LIB
  ${CMAKE_THREAD_LIBS_INIT}
  ${BRPC_LIB}
  ${OPENSSL_LIBRARIES}
  ${OPENSSL_CRYPTO_LIBRARY}
  gflags
  protobuf
  leveldb
  dl
  z
)

if(BRPC_WITH_GLOG)
  set(DYNAMIC_LIB ${DYNAMIC_LIB} glog)
endif()

if(LEVELDB_WITH_SNAPPY)
  find_library(SNAPPY_LIB NAMES snappy)
  set(DYNAMIC_LIB ${DYNAMIC_LIB} ${SNAPPY_LIB})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
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
    "-Wl,-U,_ProfilerStop"
    )
else()
  set(DYNAMIC_LIB ${DYNAMIC_LIB} rt)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
        "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
        )
endif()

include(FindOpenSSL)

set(BRAFT_OUTPUT ${CMAKE_BINARY_DIR}/output_braft)
set(BRAFT_OUTPUT_INCLUDE ${BRAFT_OUTPUT}/include)
set(BRAFT_OUTPUT_LIB ${BRAFT_OUTPUT}/lib)
set(BRAFT_OUTPUT_BIN ${BRAFT_OUTPUT}/bin)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
        "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
        )
endif()
include(FindOpenSSL)

include_directories(
  ${CMAKE_SOURCE_DIR}/raft
  ${BRAFT_OUTPUT_INCLUDE}
  ${OPENSSL_INCLUDE_DIR}
  )

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BRAFT_OUTPUT_LIB})
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BRAFT_OUTPUT_LIB})

file(GLOB BRAFT_PROTOS "braft/*.proto")
foreach(PROTO ${BRAFT_PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRCS "${BRAFT_OUTPUT_INCLUDE}/braft/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${BRAFT_OUTPUT_INCLUDE}/braft/${PROTO_WE}.pb.h" "${BRAFT_OUTPUT_INCLUDE}/braft/${PROTO_WE}.pb.cc"
    COMMAND protoc --cpp_out=${BRAFT_OUTPUT_INCLUDE}
    --proto_path=${CMAKE_SOURCE_DIR}/raft
    --proto_path=${CMAKE_SOURCE_DIR}/raft/braft ${PROTO}
    DEPENDS ${PROTO} protoc
  )
endforeach()

file(GLOB_RECURSE BRAFT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/braft/*.cpp")
set(SOURCES
  ${BRAFT_SOURCES}
  ${PROTO_SRCS}
  )

execute_process(
  COMMAND bash -c "git rev-parse --short HEAD | tr -d '\n'"
  OUTPUT_VARIABLE BRAFT_REVISION
  )

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif()
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-sign-compare -Wno-deprecated-copy -Wno-unused-variable -Wno-deprecated-declarations -Wno-inconsistent-missing-override")
endif()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBRPC_WITH_GLOG=${WITH_GLOG_VAL} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DBRAFT_REVISION=\\\"${BRAFT_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -msse4 -msse4.2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer")

if(BUILD_UNIT_TESTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNIT_TEST")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUNIT_TEST")
elseif(NOT DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
endif()

add_library(braft-shared SHARED ${SOURCES})
add_library(braft-static STATIC ${SOURCES})
target_link_libraries(braft-shared brpc-shared)
target_link_libraries(braft-static brpc-static)

SET_TARGET_PROPERTIES(braft-static PROPERTIES OUTPUT_NAME braft CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRAFT_OUTPUT_INCLUDE}
)
SET_TARGET_PROPERTIES(braft-shared PROPERTIES OUTPUT_NAME braft CLEAN_DIRECT_OUTPUT 1
  INTERFACE_INCLUDE_DIRECTORIES ${BRAFT_OUTPUT_INCLUDE}
)

install(TARGETS braft-static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIBSUFFIX}
        ARCHIVE DESTINATION lib${LIBSUFFIX}
)

install(TARGETS braft-shared
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIBSUFFIX}
        ARCHIVE DESTINATION lib${LIBSUFFIX}
)

if(BUILD_UNIT_TESTS)
  add_subdirectory(test)
endif()
add_subdirectory(tools)
if(BUILD_EXAMPLES AND LINUX)
  add_subdirectory(example)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/raft/
  DESTINATION ${BRAFT_OUTPUT_INCLUDE}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "example" EXCLUDE
  PATTERN "test" EXCLUDE
  PATTERN "tools" EXCLUDE
)

install(DIRECTORY ${BRAFT_OUTPUT_INCLUDE}
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
)
