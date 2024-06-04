#include "assembly.h"

using namespace assembly;

int main(int argc, char *argv[]) {
  Assembler as("test/tools/assemble/hello.s"); 
  CATCH(as.run(); as.run());
}
