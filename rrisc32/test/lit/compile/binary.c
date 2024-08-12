// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC


void f() {
  short h1, h2;
  long long q1, q2;

  h1 + h2;
  q1 + q2;

  h1 - h2;
  q1 - q2;

  h1 * h2;
  q1 * q2;

  h1 / h2;
  // q1 / q2;

  h1 % h2;
  // q1 % q2;

  h1 & h2;
  q1 & q2;

  h1 | h2;
  q1 | q2;

  h1 ^ h2;
  q1 ^ q2;

  h1 << h2;
  // q1 << q2;

  h1 >> h2;
  // q1 >> q2;

  h1 && h2;
  q1 && q2;

  h1 || h2;
  q1 || q2;

  h1 == h2;
  q1 == q2;

  h1 != h2;
  q1 != q2;

  h1 < h2;
  q1 < q2;

  h1 > h2;
  q1 > q2;

  h1 <= h2;
  q1 <= q2;

  h1 >= h2;
  q1 >= q2;
}
