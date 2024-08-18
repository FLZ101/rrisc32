#include <string.h>

unsigned strlen(char *s) {
  unsigned n = 0;
  while (*(s++))
    ++n;
  return n;
}

void *memset(void *s, int c, unsigned n) {
  unsigned char *p = (unsigned char *)s;
  for (unsigned i = 0; i < n; ++i)
    p[i] = c;
  return s;
}

void *memcpy(void *dest, void *src, unsigned n) {
  unsigned char *pd = (unsigned char *)dest;
  unsigned char *ps = (unsigned char *)src;
  for (unsigned i = 0; i < n; ++i)
    pd[i] = ps[i];
  return dest;
}
