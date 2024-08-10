configure_file(build.sh.in build.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    @ONLY)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/rrisc32-cc
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/build.sh
    DEPENDS setup_python main.py sema.py codegen.py requirements.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(rrisc32-compile ALL
    DEPENDS ${CMAKE_BINARY_DIR}/bin/rrisc32-cc)
