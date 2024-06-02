#ifndef ELF_H
#define ELF_H

#include <functional>
#include <map>
#include <ostream>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

#include "util.h"

namespace elf {

using namespace ELFIO;

DEFINE_EXCEPTION(ELFError)

const Elf_Half EM_RRISC32 = 0x1234;
const unsigned char ELFOSABI_RRISC32 = 0x12;

struct Symbol {
  std::string name;
  Elf64_Addr value;
  Elf_Xword size;
  unsigned char type;
  unsigned char bind;
  unsigned char other;
  Elf_Half sec;

  ELFIO::Elf_Xword idx;
  Elf_Half secBelongTo;
};

struct Relocation {
  Elf64_Addr offset;
  Elf_Word sym;
  Elf_Word type;
  Elf_Sxword addend;

  ELFIO::Elf_Xword idx;
  Elf_Half secBelongTo;
};

class Reader {
public:
  using SegFn = std::function<void(segment &)>;
  using SecFn = std::function<void(section &)>;
  using SymFn = std::function<void(Symbol &)>;
  using RelFn = std::function<void(Relocation &)>;

  Reader(const std::string &filename);

  const elfio &getEI() const;

  void forEachSegment(SegFn fn);

  section *getSection(const std::string &name);
  void forEachSection(SecFn fn);

  void getSymbol(section *sec, Elf_Xword idx, Symbol &sym);
  void forEachSymbol(SymFn fn);

  void getRelocation(section *sec, Elf_Xword idx, Relocation &rel);
  void forEachRelocation(RelFn fn);

  void dump(std::ostream &os);
  void dumpELFHeader(std::ostream &os);
  void dumpSegments(std::ostream &os);
  void dumpSections(std::ostream &os);
  void dumpSymbols(std::ostream &os);
  void dumpRelocations(std::ostream &os);

protected:
  void forEachSymbol(section *sec, SymFn fn);
  void forEachRelocation(section *sec, RelFn fn);

  std::string filename;
  elfio ei;
  std::map<std::string, section *> name2Section;
};

class RRisc32Reader : public Reader {
public:
  RRisc32Reader(std::string filename) : Reader(filename) { check(); }

private:
  void check();

  void checkSections();
  void checkSection(const std::string &name, Elf_Word type,
                    Elf_Xword flags = 0);

  bool checkSectionName(const std::string &name) {
    return std::find(secNames.begin(), secNames.end(), name) != secNames.end();
  }

  void checkSymbols();

  // clang-format off
  const std::vector<std::string> secNames = {
    ".text", ".rodata", ".data", ".bss",
    ".rela.text", ".rela.rodata", ".rela.data", ".rela.bss",
    ".strtab", ".symtab", ".shstrtab"
  };
  // clang-format on
};

class Writer {
public:
  Writer(const std::string &filename, Elf_Half type, Elf_Half machine,
         unsigned char cls, unsigned char enc, unsigned char oa);

  elfio &getEI() { return ei; }

  void save() {
    if (!ei.save(filename))
      THROW(ELFError, "save", filename);
  }

  section *getSection(const std::string &name);
  section *addSection(const std::string &name, Elf_Word type, Elf_Xword flags);

  section *getStringTableSec() {
    if (!stringTableSec)
      stringTableSec = addSection(".symtab", SHT_SYMTAB, 0);
    return stringTableSec;
  }

  section *getSymbolTableSec() {
    if (!symbolTableSec)
      symbolTableSec = addSection(".strtab", SHT_STRTAB, 0);
    return symbolTableSec;
  }

protected:
  section *stringTableSec = nullptr;
  section *symbolTableSec = nullptr;

  std::string filename;
  elfio ei;
};

class RRisc32Writer : public Writer {
public:
  RRisc32Writer(const std::string &filename, Elf_Half type)
      : Writer(filename, type, EM_RRISC32, ELFCLASS32, ELFDATA2LSB,
               ELFOSABI_RRISC32) {}

  section *addSection(const std::string &name);
};

} // namespace elf

#endif
