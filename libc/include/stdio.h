#ifndef _STDIO_H
#define _STDIO_H

int fgetc(int fd);
int getc();
int fputc(int fd, char c);
int putc(char c);
int fputs(int fd, char *s);
int puts(char *s);
int fputl(int fd, char *s);
int putl(char *s);

#endif
