# RUN: assemble -o %t %s
# RUN: dump --dis .text %t | filecheck %s --check-prefix=TEXT
# RUN: dump --hex .rodata %t | filecheck %s --check-prefix=RODATA
# RUN: dump --hex .data %t | filecheck %s --check-prefix=DATA

  .text
  add x1, x2, x3

  .section ".text"
  addi x1, x2, 1

  .rodata
  .ascii "hello", " world"

  .data
  .db 1,0x02

  .bss
  .fill 2

  .section ".rodata"
  .asciz "quick", "brown", "fox"

# TEXT: [ Disassembly/.text ]
# TEXT-NEXT: 00000000  add x1, x2, x3
