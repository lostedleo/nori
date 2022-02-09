file(GLOB SOURCES *.cc *.h)
add_style_check_target(node_lint "${SOURCES}")

set(MAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/node_main.cc)
list(REMOVE_ITEM SOURCES ${MAIN_FILE})

nori_protobuf_generate(PROTO_SRC PROTO_HEADER node.proto)
add_library(node_proto ${PROTO_SRC})
target_link_libraries(node_proto protobuf)

add_library(node ${SOURCES})
target_link_libraries(node noah_common node_proto)

add_executable(node_server ${MAIN_FILE})
target_link_libraries(node_server node ${DYNAMIC_LIB})
