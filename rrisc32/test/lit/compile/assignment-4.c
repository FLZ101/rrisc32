// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo {
  char c;
  int i;
} f1;

void f() {
  // CC:          .global $f
  // CC-NEXT:     .type $f, "function"
  // CC-NEXT:     .align 2
  // CC-NEXT: f:
  // CC-NEXT:     push ra
  // CC-NEXT:     push fp
  // CC-NEXT:     mv fp, sp
  // CC-NEXT:     addi sp, sp, -12

  int i;
  struct Foo f2;

  i = f1.i = 10;

  // CC-NEXT:     li a0, 10
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, 4
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, $f1
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw a0, a2, 0
  // CC-NEXT:     mv a0, a2
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw fp, a2, -4
  // CC-NEXT:     mv a0, a2

  i = f2.i;

  // CC-NEXT:     li a0, 4
  // CC-NEXT:     push a0
  // CC-NEXT:     addi a0, fp, -12
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     lw a0, a0, 0
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw fp, a2, -4
  // CC-NEXT:     mv a0, a2

  i += f2.i;

  // CC-NEXT:     li a0, 4
  // CC-NEXT:     push a0
  // CC-NEXT:     addi a0, fp, -12
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     lw a0, a0, 0
  // CC-NEXT:     push a0
  // CC-NEXT:     lw a0, fp, -4
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw fp, a2, -4
  // CC-NEXT:     mv a0, a2

  // CC-NEXT:     mv sp, fp
  // CC-NEXT:     pop fp
  // CC-NEXT:     pop ra
  // CC-NEXT:     ret
  // CC-NEXT:     .size $f, -($. $f)
}
