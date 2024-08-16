#ifndef _STDARG_H
#define _STDARG_H

#include <rrisc32.h>

typedef void *va_list[1];

#define __SIZE(T) (P2ALIGN(sizeof(T), 2))

#define va_start(ap, parmN)                                                    \
  do {                                                                         \
    ap[0] = ((void *)&parmN) + __SIZE(parmN);                                  \
  } while (0)

#define va_arg(ap, T) (ap[0] += __SIZE(T), *(T *)(ap[0] - __SIZE(T)))

#define va_end(ap)                                                             \
  do {                                                                         \
    ap[0] = NULL;                                                              \
  } while (0)

#define va_copy(dest, src)                                                     \
  do {                                                                         \
    va_end(dest);                                                              \
    dest[0] = src[0];                                                          \
  } while (0)

#endif
