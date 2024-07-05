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
