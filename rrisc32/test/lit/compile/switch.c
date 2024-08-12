// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void d();
void e();
void f();
void g();

void h(int i, int j) {
  switch (i) {

  case 1:
  d();

  if (j) {
  case 2:
    e();
    break;
  }

  default:
  f();
  }
}

// CC:          .global $h
// CC-NEXT:     .type $h, "function"
// CC-NEXT:     .align 2
// CC-NEXT: h:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     li a1, 1
// CC-NEXT:     beq a0, a1, $.LL_2.switch.case
// CC-NEXT:     li a1, 2
// CC-NEXT:     beq a0, a1, $.LL_5.switch.case
// CC-NEXT:     j $.LL_6.switch.default
// CC-NEXT: .LL_2.switch.case:
// CC-NEXT:     call $d
// CC-NEXT:     lw a0, fp, 12
// CC-NEXT:     beqz a0, .LL_3.if.false
// CC-NEXT: .LL_5.switch.case:
// CC-NEXT:     call $e
// CC-NEXT:     j $.LL_1.switch.end
// CC-NEXT:     j $.LL_4.if.end
// CC-NEXT: .LL_3.if.false:
// CC-NEXT: .LL_4.if.end:
// CC-NEXT: .LL_6.switch.default:
// CC-NEXT:     call $f
// CC-NEXT: .LL_1.switch.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $h, -($. $h)
