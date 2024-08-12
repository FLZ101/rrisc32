// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
  while (0)
    f();

  while (1)
    f();

  while (i)
    f();
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT: .LL_1.while.start:
// CC-NEXT:     j .LL_2.while.end
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_1.while.start
// CC-NEXT: .LL_2.while.end:
// CC-NEXT: .LL_3.while.start:
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_3.while.start
// CC-NEXT: .LL_4.while.end:
// CC-NEXT: .LL_5.while.start:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_6.while.end
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_5.while.start
// CC-NEXT: .LL_6.while.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
