#ifndef _SETJMP_H
#define _SETJMP_H

typedef struct {
  unsigned long fp;
  unsigned long ra;
  unsigned long sp;
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int status);

#endif
