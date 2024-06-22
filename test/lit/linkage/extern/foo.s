# RUN: assemble -o %t.o %s
# RUN: dump --sym %t.o | filecheck %s --check-prefix=SYM

  .text
  .rep 2048
  nop

  nop
foo_g:
  add x1, x2, x3
  ret

  .data
  .fill 2048, 4, -1
foo_x:
  .dw 0x01020304

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      200c    LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      2004    LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      2004    00      GLOBAL  NOTYPE  DEF     02      foo_g
# SYM-NEXT: 06      06      2000    00      GLOBAL  NOTYPE  DEF     04      foo_x
