// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

char g1[] = "a";
char *g2 = "b";
char g3[] = {'c', '\0'};

void f() {
  static char s1[] = "d";
  static char *s2 = "e";
  static char s3[] = {'f', '\0'};

  {
    char l1[] = "g";
    char *l2 = "h";
    char l3[] = {'i', '\0'};
  }
}

// CC:          .text
// CC-NEXT:
// CC-NEXT:     .global $f
// CC-NEXT:     .type $f, "function"
// CC-NEXT:     .align 2
// CC-NEXT: f:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -12
// CC-NEXT:     li a0, 103
// CC-NEXT:     sb fp, a0, -4
// CC-NEXT:     li a0, 0
// CC-NEXT:     sb fp, a0, -3
// CC-NEXT:     li a0, $.LS_3
// CC-NEXT:     sw fp, a0, -8
// CC-NEXT:     li a0, 2
// CC-NEXT:     push a0
// CC-NEXT:     li a0, 0
// CC-NEXT:     push a0
// CC-NEXT:     addi a0, fp, -12
// CC-NEXT:     push a0
// CC-NEXT:     call $__builtin_memset
// CC-NEXT:     addi sp, sp, 12
// CC-NEXT:     li a0, 105
// CC-NEXT:     sb fp, a0, -12
// CC-NEXT:     li a0, 0
// CC-NEXT:     sb fp, a0, -11
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
// CC-NEXT:
// CC-NEXT:     .local $__builtin_memset
// CC-NEXT:     .type $__builtin_memset, "function"
// CC-NEXT:     .align 2
// CC-NEXT: __builtin_memset:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     lw a1, fp, 12
// CC-NEXT:     lw a2, fp, 16
// CC-NEXT:     blez a2, $2f
// CC-NEXT:     add a2, a2, a0
// CC-NEXT:     mv a3, a0
// CC-NEXT: 1:
// CC-NEXT:     addi a4, a3, 1
// CC-NEXT:     sb a3, a1, 0
// CC-NEXT:     mv a3, a4
// CC-NEXT:     bltu a4, a2, $1b
// CC-NEXT: 2:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $__builtin_memset, -($. $__builtin_memset)

// CC:          .rodata
// CC-NEXT: .LS_1:
// CC-NEXT:     .asciz "b"
// CC-NEXT: .LS_2:
// CC-NEXT:     .asciz "e"
// CC-NEXT: .LS_3:
// CC-NEXT:     .asciz "h"

// CC:          .data
// CC-NEXT:
// CC-NEXT: g1:
// CC-NEXT:     .asciz "a"
// CC-NEXT:     .global $g1
// CC-NEXT:     .type $g1, "object"
// CC-NEXT:     .size $g1, -($. $g1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: g2:
// CC-NEXT:     .dw $.LS_1
// CC-NEXT:     .global $g2
// CC-NEXT:     .type $g2, "object"
// CC-NEXT:     .size $g2, -($. $g2)
// CC-NEXT:
// CC-NEXT: g3:
// CC-NEXT:     .db 99
// CC-NEXT:     .db 0
// CC-NEXT:     .global $g3
// CC-NEXT:     .type $g3, "object"
// CC-NEXT:     .size $g3, -($. $g3)
// CC-NEXT:
// CC-NEXT: f.__func__.1:
// CC-NEXT:     .asciz "f"
// CC-NEXT:     .local $f.__func__.1
// CC-NEXT:     .type $f.__func__.1, "object"
// CC-NEXT:     .size $f.__func__.1, -($. $f.__func__.1)
// CC-NEXT:
// CC-NEXT: f.s1.2:
// CC-NEXT:     .asciz "d"
// CC-NEXT:     .local $f.s1.2
// CC-NEXT:     .type $f.s1.2, "object"
// CC-NEXT:     .size $f.s1.2, -($. $f.s1.2)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f.s2.3:
// CC-NEXT:     .dw $.LS_2
// CC-NEXT:     .local $f.s2.3
// CC-NEXT:     .type $f.s2.3, "object"
// CC-NEXT:     .size $f.s2.3, -($. $f.s2.3)
// CC-NEXT:
// CC-NEXT: f.s3.4:
// CC-NEXT:     .db 102
// CC-NEXT:     .db 0
// CC-NEXT:     .local $f.s3.4
// CC-NEXT:     .type $f.s3.4, "object"
// CC-NEXT:     .size $f.s3.4, -($. $f.s3.4)
