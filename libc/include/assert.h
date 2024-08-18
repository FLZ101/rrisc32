#ifndef _ASSERT_H
#define _ASSERT_H

#include <stdio.h>
#include <stdlib.h>

#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      printf("%s:%d: %s: Assertion `%s' failed.", __FILE__, __LINE__,          \
             __func__, #x);                                                    \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)
#endif

#endif
