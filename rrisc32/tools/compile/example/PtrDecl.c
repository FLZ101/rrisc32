int *p1;
/*
  Decl: p1, [], [], [], []
    PtrDecl: []
      TypeDecl: p1, [], None
        IdentifierType: ['int']
*/

int **p2 = &p1;
/*
  Decl: p2, [], [], [], []
    PtrDecl: []
      PtrDecl: []
        TypeDecl: p2, [], None
          IdentifierType: ['int']
    UnaryOp: &
      ID: p1
*/
