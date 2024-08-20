// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <rrisc32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  void *p0 = sbrk(0);

  void *p1 = malloc(52);
  void *p2 = malloc(52);
  void *p3 = malloc(52);

  printf("%x, %x, %x\n", p1 - p0, p2 - p0, p3 - p0);
  printf("%x\n", sbrk(0) - p0);

  free(p2);
  printf("%x\n", sbrk(0) - p0);
  free(p3);
  printf("%x\n", sbrk(0) - p0);

  p1 = realloc(p1, 20);
  void *p4 = malloc(20);
  printf("%x, %x\n", p1 - p0, p4 - p0);

  memcpy(p1, "hello", 6);
  p1 = realloc(p1, 52);
  printf("%x\n", p1 - p0);
  printf("%s", p1);
}

// CHECK-NEXT: 0x0000000c, 0x0000004c, 0x0000008c
// CHECK-NEXT: 0x000000c0
// CHECK-NEXT: 0x000000c0
// CHECK-NEXT: 0x00000040
// CHECK-NEXT: 0x0000000c, 0x0000002c
// CHECK-NEXT: 0x0000004c
// CHECK-NEXT: hello
