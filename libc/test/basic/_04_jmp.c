// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <setjmp.h>
#include <stdio.h>

jmp_buf env;

void f() {
  putl("f(");
  longjmp(env, 10);
  putl(")");
}

void g() {
  putl("g(");
  f();
  putl(")");
}

void h() {
  putl("h(");
  g();
  putl(")");
}

int main(void) {
  if (setjmp(env)) {
    putl("error");
    return -1;
  }
  h();
  putl("ok");
}

// CHECK-NEXT: h(
// CHECK-NEXT: g(
// CHECK-NEXT: f(
// CHECK-NEXT: error
