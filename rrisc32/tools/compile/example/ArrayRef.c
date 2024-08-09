void f() {
    int a1[] = {1, 2, 3};
    a1[1] = a1[2] + 1;
    ++a1[1];
    a1[1]++;

    int *p1 = &a1[1];

    int a2[3][3] = { {1, 2, 3}, {4, 5, 6} };
    a2[2][1] = a2[2][2] + 1;
    ++a2[2][1];
    a2[2][1]++;

    int *p2 = &a2[2][1];
    int (*p3)[3] = &a2[2];

    // a1[1]
    *(a1 + 1);
}
/*
    Compound:
      Decl: a1, [], [], [], []
        ArrayDecl: []
          TypeDecl: a1, [], None
            IdentifierType: ['int']
        InitList:
          Constant: int, 1
          Constant: int, 2
          Constant: int, 3
      Assignment: =
        ArrayRef:
          ID: a1
          Constant: int, 1
        BinaryOp: +
          ArrayRef:
            ID: a1
            Constant: int, 2
          Constant: int, 1
      UnaryOp: ++
        ArrayRef:
          ID: a1
          Constant: int, 1
      UnaryOp: p++
        ArrayRef:
          ID: a1
          Constant: int, 1
      Decl: p1, [], [], [], []
        PtrDecl: []
          TypeDecl: p1, [], None
            IdentifierType: ['int']
        UnaryOp: &
          ArrayRef:
            ID: a1
            Constant: int, 1
      Decl: a2, [], [], [], []
        ArrayDecl: []
          ArrayDecl: []
            TypeDecl: a2, [], None
              IdentifierType: ['int']
            Constant: int, 3
          Constant: int, 3
        InitList:
          InitList:
            Constant: int, 1
            Constant: int, 2
            Constant: int, 3
          InitList:
            Constant: int, 4
            Constant: int, 5
            Constant: int, 6
      Assignment: =
        ArrayRef:
          ArrayRef:
            ID: a2
            Constant: int, 2
          Constant: int, 1
        BinaryOp: +
          ArrayRef:
            ArrayRef:
              ID: a2
              Constant: int, 2
            Constant: int, 2
          Constant: int, 1
      UnaryOp: ++
        ArrayRef:
          ArrayRef:
            ID: a2
            Constant: int, 2
          Constant: int, 1
      UnaryOp: p++
        ArrayRef:
          ArrayRef:
            ID: a2
            Constant: int, 2
          Constant: int, 1
      Decl: p2, [], [], [], []
        PtrDecl: []
          TypeDecl: p2, [], None
            IdentifierType: ['int']
        UnaryOp: &
          ArrayRef:
            ArrayRef:
              ID: a2
              Constant: int, 2
            Constant: int, 1
      Decl: p3, [], [], [], []
        PtrDecl: []
          PtrDecl: []
            TypeDecl: p3, [], None
              IdentifierType: ['int']
        UnaryOp: &
          ArrayRef:
            ID: a2
            Constant: int, 2

      UnaryOp: *
        BinaryOp: +
          ID: a1
          Constant: int, 1
*/