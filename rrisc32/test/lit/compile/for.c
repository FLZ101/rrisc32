// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
  for (;;)
    f();

  for (int i = 0;; ++i)
    f();

  for (int i = 0; i < 10; ++i)
    f();
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -4
// CC-NEXT: .LL_1.for.start:
// CC-NEXT:     call $f
// CC-NEXT: .LL_2.for.next:
// CC-NEXT:     j $.LL_1.for.start
// CC-NEXT: .LL_3.for.end:
// CC-NEXT:     li a0, 0
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT: .LL_4.for.start:
// CC-NEXT:     call $f
// CC-NEXT: .LL_5.for.next:
// CC-NEXT:     li a0, 1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -4
// CC-NEXT:     mv a0, a2
// CC-NEXT:     j $.LL_4.for.start
// CC-NEXT: .LL_6.for.end:
// CC-NEXT:     li a0, 0
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT: .LL_7.for.start:
// CC-NEXT:     li a0, 10
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     slt a0, a0, a2
// CC-NEXT:     beqz a0, .LL_9.for.end
// CC-NEXT:     call $f
// CC-NEXT: .LL_8.for.next:
// CC-NEXT:     li a0, 1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -4
// CC-NEXT:     mv a0, a2
// CC-NEXT:     j $.LL_7.for.start
// CC-NEXT: .LL_9.for.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
