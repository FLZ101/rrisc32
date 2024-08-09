// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int f() {
    int i = 10;
    {
        int i = i;
        return i;
    }
    return i;
}

// CC:          .text
// CC-NEXT:
// CC-NEXT:     .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -8
// CC-NEXT:     li a0, 10
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     sw fp, a0, -8
// CC-NEXT:     lw a0, fp, -8
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
