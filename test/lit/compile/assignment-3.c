int a1[] = { 1, 2, 3 };

void f() {

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -44

    int a2[10];

    // a1[1] = 10;

// CC-NEXT:     li a0, 10
// CC-NEXT:     push a0
// CC-NEXT:     li a0, 1
// CC-NEXT:     push a0
// CC-NEXT:     li a0, $a1
// CC-NEXT:     pop a2
// CC-NEXT:     slli a2, a2, 2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     pop a2
// CC-NEXT:     sw a0, a2, 0

    {
        a1[1] += 10;
    }

    // a2[3] = 30;
    // {
    //     a2[3] += 30;
    // }
}
