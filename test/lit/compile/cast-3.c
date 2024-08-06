// RUN: rrisc32-cc --compile -o %t.s %s

void f()
{
    char c;
    short s;
    int i;
    long long ll;

    unsigned char uc;
    unsigned short us;
    unsigned int ui;
    unsigned long long ull;

    c = -3;
    uc = -3;
    ll = -3;
    ull = -3;
}
