    .text

    .global $start
    .type $start, "function"
    .align 2
start:
    push a1 # argv
    push a0 # argc
    call $main
    push a0
    mv a1, a0 # exit
    li a0, 0
    ecall
