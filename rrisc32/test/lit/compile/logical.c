// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int f();
int g();

void h() {
  short h1, h2;
  long long q1, q2;

  h1 && h2;
  q1 && q2;

  h1 || h2;
  q1 || q2;

  f() && g();

  f() || g();
}

// CC:          .global $h
// CC-NEXT:     .type $h, "function"
// CC-NEXT:     .align 2
// CC-NEXT: h:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -24
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     beqz a0, $.LL_1.and.end
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT: .LL_1.and.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     or a0, a0, a1
// CC-NEXT:     beqz a0, $.LL_2.and.end
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     or a0, a0, a1
// CC-NEXT: .LL_2.and.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     lh a0, fp, -4
// CC-NEXT:     bnez a0, $.LL_3.or.end
// CC-NEXT:     lh a0, fp, -8
// CC-NEXT: .LL_3.or.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     lw a0, fp, -16
// CC-NEXT:     lw a1, fp, -12
// CC-NEXT:     or a0, a0, a1
// CC-NEXT:     bnez a0, $.LL_4.or.end
// CC-NEXT:     lw a0, fp, -24
// CC-NEXT:     lw a1, fp, -20
// CC-NEXT:     or a0, a0, a1
// CC-NEXT: .LL_4.or.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     call $f
// CC-NEXT:     beqz a0, $.LL_5.and.end
// CC-NEXT:     call $g
// CC-NEXT: .LL_5.and.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     call $f
// CC-NEXT:     bnez a0, $.LL_6.or.end
// CC-NEXT:     call $g
// CC-NEXT: .LL_6.or.end:
// CC-NEXT:     snez a0, a0
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $h, -($. $h)
