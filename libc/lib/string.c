#include <string.h>

unsigned strlen(char *s) {
  unsigned n = 0;
  while (*(s++))
    ++n;
  return n;
}
