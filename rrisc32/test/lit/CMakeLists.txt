configure_file(lit.sh.in lit.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    @ONLY)
add_custom_target(rrisc32-check-lit
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lit.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(rrisc32-check-lit setup-python
    rrisc32-as rrisc32-dump rrisc32-link rrisc32-emulate rrisc32-compile)
