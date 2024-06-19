# RUN: assemble -o %t %s
# RUN: dump --sym %t | tee tmp.txt | filecheck %s --check-prefix=SYM
# RUN: dump --dis .text %t | filecheck %s --check-prefix=TEXT

  .text
  nop
long_long_ago:
  .align 10

  .rep 3
  nop

  li x1, $far_away
  li x1, 0xff008800

  lb x1, $far_away
  lb x1, 0xff008800

  sb x1, x2, $far_away
  sb x1, x2, 0xff008800

  mv x1, x2
  not x1, x2
  neg x1, x2

  sext.b x1, x2
  sext.h x1, x2
  zext.b x1, x2
  zext.h x1, x2

  seqz x1, x2
  snez x1, x2
  sltz x1, x2
  sgtz x1, x2

  beqz x1, $long_long_ago
  blez x1, $long_long_ago
  bgt x1, x2, $long_long_ago

  j $long_long_ago
  jal $long_long_ago
  jr x1
  jalr x1
  ret
  call $long_long_ago
  tail $long_long_ago

  .rodata
  .asciz ":-)"
  .align 10
far_away:
  .dw $long_long_ago

SYM:      [ Symbols ]
SYM-NEXT: SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
SYM-NEXT: 06      01      00      04a4    LOCAL   SECTION DEF     02      .text
SYM-NEXT: 06      02      00      0404    LOCAL   SECTION DEF     03      .rodata
SYM-NEXT: 06      03      00      00      LOCAL   SECTION DEF     04      .data
SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
SYM-NEXT: 06      05      04      00      GLOBAL  NOTYPE  DEF     02      long_long_ago
SYM-NEXT: 06      06      0400    00      GLOBAL  NOTYPE  DEF     03      far_away

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  addi x0, x0, 0
# TEXT:      00000400  addi x0, x0, 0
# TEXT-NEXT: 00000404  addi x0, x0, 0
# TEXT-NEXT: 00000408  addi x0, x0, 0
# TEXT-NEXT: 0000040c  lui x1, 0
# TEXT-NEXT: 00000410  addi x1, x1, 0
# TEXT-NEXT: 00000414  lui x1, -4088
# TEXT-NEXT: 00000418  addi x1, x1, -2048
# TEXT-NEXT: 0000041c  lui x1, 0
# TEXT-NEXT: 00000420  lb x1, x1, 0
# TEXT-NEXT: 00000424  lui x1, -4088
# TEXT-NEXT: 00000428  lb x1, x1, -2048
# TEXT-NEXT: 0000042c  lui x2, 0
# TEXT-NEXT: 00000430  sb x1, x2, 0
# TEXT-NEXT: 00000434  lui x2, -4088
# TEXT-NEXT: 00000438  sb x1, x2, -2048
# TEXT-NEXT: 0000043c  addi x1, x2, 0
# TEXT-NEXT: 00000440  xori x1, x2, -1
# TEXT-NEXT: 00000444  sub x1, x0, x2
# TEXT-NEXT: 00000448  slli x1, x2, 24
# TEXT-NEXT: 0000044c  srai x1, x2, 24
# TEXT-NEXT: 00000450  slli x1, x2, 16
# TEXT-NEXT: 00000454  srai x1, x2, 16
# TEXT-NEXT: 00000458  andi x1, x2, 255
# TEXT-NEXT: 0000045c  slli x1, x2, 16
# TEXT-NEXT: 00000460  srli x1, x2, 16
# TEXT-NEXT: 00000464  sltiu x1, x2, 1
# TEXT-NEXT: 00000468  sltu x1, x0, x2
# TEXT-NEXT: 0000046c  slt x1, x2, x0
# TEXT-NEXT: 00000470  slt x1, x0, x2
# TEXT-NEXT: 00000474  beq x1, x0, -1136
# TEXT-NEXT: 00000478  bge x0, x1, -1140
# TEXT-NEXT: 0000047c  blt x2, x1, -1144
# TEXT-NEXT: 00000480  jal x0, -1148
# TEXT-NEXT: 00000484  jal x1, -1152
# TEXT-NEXT: 00000488  jalr x0, x1, 0
# TEXT-NEXT: 0000048c  jalr x1, x1, 0
# TEXT-NEXT: 00000490  jalr x0, x1, 0
# TEXT-NEXT: 00000494  lui x1, 0
# TEXT-NEXT: 00000498  jalr x1, x1, 0
# TEXT-NEXT: 0000049c  lui x6, 0
# TEXT-NEXT: 000004a0  jalr x0, x6, 0
