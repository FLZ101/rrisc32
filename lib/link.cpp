#include "link.h"

#include <list>

#include "elf.h"

#define DEBUG_TYPE "link"

namespace link {

class Linker {
public:
  Linker(const LinkerOpts &o) : opts(o) {}

  Linker(const Linker &) = delete;
  Linker &operator=(const Linker &) = delete;

  void run();

private:
  void concatenateSections();

  std::list<std::unique_ptr<elf::RRisc32Reader>> readers;
  std::unique_ptr<elf::RRisc32Writer> writer;

  LinkerOpts opts;
};

void Linker::run() {
  for (const std::string &filename : opts.inFiles)
    readers.emplace_back(
        std::move(std::make_unique<elf::RRisc32Reader>(filename)));
  writer = std::move(std::make_unique<elf::RRisc32Writer>(opts.outFile, elf::ET_EXEC));
}

void link(const LinkerOpts &o) { Linker(o).run(); }

} // namespace link
