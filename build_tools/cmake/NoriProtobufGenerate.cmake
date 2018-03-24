# Copyright (C) 2018 Zhenwei Zhu
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the author(s) be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.


function(NORI_PROTOBUF_GENERATE SRCS HDRS)
  cmake_parse_arguments(nori_protobuf_generate "" "EXPORT_MACRO" "" ${ARGN})

  set(PROTO_FILES "${nori_protobuf_generate_UNPARSED_ARGUMENTS}")
  if(NOT PROTO_FILES)
    message(SEND_ERROR "Error: NORI_PROTOBUF_GENERATE() called without any proto files")
    return()
  endif()

  set(${SRCS})
  set(${HDRS})
  # Create an include path for each file specified
  foreach(_file ${PROTO_FILES})
    get_filename_component(_abs_file ${_file} ABSOLUTE)
    get_filename_component(_abs_path ${_abs_file} PATH)
    list(FIND _protobuf_include_path ${_abs_path} _contains_already)
    if(${_contains_already} EQUAL -1)
      list(APPEND _protobuf_include_path -I ${_abs_path})
    endif()
  endforeach()

  foreach(PROTO ${PROTO_FILES})
    get_filename_component(PROTO_WE ${PROTO} NAME_WE)
    get_filename_component(ABS_FILE ${PROTO} ABSOLUTE)
    set(_generated_srcs)
    list(APPEND _generated_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
    list(APPEND _generated_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.h")
    list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
    list(APPEND HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.h")

    add_custom_command(
      OUTPUT ${_generated_srcs}
      COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FILE}
      DEPENDS ${ABS_FILE} protoc
      COMMENT "Running nori_protobuf_generate protocol buffer compiler on ${PROTO}"
      VERBATIM
    )
  endforeach()

  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)

endfunction()
