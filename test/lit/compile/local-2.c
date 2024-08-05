// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo {
  char c;
  int i;
  char s[2];
  char *p;
};

void f(int i) {
    {
        int arr[] = {1, 2, 3};
        char *s = "hello";
    }
    {
        struct Foo f = {'a', 10, "b", "c"};
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
// CC-NEXT:     li a0, 12
// CC-NEXT:     push a0
// CC-NEXT:     li a0, 0
// CC-NEXT:     push a0
// CC-NEXT:     addi a0, fp, -12
// CC-NEXT:     push a0
// CC-NEXT:     call $__builtin_memset
// CC-NEXT:     addi sp, sp, 12
// CC-NEXT:     li a0, 1
// CC-NEXT:     sw fp, a0, -12
// CC-NEXT:     li a0, 2
// CC-NEXT:     sw fp, a0, -8
// CC-NEXT:     li a0, 3
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT:     li a0, $.LS_1
// CC-NEXT:     sw fp, a0, -16
// CC-NEXT:     li a0, 16
// CC-NEXT:     push a0
// CC-NEXT:     li a0, 0
// CC-NEXT:     push a0
// CC-NEXT:     addi a0, fp, -16
// CC-NEXT:     push a0
// CC-NEXT:     call $__builtin_memset
// CC-NEXT:     addi sp, sp, 12
// CC-NEXT:     li a0, 97
// CC-NEXT:     sb fp, a0, -16
// CC-NEXT:     li a0, 10
// CC-NEXT:     sw fp, a0, -12
// CC-NEXT:     li a0, 98
// CC-NEXT:     sb fp, a0, -8
// CC-NEXT:     li a0, 0
// CC-NEXT:     sb fp, a0, -7
// CC-NEXT:     li a0, $.LS_2
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
// CC-NEXT:
// CC-NEXT:     .rodata
// CC-NEXT: .LS_1:
// CC-NEXT:     .asciz "hello"
// CC-NEXT: .LS_2:
// CC-NEXT:     .asciz "c"

