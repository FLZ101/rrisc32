#include <iostream>

#include "elf.h"

using namespace elf;

int main() {
  TRY()
  Reader reader(
      "/home/z30026696/opt/rrisc32/rrisc32/test/tools/assemble/hello.s.o");
  reader.dump(std::cout);
  CATCH()
}
