int a[10];
/*
  Decl: a, [], [], [], []
    ArrayDecl: []
      TypeDecl: a, [], None
        IdentifierType: ['int']
      Constant: int, 10
*/

int a[10] = {1, 2, 3};
/*
  Decl: a, [], [], [], []
    ArrayDecl: []
      TypeDecl: a, [], None
        IdentifierType: ['int']
      Constant: int, 10
    InitList:
      Constant: int, 1
      Constant: int, 2
      Constant: int, 3
*/

int a[] = {1, 2, 3};
/*
  Decl: a, [], [], [], []
    ArrayDecl: []
      TypeDecl: a, [], None
        IdentifierType: ['int']
    InitList:
      Constant: int, 1
      Constant: int, 2
      Constant: int, 3
*/

int a;
/*
  Decl: a, [], [], [], []
    TypeDecl: a, [], None
      IdentifierType: ['int']
*/

int a = 10;
/*
  Decl: a, [], [], [], []
    TypeDecl: a, [], None
      IdentifierType: ['int']
    Constant: int, 10
*/


struct Foo {
    int i;
    char c;
};

struct Foo foo;

struct Foo foo = { 1, 2 };

struct Bar {
    int i;
    struct Foo foo;
};

struct Bar bar = { 1, {2, 3} };

struct Foo;
struct Foo {};
