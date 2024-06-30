
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
