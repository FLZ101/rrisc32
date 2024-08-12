// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
  do {
    f();
  } while (0);

  do {
    f();
  } while (1);

  do {
    f();
  } while (i);
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT: .LL_1.do.start:
// CC-NEXT:     call $f
// CC-NEXT: .LL_2.do.next:
// CC-NEXT: .LL_3.do.end:
// CC-NEXT: .LL_4.do.start:
// CC-NEXT:     call $f
// CC-NEXT: .LL_5.do.next:
// CC-NEXT:     j $.LL_4.do.start
// CC-NEXT: .LL_6.do.end:
// CC-NEXT: .LL_7.do.start:
// CC-NEXT:     call $f
// CC-NEXT: .LL_8.do.next:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     bnez a0, $.LL_7.do.start
// CC-NEXT: .LL_9.do.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
