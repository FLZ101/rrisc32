// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

long long ll;

void f() {
  // CC:          .global $f
  // CC-NEXT:     .type $f, "function"
  // CC-NEXT:     .align 2
  // CC-NEXT: f:
  // CC-NEXT:     push ra
  // CC-NEXT:     push fp
  // CC-NEXT:     mv fp, sp
  // CC-NEXT:     addi sp, sp, -8

  int i = ll; // -4

  // CC-NEXT:     lw a0, +($ll 0)
  // CC-NEXT:     lw a1, +($ll 4)
  // CC-NEXT:     sw fp, a0, -4

  short s; // -8
  s = ll;

  // CC-NEXT:     lw a0, +($ll 0)
  // CC-NEXT:     lw a1, +($ll 4)
  // CC-NEXT:     sext.h a0, a0
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sh fp, a2, -8
  // CC-NEXT:     mv a0, a2

  i += 1;

  // CC-NEXT:     li a0, 1
  // CC-NEXT:     push a0
  // CC-NEXT:     lw a0, fp, -4
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw fp, a2, -4
  // CC-NEXT:     mv a0, a2

  i += s;

  // CC-NEXT:     lh a0, fp, -8
  // CC-NEXT:     push a0
  // CC-NEXT:     lw a0, fp, -4
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sw fp, a2, -4
  // CC-NEXT:     mv a0, a2

  s += i;

  // CC-NEXT:     lw a0, fp, -4
  // CC-NEXT:     push a0
  // CC-NEXT:     lh a0, fp, -8
  // CC-NEXT:     pop a2
  // CC-NEXT:     add a0, a0, a2
  // CC-NEXT:     sext.h a0, a0
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     sh fp, a2, -8
  // CC-NEXT:     mv a0, a2

  ll += s;

  // CC-NEXT:     lh a0, fp, -8
  // CC-NEXT:     srai a1, a0, 31
  // CC-NEXT:     push a1
  // CC-NEXT:     push a0
  // CC-NEXT:     lw a0, +($ll 0)
  // CC-NEXT:     lw a1, +($ll 4)
  // CC-NEXT:     pop a2
  // CC-NEXT:     pop a3
  // CC-NEXT:     add     a0, a0, a2
  // CC-NEXT:     sltu    a2, a0, a2
  // CC-NEXT:     add     a1, a1, a3
  // CC-NEXT:     add     a1, a1, a2
  // CC-NEXT:     push a1
  // CC-NEXT:     push a0
  // CC-NEXT:     pop a2
  // CC-NEXT:     pop a3
  // CC-NEXT:     sw a2, +($ll 0)
  // CC-NEXT:     sw a3, +($ll 4)
  // CC-NEXT:     mv a0, a2
  // CC-NEXT:     mv a1, a3

  // CC-NEXT:     mv sp, fp
  // CC-NEXT:     pop fp
  // CC-NEXT:     pop ra
  // CC-NEXT:     ret
  // CC-NEXT:     .size $f, -($. $f)
}

// CC:          .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: ll:
// CC-NEXT:     .fill 8
// CC-NEXT:     .global $ll
// CC-NEXT:     .type $ll, "object"
// CC-NEXT:     .size $ll, -($. $ll)
