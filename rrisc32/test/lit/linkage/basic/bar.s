# RUN: rrisc32-as -o %t.o %s
# RUN: rrisc32-link -o %t.exe %t.o

# RUN: rrisc32-dump --sym %t.o | filecheck %s --check-prefix=SYM-O
# RUN: rrisc32-dump --rel %t.o | filecheck %s --check-prefix=REL-O
# RUN: rrisc32-dump --dis .text %t.o | filecheck %s --check-prefix=TEXT-O
# RUN: rrisc32-dump --hex .data %t.o | filecheck %s --check-prefix=DATA-O

# RUN: rrisc32-dump --sec %t.exe | filecheck %s --check-prefix=SEC-E
# RUN: rrisc32-dump --seg %t.exe | filecheck %s --check-prefix=SEG-E
# RUN: rrisc32-dump --dis .text %t.exe | filecheck %s --check-prefix=TEXT-E
# RUN: rrisc32-dump --hex .data %t.exe | filecheck %s --check-prefix=DATA-E

  .text

bar_f:
  call $bar_g
  call +($bar_g 4)
  call $1f
  call +($1f 8)
  ret

  .rep 2048
  nop

1:
bar_g:
  lw x1, $bar_x
  sw x1, x2, +($bar_x 4)
  ret

  .data
  .fill 2048, 4, -1
bar_x:
  .dw 0x40302010, $bar_g, +($bar_g 4)
  .dw 0x40302010, $bar_x, +($bar_x 4)

  .bss
  .fill 1
  .align 10

# SYM-O:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-O-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-O-NEXT: 06      01      00      203c    LOCAL   SECTION DEF     02      .text
# SYM-O-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-O-NEXT: 06      03      00      2018    LOCAL   SECTION DEF     04      .data
# SYM-O-NEXT: 06      04      00      0400    LOCAL   SECTION DEF     05      .bss
# SYM-O-NEXT: 06      05      00      00      GLOBAL  NOTYPE  DEF     02      bar_f
# SYM-O-NEXT: 06      06      2024    00      GLOBAL  NOTYPE  DEF     02      bar_g
# SYM-O-NEXT: 06      07      2000    00      GLOBAL  NOTYPE  DEF     04      bar_x

# REL-O:      SecBeTo Idx     Offset  Type    Addend  Sym     SecSym  SecRel
# REL-O-NEXT: 08      00      00      HI20    0       06      06      02
# REL-O-NEXT: 08      01      04      LO12_I  0       06      06      02
# REL-O-NEXT: 08      02      08      HI20    4       06      06      02
# REL-O-NEXT: 08      03      0c      LO12_I  4       06      06      02
# REL-O-NEXT: 08      04      10      HI20    8228    01      06      02
# REL-O-NEXT: 08      05      14      LO12_I  8228    01      06      02
# REL-O-NEXT: 08      06      18      HI20    8236    01      06      02
# REL-O-NEXT: 08      07      1c      LO12_I  8236    01      06      02
# REL-O-NEXT: 08      08      2024    HI20    0       07      06      02
# REL-O-NEXT: 08      09      2028    LO12_I  0       07      06      02
# REL-O-NEXT: 08      0a      202c    HI20    4       07      06      02
# REL-O-NEXT: 08      0b      2034    LO12_S  4       07      06      02
# REL-O-NEXT: 09      00      2004    32      0       06      06      04
# REL-O-NEXT: 09      01      2008    32      4       06      06      04
# REL-O-NEXT: 09      02      2010    32      0       07      06      04
# REL-O-NEXT: 09      03      2014    32      4       07      06      04

# TEXT-O:      [ Disassembly/.text ]
# TEXT-O-NEXT: 00000000  lui x1, 0
# TEXT-O-NEXT: 00000004  jalr x1, x1, 0
# TEXT-O-NEXT: 00000008  lui x1, 0
# TEXT-O-NEXT: 0000000c  jalr x1, x1, 0
# TEXT-O-NEXT: 00000010  lui x1, 0
# TEXT-O-NEXT: 00000014  jalr x1, x1, 0
# TEXT-O-NEXT: 00000018  lui x1, 0
# TEXT-O-NEXT: 0000001c  jalr x1, x1, 0
# TEXT-O-NEXT: 00000020  jalr x0, x1, 0
# TEXT-O-NEXT: 00000024  addi x0, x0, 0

# TEXT-O:      00002020  addi x0, x0, 0
# TEXT-O-NEXT: 00002024  lui x1, 0
# TEXT-O-NEXT: 00002028  lw x1, x1, 0
# TEXT-O-NEXT: 0000202c  lui x31, 0
# TEXT-O-NEXT: 00002030  add x31, x31, x1
# TEXT-O-NEXT: 00002034  sw x31, x2, 0
# TEXT-O-NEXT: 00002038  jalr x0, x1, 0

# DATA-O:      [ Hex/.data ]
# DATA-O-NEXT: 0000000000000000  ffffffff ffffffff ffffffff ffffffff
# DATA-O:      0000000000001ff0  ffffffff ffffffff ffffffff ffffffff
# DATA-O-NEXT: 0000000000002000  40302010 00000000 00000000 40302010
# DATA-O-NEXT: 0000000000002010  00000000 00000000

# SEC-E:      Idx     Addr    Off     Size    Link    Info    AddrAli EntSize Flags   Type    Name
# SEC-E:      02      1000    1000    203c    00      00      1000    00      AE      PROGBIT .text
# SEC-E-NEXT: 03      4000    4000    00      00      00      1000    00      A       PROGBIT .rodata
# SEC-E-NEXT: 04      4000    4000    2018    00      00      1000    00      AW      PROGBIT .data
# SEC-E-NEXT: 05      7000    7000    0400    00      00      1000    00      AW      NOBITS  .bss

# SEG-E:      Type    Offset  Vaddr   Paddr   Filesz  Memsz   Flags   Align
# SEG-E-NEXT: LOAD    1000    1000    1000    203c    203c    R E     1000
# SEG-E-NEXT: LOAD    4000    4000    4000    00      00      R       1000
# SEG-E-NEXT: LOAD    4000    4000    4000    2018    2018    RW      1000
# SEG-E-NEXT: LOAD    7000    7000    7000    00      0400    RW      1000

# TEXT-E:      [ Disassembly/.text ]
# TEXT-E-NEXT: 00001000  lui x1, 3
# TEXT-E-NEXT: 00001004  jalr x1, x1, 36
# TEXT-E-NEXT: 00001008  lui x1, 3
# TEXT-E-NEXT: 0000100c  jalr x1, x1, 40
# TEXT-E-NEXT: 00001010  lui x1, 3
# TEXT-E-NEXT: 00001014  jalr x1, x1, 36
# TEXT-E-NEXT: 00001018  lui x1, 3
# TEXT-E-NEXT: 0000101c  jalr x1, x1, 44
# TEXT-E-NEXT: 00001020  jalr x0, x1, 0
# TEXT-E-NEXT: 00001024  addi x0, x0, 0
# TEXT-E:      00003020  addi x0, x0, 0
# TEXT-E-NEXT: 00003024  lui x1, 6
# TEXT-E-NEXT: 00003028  lw x1, x1, 0
# TEXT-E-NEXT: 0000302c  lui x31, 6
# TEXT-E-NEXT: 00003030  add x31, x31, x1
# TEXT-E-NEXT: 00003034  sw x31, x2, 4
# TEXT-E-NEXT: 00003038  jalr x0, x1, 0

# DATA-E:      [ Hex/.data ]
# DATA-E-NEXT: 0000000000004000  ffffffff ffffffff ffffffff ffffffff
# DATA-E:      0000000000005ff0  ffffffff ffffffff ffffffff ffffffff
# DATA-E-NEXT: 0000000000006000  40302010 00003024 00003028 40302010
# DATA-E-NEXT: 0000000000006010  00006000 00006004
