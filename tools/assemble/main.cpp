#include "assembly.h"
#include "cli.h"

using namespace assembly;

#define DEBUG_TYPE "apple"

int main(int argc, char *argv[]) {
  CLI::App app;
  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  TRY()
  Assembler as({.inFile = "test/tools/assemble/hello.s"});
  as.run();
  CATCH()

  DEBUG(std::cerr << "hello\n");
}
