// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int f() {
    static int i;
    static int j = 10;
    {
        static int i = 11;
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
// CC-NEXT:     lw a0, +($f.i.3 0)
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     lw a0, +($f.i.1 0)
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
// CC-NEXT:

// CC:          .data
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f.j.2:
// CC-NEXT:     .dw 10
// CC-NEXT:     .local $f.j.2
// CC-NEXT:     .type $f.j.2, "object"
// CC-NEXT:     .size $f.j.2, -($. $f.j.2)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f.i.3:
// CC-NEXT:     .dw 11
// CC-NEXT:     .local $f.i.3
// CC-NEXT:     .type $f.i.3, "object"
// CC-NEXT:     .size $f.i.3, -($. $f.i.3)

// CC:          .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f.i.1:
// CC-NEXT:     .fill 4
// CC-NEXT:     .local $f.i.1
// CC-NEXT:     .type $f.i.1, "object"
// CC-NEXT:     .size $f.i.1, -($. $f.i.1)
