# RUN: rrisc32-as -o %t %s
# RUN: rrisc32-dump --sym %t | filecheck %s --check-prefix=SYM
# RUN: rrisc32-dump --hex .data %t | filecheck %s --check-prefix=DATA

  .text
  .type $f, "function"
  .global $.L_f

f:
  add x1, x2, x3
.L_f:
  .rep 100
  nop
  .equ $M, 4
  .equ $g, +($.L_f $M)
  nop
  nop

  .size $f, -($. $f)

  .rodata
  .type $s, "object"
  .local $s

s:
  .ascii "the quick brown fox", " jumps over "
.L_s:
  .asciz "the lazy dog"

  .size $s, -($. $s)

  .data
  .type $a, "object"
  .weak $a

a:
  .db 1, 2, 3, 4, 5
  .dw $M
  .size $a, -($. $a)

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      019c    LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      2c      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      09      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      00      019c    GLOBAL  FUNC    DEF     02      f
# SYM-NEXT: 06      06      04      00      GLOBAL  NOTYPE  DEF     02      .L_f
# SYM-NEXT: 06      07      04      00      GLOBAL  NOTYPE  DEF     ABS     M
# SYM-NEXT: 06      08      08      00      GLOBAL  NOTYPE  DEF     02      g
# SYM-NEXT: 06      09      00      2c      LOCAL   OBJECT  DEF     03      s
# SYM-NEXT: 06      0a      1f      00      LOCAL   NOTYPE  DEF     03      .L_s
# SYM-NEXT: 06      0b      00      09      WEAK    OBJECT  DEF     04      a

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  04030201 00000405 00000000
