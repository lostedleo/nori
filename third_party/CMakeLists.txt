
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
project(third_party)

set(FETCH_URL ${CMAKE_CURRENT_SOURCE_DIR})

# This adds our `EXTERNAL()` and `PATCH_CMD()` functions.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(External)
include(Versions)

set(GOOGLETEST_URL      ${FETCH_URL}/googletest-${GOOGLETEST_VERSION}.tar.gz)
set(GFLAGS_URL          ${FETCH_URL}/gflags-${GFLAGS_VERSION}.tar.gz)
set(GLOG_URL            ${FETCH_URL}/glog-${GLOG_VERSION}.tar.gz)
set(GPERFTOOLS_URL      ${FETCH_URL}/gperftools-${GPERFTOOLS_VERSION}.tar.gz)
set(JSONCPP_URL         ${FETCH_URL}/jsoncpp-${JSONCPP_VERSION}.tar.gz)
set(PROTOBUF_URL        ${FETCH_URL}/protobuf-${PROTOBUF_VERSION}.tar.gz)
set(LEVELDB_URL         ${FETCH_URL}/leveldb-${LEVELDB_VERSION}.tar.gz)
set(BOOST_URL           ${FETCH_URL}/boost-${BOOST_VERSION}.tar.gz)
set(CURL_URL            ${FETCH_URL}/curl-${CURL_VERSION}.tar.gz)
set(HIREDIS_URL         ${FETCH_URL}/hiredis-${HIREDIS_VERSION}.tar.gz)
# Helper utilities for dealing with third-party dependencies.
#############################################################
# ExternalProject: Create custom targets to build projects in external trees.
# https://cmake.org/cmake/help/latest/module/ExternalProject.html
include(ExternalProject)

# Variables for current specified linkage.
# NOTE: Not all third-party dependencies will respect this.
if (BUILD_SHARED_LIBS)
  set(LIBRARY_LINKAGE SHARED)
  set(LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else ()
  set(LIBRARY_LINKAGE STATIC)
  set(LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif ()

# Sets a variable CMAKE_NOOP as noop operation.
#
# NOTE: This is especially important when building third-party libraries on
# Windows; the default behavior of `ExternalProject` is to try to assume that
# third-party libraries can be configured/built/installed with CMake, so in
# cases where this isn't true, we have to "trick" CMake into skipping those
# steps by giving it a noop command to run instead.
set(CMAKE_NOOP ${CMAKE_COMMAND} -E echo)
set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/third_party_install)
set(DEPS_CONFIG --with-pic)
if(BUILD_SHARED_LIBS)
else ()
  list(APPEND DEPS_CONFIG --without-shared)
endif()
set(DEFAULT_BUILD_CMD make)
set(REAL_INSTALL_CMD make install)
set(DEFAULT_INSTALL_CMD ${CMAKE_NOOP})

# This `CMAKE_FORWARD_ARGS` variable is sent as the `CMAKE_ARGS` argument to
# the `ExternalProject_Add` macro (along with any per-project arguments), and
# is used when the external project is configured as a CMake project.
# If either the `CONFIGURE_COMMAND` or `BUILD_COMMAND` arguments of
# `ExternalProject_Add` are used, then the `CMAKE_ARGS` argument will be
# ignored.
#
# NOTE: The CMAKE_GENERATOR_TOOLSET is impliticly set by `ExternalProject_Add`,
# and cannot be included twice.
list(APPEND CMAKE_FORWARD_ARGS
  -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
  # -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)

# This only matters for single-configuration generators.
# E.g. Makefile, but not Visual Studio.
if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
  list(APPEND CMAKE_FORWARD_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
endif ()

# foreach (lang C CXX)
#   list(APPEND CMAKE_FORWARD_ARGS
#     -DCMAKE_${lang}_COMPILER=${CMAKE_${lang}_COMPILER}
#     -DCMAKE_${lang}_COMPILER_LAUNCHER=${CMAKE_${lang}_COMPILER_LAUNCHER}
#     -DCMAKE_${lang}_FLAGS=${CMAKE_${lang}_FLAGS}
# )
#
#   foreach (config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
#     list(APPEND CMAKE_FORWARD_ARGS
#       -DCMAKE_${lang}_FLAGS_${config}=${CMAKE_${lang}_FLAGS_${config}})
#   endforeach ()
# endforeach ()

# This function works around a CMake issue with setting include directories of
# imported libraries built with `ExternalProject_Add`.
# https://gitlab.kitware.com/cmake/cmake/issues/15052
function(MAKE_INCLUDE_DIR TARGET)
  get_target_property(DIR ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  file(MAKE_DIRECTORY ${DIR})
endfunction()

# This function works around a CMake issue with the Ninja generator where it
# does not understand imported libraries, and instead needs `BUILD_BYPRODUCTS`
# explicitly set.
# https://cmake.org/pipermail/cmake/2015-April/060234.html
function(GET_BYPRODUCTS TARGET)
  get_target_property(BYPRODUCTS ${TARGET} IMPORTED_LOCATION)
  string(TOUPPER ${TARGET} NAME)
  set(${NAME}_BYPRODUCTS ${BYPRODUCTS} PARENT_SCOPE)
endfunction()


# Boost: C++ Libraries.
# http://www.boost.org
#######################
#EXTERNAL(boost ${BOOST_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
#add_library(boost INTERFACE)
#add_dependencies(boost ${BOOST_TARGET})
#target_include_directories(boost INTERFACE ${BOOST_ROOT})
#
#ExternalProject_Add(
#  ${BOOST_TARGET}
#  PREFIX            ${BOOST_CMAKE_ROOT}
#  CONFIGURE_COMMAND ${CMAKE_NOOP}
#  BUILD_COMMAND     ${CMAKE_NOOP}
#  INSTALL_COMMAND   ${CMAKE_NOOP}
#  URL               ${BOOST_URL}
#  URL_HASH          ${BOOST_HASH})


# Google Test: Google's C++ test framework (GoogleTest and GoogleMock).
# https://github.com/google/googletest
#######################################################################
EXTERNAL(googletest ${GOOGLETEST_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(googletest INTERFACE)
add_dependencies(googletest ${GOOGLETEST_TARGET})
target_link_libraries(googletest INTERFACE gmock gtest)

# Note that Google Test is always built with static libraries because of the
# following open issue when using shared libraries, on both Windows and Linux:
# https://github.com/google/googletest/issues/930
add_library(gmock STATIC IMPORTED GLOBAL)
add_library(gtest STATIC IMPORTED GLOBAL)
add_library(gtest_main STATIC IMPORTED GLOBAL)

set_target_properties(
  gmock PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GOOGLETEST_ROOT}/googlemock/include
)

set_target_properties(
  gtest PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GOOGLETEST_ROOT}/googletest/include
)

set(GOOGLETEST_CMAKE_FORWARD_ARGS ${CMAKE_FORWARD_ARGS})

set_target_properties(
  gmock PROPERTIES
  IMPORTED_LOCATION ${GOOGLETEST_ROOT}-build/googlemock/libgmock${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set_target_properties(
  gtest PROPERTIES
  IMPORTED_LOCATION ${GOOGLETEST_ROOT}-build/googlemock/gtest/libgtest${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set_target_properties(
  gtest_main PROPERTIES
  IMPORTED_LOCATION ${GOOGLETEST_ROOT}-build/googlemock/gtest/libgtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
)

MAKE_INCLUDE_DIR(gmock)
MAKE_INCLUDE_DIR(gtest)
GET_BYPRODUCTS(gmock)
GET_BYPRODUCTS(gtest)

# Unconditionally build static libraries.
list(APPEND GOOGLETEST_CMAKE_FORWARD_ARGS -DBUILD_SHARED_LIBS=OFF)

# Link to the correct CRT based on other libraries' linkage.
list(APPEND GOOGLETEST_CMAKE_FORWARD_ARGS
  -Dgtest_force_shared_crt=${BUILD_SHARED_LIBS}
)

ExternalProject_Add(
  ${GOOGLETEST_TARGET}
  PREFIX            ${GOOGLETEST_CMAKE_ROOT}
  BUILD_BYPRODUCTS  ${GMOCK_BYPRODUCTS};${GTEST_BYPRODUCTS}
  CMAKE_ARGS        ${GOOGLETEST_CMAKE_FORWARD_ARGS}
  INSTALL_COMMAND   ${DEFAULT_INSTALL_CMD}
  URL               ${GOOGLETEST_URL}
  URL_HASH          ${GOOGLETEST_HASH}
)

# gflags: The gflags package contains a C++ library that implements commandline flags processing.
# https://github.com/gflags/gflags
################################
EXTERNAL(gflags ${GFLAGS_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(gflags ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
add_dependencies(gflags ${GFLAGS_TARGET})

set(GFLAGS_ARGS ${CMAKE_FORWARD_ARGS}
  -DCMAKE_INSTALL_PREFIX=${GFLAGS_CMAKE_ROOT}/install
  -DGFLAGS_NAMESPACE=google -DBUILD_TESTING=OFF)

set(GFLAGS_LIB_PATH ${GFLAGS_ROOT}-build/lib)
set(GFLAGS_INCLUDE_PATH ${GFLAGS_ROOT}-build/include)
set_target_properties(
  gflags PROPERTIES
  IMPORTED_LOCATION ${GFLAGS_LIB_PATH}/libgflags${LIBRARY_SUFFIX}
  INTERFACE_INCLUDE_DIRECTORIES ${GFLAGS_INCLUDE_PATH}
)

MAKE_INCLUDE_DIR(gflags)
GET_BYPRODUCTS(gflags)

ExternalProject_Add(
  ${GFLAGS_TARGET}
  PREFIX            ${GFLAGS_CMAKE_ROOT}
  CMAKE_ARGS        ${GFLAGS_ARGS}
  BUILD_BYPRODUCTS  ${GFLAGS_BYPRODUCTS}
  INSTALL_COMMAND   ${REAL_INSTALL_CMD}
  URL               ${GFLAGS_URL}
  URL_HASH          ${GFLAGS_HASH}
)

# glog: The glog package contains a C++ library that implements commandline flags processing.
# https://github.com/google/glog
################################
EXTERNAL(glog ${GLOG_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(glog ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
add_dependencies(glog ${GFLAGS_TARGET})
add_dependencies(glog ${GLOG_TARGET})

set_target_properties(
  glog PROPERTIES
  IMPORTED_LOCATION ${GLOG_ROOT}-build/.libs/libglog${LIBRARY_SUFFIX}
  INTERFACE_INCLUDE_DIRECTORIES ${GLOG_ROOT}-build/include
)

set(GLOG_CONFIG_CMD ${GLOG_ROOT}/configure --with-pic GTEST_CONFIG=no
  --prefix=${GLOG_ROOT}-build CPPFLAGS=-I${GFLAGS_INCLUDE_PATH}
  LDFLAGS=-L${GFLAGS_LIB_PATH}
)

set(GLOG_CMAKE_ARGS ${CMAKE_FORWARD_ARGS} -DCMAKE_INSTALL_PREFIX=${GLOG_ROOT}-build
  -DBUILD_TESTING=OFF -DCMAKE_INCLUDE_PATH="${GFLAGS_INCLUDE_PATH}"
  -DCMAKE_LIBRARY_PATH="${GFLAGS_LIB_PATH}")

MAKE_INCLUDE_DIR(glog)
GET_BYPRODUCTS(glog)

ExternalProject_Add(
  ${GLOG_TARGET}
  DEPENDS           ${GFLAGS_TARGET}
  PREFIX            ${GLOG_CMAKE_ROOT}
  BUILD_BYPRODUCTS  ${GLOG_BYPRODUCTS}
  #CMAKE_ARGS        ${GLOG_CMAKE_ARGS}
  CONFIGURE_COMMAND ${GLOG_CONFIG_CMD}
  BUILD_COMMAND     ${DEFAULT_BUILD_CMD}
  INSTALL_COMMAND   ${REAL_INSTALL_CMD}
  URL               ${GLOG_URL}
  URL_HASH          ${GLOG_HASH}
)

# gperftools: The gperftools package contains a C++ library that implements commandline flags processing.
# https://github.com/gperftools/gperftools
################################
EXTERNAL(gperftools ${GPERFTOOLS_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(gperftools ${LIBRARY_LINKAGE} IMPORTED GLOBAL)

if (APPLE)
  add_dependencies(gperftools ${GPERFTOOLS_TARGET})
  set(GPERFTOOLS_CONFIG_CMD ${GPERFTOOLS_ROOT}/configure ${DEPS_CONFIG}
    --prefix=${GPERFTOOLS_CMAKE_ROOT}/install)
  # MACOS must use static library
  set_target_properties(
    gperftools PROPERTIES
    IMPORTED_LOCATION ${GPERFTOOLS_CMAKE_ROOT}/install/lib/libtcmalloc_and_profiler.a
    INTERFACE_INCLUDE_DIRECTORIES ${GPERFTOOLS_CMAKE_ROOT}/install/include)
else ()
  add_dependencies(gperftools ${GPERFTOOLS_TARGET} unwind)
  set(GPERFTOOLS_CONFIG_CMD ${GPERFTOOLS_ROOT}/configure ${DEPS_CONFIG}
    --prefix=${GPERFTOOLS_CMAKE_ROOT}/install --enable-libunwind)
  set_target_properties(
    gperftools PROPERTIES
    IMPORTED_LOCATION ${GPERFTOOLS_CMAKE_ROOT}/install/lib/libtcmalloc_and_profiler${LIBRARY_SUFFIX}
    INTERFACE_INCLUDE_DIRECTORIES ${GPERFTOOLS_CMAKE_ROOT}/install/include)
endif ()

MAKE_INCLUDE_DIR(gperftools)
GET_BYPRODUCTS(gperftools)

ExternalProject_Add(
  ${GPERFTOOLS_TARGET}
  PREFIX            ${GPERFTOOLS_CMAKE_ROOT}
  CONFIGURE_COMMAND ${GPERFTOOLS_CONFIG_CMD}
  BUILD_BYPRODUCTS  ${GPERFTOOLS_BYPRODUCTS}
  BUILD_COMMAND     ${DEFAULT_BUILD_CMD}
  INSTALL_COMMAND   ${REAL_INSTALL_CMD}
  URL               ${GPERFTOOLS_URL}
  URL_HASH          ${GPERFTOOLS_HASH}
)

# JSONCPP: JSON parser / serializer.
# https://github.com/open-source-parsers/jsoncpp
#####################################
#EXTERNAL(jsoncpp ${JSONCPP_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
#add_library(jsoncpp ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
#add_dependencies(jsoncpp ${JSONCPP_TARGET})
#
#set_target_properties(
#  jsoncpp PROPERTIES
#  IMPORTED_LOCATION ${JSONCPP_ROOT}-build/src/lib_json/libjsoncpp${LIBRARY_SUFFIX}
#  INTERFACE_INCLUDE_DIRECTORIES ${JSONCPP_ROOT}/include
#)
#
#MAKE_INCLUDE_DIR(jsoncpp)
#GET_BYPRODUCTS(jsoncpp)
#
#ExternalProject_Add(
#  ${JSONCPP_TARGET}
#  PREFIX            ${JSONCPP_CMAKE_ROOT}
#  CMAKE_ARGS        ${CMAKE_FORWARD_ARGS};-DJSONCPP_WITH_TESTS=OFF
#  BUILD_COMMAND     ${CMAKE_NOOP}
#  INSTALL_COMMAND   ${DEFAULT_INSTALL_CMD}
#  URL               ${JSONCPP_URL}
#  URL_HASH          ${JSONCPP_HASH}
#)
#
# Protocol Buffers: Google's data interchange format.
# https://developers.google.com/protocol-buffers/
#####################################################
EXTERNAL(protobuf ${PROTOBUF_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(protobuf ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
add_dependencies(protobuf ${PROTOBUF_TARGET})
add_library(libprotoc ${LIBRARY_LINKAGE} IMPORTED GLOBAL)

add_executable(protoc IMPORTED GLOBAL)
add_dependencies(protoc ${PROTOBUF_TARGET})

set_target_properties(
  protobuf PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${PROTOBUF_CMAKE_ROOT}/install/include
  IMPORTED_LOCATION ${PROTOBUF_CMAKE_ROOT}/install/lib/libprotobuf${LIBRARY_SUFFIX}
)

set_target_properties(
  libprotoc PROPERTIES
  IMPORTED_LOCATION ${PROTOBUF_CMAKE_ROOT}/install/lib/libprotoc${LIBRARY_SUFFIX}
)

set_target_properties(
  protoc PROPERTIES
  IMPORTED_LOCATION ${PROTOBUF_CMAKE_ROOT}/install/bin/protoc
)

MAKE_INCLUDE_DIR(protobuf)
GET_BYPRODUCTS(protobuf)

set(PROTOBUF_CONFIG_CMD ${PROTOBUF_ROOT}/configure ${DEPS_CONFIG} --prefix=${PROTOBUF_CMAKE_ROOT}/install)
ExternalProject_Add(
  ${PROTOBUF_TARGET}
  PREFIX            ${PROTOBUF_CMAKE_ROOT}
  BUILD_BYPRODUCTS  ${PROTOBUF_BYPRODUCTS}
  #CMAKE_ARGS        ${CMAKE_FORWARD_ARGS};-Dprotobuf_BUILD_TESTS=OFF
  #SOURCE_SUBDIR     cmake
  CONFIGURE_COMMAND ${PROTOBUF_CONFIG_CMD}
  INSTALL_COMMAND   ${REAL_INSTALL_CMD}
  URL               ${PROTOBUF_URL}
  URL_HASH          ${PROTOBUF_HASH}
)

# leveldb: The leveldb package contains a C++ library that implements commandline flags processing.
# https://github.com/google/leveldb
################################
EXTERNAL(leveldb ${LEVELDB_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
add_library(leveldb ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
add_dependencies(leveldb ${LEVELDB_TARGET})

set_target_properties(
  leveldb PROPERTIES
  IMPORTED_LOCATION ${LEVELDB_ROOT}/libleveldb${LIBRARY_SUFFIX}
  INTERFACE_INCLUDE_DIRECTORIES ${LEVELDB_ROOT}/include
)

MAKE_INCLUDE_DIR(leveldb)
GET_BYPRODUCTS(leveldb)

set(LEVELDB_ROOT_PATH ${LEVELDB_CMAKE_ROOT}/src)
set(LEVELDB_CMAKE_ARGS ${CMAKE_FORWARD_ARGS} -DLEVELDB_BUILD_TESTS=OFF
  -DLEVELDB_BUILD_BENCHMARKS=OFF
  -DCMAKE_INSTALL_PREFIX=${LEVELDB_ROOT_PATH}
  )

set(LEVELDB_INSTALL_CMD COMMAND
  bash -c "ln -s ${LEVELDB_ROOT_PATH}/leveldb-${LEVELDB_VERSION} ${LEVELDB_ROOT_PATH}/lib")
ExternalProject_Add(
  ${LEVELDB_TARGET}
  PREFIX            ${LEVELDB_CMAKE_ROOT}
  CMAKE_ARGS        ${LEVELDB_CMAKE_ARGS}
  BUILD_BYPRODUCTS  ${LEVELDB_BYPRODUCTS}
  BUILD_IN_SOURCE   1
  BUILD_COMMAND     ${DEFAULT_BUILD_CMD}
  INSTALL_COMMAND   ${LEVELDB_INSTALL_CMD}
  URL               ${LEVELDB_URL}
  URL_HASH          ${LEVELDB_HASH}
)

# cURL: Command line tool and library for transferring data with URLs.
# https://curl.haxx.se
######################################################################
#EXTERNAL(curl ${CURL_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
#
## This is named `libcurl` to not conflict with the executable.
#add_library(libcurl ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
#add_dependencies(libcurl ${CURL_TARGET})
#
#set(CURL_CMAKE_ARGS ${CMAKE_FORWARD_ARGS}
#  -DBUILD_TESTING=OFF
#  -DCURL_STATICLIB=ON
#)
#
#set_target_properties(
#  libcurl PROPERTIES
#  INTERFACE_COMPILE_DEFINITIONS CURL_STATICLIB
#)
#
#set_target_properties(
#  libcurl PROPERTIES
#  IMPORTED_LOCATION ${CURL_ROOT}-build/lib/libcurl${LIBRARY_SUFFIX}
#  INTERFACE_INCLUDE_DIRECTORIES ${CURL_ROOT}/include
#)
#
#set(CURL_CONFIG_CMD ${CURL_ROOT}/configure ${DEPS_CONFIG})
#
#MAKE_INCLUDE_DIR(libcurl)
#GET_BYPRODUCTS(libcurl)
#
#ExternalProject_Add(
#  ${CURL_TARGET}
#  PREFIX            ${CURL_CMAKE_ROOT}
#  BUILD_BYPRODUCTS  ${LIBCURL_BYPRODUCTS};${CURL_BYPRODUCTS}
#  CMAKE_ARGS        ${CURL_CMAKE_ARGS}
#  #CONFIGURE_COMMAND ${CURL_CONFIG_CMD}
#  BUILD_COMMAND     ${DEFAULT_BUILD_CMD}
#  INSTALL_COMMAND   ${DEFAULT_INSTALL_CMD}
#  URL               ${CURL_URL}
#  URL_HASH          ${CURL_HASH}
#)
#
#
# Hiredis: Hiredis is a minimalistic C client library for the Redis database.
# https://github.com/redis/hiredis
################################
#EXTERNAL(hiredis ${HIREDIS_VERSION} ${CMAKE_CURRENT_BINARY_DIR})
#add_library(hiredis ${LIBRARY_LINKAGE} IMPORTED GLOBAL)
#add_dependencies(hiredis ${HIREDIS_TARGET})
#
#set_target_properties(
#  hiredis PROPERTIES
#  IMPORTED_LOCATION ${HIREDIS_ROOT}-build/libhiredis${LIBRARY_SUFFIX}
#  INTERFACE_INCLUDE_DIRECTORIES ${HIREDIS_ROOT})
#
#MAKE_INCLUDE_DIR(hiredis)
#GET_BYPRODUCTS(hiredis)
#
#ExternalProject_Add(
#  ${HIREDIS_TARGET}
#  PREFIX            ${HIREDIS_CMAKE_ROOT}
#  BUILD_BYPRODUCTS  ${HIREDIS_BYPRODUCTS}
#  CMAKE_ARGS        ${CMAKE_FORWARD_ARGS}
#  BUILD_COMMAND     ${DEFAULT_BUILD_CMD}
#  INSTALL_COMMAND   ${CMAKE_NOOP}
#  URL               ${HIREDIS_URL}
#  URL_HASH          ${HIREDIS_HASH})
#
# In-tree dependencies.
#######################
