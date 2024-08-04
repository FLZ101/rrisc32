// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo { int i; };

int i1 = sizeof(int);
int i2 = sizeof(i1);

struct Foo f1;
int i3 = sizeof(struct Foo);
int i4 = sizeof(f1);

int arr1[] = { 1, 2, 3};
int i5 = sizeof(arr1);

int f(int a, int b) {
  return 0;
}

void g(int a, int b) {
  int i6 = sizeof(a + b);
  int i7 = sizeof(f(a, b));
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
// CC-NEXT:     li a0, 0
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $f, -($. $f)
// CC-NEXT:
// CC-NEXT:     .global $g
// CC-NEXT:     .type $g, "function"
// CC-NEXT:     .align 2
// CC-NEXT: g:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     addi sp, sp, -8
// CC-NEXT:     li a0, 4
// CC-NEXT:     sw fp, a0, -4
// CC-NEXT:     li a0, 4
// CC-NEXT:     sw fp, a0, -8
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $g, -($. $g)
// CC-NEXT:
// CC-NEXT:     .rodata
// CC-NEXT:
// CC-NEXT:     .data
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i1:
// CC-NEXT:     .dw 4
// CC-NEXT:     .global $i1
// CC-NEXT:     .type $i1, "object"
// CC-NEXT:     .size $i1, -($. $i1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i2:
// CC-NEXT:     .dw 4
// CC-NEXT:     .global $i2
// CC-NEXT:     .type $i2, "object"
// CC-NEXT:     .size $i2, -($. $i2)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i3:
// CC-NEXT:     .dw 4
// CC-NEXT:     .global $i3
// CC-NEXT:     .type $i3, "object"
// CC-NEXT:     .size $i3, -($. $i3)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i4:
// CC-NEXT:     .dw 4
// CC-NEXT:     .global $i4
// CC-NEXT:     .type $i4, "object"
// CC-NEXT:     .size $i4, -($. $i4)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr1:
// CC-NEXT:     .dw 1
// CC-NEXT:     .dw 2
// CC-NEXT:     .dw 3
// CC-NEXT:     .global $arr1
// CC-NEXT:     .type $arr1, "object"
// CC-NEXT:     .size $arr1, -($. $arr1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i5:
// CC-NEXT:     .dw 12
// CC-NEXT:     .global $i5
// CC-NEXT:     .type $i5, "object"
// CC-NEXT:     .size $i5, -($. $i5)
// CC-NEXT:
// CC-NEXT:     .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f1:
// CC-NEXT:     .fill 4
// CC-NEXT:     .global $f1
// CC-NEXT:     .type $f1, "object"
// CC-NEXT:     .size $f1, -($. $f1)
