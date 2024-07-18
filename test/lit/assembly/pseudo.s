# RUN: rrisc32-as -o %t %s
# RUN: rrisc32-dump --dis .text %t | filecheck %s --check-prefix=TEXT
# RUN: rrisc32-dump --sec %t | filecheck %s --check-prefix=SEC
# RUN: rrisc32-dump --sym %t | filecheck %s --check-prefix=SYM
# RUN: rrisc32-dump --rel %t | filecheck %s --check-prefix=REL

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

  push x1
  pop
  pop x1

  .rodata
  .asciz ":-)"
  .align 10
far_away:
  .dw $long_long_ago

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
# TEXT-NEXT: 0000042c  lui x1, 0
# TEXT-NEXT: 00000430  sb x1, x2, 0
# TEXT-NEXT: 00000434  lui x1, -4088
# TEXT-NEXT: 00000438  sb x1, x2, -2048
# TEXT-NEXT: 0000043c  addi x1, x2, 0
# TEXT-NEXT: 00000440  xori x1, x2, -1
# TEXT-NEXT: 00000444  sub x1, x0, x2
# TEXT-NEXT: 00000448  slli x1, x2, 24
# TEXT-NEXT: 0000044c  srai x1, x1, 24
# TEXT-NEXT: 00000450  slli x1, x2, 16
# TEXT-NEXT: 00000454  srai x1, x1, 16
# TEXT-NEXT: 00000458  andi x1, x2, 255
# TEXT-NEXT: 0000045c  slli x1, x2, 16
# TEXT-NEXT: 00000460  srli x1, x1, 16
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
# TEXT-NEXT: 000004a4  addi x2, x2, -4
# TEXT-NEXT: 000004a8  sw x2, x1, 0
# TEXT-NEXT: 000004ac  addi x2, x2, 4
# TEXT-NEXT: 000004b0  lw x1, x2, 0
# TEXT-NEXT: 000004b4  addi x2, x2, 4

# SEC:      Idx     Addr    Off     Size    Link    Info    AddrAli EntSize Flags   Type    Name
# SEC-NEXT: 01      00      34      4c      00      00      01      00              STRTAB  .shstrtab
# SEC-NEXT: 02      00      0400    04b8    00      00      0400    00      AE      PROGBIT .text
# SEC-NEXT: 03      00      0c00    0404    00      00      0400    00      A       PROGBIT .rodata
# SEC-NEXT: 04      00      1004    00      00      00      04      00      AW      PROGBIT .data
# SEC-NEXT: 05      00      1004    00      00      00      04      00      AW      NOBITS  .bss
# SEC-NEXT: 06      00      1004    70      07      05      00      10              SYMTAB  .symtab
# SEC-NEXT: 07      00      1074    31      00      00      00      00              STRTAB  .strtab
# SEC-NEXT: 08      00      10a5    78      06      02      00      0c              RELA    .rela.text
# SEC-NEXT: 09      00      111d    0c      06      03      00      0c              RELA    .rela.rodata

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      04b8    LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      0404    LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      00      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      04      00      GLOBAL  NOTYPE  DEF     02      long_long_ago
# SYM-NEXT: 06      06      0400    00      GLOBAL  NOTYPE  DEF     03      far_away

# REL:      SecBeTo Idx     Offset  Type    Addend  Sym     SecSym  SecRel
# REL-NEXT: 08      00      040c    HI20    0       06      06      02
# REL-NEXT: 08      01      0410    LO12_I  0       06      06      02
# REL-NEXT: 08      02      041c    HI20    0       06      06      02
# REL-NEXT: 08      03      0420    LO12_I  0       06      06      02
# REL-NEXT: 08      04      042c    HI20    0       06      06      02
# REL-NEXT: 08      05      0430    LO12_S  0       06      06      02
# REL-NEXT: 08      06      0494    HI20    0       05      06      02
# REL-NEXT: 08      07      0498    LO12_I  0       05      06      02
# REL-NEXT: 08      08      049c    HI20    0       05      06      02
# REL-NEXT: 08      09      04a0    LO12_I  0       05      06      02
# REL-NEXT: 09      00      0400    32      0       05      06      03
