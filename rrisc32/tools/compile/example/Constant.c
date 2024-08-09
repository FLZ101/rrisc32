int i1 = 10;
/*
  Decl: i1, [], [], [], []
    TypeDecl: i1, [], None
      IdentifierType: ['int']
    Constant: int, 10
*/

int i2 = 'a';
/*
  Decl: i2, [], [], [], []
    TypeDecl: i2, [], None
      IdentifierType: ['int']
    Constant: char, 'a'
*/

int i3 = 10UL;
/*
  Decl: i3, [], [], [], []
    TypeDecl: i3, [], None
      IdentifierType: ['int']
    Constant: unsigned long int, 10UL
*/

unsigned i4 = -1U;
/*
  Decl: i4, [], [], [], []
    TypeDecl: i4, [], None
      IdentifierType: ['unsigned']
    UnaryOp: -
      Constant: unsigned int, 1U
*/

char *s1 = "hello";
/*
  Decl: s, [], [], [], []
    PtrDecl: []
      TypeDecl: s, [], None
        IdentifierType: ['char']
    Constant: string, "hello"
*/

char s2[] = "world";
/*
  Decl: s2, [], [], [], []
    ArrayDecl: []
      TypeDecl: s2, [], None
        IdentifierType: ['char']
    Constant: string, "world"
*/

int i5 = sizeof("hello");
/*
  Decl: i5, [], [], [], []
    TypeDecl: i5, [], None
      IdentifierType: ['int']
    UnaryOp: sizeof
      Constant: string, "hello"
*/