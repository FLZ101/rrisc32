# RUN: assemble -o %t.o %s
# RUN: link -o %t.exe %t.o
# RUN: emulate %t.exe | filecheck %s

  .text

  # write msg
  li a0, 3
  li a1 $msg
  li a2, $msgSize
  li a3, $msgSize
  li a4, 1
  ecall

  # exit 0
  li a0, 4
  li a1, 0
  ecall

  .rodata
msg:
  .asciz "hello world\n"
  .equ $msgSize, -($. $msg)

# CHECK: hello world
