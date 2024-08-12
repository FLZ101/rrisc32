configure_file(lit.sh.in lit.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    @ONLY)
add_custom_target(libc-check
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lit.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(libc-check setup-python
    libc
    rrisc32-compile)
