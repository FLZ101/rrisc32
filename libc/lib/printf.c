#include <stdio.h>

static void sputc(char **buf, char c) {
  *((*buf)++) = c;
  **buf = '\0';
}

static void sputs(char **buf, char *str) {
  char c;
  while ((c = *(str++)))
    sputc(buf, c);
}

static void sputx(char **buf, unsigned int n) {
  int i, j;
  char s[11] = {'0', 'x'};

  for (i = 9; i > 1; i--) {
    if (n) {
      j = n;
      n = n >> 4;
      j = j - (n << 4);
      s[i] = j + (j < 10 ? '0' : -10 + 'a');
    } else {
      s[i] = '0';
    }
  }
  sputs(buf, s);
}

static void sputu(char **buf, unsigned int n) {
  int i, j;
  char s[11] = {0};

  if (n == 0) {
    sputs(buf, "0");
    return;
  }

  for (i = 9; i >= 0; i--) {
    if (n) {
      j = n;
      n = n / 10;
      s[i] = j - n * 10 + '0';
    } else {
      break;
    }
  }
  sputs(buf, s + i + 1);
}

static void sputd(char **buf, int n) {
  if (0 <= n) {
    sputu(buf, n);
  } else {
    sputs(buf, "-");
    sputu(buf, -n);
  }
}

int vsprintf(char *__buf, const char *fmt, va_list ap) {
  char c, *__buf0 = __buf, **buf = &__buf;
  while ((c = *(fmt++))) {
    if (c == '%') {
      c = *(fmt++);
      switch (c) {
      case 'u':
        sputu(buf, va_arg(ap, unsigned int));
        break;
      case 'd':
        sputd(buf, va_arg(ap, int));
        break;
      case 'x':
        sputx(buf, va_arg(ap, unsigned int));
        break;
      case 'c':
        sputc(buf, va_arg(ap, char));
        break;
      case 's':
        sputc(buf, 'S');
        sputs(buf, va_arg(ap, char *));
        break;
      case '%':
        sputc(buf, c);
        break;
      }
      continue;
    }
    sputc(buf, c);
  }
  return __buf - __buf0;
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  int res = vsprintf(buf, fmt, ap);
  va_end(ap);

  return res;
}

int vfprintf(int fd, const char *fmt, va_list ap) {
  static char buf[4096] = {0};
  vsprintf(buf, fmt, ap);
  return fputs(fd, buf);
}

int fprintf(int fd, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  int res = vfprintf(fd, fmt, ap);
  va_end(ap);

  return res;
}
