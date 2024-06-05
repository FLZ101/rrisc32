include(FetchContent)
FetchContent_Declare(
    elfio
    GIT_REPOSITORY https://github.com/serge1/ELFIO.git
    GIT_TAG        8ae6cec5d60495822ecd57d736f66149da9b1830 # Release_3.12
)
FetchContent_MakeAvailable(elfio)

FetchContent_Declare(
    cli11
    QUIET
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)
FetchContent_MakeAvailable(cli11)

add_library(rrisc32 STATIC
    util.cpp
    rrisc32.cpp
    elf.cpp
    assembly.cpp
)
target_link_libraries(rrisc32
    PUBLIC elfio::elfio
    PUBLIC CLI11::CLI11)
