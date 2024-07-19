int a1[10];
/*
  Decl: a1, [], [], [], []
    ArrayDecl: []
      TypeDecl: a1, [], None
        IdentifierType: ['int']
      Constant: int, 10
*/

int a2[10] = {1, 2, 3};
/*
  Decl: a2, [], [], [], []
    ArrayDecl: []
      TypeDecl: a2, [], None
        IdentifierType: ['int']
      Constant: int, 10
    InitList:
      Constant: int, 1
      Constant: int, 2
      Constant: int, 3
*/

int a3[] = {1, 2, 3};
/*
  Decl: a3, [], [], [], []
    ArrayDecl: []
      TypeDecl: a3, [], None
        IdentifierType: ['int']
    InitList:
      Constant: int, 1
      Constant: int, 2
      Constant: int, 3
*/

int a4[2 * 5];
/*
  Decl: a4, [], [], [], []
    ArrayDecl: []
      TypeDecl: a4, [], None
        IdentifierType: ['int']
      BinaryOp: *
        Constant: int, 2
        Constant: int, 5
*/

int a5[2][3];
/*
  Decl: a5, [], [], [], []
    ArrayDecl: []
      ArrayDecl: []
        TypeDecl: a5, [], None
          IdentifierType: ['int']
        Constant: int, 3
      Constant: int, 2
*/

int a6[2][3] = {{ 1, 2, 3}, {4, 5}};
/*
  Decl: a6, [], [], [], []
    ArrayDecl: []
      ArrayDecl: []
        TypeDecl: a6, [], None
          IdentifierType: ['int']
        Constant: int, 3
      Constant: int, 2
    InitList:
      InitList:
        Constant: int, 1
        Constant: int, 2
        Constant: int, 3
      InitList:
        Constant: int, 4
        Constant: int, 5
*/

void f() {
  static int a7[] = {1};
  int a8[] = {1};

  int i1 = a1[0];
  int i2 = a5[1][2];

  int *p1 = a1;
  int i3 = p1[0];
}
/*
  FuncDef:
    Decl: f, [], [], [], []
      FuncDecl:
        TypeDecl: f, [], None
          IdentifierType: ['void']
    Compound:
      Decl: a7, [], [], ['static'], []
        ArrayDecl: []
          TypeDecl: a7, [], None
            IdentifierType: ['int']
        InitList:
          Constant: int, 1
      Decl: a8, [], [], [], []
        ArrayDecl: []
          TypeDecl: a8, [], None
            IdentifierType: ['int']
        InitList:
          Constant: int, 1
      Decl: i1, [], [], [], []
        TypeDecl: i1, [], None
          IdentifierType: ['int']
        ArrayRef:
          ID: a1
          Constant: int, 0
      Decl: i2, [], [], [], []
        TypeDecl: i2, [], None
          IdentifierType: ['int']
        ArrayRef:
          ArrayRef:
            ID: a5
            Constant: int, 1
          Constant: int, 2
      Decl: p1, [], [], [], []
        PtrDecl: []
          TypeDecl: p1, [], None
            IdentifierType: ['int']
        ID: a1
      Decl: i3, [], [], [], []
        TypeDecl: i3, [], None
          IdentifierType: ['int']
        ArrayRef:
          ID: p1
          Constant: int, 0
*/