// RUN: rrisc32-cc -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int i1;
int i2 = 100;

int arr1[] = {};
int arr2[] = {1, 2, 3};
int arr3[3] = {1};

char *s1 = "";
char *s2 = "hello";
char s3[] = "hello";
char s4[] = {'h', 'e', 'l', 'l', 'o'};

struct Foo {
  char c;
  int i;
  char s[2];
};

struct Foo f1 = {'a', 10, "b"};

typedef struct Foo Goo;
Goo g1 = {'a'};

Goo *pg1 = &g1;

// CC:          .text
// CC-NEXT:
// CC-NEXT:     .rodata
// CC-NEXT: .L_r1:
// CC-NEXT:     .asciz ""
// CC-NEXT: .L_r2:
// CC-NEXT:     .asciz "hello"
// CC-NEXT: .L_r3:
// CC-NEXT:     .asciz "b"
// CC-NEXT:
// CC-NEXT:     .data
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i2:
// CC-NEXT:     .dw 100
// CC-NEXT:     .global $i2
// CC-NEXT:     .type $i2, "object"
// CC-NEXT:     .size $i2, 4
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr1:
// CC-NEXT:     .global $arr1
// CC-NEXT:     .type $arr1, "object"
// CC-NEXT:     .size $arr1, 0
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr2:
// CC-NEXT:     .dw 1
// CC-NEXT:     .dw 2
// CC-NEXT:     .dw 3
// CC-NEXT:     .global $arr2
// CC-NEXT:     .type $arr2, "object"
// CC-NEXT:     .size $arr2, 12
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr3:
// CC-NEXT:     .dw 1
// CC-NEXT:     .fill 8
// CC-NEXT:     .global $arr3
// CC-NEXT:     .type $arr3, "object"
// CC-NEXT:     .size $arr3, 12
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: s1:
// CC-NEXT:     .dw $.L_r1
// CC-NEXT:     .global $s1
// CC-NEXT:     .type $s1, "object"
// CC-NEXT:     .size $s1, 4
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: s2:
// CC-NEXT:     .dw $.L_r2
// CC-NEXT:     .global $s2
// CC-NEXT:     .type $s2, "object"
// CC-NEXT:     .size $s2, 4
// CC-NEXT:
// CC-NEXT: s3:
// CC-NEXT:     .asciz "hello"
// CC-NEXT:     .global $s3
// CC-NEXT:     .type $s3, "object"
// CC-NEXT:     .size $s3, 6
// CC-NEXT:
// CC-NEXT: s4:
// CC-NEXT:     .db 104
// CC-NEXT:     .db 101
// CC-NEXT:     .db 108
// CC-NEXT:     .db 108
// CC-NEXT:     .db 111
// CC-NEXT:     .global $s4
// CC-NEXT:     .type $s4, "object"
// CC-NEXT:     .size $s4, 5
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f1:
// CC-NEXT:     .db 97
// CC-NEXT:     .fill 3
// CC-NEXT:     .dw 10
// CC-NEXT:     .asciz "b"
// CC-NEXT:     .fill 2
// CC-NEXT:     .global $f1
// CC-NEXT:     .type $f1, "object"
// CC-NEXT:     .size $f1, 12
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: g1:
// CC-NEXT:     .db 97
// CC-NEXT:     .fill 11
// CC-NEXT:     .global $g1
// CC-NEXT:     .type $g1, "object"
// CC-NEXT:     .size $g1, 12
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: pg1:
// CC-NEXT:     .dw $g1
// CC-NEXT:     .global $pg1
// CC-NEXT:     .type $pg1, "object"
// CC-NEXT:     .size $pg1, 4
// CC-NEXT:
// CC-NEXT:     .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i1:
// CC-NEXT:     .fill 4
// CC-NEXT:     .global $i1
// CC-NEXT:     .type $i1, "object"
// CC-NEXT:     .size $i1, 4
// CC-NEXT:
// CC-NEXT: c:
// CC-NEXT:     .fill 1
// CC-NEXT:     .global $c
// CC-NEXT:     .type $c, "object"
// CC-NEXT:     .size $c, 1
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i:
// CC-NEXT:     .fill 4
// CC-NEXT:     .global $i
// CC-NEXT:     .type $i, "object"
// CC-NEXT:     .size $i, 4
// CC-NEXT:
// CC-NEXT: s:
// CC-NEXT:     .fill 2
// CC-NEXT:     .global $s
// CC-NEXT:     .type $s, "object"
// CC-NEXT:     .size $s, 2
