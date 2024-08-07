// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int a1[] = {1, 2, 3};

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

  a1[1] = 10;

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

  { a1[1] += 10; }

  // CC-NEXT:     li a0, 10
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, 1
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, $a1
  // CC-NEXT:     pop a2
  // CC-NEXT:     slli a2, a2, 2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     sw fp, a0, -44
  // CC-NEXT:     lw a0, fp, -44
  // CC-NEXT:     lw a0, a0, 0
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     lw a0, fp, -44
  // CC-NEXT:     sw a0, a2, 0

  a2[3] = 30;

  // CC-NEXT:     li a0, 30
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, 3
  // CC-NEXT:     push a0
  // CC-NEXT:     addi a0, fp, -40
  // CC-NEXT:     pop a2
  // CC-NEXT:     slli a2, a2, 2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw a0, a2, 0

  { a2[3] += 30; }

  // CC-NEXT:     li a0, 30
  // CC-NEXT:     push a0
  // CC-NEXT:     li a0, 3
  // CC-NEXT:     push a0
  // CC-NEXT:     addi a0, fp, -40
  // CC-NEXT:     pop a2
  // CC-NEXT:     slli a2, a2, 2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     sw fp, a0, -44
  // CC-NEXT:     lw a0, fp, -44
  // CC-NEXT:     lw a0, a0, 0
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     lw a0, fp, -44
  // CC-NEXT:     sw a0, a2, 0
}

// CC:          .data
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: a1:
// CC-NEXT:     .dw 1
// CC-NEXT:     .dw 2
// CC-NEXT:     .dw 3
// CC-NEXT:     .global $a1
// CC-NEXT:     .type $a1, "object"
// CC-NEXT:     .size $a1, -($. $a1)
