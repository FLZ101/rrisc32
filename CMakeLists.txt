cmake_minimum_required(VERSION 3.20.0)
project(rrisc32
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test EXCLUDE_FROM_ALL)
