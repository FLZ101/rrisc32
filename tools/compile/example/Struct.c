
struct A;
/*
  Decl: None, [], [], [], []
    Struct: A
*/

// struct A {};

struct A {
  int i;
  unsigned int b : 3;
  char c;
};
/*
  Decl: None, [], [], [], []
    Struct: A
      Decl: i, [], [], [], []
        TypeDecl: i, [], None
          IdentifierType: ['int']
      Decl: b, [], [], [], []
        TypeDecl: b, [], None
          IdentifierType: ['unsigned', 'int']
        Constant: int, 3
      Decl: c, [], [], [], []
        TypeDecl: c, [], None
          IdentifierType: ['char']
*/

struct A a;
/*
  Decl: a, [], [], [], []
    TypeDecl: a, [], None
      Struct: A
*/
