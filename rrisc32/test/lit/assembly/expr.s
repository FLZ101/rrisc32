# RUN: rrisc32-as -o %t %s
# RUN: rrisc32-dump --dis .text %t | filecheck %s --check-prefix=TEXT
# RUN: rrisc32-dump --hex .data %t | filecheck %s --check-prefix=DATA

  .text
  addi x1, x2, +(-1 *(3 /(8 2)))

  .data
  .dw +(1 %hi(0xffff8888)), %lo(0xffff8888)

# TEXT:      [ Disassembly/.text ]
# TEXT-NEXT: 00000000  addi x1, x2, 11

# DATA:      [ Hex/.data ]
# DATA-NEXT: 0000000000000000  fffffffa fffff888
