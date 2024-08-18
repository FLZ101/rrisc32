#ifndef _STDLIB_H
#define _STDLIB_H

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void *malloc(unsigned size);
void *calloc(unsigned num, unsigned size);
void *realloc(void *ptr, unsigned new_size);
void free(void *ptr);

#endif
