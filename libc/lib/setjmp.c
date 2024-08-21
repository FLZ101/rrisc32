#include <setjmp.h>
#include <stdio.h>

// longjmp is intended for handling unexpected error conditions where the
// function cannot return meaningfully. This is similar to exception handling in
// other programming languages.
int setjmp(jmp_buf env) {
  // clang-format off
  #pragma ASM \
    lw a0, fp, 8; \
    lw t0, sp, 0; \
    sw a0, t0, 0; \
    lw t0, sp, 4; \
    sw a0, t0, 4; \
    sw a0, sp, 8
  // clang-format on
  return 0;
}

// status - the value to return from setjmp. If it is equal to ​0​, 1 is
// used instead
void longjmp(jmp_buf env, int status) {
  // clang-format off
  #pragma ASM \
    lw a0, fp, 8;  \
    lw a1, fp, 12; \
    lw sp, a0, 8;  \
    lw t0, a0, 0;  \
    sw sp, t0, 0;  \
    lw t0, a0, 4;  \
    sw sp, t0, 4;  \
    mv fp, sp;     \
    seqz a0, a1;   \
    add a0, a0, a1
  // clang-format on
  return;
}
