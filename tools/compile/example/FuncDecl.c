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
