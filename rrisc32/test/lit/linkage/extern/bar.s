# RUN: rrisc32-as -o %t.o %s
# RUN: rrisc32-dump --sym %t.o | filecheck %s --check-prefix=SYM
# RUN: rrisc32-dump --dis .text %t.o | filecheck %s --check-prefix=TEXT
# RUN: rrisc32-dump --hex .data %t.o | filecheck %s --check-prefix=DATA
# RUN: rrisc32-dump --rel %t.o | filecheck %s --check-prefix=REL

  .text
bar_f:
  call $foo_g
  call +(4 $foo_g)
  lw x1, $foo_x
  sw x1, x2, +($foo_x 4)
  ret

  .data
bar_x:
  .dw 0x40302010, +($foo_g 4)
  .dw 0x40302010, $foo_x

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      28      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      10      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      00      00      GLOBAL  NOTYPE  DEF     02      bar_f
# SYM-NEXT: 06      06      00      00      GLOBAL  NOTYPE  DEF     UND     foo_x
# SYM-NEXT: 06      07      00      00      GLOBAL  NOTYPE  DEF     04      bar_x
# SYM-NEXT: 06      08      00      00      GLOBAL  NOTYPE  DEF     UND     foo_g

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  lui x1, 0
# TEXT-NEXT: 00000004  jalr x1, x1, 0
# TEXT-NEXT: 00000008  lui x1, 0
# TEXT-NEXT: 0000000c  jalr x1, x1, 0
# TEXT-NEXT: 00000010  lui x1, 0
# TEXT-NEXT: 00000014  lw x1, x1, 0
# TEXT-NEXT: 00000018  lui x31, 0
# TEXT-NEXT: 0000001c  add x31, x31, x1
# TEXT-NEXT: 00000020  sw x31, x2, 0
# TEXT-NEXT: 00000024  jalr x0, x1, 0

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  40302010 00000000 40302010 00000000

# REL:      SecBeTo Idx     Offset  Type    Addend  Sym     SecSym  SecRel
# REL-NEXT: 08      00      00      HI20    0       08      06      02
# REL-NEXT: 08      01      04      LO12_I  0       08      06      02
# REL-NEXT: 08      02      08      HI20    4       08      06      02
# REL-NEXT: 08      03      0c      LO12_I  4       08      06      02
# REL-NEXT: 08      04      10      HI20    0       06      06      02
# REL-NEXT: 08      05      14      LO12_I  0       06      06      02
# REL-NEXT: 08      06      18      HI20    4       06      06      02
# REL-NEXT: 08      07      20      LO12_S  4       06      06      02
# REL-NEXT: 09      00      04      32      4       08      06      04
# REL-NEXT: 09      01      0c      32      0       06      06      04
