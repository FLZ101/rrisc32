// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f()
{
    {
        int arr[3];
        int *p = arr;
    }
    {
        int (*pArr)[3];
        int *p = *pArr;
    }
}

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -16
// CC-NEXT:     addi a0, fp, -12
// CC-NEXT:     sw fp, a0, -16
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     sw fp, a0, -8
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
