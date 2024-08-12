// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC


void f() {
  short h1, h2;
  long long q1, q2;

  h1 + h2;
  q1 + q2;

  h1 - h2;
  q1 - q2;

  h1 * h2;
  q1 * q2;

  h1 / h2;
  // q1 / q2;

  h1 % h2;
  // q1 % q2;

  h1 & h2;
  q1 & q2;

  h1 | h2;
  q1 | q2;

  h1 ^ h2;
  q1 ^ q2;

  h1 << h2;
  // q1 << q2;

  h1 >> h2;
  // q1 >> q2;

  h1 == h2;
  q1 == q2;

  h1 != h2;
  q1 != q2;

  h1 < h2;
  q1 < q2;

  h1 > h2;
  q1 > q2;

  h1 <= h2;
  q1 <= q2;

  h1 >= h2;
  q1 >= q2;
}

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -24
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     sltu a2, a0, a2
// CC-NEXT:     add a1, a1, a3
// CC-NEXT:     add a1, a1, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     sub a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sub a1, a1, a3
// CC-NEXT:     sltu a3, a0, a2
// CC-NEXT:     sub a1, a1, a3
// CC-NEXT:     sub a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     mul a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     mul a1, a2, a1
// CC-NEXT:     mul a3, a3, a0
// CC-NEXT:     add a1, a1, a3
// CC-NEXT:     mulhu a3, a2, a0
// CC-NEXT:     add a1, a1, a3
// CC-NEXT:     mul a0, a2, a0
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     div a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     rem a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     and a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     and a1, a1, a3
// CC-NEXT:     and a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     or a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     or a1, a1, a3
// CC-NEXT:     or a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     xor a1, a1, a3
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     sll a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     sra a0, a0, a2
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     seqz a0, a0
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     xor a1, a1, a3
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     or a0, a0, a1
// CC-NEXT:     seqz a0, a0
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     snez a0, a0
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     xor a1, a1, a3
// CC-NEXT:     xor a0, a0, a2
// CC-NEXT:     or a0, a0, a1
// CC-NEXT:     snez a0, a0
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     slt a0, a0, a2
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     beq a1, a3, $1f
// CC-NEXT:     slt a0, a1, a3
// CC-NEXT:     j $2f
// CC-NEXT: 1:
// CC-NEXT:     sltu a0, a0, a2
// CC-NEXT: 2:
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     pop a2
// CC-NEXT:     slt a0, a0, a2
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     beq a1, a3, $1f
// CC-NEXT:     slt a0, a1, a3
// CC-NEXT:     j $2f
// CC-NEXT: 1:
// CC-NEXT:     sltu a0, a0, a2
// CC-NEXT: 2:
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     pop a2
// CC-NEXT:     slt a0, a0, a2
// CC-NEXT:     xori a0, a0, 1
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     beq a1, a3, $1f
// CC-NEXT:     slt a0, a1, a3
// CC-NEXT:     j $2f
// CC-NEXT: 1:
// CC-NEXT:     sltu a0, a0, a2
// CC-NEXT: 2:
// CC-NEXT:     xori a0, a0, 1
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     slt a0, a0, a2
// CC-NEXT:     xori a0, a0, 1
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     beq a1, a3, $1f
// CC-NEXT:     slt a0, a1, a3
// CC-NEXT:     j $2f
// CC-NEXT: 1:
// CC-NEXT:     sltu a0, a0, a2
// CC-NEXT: 2:
// CC-NEXT:     xori a0, a0, 1
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
