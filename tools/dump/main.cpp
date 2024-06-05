#include <iostream>

#include "elf.h"

using namespace elf;

int main() {
  try {
    {
      Reader reader("/media/d/work/learn-riscv/read/example/elf/a.o");
      reader.dump(std::cout);
    }
    {
      Reader reader("/media/d/work/learn-riscv/read/example/elf/b.exe");
      reader.dump(std::cout);
    }
  } catch (Exception &ex) {
    std::cerr << "!!! " << ex.what() << "\n";
  }
}
