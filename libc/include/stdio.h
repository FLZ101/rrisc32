#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

#define EOF (-1)

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fgetc(int fd);
int getc();
int fputc(int fd, char c);
int putc(char c);
int fputs(int fd, char *s);
int puts(char *s);
int fputl(int fd, char *s);
int putl(char *s);

int vsprintf(char *buf, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);
int vfprintf(int fd, const char *fmt, va_list ap);
int fprintf(int fd, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif
