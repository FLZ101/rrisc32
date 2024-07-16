// RUN: rrisc32-cc -o %t.s %s

// int i1;
// int i2 = 100;

// int arr1[] = {};
// int arr2[] = {1, 2, 3};
// int arr3[3] = {1};

// char *s1 = "";
// char *s2 = "hello";
// char s3[] = "hello";
// char s4[] = {'h', 'e', 'l', 'l', 'o'};

struct Foo
{
    char c;
    int i;
    char s[2];
};

// struct Foo f1 = { 'a', 10, "b" };

typedef struct Foo Goo;
Goo g1 = { 'a' };

Goo *pg1 = &g1;
