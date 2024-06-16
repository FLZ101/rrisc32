#include <iostream>

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

  std::string filename;
};

void dump(const Opts &o) {
  TRY()
  elf::Reader reader(o.filename);
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
  CATCH()
}
} // namespace

int main(int argc, char **argv) {
  CLI::App app{"Dump an ELF file"};

  Opts o;
  app.add_flag("--all", o.dumpAll, "Same as --elf --seg --sec --sym --rel");
  app.add_flag("--elf", o.dumpELFHeader, "Display the ELF header");
  app.add_flag("--seg", o.dumpSegments, "Display segment headers");
  app.add_flag("--sec", o.dumpSections, "Display section headers");
  app.add_flag("--sym", o.dumpSymbols, "Display symbols");
  app.add_flag("--rel", o.dumpRelocations, "Display relocations");
  app.add_option("<file>", o.filename)->required();

  ADD_DEBUG_OPT(app);
  CLI11_PARSE(app, argc, argv);

  dump(o);
}
