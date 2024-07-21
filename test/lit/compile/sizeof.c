// RUN: rrisc32-cc -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo { int i; };

int i1 = sizeof(int);
/*
    UnaryOp: sizeof
      Typename: None, [], None
        TypeDecl: None, [], None
          IdentifierType: ['int']
*/
int i2 = sizeof(i1);
/*
    UnaryOp: sizeof
      ID: i1
*/
struct Foo f1;
int i3 = sizeof(struct Foo);
/*
    UnaryOp: sizeof
      Typename: None, [], None
        TypeDecl: None, [], None
          Struct: Foo
*/
int i4 = sizeof(f1);
/*
    UnaryOp: sizeof
      ID: f1
*/
int arr1[] = { 1, 2, 3};
int i5 = sizeof(arr1);
/*
    UnaryOp: sizeof
      ID: arr1
*/
typedef struct Foo Goo;
int i6 = sizeof(Goo);
/*
    UnaryOp: sizeof
      Typename: None, [], None
        TypeDecl: None, [], None
          IdentifierType: ['Goo']
*/

// CC:         .text
// CC-NEXT:
// CC-NEXT:    .rodata
// CC-NEXT:
// CC-NEXT:    .data
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i1:
// CC-NEXT:    .dw 4
// CC-NEXT:    .global $i1
// CC-NEXT:    .type $i1, "object"
// CC-NEXT:    .size $i1, -($. $i1)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i2:
// CC-NEXT:    .dw 4
// CC-NEXT:    .global $i2
// CC-NEXT:    .type $i2, "object"
// CC-NEXT:    .size $i2, -($. $i2)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i3:
// CC-NEXT:    .dw 4
// CC-NEXT:    .global $i3
// CC-NEXT:    .type $i3, "object"
// CC-NEXT:    .size $i3, -($. $i3)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i4:
// CC-NEXT:    .dw 4
// CC-NEXT:    .global $i4
// CC-NEXT:    .type $i4, "object"
// CC-NEXT:    .size $i4, -($. $i4)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:arr1:
// CC-NEXT:    .dw 1
// CC-NEXT:    .dw 2
// CC-NEXT:    .dw 3
// CC-NEXT:    .global $arr1
// CC-NEXT:    .type $arr1, "object"
// CC-NEXT:    .size $arr1, -($. $arr1)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i5:
// CC-NEXT:    .dw 12
// CC-NEXT:    .global $i5
// CC-NEXT:    .type $i5, "object"
// CC-NEXT:    .size $i5, -($. $i5)
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:i6:
// CC-NEXT:    .dw 4
// CC-NEXT:    .global $i6
// CC-NEXT:    .type $i6, "object"
// CC-NEXT:    .size $i6, -($. $i6)
// CC-NEXT:
// CC-NEXT:    .bss
// CC-NEXT:
// CC-NEXT:    .align 2
// CC-NEXT:f1:
// CC-NEXT:    .fill 4
// CC-NEXT:    .global $f1
// CC-NEXT:    .type $f1, "object"
// CC-NEXT:    .size $f1, -($. $f1)
