#include <stdio.h>
#include <rrisc32.h>
#include <string.h>

int fgetc(int fd) {
  char c;
  return read(fd, &c, 1) == 1 ? c : EOF;
}

int getc() { return fgetc(STDIN); }

int fputc(int fd, char c) { return write(fd, &c, 1) == 1 ? c : EOF; }

int putc(char c) { return fputc(STDOUT, c); }

int fputs(int fd, char *s) {
  unsigned n = strlen(s);
  return n == write(fd, s, n) ? n : -1;
}

int puts(char *s) { return fputs(STDOUT, s); }
