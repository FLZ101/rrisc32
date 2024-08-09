
void f() {}

void g() {
    f();
/*
FuncCall:
        ID: f
*/
    void (*p)() = f;

    (*p)();
/*
FuncCall:
        UnaryOp: *
          ID: p
*/

    p();
/*
FuncCall:
        ID: p
*/
}
