// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g(int i) {
start:
  f();
  if (i)
    goto end;
  goto start;
end:;
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT: .LF.g.start:
// CC-NEXT:     call $f
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_1.if.false
// CC-NEXT:     j $.LF.g.end
// CC-NEXT:     j $.LL_2.if.end
// CC-NEXT: .LL_1.if.false:
// CC-NEXT: .LL_2.if.end:
// CC-NEXT:     j $.LF.g.start
// CC-NEXT: .LF.g.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
