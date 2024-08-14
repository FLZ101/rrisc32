// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe quick brown fox | filecheck %s

#include <stdio.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; ++i) {
    puts(argv[i]);
    putc('\n');
  }
  return 0;
}

// CHECK:      quick
// CHECK-NEXT: brown
// CHECK-NEXT: fox
