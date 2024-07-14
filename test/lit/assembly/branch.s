# RUN: rrisc32-as -o %t %s
# RUN: rrisc32-dump --dis .text %t | filecheck %s --check-prefix=TEXT

  .text
  .rep 3
  add x1, x2, x3
1:
  .rep 4
  nop
  j $1b
  j $1f
  .rep 5
  nop
1:
  add x1, x2, x3

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  add x1, x2, x3
# TEXT-NEXT: 00000004  add x1, x2, x3
# TEXT-NEXT: 00000008  add x1, x2, x3
# TEXT-NEXT: 0000000c  addi x0, x0, 0
# TEXT-NEXT: 00000010  addi x0, x0, 0
# TEXT-NEXT: 00000014  addi x0, x0, 0
# TEXT-NEXT: 00000018  addi x0, x0, 0
# TEXT-NEXT: 0000001c  jal x0, -16
# TEXT-NEXT: 00000020  jal x0, 24
# TEXT-NEXT: 00000024  addi x0, x0, 0
# TEXT-NEXT: 00000028  addi x0, x0, 0
# TEXT-NEXT: 0000002c  addi x0, x0, 0
# TEXT-NEXT: 00000030  addi x0, x0, 0
# TEXT-NEXT: 00000034  addi x0, x0, 0
# TEXT-NEXT: 00000038  add x1, x2, x3
