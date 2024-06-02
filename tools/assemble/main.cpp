#include "assembly.h"

using namespace assembly;

int main(int argc, char *argv[]) {
  CATCH(
    Assembler as("/media/d/work/llvm/rrisc32/test/tools/assemble/hello.s");
    as.run()
  );
}
