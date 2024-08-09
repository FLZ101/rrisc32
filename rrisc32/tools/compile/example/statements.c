void g();

void h();

void f()
{
    int a = 1;

    a;
/*
ID: a
*/
    if (a)
        ;
/*
      If:
        ID: a
        EmptyStatement:
*/
    if (a) {
        g();
    }
/*
      If:
        ID: a
        Compound:
          FuncCall:
            ID: g
*/
    if (a) {
        g();
    } else {
        h();
    }

/*
      If:
        ID: a
        Compound:
          FuncCall:
            ID: g
        Compound:
          FuncCall:
            ID: h
*/

    if (a)
        g();
/*
      If:
        ID: a
        FuncCall:
          ID: g
*/
    for (int i = 0; i < 10; ++i) {
        g();
    }
/*
      For:
        DeclList:
          Decl: i, [], [], [], []
            TypeDecl: i, [], None
              IdentifierType: ['int']
            Constant: int, 0
        BinaryOp: <
          ID: i
          Constant: int, 10
        UnaryOp: ++
          ID: i
        Compound:
          FuncCall:
            ID: g
*/
    switch (a) {
    case 1:
        ;
    case 2:
        g();
    default:
        ;
    }
/*
      Switch:
        ID: a
        Compound:
          Case:
            Constant: int, 1
            EmptyStatement:
          Case:
            Constant: int, 2
            FuncCall:
              ID: g
          Default:
            EmptyStatement:
*/

forever:
    ;
    goto forever;
/*
      Label: forever
        EmptyStatement:
      Goto: forever
*/
}
