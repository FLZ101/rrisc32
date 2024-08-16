#ifndef _RRISC32_H
#define _RRISC32_H

#include "syscall.inc"

#define NULL ((void *)0)

#define ALIGN(x, n) (((x) + ((n) - 1)) & ~((n) - 1))
#define P2ALIGN(x, n) ALIGN((x), (1 << (n)))

#endif
