
A RISC-V 32 emulator and a toolchain (C99 compiler, assembler, link editor, etc.) for it.

## Build & Test

```
$ bash build.sh
```

## Usage

hello.c:

```c
#include <stdio.h>

int main()
{
  printf("hello rrisc32\n");
}
```

```
$ ./build/bin/rrisc32-cc -o hello.exe hello.c
$ ./build/bin/rrisc32-emulate hello.exe
hello rrisc32
```
