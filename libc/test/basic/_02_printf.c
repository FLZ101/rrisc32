// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <stdio.h>

int main() {
    printf("hello\n");
    printf("hello %s\n", "wuwenchen");
}

// CHECK-NEXT: hello
// CHECK-NEXT: hello wuwenchen
