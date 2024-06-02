add_library(rrisc32 STATIC
    util.cpp
    rrisc32.cpp
    elf.cpp
    assembly.cpp
)
target_link_libraries(rrisc32 elfio::elfio)
