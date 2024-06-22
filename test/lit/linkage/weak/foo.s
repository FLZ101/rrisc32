# RUN: assemble -o %t.o %s
# RUN: dump --sym %t.o | filecheck %s --check-prefix=SYM

  .text
g:
  add x2, x3, x4
  ret

# SYM:      SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      08      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      00      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      00      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      00      LOCAL   SECTION DEF     05      .bss
# SYM-NEXT: 06      05      00      00      GLOBAL  NOTYPE  DEF     02      g
