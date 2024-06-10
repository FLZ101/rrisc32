#include "elf.h"

namespace elf {

Reader::Reader(const std::string &filename) : filename(filename) {
  if (!ei.load(filename))
    THROW(ELFError, "load", filename);

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

void Reader::getSymbol(section *sec, Elf_Xword idx, Symbol &sym) {
  bool ok = symbol_section_accessor(ei, sec).get_symbol(
      idx, sym.name, sym.value, sym.size, sym.bind, sym.type, sym.sec,
      sym.other);
  if (!ok)
    THROW(ELFError, "get symbol", sec->get_name(), idx);
  sym.idx = idx;
  sym.secBelongTo = sec->get_index();
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
  const symbol_section_accessor &ssa = symbol_section_accessor(ei, sec);
  for (Elf_Xword i = 0; i < ssa.get_symbols_num(); ++i) {
    Symbol sym;
    bool ok = ssa.get_symbol(i, sym.name, sym.value, sym.size, sym.bind,
                             sym.type, sym.sec, sym.other);
    if (!ok)
      THROW(ELFError, "get symbol", sec->get_name(), i);
    sym.idx = i;
    sym.secBelongTo = sec->get_index();
    fn(sym);
  }
}

void Reader::getRelocation(section *sec, Elf_Xword idx, Relocation &rel) {
  bool ok = relocation_section_accessor(ei, sec).get_entry(
      idx, rel.offset, rel.sym, rel.type, rel.addend);
  if (!ok)
    THROW(ELFError, "get relocation", sec->get_name(), idx);
  rel.idx = idx;
  rel.secBelongTo = sec->get_index();
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
  const relocation_section_accessor &rsa = relocation_section_accessor(ei, sec);
  for (Elf_Xword i = 0; i < rsa.get_entries_num(); ++i) {
    Relocation rel;
    bool ok = rsa.get_entry(i, rel.offset, rel.sym, rel.type, rel.addend);
    if (!ok)
      THROW(ELFError, "get relocation", sec->get_name(), i);
    rel.idx = i;
    rel.secBelongTo = sec->get_index();
    fn(rel);
  }
}

void Reader::dump(std::ostream &os) {
  dumpELFHeader(os);
  dumpSegments(os);
  dumpSections(os);
  dumpSymbols(os);
  dumpRelocations(os);
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
     << str_os_abi(ei.get_os_abi()) << "\n";
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
      os << "Idx\tAddr\tOff\tSize\tLink\tInfo\tAddrAlign\tEntSize\tFlags\t"
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
  forEachSymbol([&](Symbol &sym) {
    switch (sym.type) {
    case STT_OBJECT:
    case STT_FUNC:
    case STT_SECTION:
      break;
    default:
      return;
    }
    if (!headerDumped) {
      os << "[ Symbols ]\n";
      os << "SecBT\tIdx\tValue\tSize\tBind\tType\tVis\tSec\tName\n";
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
  case R_RRISC32_CALL:
    return "CALL";
  case R_RRISC32_HI20:
    return "HI20";
  case R_RRISC32_LO12_I:
    return "LO12_I";
  case R_RRISC32_LO12_S:
    return "LO12_S";
  default:
    return toHexStr(type);
  }
}

void Reader::dumpRelocations(std::ostream &os) {
  bool headerDumped = false;
  forEachRelocation([&](Relocation &rel) {
    if (!headerDumped) {
      os << "[ Relocations ]\n";
      os << "SecBT\tIdx\tOffset\tType\tAddend\tSym\tSecSym\tSecRel\n";
      headerDumped = true;
    }
    // clang-format off
    os << toHexStr(rel.secBelongTo) << "\t"
       << toHexStr(rel.idx) << "\t"
       << toHexStr(rel.offset) << "\t"
       << str_relocation_type(rel.type) << "\t"
       << toHexStr(rel.addend) << "\t"
       << toHexStr(rel.sym) << "\t"
       << toHexStr(ei.sections[rel.secBelongTo]->get_link()) << "\t"
       << toHexStr(ei.sections[rel.secBelongTo]->get_info()) << "\n";
    // clang-format on
  });
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
}

void RRisc32Reader::checkSections() {
  forEachSection([this](section &sec) {
    std::string name = sec.get_name();
    if (!checkSectionName(name))
      THROW(ELFError, "unexpected section name", name);
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
  assert(checkSectionName(name));
  auto sec = getSection(name);
  if (!sec)
    return;
  if (sec->get_type() != type)
    THROW(ELFError, "unexpected section flags", name,
          dump::str_section_type(type));
  if (sec->get_flags() != flags)
    THROW(ELFError, "unexpected section flags", name,
          toHexStr(sec->get_flags()));
}

void RRisc32Reader::checkSymbols() {
  forEachSymbol([](const Symbol &sym) {
    if (ELF_ST_VISIBILITY(sym.other) == STV_DEFAULT)
      THROW(ELFError, "unexpected symbol visibility", sym.name,
            toHexStr(sym.other));
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

section *RRisc32Writer::addSection(const std::string &name) {
  if (name == ".text")
    return Writer::addSection(name, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
  else if (name == ".rodata")
    return Writer::addSection(name, SHT_PROGBITS, SHF_ALLOC);
  else if (name == ".data")
    return Writer::addSection(name, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
  else if (name == ".bss")
    return Writer::addSection(name, SHT_NOBITS, SHF_ALLOC | SHF_WRITE);
  else if (name == ".rela.text" || name == ".rela.rodata" ||
           name == ".rela.data" || name == ".rela.bss")
    return Writer::addSection(name, SHT_RELA, 0);
  else
    THROW(ELFError, "addSection", "unexpected section name", name);
}

} // namespace elf
