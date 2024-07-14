cmake_minimum_required(VERSION 3.20.0)
project(rrisc32
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CHMOD ${CMAKE_CURRENT_SOURCE_DIR}/setup_python.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
add_custom_target(setup_python
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/setup_python.sh
)

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test EXCLUDE_FROM_ALL)
