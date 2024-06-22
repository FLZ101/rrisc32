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

const Elf_Half EM_RRISC32 = EM_RISCV;
const unsigned char ELFOSABI_RRISC32 = ELFOSABI_NONE;

const Elf_Word R_RRISC32_NONE = 0;
const Elf_Word R_RRISC32_32 = 1;
// const Elf_Word R_RRISC32_CALL = 18;
const Elf_Word R_RRISC32_HI20 = 26;
const Elf_Word R_RRISC32_LO12_I = 27;
const Elf_Word R_RRISC32_LO12_S = 28;

const u8 RRISC32_MAX_ALIGN = 12;
const unsigned RRISC32_PAGE_SIZE = 1 << RRISC32_MAX_ALIGN;
const Elf64_Addr RRISC32_ENTRY = RRISC32_PAGE_SIZE;

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

void getSymbol(const elfio &ei, section *sec, Elf_Xword idx, Symbol &sym);
void getRelocation(const elfio &ei, section *sec, Elf_Xword idx,
                   Relocation &rel);

class Reader {
public:
  using SegFn = std::function<void(segment &)>;
  using SecFn = std::function<void(section &)>;
  using SymFn = std::function<void(const Symbol &)>;
  using RelFn = std::function<void(const Relocation &)>;

  Reader(const std::string &filename);

  const elfio &getEI() const;

  void forEachSegment(SegFn fn);

  section *getSection(const Symbol &sym) {
    if (1 <= sym.sec && sym.sec < ei.sections.size())
      return ei.sections[sym.sec];
    return nullptr;
  }

  section *getSection(const Relocation &rel) {
    section *secBeTo = ei.sections[rel.secBelongTo];
    return ei.sections[secBeTo->get_info()];
  }

  Elf_Word getSymTabSecIdx(const Relocation &rel) {
    return ei.sections[rel.secBelongTo]->get_link();
  }

  section *getSymTabSec(const Relocation &rel) {
    return ei.sections[getSymTabSecIdx(rel)];
  }

  section *getSection(const std::string &name);
  void forEachSection(SecFn fn);

  void forEachSymbol(SymFn fn);
  void forEachRelocation(RelFn fn);

  void dumpELFHeader(std::ostream &os);
  void dumpSegments(std::ostream &os);
  void dumpSections(std::ostream &os);
  void dumpSymbols(std::ostream &os);
  void dumpRelocations(std::ostream &os);
  void dumpHex(std::ostream &os);
  void dumpHex(std::ostream &os, const section &sec);
  void dumpHex(std::ostream &os, const std::string &name);

protected:
  void forEachSymbol(section *sec, SymFn fn);
  void forEachRelocation(section *sec, RelFn fn);

  std::string filename;
  elfio ei;
  std::map<std::string, section *> name2Section;
};

class RRisc32Reader : public Reader {
public:
  explicit RRisc32Reader(const std::string &filename) : Reader(filename) {
    check();
  }

  void dumpDisassembly(std::ostream &os);
  void dumpDisassembly(std::ostream &os, const section &sec);
  void dumpDisassembly(std::ostream &os, const std::string &name);

private:
  void check();

  void checkSections();
  void checkSection(const std::string &name, Elf_Word type,
                    Elf_Xword flags = 0);

  void checkSymbols();
  void checkRelocations();

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

  void save();

  section *getSection(const std::string &name);
  section *addSection(const std::string &name, Elf_Word type, Elf_Xword flags);

  segment *addSegment(Elf_Word type, Elf_Word flags,
                      std::vector<section *> secs);

protected:
  std::string filename;
  elfio ei;
};

class RRisc32Writer : public Writer {
public:
  RRisc32Writer(const std::string &filename, Elf_Half type)
      : Writer(filename, type, EM_RRISC32, ELFCLASS32, ELFDATA2LSB,
               ELFOSABI_RRISC32) {}

  section *getSection(const std::string &name);

  Elf_Word addString(const std::string &s);
  Elf_Word addSymbol(const Symbol *sym);
  void addRelocation(const Relocation &rel);

private:
  std::map<std::string, Elf_Word> stringCache;
  std::map<const Symbol *, Elf_Word> symbolCache;
};

} // namespace elf

#endif
