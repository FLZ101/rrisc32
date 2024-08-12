// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f() {
  short h;
  long long q;

  +h;
  +q;

  -h;
  -q;

  ~h;
  ~q;

  !h;
  !q;
}

// CC:          .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -12
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     lw a0, fp, -12
// CC-NEXT:     lw a1, fp, -8
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     neg a0, a0
// CC-NEXT:     li a1, 0x0
// CC-NEXT:     li a0, 0x1
// CC-NEXT:     push a1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -12
// CC-NEXT:     lw a1, fp, -8
// CC-NEXT:     not a1, a1
// CC-NEXT:     not a0, a0
// CC-NEXT:     pop a2
// CC-NEXT:     pop a3
// CC-NEXT:     add     a0, a0, a2
// CC-NEXT:     sltu    a2, a0, a2
// CC-NEXT:     add     a1, a1, a3
// CC-NEXT:     add     a1, a1, a2
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     not a0, a0
// CC-NEXT:     lw a0, fp, -12
// CC-NEXT:     lw a1, fp, -8
// CC-NEXT:     not a1, a1
// CC-NEXT:     not a0, a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     seqz a0, a0
// CC-NEXT:     lw a0, fp, -12
// CC-NEXT:     lw a1, fp, -8
// CC-NEXT:     or a0, a0, a1
// CC-NEXT:     seqz a0, a0
// CC-NEXT:     li a1, 0
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
