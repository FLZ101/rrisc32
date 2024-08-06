// RUN: rrisc32-cc --compile -o %t.s %s
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
  char *p;
};

struct Foo f1 = {'a', 10, "b", "c"};

typedef struct Foo Goo;
Goo g1 = {'a'};

Goo *pg1 = &g1;

// CC:          .rodata
// CC-NEXT: .LS_1:
// CC-NEXT:     .asciz ""
// CC-NEXT: .LS_2:
// CC-NEXT:     .asciz "hello"
// CC-NEXT: .LS_3:
// CC-NEXT:     .asciz "c"

// CC:          .data
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i2:
// CC-NEXT:     .dw 100
// CC-NEXT:     .global $i2
// CC-NEXT:     .type $i2, "object"
// CC-NEXT:     .size $i2, -($. $i2)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr1:
// CC-NEXT:     .global $arr1
// CC-NEXT:     .type $arr1, "object"
// CC-NEXT:     .size $arr1, -($. $arr1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr2:
// CC-NEXT:     .dw 1
// CC-NEXT:     .dw 2
// CC-NEXT:     .dw 3
// CC-NEXT:     .global $arr2
// CC-NEXT:     .type $arr2, "object"
// CC-NEXT:     .size $arr2, -($. $arr2)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: arr3:
// CC-NEXT:     .dw 1
// CC-NEXT:     .fill 8
// CC-NEXT:     .global $arr3
// CC-NEXT:     .type $arr3, "object"
// CC-NEXT:     .size $arr3, -($. $arr3)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: s1:
// CC-NEXT:     .dw $.LS_1
// CC-NEXT:     .global $s1
// CC-NEXT:     .type $s1, "object"
// CC-NEXT:     .size $s1, -($. $s1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: s2:
// CC-NEXT:     .dw $.LS_2
// CC-NEXT:     .global $s2
// CC-NEXT:     .type $s2, "object"
// CC-NEXT:     .size $s2, -($. $s2)
// CC-NEXT:
// CC-NEXT: s3:
// CC-NEXT:     .asciz "hello"
// CC-NEXT:     .global $s3
// CC-NEXT:     .type $s3, "object"
// CC-NEXT:     .size $s3, -($. $s3)
// CC-NEXT:
// CC-NEXT: s4:
// CC-NEXT:     .db 104
// CC-NEXT:     .db 101
// CC-NEXT:     .db 108
// CC-NEXT:     .db 108
// CC-NEXT:     .db 111
// CC-NEXT:     .global $s4
// CC-NEXT:     .type $s4, "object"
// CC-NEXT:     .size $s4, -($. $s4)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: f1:
// CC-NEXT:     .db 97
// CC-NEXT:     .fill 3
// CC-NEXT:     .dw 10
// CC-NEXT:     .asciz "b"
// CC-NEXT:     .fill 2
// CC-NEXT:     .dw $.LS_3
// CC-NEXT:     .global $f1
// CC-NEXT:     .type $f1, "object"
// CC-NEXT:     .size $f1, -($. $f1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: g1:
// CC-NEXT:     .db 97
// CC-NEXT:     .fill 15
// CC-NEXT:     .global $g1
// CC-NEXT:     .type $g1, "object"
// CC-NEXT:     .size $g1, -($. $g1)
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: pg1:
// CC-NEXT:     .dw $g1
// CC-NEXT:     .global $pg1
// CC-NEXT:     .type $pg1, "object"
// CC-NEXT:     .size $pg1, -($. $pg1)

// CC:          .bss
// CC-NEXT:
// CC-NEXT:     .align 2
// CC-NEXT: i1:
// CC-NEXT:     .fill 4
// CC-NEXT:     .global $i1
// CC-NEXT:     .type $i1, "object"
// CC-NEXT:     .size $i1, -($. $i1)
