    .text

    .global $start
    .type $start, "function"
    .align 2
start:
    push a1 # argv
    push a0 # argc
    call $main
    addi sp, sp, 8
    # exit
    mv a1, a0
    li a0, 0
    ecall
    .size $start, -($. $start)
