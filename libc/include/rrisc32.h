#ifndef _RRISC32_H
#define _RRISC32_H

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#include "syscall.inc"

#define EOF (-1)
#define NULL ((void *)0)

#define ALIGN(x, n) (((x) + ((n) - 1)) & ~((n) - 1))
#define P2ALIGN(x, n) ALIGN((x), (1 << (n)))

#endif
