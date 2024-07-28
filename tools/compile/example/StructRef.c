struct Foo
{
    int i;
};

void f()
{
    struct Foo foo;
    foo.i = 10;
    foo.i++;
    ++foo.i;

    struct Foo *p = &foo;
    p->i = 10;
    ++(p->i);
    p->i++;
}
/*
      Assignment: =
        StructRef: .
          ID: foo
          ID: i
        Constant: int, 10
      UnaryOp: p++
        StructRef: .
          ID: foo
          ID: i
      UnaryOp: ++
        StructRef: .
          ID: foo
          ID: i
*/
