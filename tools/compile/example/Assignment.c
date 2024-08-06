void f()
{
    int a, b;
    a += b;

    int *p = &a;
    *p += b;
    *p = *p + b;
/*
      Assignment: +=
        ID: a
        ID: b
      Decl: p, [], [], [], []
        PtrDecl: []
          TypeDecl: p, [], None
            IdentifierType: ['int']
        UnaryOp: &
          ID: a
      Assignment: +=
        UnaryOp: *
          ID: p
        ID: b
      Assignment: =
        UnaryOp: *
          ID: p
        BinaryOp: +
          UnaryOp: *
            ID: p
          ID: b
*/
}
