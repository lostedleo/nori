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
cmake_minimum_required(VERSION 3.10.0)

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

macro(use_cxx11)
  if(CMAKE_VERSION VERSION_LESS "3.1.3")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
  else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
  endif()
endmacro(use_cxx11)

use_cxx11()
# In-tree dependencies.
#######################
add_subdirectory(third_party)
add_subdirectory(net)
add_subdirectory(raft)
