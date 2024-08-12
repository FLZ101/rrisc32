// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
  while (i) {
    f();
    continue;
    f();
  }

  do {
    f();
    continue;
    f();
  } while (i);

  for (; i; ++i) {
    f();
    continue;
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
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_2.while.end
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_1.while.start
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_1.while.start
// CC-NEXT: .LL_2.while.end:
// CC-NEXT: .LL_3.do.start:
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_4.do.next
// CC-NEXT:     call $f
// CC-NEXT: .LL_4.do.next:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     bnez a0, .LL_3.do.start
// CC-NEXT: .LL_5.do.end:
// CC-NEXT: .LL_6.for.start:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_8.for.end
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_7.for.next
// CC-NEXT:     call $f
// CC-NEXT: .LL_7.for.next:
// CC-NEXT:     li a0, 1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     pop a2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, 8
// CC-NEXT:     mv a0, a2
// CC-NEXT:     j $.LL_6.for.start
// CC-NEXT: .LL_8.for.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
