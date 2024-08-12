// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
  while (1) {
    f();
    break;
    f();
  }

  switch (i) {
  case 1:
    f();
  case 2:
    f();
    break;
  case 3:
    f();
  }
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT: .LL_1.while.start:
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_2.while.end
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_1.while.start
// CC-NEXT: .LL_2.while.end:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     li a1, 1
// CC-NEXT:     beq a0, a1, $.LL_4.switch.case
// CC-NEXT:     li a1, 2
// CC-NEXT:     beq a0, a1, $.LL_5.switch.case
// CC-NEXT:     li a1, 3
// CC-NEXT:     beq a0, a1, $.LL_6.switch.case
// CC-NEXT: .LL_4.switch.case:
// CC-NEXT:     call $f
// CC-NEXT: .LL_5.switch.case:
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_3.switch.end
// CC-NEXT: .LL_6.switch.case:
// CC-NEXT:     call $f
// CC-NEXT: .LL_3.switch.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
