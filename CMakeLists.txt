cmake_minimum_required(VERSION 3.20.0)
project(rrisc32
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(setup-python.sh.in setup-python.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    @ONLY)
add_custom_target(setup-python
    COMMAND ${CMAKE_BINARY_DIR}/setup-python.sh
    DEPENDS requirements.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_subdirectory(rrisc32)
add_subdirectory(libc)
