#include "linkage.h"

#include <list>
#include <map>

#include "elf.h"

#define DEBUG_TYPE "link"

namespace linkage {

struct InputSection {
  elf::section *sec = nullptr;
};

struct OutputSection {
  explicit OutputSection(const std::string &name) : name(name) {}

  OutputSection(const OutputSection &) = delete;
  OutputSection &operator=(const OutputSection &) = delete;

  const std::string name;
  u64 addr = 0;
  ByteBuffer bb;
};

class Linker {
public:
  Linker(const LinkerOpts &o) : opts(o) {}

  Linker(const Linker &) = delete;
  Linker &operator=(const Linker &) = delete;

  void run();

private:
  void saveToFile();

  OutputSection *getOSec(const std::string &name);

  u64 offset = elf::RRISC32_ENTRY;

  OutputSection oSecText{".text"};
  OutputSection oSecRodata{".rodata"};
  OutputSection oSecData{".data"};
  OutputSection oSecBss{".bss"};

  OutputSection *oSections[4] = {&oSecText, &oSecRodata, &oSecData, &oSecBss};

  std::list<std::unique_ptr<elf::RRisc32Reader>> readers;

  LinkerOpts opts;
};

OutputSection *Linker::getOSec(const std::string &name) {
  for (auto *oSec : oSections)
    if (oSec->name == name) {
      if (!oSec->addr) {
        offset = P2ALIGN(offset, elf::RRISC32_MAX_ALIGN);
        oSec->addr = offset;
      }
      return oSec;
    }
  THROW(LinkageError, "unexpected section name", name);
}

/*
for name in .text, .rodata, .data, .bss
  for each reader
    if has name
      create sec (max_align) if not
      align by reader align
      add reader sec
        set sec addr

for each reader
  map symbols
    value += sec addr

apply relocations
  resolve symbol
    local ?
    weak ?
    global ?

  sec, offset, symbol (value)

add symbols

create segments
  .text
  .rodata
  .data
  .bss
*/

void Linker::saveToFile() {
  elf::RRisc32Writer writer(opts.outFile, elf::ET_EXEC);
}

void Linker::run() {
  for (const std::string &filename : opts.inFiles)
    readers.emplace_back(
        std::move(std::make_unique<elf::RRisc32Reader>(filename)));

  for (const std::string &name : {".text", ".rodata", ".data", ".bss"}) {
    for (const auto &reader : readers) {
      elf::section *sec = reader->getSection(name);
      if (!sec)
        continue;
      u8 n = 0;
      if (!log2(sec->get_addr_align(), n) || n > elf::RRISC32_MAX_ALIGN)
        THROW(LinkageError, "invalid section alignment", sec->get_addr_align());
      offset = P2ALIGN(offset, n);
      sec->set_address(offset);

      getOSec(name)->bb.append(sec->get_data(), sec->get_size());
      offset += sec->get_size();
    }
  }
}

void link(const LinkerOpts &o) { Linker(o).run(); }

} // namespace linkage
