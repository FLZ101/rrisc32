#ifndef _STDARG_H
#define _STDARG_H

#include <rrisc32.h>

typedef void *va_list;

#define va_start(ap, parmN)                                                    \
  do {                                                                         \
    (ap) = ((void *)&parmN) + P2ALIGN(sizeof(parmN), 2);                       \
  } while (0)

#define va_arg(ap, T)                                                          \
  ((ap) += P2ALIGN(sizeof(T), 2), *(T *)((ap) - P2ALIGN(sizeof(T), 2)))

#define va_end(ap)                                                             \
  do {                                                                         \
    (ap) = NULL;                                                               \
  } while (0)

#define va_copy(dest, src)                                                     \
  do {                                                                         \
    va_end(dest);                                                              \
    (dest) = (src);                                                            \
  } while (0)

#endif
