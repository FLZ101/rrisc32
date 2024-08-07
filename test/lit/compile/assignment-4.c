// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

struct Foo {
  char c;
  int i;
} f1;

void f()
{
    struct Foo f2;

    f1.i = 10;

    f1.i += 20;
}
