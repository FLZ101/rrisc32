// RUN: rrisc32-cc -o %t.s %s

void f() {
    static int i;
    static int j = 10;
    {
        static int i = 11;
    }
}
