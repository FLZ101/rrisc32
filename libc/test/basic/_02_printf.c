// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <stdio.h>

int main() {
    printf("hello\n");
    printf("hello %s\n", "wuwenchen");
    printf("%d, %d, %d\n", -10, 0, 10);
    printf("%u, %u, %u\n", -10, 0, 10);
    printf("%x, %x, %x\n", -10, 0, 10);
    printf("%c, %c, %%\n", '@', 88);
}

// CHECK:      hello
// CHECK-NEXT: hello wuwenchen
// CHECK-NEXT: -10, 0, 10
// CHECK-NEXT: 4294967286, 0, 10
// CHECK-NEXT: 0xfffffff6, 0x00000000, 0x0000000a
// CHECK-NEXT: @, X, %
