# RUN: rrisc32-as -o %t.o %s
# RUN: rrisc32-dump --sym %t.o | filecheck %s --check-prefix=SYM
# RUN: rrisc32-dump --dis .text %t.o | filecheck %s --check-prefix=TEXT
# RUN: rrisc32-dump --hex .data %t.o | filecheck %s --check-prefix=DATA

  .text
  call $g
  call +($g 4)
  ret

  .weak $g
g:
  add x1, x2, x3
  ret

  .data
  .dw $g, +($g 4)

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      1c      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      08      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      14      00      WEAK    NOTYPE  DEF     02      g

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  lui x1, 0
# TEXT-NEXT: 00000004  jalr x1, x1, 0
# TEXT-NEXT: 00000008  lui x1, 0
# TEXT-NEXT: 0000000c  jalr x1, x1, 0
# TEXT-NEXT: 00000010  jalr x0, x1, 0
# TEXT-NEXT: 00000014  add x1, x2, x3
# TEXT-NEXT: 00000018  jalr x0, x1, 0

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  00000000 00000000
