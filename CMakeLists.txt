cmake_minimum_required(VERSION 3.20.0)
project(rrisc32
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/setup_python.sh [=[
#!/bin/bash
[[ -e python ]] || python3 -m venv python
]=])
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/setup_python.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
add_custom_target(setup_python
    COMMAND ./setup_python.sh
)

add_subdirectory(rrisc32)
