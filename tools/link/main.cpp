#include "cli.h"
#include "linkage.h"

using namespace linkage;

int main(int argc, char *argv[]) {
  LinkerOpts o;

  CLI::App app;
  app.add_option("-o", o.outFile, "Output file")->required()->type_name("FILE");
  app.add_option("<input_file>", o.inFiles)->required()->type_name("");
  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  TRY()
  link(o);
  CATCH()
}