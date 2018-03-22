#include(FindProtobuf)
#protobuf_generate_cpp(PROTO_SRC PROTO_HEADER view.proto)
file(GLOB PROTOS "*.proto")
set(PROTO_SRCS "")
list(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_BINARY_DIR})
foreach(PROTO ${PROTOS})
  get_filename_component(PROTO_WE ${PROTO} NAME_WE)
  list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
  add_custom_command(
    OUTPUT "${PROTO_WE}.pb.h" "${PROTO_WE}.pb.cc"
    COMMAND protoc ${PROTO_FLAGS} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${PROTO}
    DEPENDS ${PROTO}
    )
endforeach()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(rpc_view rpc_view.cpp ${PROTO_SRCS})
target_link_libraries(rpc_view brpc-static ${DYNAMIC_LIB})
