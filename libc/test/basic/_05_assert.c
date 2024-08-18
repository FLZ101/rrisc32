// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <assert.h>

void f(int i) { assert(i < 10); }

int main() {
  f(9);
  f(10);
}

// CHECK-NEXT: libc/test/basic/_05_assert.c:6: f: Assertion `i < 10' failed.
