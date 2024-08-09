
void f() {
    int a;
    int *p = &a;
    /*
        UnaryOp: &
          ID: a
    */

   ++a;
   a--;
    /*
        UnaryOp: ++
            ID: a
        UnaryOp: p--
            ID: a
    */
}
