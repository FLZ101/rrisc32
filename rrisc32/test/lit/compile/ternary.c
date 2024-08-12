// RUN: rrisc32-cc --compile -o %t.s %s
// RUN: cat %t.s | filecheck %s --check-prefix=CC

int f();
int g();

int h(int i) {
    return i ? f() : g();
}

// CC:          .global $h
// CC-NEXT:     .type $h, "function"
// CC-NEXT:     .align 2
// CC-NEXT: h:
// CC-NEXT:     push ra
// CC-NEXT:     push fp
// CC-NEXT:     mv fp, sp
// CC-NEXT:     lw a0, fp, 8
// CC-NEXT:     beqz a0, .LL_1.false
// CC-NEXT:     call $f
// CC-NEXT:     j $.LL_2.end
// CC-NEXT: .LL_1.false:
// CC-NEXT:     call $g
// CC-NEXT: .LL_2.end:
// CC-NEXT:     mv sp, fp
// CC-NEXT:     pop fp
// CC-NEXT:     pop ra
// CC-NEXT:     ret
// CC-NEXT:     .size $h, -($. $h)
