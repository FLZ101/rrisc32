// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <stdarg.h>
#include <stdio.h>

void print(unsigned n, ...) {
  va_list ap;
  va_start(ap, n);
  for (unsigned i = 0; i < n; ++i) {
    char *s = va_arg(ap, char *);
    putl(s);
  }
}

int main() { print(3, "quick", "brown", "fox"); }

// CHECK:      quick
// CHECK-NEXT: brown
// CHECK-NEXT: fox
