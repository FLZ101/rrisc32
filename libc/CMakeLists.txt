set(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
    COMMAND ${CMAKE_BINARY_DIR}/python/bin/python gen_syscall.py
        ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
    DEPENDS setup_python include/syscall.inc gen_syscall.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/lib/libc.a
    COMMAND ${CMAKE_BINARY_DIR}/bin/rrisc32-cc
        --archive --include include
        -o ${CMAKE_BINARY_DIR}/lib/libc.a
        ${FILES}
    DEPENDS rrisc32-compile ${FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(rrisc32-libc ALL
    DEPENDS ${CMAKE_BINARY_DIR}/lib/libc.a)
