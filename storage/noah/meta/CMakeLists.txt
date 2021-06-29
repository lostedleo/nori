file(GLOB SOURCES *.cc *.h)
add_style_check_target(meta_lint "${SOURCES}")

set(MAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/meta_main.cc)
list(REMOVE_ITEM SOURCES ${MAIN_FILE})

nori_protobuf_generate(PROTO_SRC PROTO_HEADER meta.proto)
add_library(meta_proto ${PROTO_SRC})
target_link_libraries(meta_proto protobuf)

add_library(meta ${SOURCES})
target_link_libraries(meta noah_common)

add_executable(meta_server ${MAIN_FILE})
target_link_libraries(meta_server meta)
