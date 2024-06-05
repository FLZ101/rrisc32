#include "assembly.h"

using namespace assembly;

#define DEBUG_TYPE "apple"

int main(int argc, char *argv[]) {
  CLI::App app;

  ADD_DEBUG_OPT(app);

  CLI11_PARSE(app, argc, argv);

  try {
    Assembler as("test/tools/assemble/hello.s");
    as.run();
  } catch (Exception &ex) {
    std::cerr << "!!! " << ex.what() << "\n";
  }

  DEBUG(std::cerr << "hello\n");
}
