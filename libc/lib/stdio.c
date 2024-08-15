#include <rrisc32.h>
#include <stdio.h>
#include <string.h>

int fgetc(int fd) {
  char c;
  return read(fd, &c, 1) == 1 ? c : EOF;
}

int getc() { return fgetc(STDIN); }

int fputc(int fd, char c) { return write(fd, &c, 1) == 1 ? c : -1; }

int putc(char c) { return fputc(STDOUT, c); }

int fputs(int fd, char *s) {
  unsigned n = strlen(s);
  return n == write(fd, s, n) ? n : -1;
}

int puts(char *s) { return fputs(STDOUT, s); }

int fputl(int fd, char *s) {
  int n1 = fputs(fd, s);
  int n2 = fputc(fd, '\n');
  if (n1 == -1 || n2 == -1)
    return -1;
  return n1 + n2;
}

int putl(char *s) { return fputl(STDOUT, s); }
