configure_file(build.sh.in build.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    @ONLY)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rrisc32-cc
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/build.sh
    DEPENDS setup_python main.py cc.py requirements.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(rrisc32-compile DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rrisc32-cc)
