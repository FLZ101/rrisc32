// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

#pragma ASM ; start: ; mv a0, a1 ; add a0, a0, a1

// CC:          .text
// CC-NEXT:
// CC-NEXT: start:
// CC-NEXT:     mv a0, a1
// CC-NEXT:     add a0, a0, a1
