#include "cli.h"
#include "emulation.h"
#include "rrisc32.h"

using namespace emulation;

int main(int argc, char *argv[]) {
  EmulatorOpts o;

  CLI::App app;
  app.add_option("<exe_file>", o.exeFile)->required()->type_name("");
  app.add_option("<arg>", o.args)->type_name("");
  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  TRY()
  rrisc32::setRegNameX(false);
  emulate(o);
  CATCH()
}
