link_libraries(rrisc32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(assemble)
add_subdirectory(dump)
add_subdirectory(link)
add_subdirectory(emulate)
add_subdirectory(compile)
