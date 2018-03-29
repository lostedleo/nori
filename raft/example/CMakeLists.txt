file(COPY ./shflags DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(atomic)
add_subdirectory(block)
add_subdirectory(counter)
