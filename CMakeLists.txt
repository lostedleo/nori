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
cmake_minimum_required(VERSION 3.10...3.21)

if(APPLE)
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
else ()
endif()

project(NORI)

# CMAKE MODULE SETUP.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/build_tools/cmake)

include(CppLint)
include(NoriProtobufGenerate)

option(BUILD_SHARED_LIBS "build shared lib" OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # require at least gcc 4.8
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "GCC is too old, please install a newer version supporting C++11")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # require at least clang 3.3
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
    message(FATAL_ERROR "Clang is too old, please install a newer version supporting C++11")
  endif()
endif()

# Use C++11
macro(use_cxx11)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro(use_cxx11)

use_cxx11()

# Set CXX_FLAGS
set(GFLAGS_NS google)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CPP_FLAGS "-Wno-unused-local-typedef -Wno-inconsistent-missing-override")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
  if(NOT HAVE_CLOCK_GETTIME)
    set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEFINE_CLOCK_GETTIME}")
  endif()

  # Homebrew installed OpenSSL
  set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -pipe -O2 -Wall -W -fPIC -fstrict-aliasing -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-invalid-offsetof -Wno-sign-compare")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DGFLAGS_NS=${GFLAGS_NS}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #required by butil/crc32.cc to boost performance for 10x
  if((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
  elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
      # segmentation fault in libcontext
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-gcse")
  endif()
  if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new -Wno-format-security -Wno-implicit-fallthrough")
  endif()
endif()

# In-tree dependencies.
#######################
add_subdirectory(third_party)
add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(raft)
add_subdirectory(storage)
add_subdirectory(test)
