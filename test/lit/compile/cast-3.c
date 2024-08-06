// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f() {
  char c;       // -4
  short s;      // -8
  int i;        // -12
  long long ll; // -20

  unsigned char uc;       // -24
  unsigned short us;      // -28
  unsigned int ui;        // -32
  unsigned long long ull; // -40

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -40

  c = -3;
  uc = -3;

// CC-NEXT:     li a0, -3
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sb fp, a2, -4
// CC-NEXT:     li a0, 253
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sb fp, a2, -24

  c = 3;
  uc = 3;

// CC-NEXT:     li a0, 3
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sb fp, a2, -4
// CC-NEXT:     li a0, 3
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sb fp, a2, -24

  ll = -3;
  ull = -3;

// CC-NEXT:     li a1, 0xffffffff
// CC-NEXT:     li a0, 0xfffffffd
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -20
// CC-NEXT:     sw fp, a3, -16
// CC-NEXT:     li a1, 0xffffffff
// CC-NEXT:     li a0, 0xfffffffd
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -40
// CC-NEXT:     sw fp, a3, -36

  ll = 3;
  ull = 3;

// CC-NEXT:     li a1, 0x0
// CC-NEXT:     li a0, 0x3
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -20
// CC-NEXT:     sw fp, a3, -16
// CC-NEXT:     li a1, 0x0
// CC-NEXT:     li a0, 0x3
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -40
// CC-NEXT:     sw fp, a3, -36

  c = uc;
  ull = ll;

// CC-NEXT:     lb a0, fp, -24
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sb fp, a2, -4
// CC-NEXT:     lw a0, fp, -20
// CC-NEXT:     lw a1, fp, -16
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -40
// CC-NEXT:     sw fp, a3, -36

  i = s;
  i = us;

// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -12
// CC-NEXT:     lhu a0, fp, -28
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -12

  ll = s;
  ll = us;

// CC-NEXT:     lh a0, fp, -8
// CC-NEXT:     srai a1, a0, 31
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -20
// CC-NEXT:     sw fp, a3, -16
// CC-NEXT:     lhu a0, fp, -28
// CC-NEXT:     srai a1, a0, 31
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     sw fp, a2, -20
// CC-NEXT:     sw fp, a3, -16

  i = ll;

// CC-NEXT:     lw a0, fp, -20
// CC-NEXT:     lw a1, fp, -16
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -12

  s = ll;
  us = i;

// CC-NEXT:     lw a0, fp, -20
// CC-NEXT:     lw a1, fp, -16
// CC-NEXT:     sext.h a0, a0
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sh fp, a2, -8
// CC-NEXT:     lw a0, fp, -12
// CC-NEXT:     zext.h a0, a0
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sh fp, a2, -28

// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
}
