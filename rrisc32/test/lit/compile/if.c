// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();
void g();

void h(int i) {
  if (1)
    f();

  if (1)
    ;
  else
    g();

  if (1)
    f();
  else
    g();

  if (0)
    f();
  else
    g();

  if (i)
    f();
  else
    g();
}

// CC:          .global $h
// CC-NEXT:     .type $h, "function"
// CC-NEXT:     .align 2
// CC-NEXT: h:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_2.if.end
// CC-NEXT: .LL_1.if.false:
// CC-NEXT: .LL_2.if.end:
// CC-NEXT:     j $.LL_4.if.end
// CC-NEXT: .LL_3.if.false:
// CC-NEXT:     call $g
// CC-NEXT: .LL_4.if.end:
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_6.if.end
// CC-NEXT: .LL_5.if.false:
// CC-NEXT:     call $g
// CC-NEXT: .LL_6.if.end:
// CC-NEXT:     j .LL_7.if.false
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_8.if.end
// CC-NEXT: .LL_7.if.false:
// CC-NEXT:     call $g
// CC-NEXT: .LL_8.if.end:
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_9.if.false
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_10.if.end
// CC-NEXT: .LL_9.if.false:
// CC-NEXT:     call $g
// CC-NEXT: .LL_10.if.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $h, -($. $h)
