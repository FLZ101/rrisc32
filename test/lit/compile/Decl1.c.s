    .text

    .rodata
.L_r1:
    .asciz "b"

    .data

    .align 2
f1:
    .db 97
    .fill 3
    .dw 10
    .asciz "b"
    .fill 2
    .global $f1
    .type $f1, "object"
    .size $f1, 12

    .align 2
g1:
    .db 97
    .fill 11
    .global $g1
    .type $g1, "object"
    .size $g1, 12

    .bss

c:
    .fill 1
    .global $c
    .type $c, "object"
    .size $c, 1

    .align 2
i:
    .fill 4
    .global $i
    .type $i, "object"
    .size $i, 4

s:
    .fill 2
    .global $s
    .type $s, "object"
    .size $s, 2

