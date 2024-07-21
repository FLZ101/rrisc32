// RUN: rrisc32-cc -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

static void f1() {}

int f2() { return 1; }

int f3(int i, int j, int k) { return k; }

// CC:         .text
// CC-NEXT:
// CC-NEXT:    .local $f1
// CC-NEXT:    .type $f1, "function"
// CC-NEXT:    .align 2
// CC-NEXT:f1:
// CC-NEXT:    push ra
// CC-NEXT:    push fp
// CC-NEXT:    mv fp, sp
// CC-NEXT:    mv sp, fp
// CC-NEXT:    pop fp
// CC-NEXT:    pop ra
// CC-NEXT:    ret
// CC-NEXT:    .size $f1, -($. $f1)
// CC-NEXT:
// CC-NEXT:    .global $f2
// CC-NEXT:    .type $f2, "function"
// CC-NEXT:    .align 2
// CC-NEXT:f2:
// CC-NEXT:    push ra
// CC-NEXT:    push fp
// CC-NEXT:    mv fp, sp
// CC-NEXT:    li a0, 1
// CC-NEXT:    mv sp, fp
// CC-NEXT:    pop fp
// CC-NEXT:    pop ra
// CC-NEXT:    ret
// CC-NEXT:    .size $f2, -($. $f2)
// CC-NEXT:
// CC-NEXT:    .global $f3
// CC-NEXT:    .type $f3, "function"
// CC-NEXT:    .align 2
// CC-NEXT:f3:
// CC-NEXT:    push ra
// CC-NEXT:    push fp
// CC-NEXT:    mv fp, sp
// CC-NEXT:    lw a0, fp, 16
// CC-NEXT:    mv sp, fp
// CC-NEXT:    pop fp
// CC-NEXT:    pop ra
// CC-NEXT:    ret
// CC-NEXT:    .size $f3, -($. $f3)
// CC-NEXT:
// CC-NEXT:    .rodata
// CC-NEXT:
// CC-NEXT:    .data
// CC-NEXT:
// CC-NEXT:    .bss
