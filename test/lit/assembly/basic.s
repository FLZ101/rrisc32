# RUN: rrisc32-as -o %t %s
# RUN: rrisc32-dump --dis .text %t | filecheck %s --check-prefix=TEXT
# RUN: rrisc32-dump --hex .rodata %t | filecheck %s --check-prefix=RODATA
# RUN: rrisc32-dump --hex .data %t | filecheck %s --check-prefix=DATA
# RUN: rrisc32-dump --sym %t | filecheck %s --check-prefix=SYM

  .text
  add x1, x2, x3

  .rodata
  .ascii "a", "b"

  .data
  .db 1, 0x34
  .dh -1, -0xffff

  .bss
  .fill 2

  .section ".text"
  addi x1, x2, -1

  .section ".rodata"
  .asciz "c"

  .section ".data"
  .dw 1, 0x12
  .dq -1, 0x1234
  .fill 2, 2, 0xab

  .section ".bss"
  .fill 2, 4

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  add x1, x2, x3
# TEXT-NEXT: 00000004  addi x1, x2, -1

# RODATA:      [ Hex/.rodata ]
# RODATA-NEXT: 0000000000000000  00636261

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  ffff3401 00010001 00120000 ffff0000
# DATA-NEXT: 0000000000000010  ffffffff 1234ffff 00000000 00ab0000
# DATA-NEXT: 0000000000000020  000000ab

# SYM:      [ Symbols ]
# SYM-NEXT: SecBeTo Idx     Value   Size    Bind    Type    Vis     Sec     Name
# SYM-NEXT: 06      00      00      00      LOCAL   NOTYPE  DEF     UND
# SYM-NEXT: 06      01      00      08      LOCAL   SECTION DEF     02      .text
# SYM-NEXT: 06      02      00      04      LOCAL   SECTION DEF     03      .rodata
# SYM-NEXT: 06      03      00      22      LOCAL   SECTION DEF     04      .data
# SYM-NEXT: 06      04      00      0a      LOCAL   SECTION DEF     05      .bss
