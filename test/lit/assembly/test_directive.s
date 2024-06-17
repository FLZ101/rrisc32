# RUN: assemble -o %t %s
# RUN: dump --dis .text %t | filecheck %s

.text
add x1, x2, x3

# CHECK: [ Disassembly/.text ]
# CHECK-NEXT: 00000000  add x1, x2, x3
