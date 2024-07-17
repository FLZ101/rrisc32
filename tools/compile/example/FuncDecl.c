void f1();
/*
  Decl: f1, [], [], [], []
    FuncDecl:
      TypeDecl: f1, [], None
        IdentifierType: ['void']
*/

void f2(void);
/*
  Decl: f2, [], [], [], []
    FuncDecl:
      ParamList:
        Typename: None, [], None
          TypeDecl: None, [], None
            IdentifierType: ['void']
      TypeDecl: f2, [], None
        IdentifierType: ['void']
*/

int f3(int, char);
/*
  Decl: f3, [], [], [], []
    FuncDecl:
      ParamList:
        Typename: None, [], None
          TypeDecl: None, [], None
            IdentifierType: ['int']
        Typename: None, [], None
          TypeDecl: None, [], None
            IdentifierType: ['char']
      TypeDecl: f3, [], None
        IdentifierType: ['int']
*/

int f4(int i, char c);
/*
  Decl: f4, [], [], [], []
    FuncDecl:
      ParamList:
        Decl: i, [], [], [], []
          TypeDecl: i, [], None
            IdentifierType: ['int']
        Decl: c, [], [], [], []
          TypeDecl: c, [], None
            IdentifierType: ['char']
      TypeDecl: f4, [], None
        IdentifierType: ['int']
*/

int f5(int i, char c) {
  return 0;
}
/*
  FuncDef:
    Decl: f5, [], [], [], []
      FuncDecl:
        ParamList:
          Decl: i, [], [], [], []
            TypeDecl: i, [], None
              IdentifierType: ['int']
          Decl: c, [], [], [], []
            TypeDecl: c, [], None
              IdentifierType: ['char']
        TypeDecl: f5, [], None
          IdentifierType: ['int']
    Compound:
      Return:
        Constant: int, 0
*/

int f6(int arr[3]) {
    return arr[0] + sizeof(arr); // warning: 'sizeof' on array function parameter 'arr' will return size of 'int *' [-Wsizeof-array-argument]
}

int f7() {}
/*
  FuncDef:
    Decl: f7, [], [], [], []
      FuncDecl:
        TypeDecl: f7, [], None
          IdentifierType: ['int']
    Compound:
*/

int max(a, b)
    int a, b;
{
    return a > b ? a : b;
}
/*
  FuncDef:
    Decl: max, [], [], [], []
      FuncDecl:
        ParamList:
          ID: a
          ID: b
        TypeDecl: max, [], None
          IdentifierType: ['int']
    Compound:
      Return:
        TernaryOp:
          BinaryOp: >
            ID: a
            ID: b
          ID: a
          ID: b
    Decl: a, [], [], [], []
      TypeDecl: a, [], None
        IdentifierType: ['int']
    Decl: b, [], [], [], []
      TypeDecl: b, [], None
        IdentifierType: ['int']
*/

int printf(const char *restrict format, ...);
/*
  Decl: printf, [], [], [], []
    FuncDecl:
      ParamList:
        Decl: format, ['const'], [], [], []
          PtrDecl: ['restrict']
            TypeDecl: format, ['const'], None
              IdentifierType: ['char']
        EllipsisParam:
      TypeDecl: printf, [], None
        IdentifierType: ['int']
*/
