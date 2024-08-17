file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
    SYMBOLIC)

set(C_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
    lib/stdio.c
    lib/printf.c
    lib/string.c
    lib/setjmp.c
)

set(H_FILES
    include/rrisc32.h
    include/stdarg.h
    include/stdio.h
    include/string.h
    include/setjmp.h
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
    COMMAND ${CMAKE_BINARY_DIR}/python/bin/python gen_syscall.py
        ${CMAKE_CURRENT_BINARY_DIR}/lib/syscall.c
    DEPENDS setup-python include/syscall.inc gen_syscall.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/lib/libc.a
    COMMAND ${CMAKE_BINARY_DIR}/bin/rrisc32-cc
        --archive --include include --nostdinc
        -o ${CMAKE_BINARY_DIR}/lib/libc.a
        ${C_FILES}
    DEPENDS rrisc32-compile ${CMAKE_BINARY_DIR}/bin/rrisc32-cc ${C_FILES} ${H_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/lib/crt.o
    COMMAND ${CMAKE_BINARY_DIR}/bin/rrisc32-cc
        --assemble
        -o ${CMAKE_BINARY_DIR}/lib/crt.o
       lib/crt.s
    DEPENDS rrisc32-compile ${CMAKE_BINARY_DIR}/bin/rrisc32-cc lib/crt.s
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(libc ALL
    DEPENDS ${CMAKE_BINARY_DIR}/lib/libc.a
            ${CMAKE_BINARY_DIR}/lib/crt.o)

add_subdirectory(test EXCLUDE_FROM_ALL)
