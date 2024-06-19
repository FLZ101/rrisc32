# RUN: assemble -o %t %s
# RUN: dump --dis .text %t | filecheck %s --check-prefix=TEXT
# RUN: dump --hex .data %t | filecheck %s --check-prefix=DATA
# RUN: dump --sym %t | filecheck %s --check-prefix=SYM

  .text
  add x1, x2, x3

  .align 3
f:
  addi x1, x2, -1

  .data
  .dh 0x1234
  .align 2
  .dh 0xabcd

  .bss
  .fill 3
  .align 3
  .fill 1

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  add x1, x2, x3
# TEXT-NEXT: 00000004  addi x0, x0, 0
# TEXT-NEXT: 00000008  addi x1, x2, -1

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  00001234 0000abcd

# SYM:      [ Symbols ]
# SYM-NEXT: SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      0c      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      06      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      09      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      08      00      GLOBAL  NOTYPE  DEF     02      f
