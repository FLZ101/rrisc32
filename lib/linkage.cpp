#include "linkage.h"

#include <list>
#include <map>

#include "elf.h"

#define DEBUG_TYPE "linkage"

namespace linkage {

struct InputSection {
  InputSection(elf::section *sec) : sec(sec) {}

  elf::section *sec = nullptr;
  u64 addr = 0;
};

struct OutputSection {
  explicit OutputSection(const std::string &name) { sym.name = name; }

  OutputSection(const OutputSection &) = delete;
  OutputSection &operator=(const OutputSection &) = delete;

  elf::Symbol sym = {.name = "",
                     .value = 0,
                     .size = 0,
                     .type = elf::STT_SECTION,
                     .bind = elf::STB_LOCAL,
                     .other = elf::STV_DEFAULT,
                     .sec = elf::SHN_UNDEF};

  u64 addr = 0;
  ByteBuffer bb;

  elf::section *sec = nullptr;
};

struct OutputSymbol {
  OutputSymbol(const elf::Symbol &sym) : sym(sym) {}

  u64 getValue() const {
    assert(sym.sec != elf::SHN_UNDEF);
    if (sym.sec == elf::SHN_ABS)
      return sym.value;
    if (sym.bind == elf::STB_WEAK)
      return 0;
    assert(oSec);
    return oSec->addr + sym.value;
  }

  elf::Symbol sym;
  OutputSection *oSec = nullptr;
};

struct OutputRelocation {
  OutputRelocation(const elf::Relocation &rel) : rel(rel) {}

  char *getAddr() { return oSec->bb.getData().data() + rel.offset; }

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
      if (sym.name.empty())
        return;
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

void Linker::saveToFile() {
  elf::RRisc32Writer writer(opts.outFile, elf::ET_EXEC);

  for (OutputSection *oSec : oSecs) {
    oSec->sec = writer.getSection(oSec->sym.name);
    oSec->sec->set_address(oSec->addr);
    oSec->sec->set_addr_align(elf::RRISC32_PAGE_SIZE);
    if (oSec->sym.name != ".bss")
      oSec->sec->set_data(oSec->bb.getData());
    else
      oSec->sec->set_size(oSec->sym.size);

    oSec->sym.sec = oSec->sec->get_index();
  }

  writer.getEI().set_entry(elf::RRISC32_ENTRY);

  for (OutputSection *oSec : oSecs)
    writer.addSymbol(&oSec->sym);

  for (auto &reader : readers) {
    for (auto &oSym : reader->oSyms) {
      elf::Symbol &sym = oSym->sym;
      if (sym.sec == elf::SHN_UNDEF || sym.type == elf::STT_SECTION)
        continue;
      switch (sym.bind) {
      case elf::STB_LOCAL:
        if (oSym->oSec)
          sym.sec = oSym->oSec->sec->get_index();
        writer.addSymbol(&sym);
        break;
      default:;
      }
    }
  }

  for (auto &pair : gSyms) {
    OutputSymbol *oSym = pair.second;
    elf::Symbol &sym = oSym->sym;
    if (oSym->oSec)
      sym.sec = oSym->oSec->sec->get_index();
    writer.addSymbol(&sym);
  }

  writer.addSegment(elf::PT_LOAD, elf::PF_R | elf::PF_X, {oSecText.sec});
  writer.addSegment(elf::PT_LOAD, elf::PF_R, {oSecRodata.sec});
  writer.addSegment(elf::PT_LOAD, elf::PF_R | elf::PF_W, {oSecData.sec});
  writer.addSegment(elf::PT_LOAD, elf::PF_R | elf::PF_W, {oSecBss.sec});

  writer.save();
}

void Linker::applyRelocations() {
  for (auto &reader : readers) {
    for (auto &oRel : reader->oRels) {
      const OutputSymbol *oSym = oRel->oSym;
      if (oSym->sym.sec == elf::SHN_UNDEF || oSym->sym.bind == elf::STB_WEAK)
        oSym = gSyms[oSym->sym.name];

      const elf::Relocation &rel = oRel->rel;
      u64 v = oSym->getValue() + rel.addend;

      char *addr = oRel->getAddr();
      u32 x = readLE<u32>(addr);
      switch (rel.type) {
      case elf::R_RRISC32_32:
        x = static_cast<u32>(v);
        break;
      case elf::R_RRISC32_HI20: {
        s32 imm = hi20(v);
        x &= 0xfff;
        x |= imm << 12;
        break;
      }
      case elf::R_RRISC32_LO12_I: {
        s32 imm = lo12(v);
        x &= 0xfffff;
        x |= imm << 20;
        break;
      }
      case elf::R_RRISC32_LO12_S: {
        s32 imm = lo12(v);
        s32 y = 0;
        y |= x & 0x7f;
        y |= (imm & 0b11111) << 7;
        y |= (x >> 12 & 0x1fff) << 12;
        y |= imm >> 5 << 25;
        x = y;
        break;
      }
      default:
        THROW(LinkageError, "unknown relocation type", toHexStr(rel.type));
      }
      writeLE(addr, x);
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
    OutputSection *oSec = getOSec(name);

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

      if (name != ".bss")
        oSec->bb.append(sec->get_data(), sec->get_size());
      offset += sec->get_size();
      oSec->sym.size = offset - oSec->addr;
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
    if (oSec->sym.name == name) {
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
  applyRelocations();
  saveToFile();
}

void link(const LinkerOpts &o) { Linker(o).run(); }

} // namespace linkage
