# for protoc-gen-mcpack
set(EXECUTABLE_OUTPUT_PATH ${BRPC_OUTPUT_BIN})

set(protoc_gen_mcpack_SOURCES
  ${CMAKE_SOURCE_DIR}/net/mcpack2pb/generator.cpp
  )

add_executable(protoc-gen-mcpack ${protoc_gen_mcpack_SOURCES})
if (BUILD_SHARED_LIBS)
  target_link_libraries(protoc-gen-mcpack brpc-shared glog)
else ()
  target_link_libraries(protoc-gen-mcpack brpc-static glog)
endif ()
