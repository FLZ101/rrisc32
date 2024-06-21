# RUN: assemble -o %t %s
# RUN: dump --sym %t | filecheck %s --check-prefix=SYM
# RUN: dump --rel %t | filecheck %s --check-prefix=REL

  .text
  .rep 10
f:
  nop
1:
  add x1, x2, x3
  ret
g:
  call +($f 4)
  ret
  call +($1b -4)

  call $foo
  call +($foo 8)

  .data
  .dw $g, +($f -8), +($foo 8)

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      54      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      0c      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      00      00      GLOBAL  NOTYPE  DEF     02      f
# SYM-NEXT: 06      06      30      00      GLOBAL  NOTYPE  DEF     02      g
# SYM-NEXT: 06      07      00      00      GLOBAL  NOTYPE  DEF     UND     foo

# REL:      SecBeTo Idx     Offset  Type    Addend  Sym     SecSym  SecRel
# REL-NEXT: 08      00      30      HI20    4       05      06      02
# REL-NEXT: 08      01      34      LO12_I  4       05      06      02
# REL-NEXT: 08      02      3c      HI20    36      01      06      02
# REL-NEXT: 08      03      40      LO12_I  36      01      06      02
# REL-NEXT: 08      04      44      HI20    0       07      06      02
# REL-NEXT: 08      05      48      LO12_I  0       07      06      02
# REL-NEXT: 08      06      4c      HI20    8       07      06      02
# REL-NEXT: 08      07      50      LO12_I  8       07      06      02
# REL-NEXT: 09      00      00      32      0       06      06      04
# REL-NEXT: 09      01      04      32      -8      05      06      04
# REL-NEXT: 09      02      08      32      8       07      06      04
