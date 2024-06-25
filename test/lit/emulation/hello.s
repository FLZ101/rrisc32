# RUN: assemble -o %t.o %s
# RUN: link -o %t.exe %t.o
# RUN: emulate %t.exe | filecheck %s

  .text

  # write msg
  li a0, 4
  li a1, 1
  li a2 $msg
  li a3, $msgSize
  ecall

  # exit 0
  li a0, 0
  li a1, 0
  ecall

  .rodata
msg:
  .asciz "hello world\n"
  .equ $msgSize, -($. $msg)

# CHECK: hello world
