// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <stdarg.h>
#include <stdio.h>

void print(unsigned n, char *t, ...) {
  va_list ap;
  va_start(ap, n);
  char *s = *(char **)ap;
  putl(s);
//   ap += 4;
//   s = *(char **)ap
//   putl(s);
  va_end(ap);
}

int main() { print(3, "quick", "brown", "fox"); }




// CHECK-NEXT: quick
