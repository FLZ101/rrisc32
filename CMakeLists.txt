cmake_minimum_required(VERSION 3.20.0)
project(rrisc32
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  elfio
  GIT_REPOSITORY https://github.com/serge1/ELFIO.git
  GIT_TAG        8ae6cec5d60495822ecd57d736f66149da9b1830 # Release_3.12
)

FetchContent_MakeAvailable(elfio)

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tools)
