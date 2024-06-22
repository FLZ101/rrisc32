#include <iostream>
#include <vector>

#include "cli.h"
#include "elf.h"

namespace {
struct Opts {
  bool dumpAll = false;
  bool dumpELFHeader = false;
  bool dumpSegments = false;
  bool dumpSections = false;
  bool dumpSymbols = false;
  bool dumpRelocations = false;

  std::vector<std::string> dumpHex;
  std::vector<std::string> dumpDisassembly;

  std::string filename;
};

void dump(const Opts &o) {
  TRY()
  elf::RRisc32Reader reader(o.filename);
  if (o.dumpELFHeader || o.dumpAll)
    reader.dumpELFHeader(std::cout);
  if (o.dumpSegments || o.dumpAll)
    reader.dumpSegments(std::cout);
  if (o.dumpSections || o.dumpAll)
    reader.dumpSections(std::cout);
  if (o.dumpSymbols || o.dumpAll)
    reader.dumpSymbols(std::cout);
  if (o.dumpRelocations || o.dumpAll)
    reader.dumpRelocations(std::cout);
  for (const std::string &name : o.dumpHex)
    reader.dumpHex(std::cout, name);
  for (const std::string &name : o.dumpDisassembly)
    reader.dumpDisassembly(std::cout, name);
  CATCH()
}
} // namespace

int main(int argc, char **argv) {
  CLI::App app;

  Opts o;
  app.add_flag("--all", o.dumpAll, "Same as --elf --seg --sec --sym --rel");
  app.add_flag("--elf", o.dumpELFHeader, "Display the ELF header");
  app.add_flag("--seg", o.dumpSegments, "Display segment headers");
  app.add_flag("--sec", o.dumpSections, "Display section headers");
  app.add_flag("--sym", o.dumpSymbols, "Display symbols");
  app.add_flag("--rel", o.dumpRelocations, "Display relocations");
  app.add_option("--hex", o.dumpHex, "Display hex of sections")
      ->type_name("SEC");
  app.add_option("--dis", o.dumpDisassembly, "Display disassembly of sections")
      ->type_name("SEC");
  app.add_option("<file>", o.filename)->required()->type_name("");

  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  dump(o);
}
