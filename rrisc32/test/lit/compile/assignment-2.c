// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo {
  char c;
  int i;
} f1;

void f()
{
    struct Foo f2 = f1;

    f2 = f1;
}

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -8
// CC-NEXT:     li a0, 8
// CC-NEXT:     push a0
// CC-NEXT:     li a0, $f1
// CC-NEXT:     push a0
// CC-NEXT:     addi a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     call $__builtin_memcpy
// CC-NEXT:     addi sp, sp, 12
// CC-NEXT:     li a0, 8
// CC-NEXT:     push a0
// CC-NEXT:     li a0, $f1
// CC-NEXT:     push a0
// CC-NEXT:     addi a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     call $__builtin_memcpy
// CC-NEXT:     addi sp, sp, 12
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)

// CC:          .local $__builtin_memcpy
// CC-NEXT:     .type $__builtin_memcpy, "function"
// CC-NEXT:     .align 2
// CC-NEXT: __builtin_memcpy:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     lw a1, fp, 12
// CC-NEXT:     lw a2, fp, 16
// CC-NEXT:     blez    a2, $2f
// CC-NEXT:     add     a3, a1, a2
// CC-NEXT: 1:
// CC-NEXT:     lbu     a4, a1, 0
// CC-NEXT:     addi    a5, a1, 1
// CC-NEXT:     addi    a2, a0, 1
// CC-NEXT:     sb      a0, a4, 0
// CC-NEXT:     mv      a0, a2
// CC-NEXT:     mv      a1, a5
// CC-NEXT:     bltu    a5, a3, $1b
// CC-NEXT:     mv      a0, a2
// CC-NEXT:     ret
// CC-NEXT: 2:
// CC-NEXT:     ret
// CC-NEXT:     .size $__builtin_memcpy, -($. $__builtin_memcpy)
// CC-NEXT:

// CC:          .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f1:
// CC-NEXT:     .fill 8
// CC-NEXT:     .global $f1
// CC-NEXT:     .type $f1, "object"
// CC-NEXT:     .size $f1, -($. $f1)
