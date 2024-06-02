#include <iostream>

#include "elf.h"

using namespace elf;

int main() {
  CATCH({
    Reader reader("/media/d/work/learn-riscv/read/example/elf/a.o");
    reader.dump(std::cout);
  });
  CATCH({
    Reader reader("/media/d/work/learn-riscv/read/example/elf/b.exe");
    reader.dump(std::cout);
  });
}
