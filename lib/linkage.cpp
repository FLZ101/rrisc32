#include "linkage.h"

#include <list>
#include <map>

#include "elf.h"

#define DEBUG_TYPE "link"

namespace linkage {

struct InputSection {
  InputSection(elf::section *sec) : sec(sec) {}

  elf::section *sec = nullptr;
  u64 addr = 0;
};

struct OutputSection {
  explicit OutputSection(const std::string &name) : name(name) {}

  OutputSection(const OutputSection &) = delete;
  OutputSection &operator=(const OutputSection &) = delete;

  const std::string name;
  u64 addr = 0;
  ByteBuffer bb;
};

struct OutputSymbol {
  OutputSymbol(const elf::Symbol &sym) : sym(sym) {}

  elf::Symbol sym;
  OutputSection *oSec = nullptr;
};

struct OutputRelocation {
  OutputRelocation(const elf::Relocation &rel) : rel(rel) {}

  elf::Relocation rel;
  OutputSection *oSec = nullptr;
  OutputSymbol *oSym = nullptr;
};

struct Reader : elf::RRisc32Reader {
  Reader(const std::string &name) : elf::RRisc32Reader(name) {
    forEachSection([this](elf::section &sec) {
      if (sec.get_name().empty())
        return;
      iSecs.emplace_back(std::move(std::make_unique<InputSection>(&sec)));
    });
    forEachSymbol([this](const elf::Symbol &sym) {
      oSyms.emplace_back(std::move(std::make_unique<OutputSymbol>(sym)));
    });
    forEachRelocation([this](const elf::Relocation &rel) {
      oRels.emplace_back(std::move(std::make_unique<OutputRelocation>(rel)));
    });
  }

  OutputSymbol *getOSym(OutputRelocation *oRel) { return getOSym(oRel->rel); }

  OutputSymbol *getOSym(const elf::Relocation &rel) {
    return getOSym(getSymTabSecIdx(rel), rel.sym);
  }

  OutputSymbol *getOSym(elf::Elf_Half secIdx, elf::Elf_Word symIdx) {
    for (auto &oSym : oSyms)
      if (oSym->sym.secBelongTo == secIdx && oSym->sym.idx == symIdx)
        return oSym.get();
    return nullptr;
  }

  InputSection *getISec(elf::section *sec) { return getISec(sec->get_name()); }

  InputSection *getISec(const std::string &name) {
    for (auto &iSec : iSecs)
      if (iSec->sec->get_name() == name)
        return iSec.get();
    return nullptr;
  }

  std::vector<std::unique_ptr<InputSection>> iSecs;
  std::vector<std::unique_ptr<OutputSymbol>> oSyms;
  std::vector<std::unique_ptr<OutputRelocation>> oRels;
};

class Linker {
public:
  Linker(const LinkerOpts &o) : opts(o) {}

  Linker(const Linker &) = delete;
  Linker &operator=(const Linker &) = delete;

  void run();

private:
  void saveToFile();

  void applyRelocations();
  void linkSymbols();
  void concatenateISecs();

  OutputSection *getOSec(elf::section *sec) { return getOSec(sec->get_name()); }

  OutputSection *getOSec(const std::string &name);

  std::map<std::string, OutputSymbol *> gSyms;

  OutputSection oSecText{".text"};
  OutputSection oSecRodata{".rodata"};
  OutputSection oSecData{".data"};
  OutputSection oSecBss{".bss"};

  OutputSection *oSecs[4] = {&oSecText, &oSecRodata, &oSecData, &oSecBss};

  u64 offset = elf::RRISC32_ENTRY;

  std::list<std::unique_ptr<Reader>> readers;

  LinkerOpts opts;
};

/*

resolve symbols

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

void Linker::applyRelocations() {
  for (auto &reader : readers) {
    for (auto &oRel : reader->oRels) {

    }
  }
}

void Linker::linkSymbols() {
  for (auto &reader : readers) {
    for (auto &oSym : reader->oSyms) {
      const elf::Symbol &sym = oSym->sym;
      if (sym.sec == elf::SHN_UNDEF)
        continue;
      const std::string &name = sym.name;
      switch (sym.bind) {
      case elf::STB_GLOBAL:
        if (gSyms.contains(name) && gSyms[name]->sym.bind != elf::STB_WEAK)
          THROW(LinkageError, "redefined symbol", name);
        gSyms[name] = oSym.get();
        break;
      case elf::STB_WEAK:
        if (!gSyms.contains(name))
          gSyms[name] = oSym.get();
        break;
      case elf::STB_LOCAL:
        break;
      default:
        THROW(LinkageError, elf::dump::str_symbol_bind(sym.bind));
      }
    }
  }

  for (auto &reader : readers) {
    for (auto &oSym : reader->oSyms) {
      const elf::Symbol &sym = oSym->sym;
      if (sym.sec == elf::SHN_UNDEF && !gSyms.contains(sym.name))
        THROW(LinkageError, "undefined symbol", sym.name);
    }
  }
}

void Linker::concatenateISecs() {
  for (const std::string &name : {".text", ".rodata", ".data", ".bss"}) {
    for (auto &reader : readers) {
      InputSection *iSec = reader->getISec(name);
      if (!iSec)
        continue;
      elf::section *sec = iSec->sec;

      u8 n = 0;
      if (!log2(sec->get_addr_align(), n) || n > elf::RRISC32_MAX_ALIGN)
        THROW(LinkageError, "invalid section alignment", sec->get_addr_align());
      offset = P2ALIGN(offset, n);
      iSec->addr = offset;

      getOSec(name)->bb.append(sec->get_data(), sec->get_size());
      offset += sec->get_size();
    }
  }

  // map symbols and relocations
  for (auto &reader : readers) {
    for (auto &oSym : reader->oSyms) {
      elf::section *sec = reader->getSection(oSym->sym);
      if (!sec)
        continue;
      oSym->oSec = getOSec(sec);
      oSym->sym.value += reader->getISec(sec)->addr;
      oSym->sym.value -= oSym->oSec->addr;
    }
    for (auto &oRel : reader->oRels) {
      elf::section *sec = reader->getSection(oRel->rel);
      oRel->oSec = getOSec(sec);
      oRel->rel.offset += reader->getISec(sec)->addr;
      oRel->rel.offset -= oRel->oSec->addr;
      oRel->oSym = reader->getOSym(oRel.get());
    }
  }
}

OutputSection *Linker::getOSec(const std::string &name) {
  for (auto *oSec : oSecs)
    if (oSec->name == name) {
      if (!oSec->addr) {
        offset = P2ALIGN(offset, elf::RRISC32_MAX_ALIGN);
        oSec->addr = offset;
      }
      return oSec;
    }
  THROW(LinkageError, "unexpected section name", name);
}

void Linker::run() {
  for (const std::string &filename : opts.inFiles)
    readers.emplace_back(std::move(std::make_unique<Reader>(filename)));

  concatenateISecs();
  linkSymbols();
}

void link(const LinkerOpts &o) { Linker(o).run(); }

} // namespace linkage
