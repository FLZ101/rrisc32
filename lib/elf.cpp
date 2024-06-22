#include "elf.h"

#include "rrisc32.h"

#define DEBUG_TYPE "elf"

namespace elf {

void getSymbol(const elfio &ei, section *sec, Elf_Xword idx, Symbol &sym) {
  bool ok = symbol_section_accessor(ei, sec).get_symbol(
      idx, sym.name, sym.value, sym.size, sym.bind, sym.type, sym.sec,
      sym.other);
  if (!ok)
    THROW(ELFError, "get symbol", sec->get_name(), idx);
  sym.idx = idx;
  sym.secBelongTo = sec->get_index();
}

void getRelocation(const elfio &ei, section *sec, Elf_Xword idx,
                   Relocation &rel) {
  bool ok = relocation_section_accessor(ei, sec).get_entry(
      idx, rel.offset, rel.sym, rel.type, rel.addend);
  if (!ok)
    THROW(ELFError, "get relocation", sec->get_name(), idx);
  rel.idx = idx;
  rel.secBelongTo = sec->get_index();
}

Reader::Reader(const std::string &filename) : filename(filename) {
  if (!ei.load(filename))
    THROW(ELFError, "load", escape(filename));

  forEachSection([this](section &sec) {
    auto name = sec.get_name();
    if (name2Section.count(name))
      THROW(ELFError, "duplicated section name", name);
    name2Section[name] = &sec;
  });
}

const elfio &Reader::getEI() const { return ei; }

void Reader::forEachSegment(Reader::SegFn fn) {
  for (auto &seg : ei.segments) {
    fn(*seg.get());
  }
}

section *Reader::getSection(const std::string &name) {
  if (name2Section.count(name))
    return name2Section[name];
  return nullptr;
}

void Reader::forEachSection(Reader::SecFn fn) {
  for (auto &sec : ei.sections) {
    fn(*sec.get());
  }
}

void Reader::forEachSymbol(Reader::SymFn fn) {
  forEachSection([&](section &sec) {
    if (sec.get_type() == SHT_SYMTAB)
      forEachSymbol(&sec, fn);
  });
}

void Reader::forEachSymbol(section *sec, Reader::SymFn fn) {
  if (!sec)
    return;
  Elf_Xword n = symbol_section_accessor(ei, sec).get_symbols_num();
  for (Elf_Xword i = 0; i < n; ++i) {
    Symbol sym;
    getSymbol(ei, sec, i, sym);
    fn(sym);
  }
}

void Reader::forEachRelocation(Reader::RelFn fn) {
  forEachSection([&](section &sec) {
    auto type = sec.get_type();
    if (type == SHT_REL || type == SHT_RELA)
      forEachRelocation(&sec, fn);
  });
}

void Reader::forEachRelocation(section *sec, Reader::RelFn fn) {
  if (!sec)
    return;
  Elf_Xword n = relocation_section_accessor(ei, sec).get_entries_num();
  for (Elf_Xword i = 0; i < n; ++i) {
    Relocation rel;
    getRelocation(ei, sec, i, rel);
    fn(rel);
  }
}

static std::string str_os_abi(unsigned char oa) {
  if (oa == ELFOSABI_RRISC32)
    return "ELFOSABI_RRISC32";
  return dump::str_os_abi(oa);
}

static std::string str_machine(Elf_Half machine) {
  if (machine == EM_RRISC32)
    return "EM_RRISC32";
  return dump::str_machine(machine);
}

void Reader::dumpELFHeader(std::ostream &os) {
  os << "[ ELF Header ]\n";
  // clang-format off
  os << dump::str_type(ei.get_type()) << "\n"
     << str_machine(ei.get_machine()) << "\n"
     << dump::str_class(ei.get_class()) << "\n"
     << dump::str_endian(ei.get_encoding()) << "\n"
     << str_os_abi(ei.get_os_abi()) << "\n"
     << toHexStr(ei.get_entry()) << "\n";
  // clang-format on
}

void Reader::dumpSegments(std::ostream &os) {
  bool headerDumped = false;
  forEachSegment([&](segment &seg) {
    auto type = seg.get_type();
    if (type != PT_LOAD)
      return;
    if (!headerDumped) {
      os << "[ Segments ]\n";
      os << "Type\tOffset\tVaddr\tPaddr\tFilesz\tMemsz\tFlags\tAlign\n";
      headerDumped = true;
    }
    // clang-format off
    os << dump::str_segment_type(type) << "\t"
       << toHexStr(seg.get_offset()) << "\t"
       << toHexStr(seg.get_virtual_address()) << "\t"
       << toHexStr(seg.get_physical_address()) << "\t"
       << toHexStr(seg.get_file_size()) << "\t"
       << toHexStr(seg.get_memory_size()) << "\t"
       << dump::str_segment_flag(seg.get_flags()) << "\t"
       << toHexStr(seg.get_align()) << "\n";
    // clang-format on
  });
}

static std::string str_section_flag(Elf_Xword flags) {
  std::string res = "";
  if (flags & SHF_ALLOC)
    res += "A";
  if (flags & SHF_EXECINSTR)
    res += "E";
  if (flags & SHF_WRITE)
    res += "W";
  return res;
}

void Reader::dumpSections(std::ostream &os) {
  bool headerDumped = false;
  forEachSection([&](section &sec) {
    auto type = sec.get_type();
    switch (type) {
    case SHT_PROGBITS:
    case SHT_SYMTAB:
    case SHT_STRTAB:
    case SHT_RELA:
    case SHT_NOBITS:
    case SHT_REL:
      break;
    default:
      return;
    }
    if (!headerDumped) {
      os << "[ Sections ]\n";
      os << "Idx\tAddr\tOff\tSize\tLink\tInfo\tAddrAli\tEntSize\tFlags\t"
            "Type\tName\n";
      headerDumped = true;
    }
    // clang-format off
    os << toHexStr(sec.get_index()) << "\t"
       << toHexStr(sec.get_address()) << "\t"
       << toHexStr(sec.get_offset()) << "\t"
       << toHexStr(sec.get_size()) << "\t"
       << toHexStr(sec.get_link()) << "\t"
       << toHexStr(sec.get_info()) << "\t"
       << toHexStr(sec.get_addr_align()) << "\t"
       << toHexStr(sec.get_entry_size()) << "\t"
       << str_section_flag(sec.get_flags()) << "\t"
       << substr(dump::str_section_type(sec.get_type()), 0, 7) << "\t"
       << sec.get_name() << "\n";
    // clang-format on
  });
}

static std::string str_symbol_vis(unsigned char other) {
  switch (ELF_ST_VISIBILITY(other)) {
  case (STV_DEFAULT):
    return "DEF";
  case (STV_HIDDEN):
    return "HID";
  case (STV_INTERNAL):
    return "INT";
  case (STV_PROTECTED):
    return "PRO";
  }
  return "";
}

static std::string str_section_idx(Elf_Half idx) {
  switch (idx) {
  case SHN_ABS:
    return "ABS";
  case SHN_COMMON:
    return "COM";
  case SHN_UNDEF:
    return "UND";
  default:
    return toHexStr(idx);
  }
}

void Reader::dumpSymbols(std::ostream &os) {
  bool headerDumped = false;
  forEachSymbol([&](const Symbol &sym) {
    switch (sym.type) {
    case STT_OBJECT:
    case STT_FUNC:
    case STT_SECTION:
    case STT_NOTYPE:
      break;
    default:
      return;
    }
    if (!headerDumped) {
      os << "[ Symbols ]\n";
      os << "SecBeTo\tIdx\tValue\tSize\tBind\tType\tVis\tSec\tName\n";
      headerDumped = true;
    }
    // clang-format off
    os << str_section_idx(sym.secBelongTo) << "\t"
       << toHexStr(sym.idx) << "\t"
       << toHexStr(sym.value) << "\t"
       << toHexStr(sym.size) << "\t"
       << dump::str_symbol_bind(sym.bind) << "\t"
       << dump::str_symbol_type(sym.type) << "\t"
       << str_symbol_vis(sym.other) << "\t"
       << str_section_idx(sym.sec) << "\t"
       << sym.name << "\n";
    // clang-format on
  });
}

std::string str_relocation_type(Elf_Word type) {
  switch (type) {
  case R_RRISC32_NONE:
    return "NONE";
  case R_RRISC32_32:
    return "32";
  case R_RRISC32_HI20:
    return "HI20";
  case R_RRISC32_LO12_I:
    return "LO12_I";
  case R_RRISC32_LO12_S:
    return "LO12_S";
  default:
    return toHexStr(type, true);
  }
}

void Reader::dumpRelocations(std::ostream &os) {
  bool headerDumped = false;
  forEachRelocation([&](const Relocation &rel) {
    if (!headerDumped) {
      os << "[ Relocations ]\n";
      os << "SecBeTo\tIdx\tOffset\tType\tAddend\tSym\tSecSym\tSecRel\n";
      headerDumped = true;
    }
    // clang-format off
    os << toHexStr(rel.secBelongTo) << "\t"
       << toHexStr(rel.idx) << "\t"
       << toHexStr(rel.offset) << "\t"
       << str_relocation_type(rel.type) << "\t"
       << rel.addend << "\t"
       << toHexStr(rel.sym) << "\t"
       << toHexStr(ei.sections[rel.secBelongTo]->get_link()) << "\t"
       << toHexStr(ei.sections[rel.secBelongTo]->get_info()) << "\n";
    // clang-format on
  });
}

void Reader::dumpHex(std::ostream &os) {
  forEachSection([this, &os](const section &sec) {
    if (sec.get_type() == SHT_PROGBITS && !(sec.get_flags() & SHF_EXECINSTR))
      dumpHex(os, sec);
  });
}

void Reader::dumpHex(std::ostream &os, const section &sec) {
  const char *s = sec.get_data();
  Elf_Xword n = sec.get_size();
  Elf64_Addr addr = sec.get_address();
  os << "[ Hex/" << sec.get_name() << " ]\n";
  for (Elf_Xword i = 0; i < n;) {
    u32 b = 0;
    memcpy(&b, s + i, n - i < 4 ? n - i : 4);
    if (i % 16 == 0)
      os << toHexStr(addr + i, false, false) << "  ";
    else
      os << " ";
    os << toHexStr(b, false, false);
    i += 4;
    if (i % 16 == 0 || i >= n)
      os << "\n";
  }
}

void Reader::dumpHex(std::ostream &os, const std::string &name) {
  if (name2Section.contains(name))
    dumpHex(os, *name2Section[name]);
}

void RRisc32Reader::dumpDisassembly(std::ostream &os) {
  forEachSection([this, &os](const section &sec) {
    if (sec.get_type() == SHT_PROGBITS && sec.get_flags() & SHF_EXECINSTR)
      dumpDisassembly(os, sec);
  });
}

void RRisc32Reader::dumpDisassembly(std::ostream &os, const section &sec) {
  const char *s = sec.get_data();
  Elf_Xword n = sec.get_size();
  Elf64_Addr addr = sec.get_address();
  os << "[ Disassembly/" << sec.get_name() << " ]\n";
  for (Elf_Xword i = 0; i + 4 <= n; i += 4) {
    u32 b = *reinterpret_cast<const u32 *>(s + i);
    os << toHexStr(static_cast<u32>(addr + i), false, false) << "  "
       << rrisc32::test::decode(b) << "\n";
  }
}

void RRisc32Reader::dumpDisassembly(std::ostream &os, const std::string &name) {
  if (name2Section.contains(name))
    dumpDisassembly(os, *name2Section[name]);
}

void RRisc32Reader::check() {
  auto cls = ei.get_class();
  if (cls != ELFCLASS32)
    THROW(ELFError, dump::str_class(cls));

  auto enc = ei.get_encoding();
  if (enc != ELFDATA2LSB)
    THROW(ELFError, dump::str_endian(enc));

  auto type = ei.get_type();
  switch (type) {
  case ET_REL:
  case ET_EXEC:
    break;
  default:
    THROW(ELFError, dump::str_type(type));
  }

  auto oa = ei.get_os_abi();
  if (oa != ELFOSABI_RRISC32)
    THROW(ELFError, dump::str_os_abi(oa));

  auto machine = ei.get_machine();
  if (machine != EM_RRISC32)
    THROW(ELFError, dump::str_machine(machine));

  checkSections();
  checkSymbols();
  checkRelocations();
}

#define UNEXPECTED_SECTION_NAME(name)                                          \
  THROW(ELFError, "unexpected section name", escape(name))

void RRisc32Reader::checkSections() {
  forEachSection([this](section &sec) {
    std::string name = sec.get_name();
    if (name.empty())
      return;
    if (!isOneOf(name, secNames))
      UNEXPECTED_SECTION_NAME(name);
  });
  checkSection(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
  checkSection(".rodata", SHT_PROGBITS, SHF_ALLOC);
  checkSection(".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
  checkSection(".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE);
  checkSection(".rela.text", SHT_RELA);
  checkSection(".rela.rodata", SHT_RELA);
  checkSection(".rela.data", SHT_RELA);
  checkSection(".rela.bss", SHT_RELA);
  checkSection(".strtab", SHT_STRTAB);
  checkSection(".symtab", SHT_SYMTAB);
  checkSection(".shstrtab", SHT_STRTAB);
}

void RRisc32Reader::checkSection(const std::string &name, Elf_Word type,
                                 Elf_Xword flags) {
  auto sec = getSection(name);
  if (!sec)
    return;
  if (sec->get_type() != type)
    THROW(ELFError, "unexpected section type", name,
          dump::str_section_type(type));
  if (sec->get_flags() != flags)
    THROW(ELFError, "unexpected section flags", name,
          toHexStr(sec->get_flags()));
}

void RRisc32Reader::checkSymbols() {
  forEachSymbol([](const Symbol &sym) {
    if (ELF_ST_VISIBILITY(sym.other) != STV_DEFAULT)
      THROW(ELFError, "unexpected symbol visibility", sym.name,
            toHexStr(sym.other));
  });
}

void RRisc32Reader::checkRelocations() {
  forEachRelocation([](const Relocation &rel) {
    switch (rel.type) {
    case R_RRISC32_NONE:
    case R_RRISC32_32:
    case R_RRISC32_HI20:
    case R_RRISC32_LO12_I:
    case R_RRISC32_LO12_S:
      break;
    default:
      THROW(ELFError, "unexpected relocation type", toHexStr(rel.type));
    }
  });
}

Writer::Writer(const std::string &filename, Elf_Half type, Elf_Half machine,
               unsigned char cls, unsigned char enc, unsigned char oa)
    : filename(filename) {
  ei.create(cls, enc);
  ei.set_type(type);
  ei.set_machine(machine);
  ei.set_os_abi(oa);
}

section *Writer::getSection(const std::string &name) {
  for (auto &sec : ei.sections) {
    if (sec->get_name() == name)
      return sec.get();
  }
  return nullptr;
}

section *Writer::addSection(const std::string &name, Elf_Word type,
                            Elf_Xword flags) {
  assert(!getSection(name));

  section *sec = ei.sections.add(name);
  sec->set_type(type);
  sec->set_entry_size(ei.get_default_entry_size(type));
  sec->set_flags(flags);
  return sec;
}

segment *Writer::addSegment(Elf_Word type, Elf_Word flags,
                            std::vector<section *> secs) {
  segment *seg = ei.segments.add();
  seg->set_type(type);
  seg->set_flags(flags);
  for (section *sec : secs)
    seg->add_section(sec, sec->get_addr_align());
  if (!secs.empty()) {
    seg->set_virtual_address(secs[0]->get_address());
    seg->set_physical_address(secs[0]->get_address());
  }
  return seg;
}

void Writer::save() {
  for (auto &sec : ei.sections) {
    if (sec->get_type() != SHT_SYMTAB)
      continue;

    Elf_Xword n = symbol_section_accessor(ei, sec.get()).get_symbols_num();
    Elf_Xword j = 0;
    for (Elf_Xword i = 0; i < n; ++i) {
      Symbol sym;
      getSymbol(ei, sec.get(), i, sym);
      if (sym.bind == STB_LOCAL)
        j = i;
    }

    // One greater than the symbol table index of the last local symbol
    sec->set_info(j + 1);
  }

  if (!ei.save(filename))
    THROW(ELFError, "save", escape(filename));
}

section *RRisc32Writer::getSection(const std::string &name) {
  section *sec = Writer::getSection(name);
  if (sec)
    return sec;

  if (name == ".text")
    return addSection(name, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
  if (name == ".rodata")
    return addSection(name, SHT_PROGBITS, SHF_ALLOC);
  if (name == ".data")
    return addSection(name, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
  if (name == ".bss")
    return addSection(name, SHT_NOBITS, SHF_ALLOC | SHF_WRITE);

  if (name == ".rela.text" || name == ".rela.rodata" || name == ".rela.data" ||
      name == ".rela.bss") {
    sec = addSection(name, SHT_RELA, 0);
    if (ei.get_type() == ET_REL) {
      sec->set_info(getSection(substr(name, 5))->get_index());
      sec->set_flags(sec->get_flags() & SHF_INFO_LINK);
    }
    sec->set_link(getSection(".symtab")->get_index());
    return sec;
  }

  if (name == ".symtab") {
    sec = addSection(name, SHT_SYMTAB, 0);
    sec->set_link(getSection(".strtab")->get_index());
    return sec;
  }
  if (name == ".strtab")
    return addSection(name, SHT_STRTAB, 0);
  UNEXPECTED_SECTION_NAME(name);
}

Elf_Word RRisc32Writer::addString(const std::string &s) {
  if (!stringCache.contains(s))
    stringCache[s] =
        string_section_accessor(getSection(".strtab")).add_string(s);
  return stringCache[s];
}

Elf_Word RRisc32Writer::addSymbol(const Symbol *sym) {
  if (!symbolCache.contains(sym))
    symbolCache[sym] =
        symbol_section_accessor(ei, getSection(".symtab"))
            .add_symbol(addString(sym->name), sym->value, sym->size, sym->bind,
                        sym->type, sym->other, sym->sec);
  return symbolCache[sym];
}

void RRisc32Writer::addRelocation(const Relocation &rel) {
  relocation_section_accessor(ei, ei.sections[rel.secBelongTo])
      .add_entry(rel.offset, rel.sym, rel.type, rel.addend);
}

} // namespace elf
