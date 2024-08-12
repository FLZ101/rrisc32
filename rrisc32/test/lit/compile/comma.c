// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

void f();

void g() {
  int i;
  i = (1, f(), i + 1);
}

// CC:          .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -4
// CC-NEXT:     call $f
// CC-NEXT:     li a0, 1
// CC-NEXT:     push a0
// CC-NEXT:     lw a0, fp, -4
// CC-NEXT:     pop a2
// CC-NEXT:     add a0, a0, a2
// CC-NEXT:     push a0
// CC-NEXT:     pop a2
// CC-NEXT:     sw fp, a2, -4
// CC-NEXT:     mv a0, a2
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
