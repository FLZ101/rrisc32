#include "assembly.h"
#include "cli.h"

using namespace assembly;

int main(int argc, char *argv[]) {
  AssemblerOpts o;

  CLI::App app;
  app.add_option("-o", o.outFile, "Output file (default <input_file>.o)")
      ->type_name("FILE");
  app.add_option("<input_file>", o.inFile)->required()->type_name("");
  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  TRY()
  assemble(o);
  CATCH()
}
