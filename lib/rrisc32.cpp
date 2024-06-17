#include "rrisc32.h"

#include <map>

namespace rrisc32 {

u32 Machine::ri() { return ip; }

void Machine::wi(u32 value) { ip = value; }

template <typename T> T Machine::rr(unsigned i) {
  assert(i < 32);
  return static_cast<T>(reg[i]);
}

void Machine::wr(unsigned i, s32 value) {
  assert(i < 32);
  reg[i] = value;
}

template <typename T> T Machine::rm(u32 addr) {
  if (addr + sizeof(T) >= sz)
    SEGMENT_FAULT(toHexStr(addr, true, false));
  return *reinterpret_cast<T *>(mem + addr);
}

template <typename T> void Machine::wm(u32 addr, T value) {
  if (addr + sizeof(T) >= sz)
    SEGMENT_FAULT(toHexStr(addr, true, false));
  *reinterpret_cast<T *>(mem + addr) = value;
}

void Machine::ecall() {}

void Machine::ebreak() {}

// clang-format off
const std::string reg2NameX[] = {
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};

const std::string reg2Name[] = {
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
// clang-format on

std::map<std::string, u32> name2Reg;

void initRegisterInfo() {
  for (u32 i = 0; i < 31; ++i) {
    name2Reg[reg2NameX[i]] = i;
    name2Reg[reg2Name[i]] = i;
    if (reg2Name[i] == "s0")
      name2Reg["fp"] = i;
  }
}

const std::string &getRegName(u32 i, bool x = true) {
  if (i < 32)
    return x ? reg2NameX[i] : reg2Name[i];
  UNKNOWN_REGISTER(i);
}

u32 getReg(const std::string &name) {
  if (name2Reg.count(name))
    return name2Reg[name];
  UNKNOWN_REGISTER(name);
}

// Except for the 5-bit immediates used in CSR instructions, immediates are
// always sign-extended.

const InstrDesc instrDescs[] = {
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x00, "add",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) + m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x20, "sub",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) - m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x4, 0x00, "xor",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) ^ m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x6, 0x00, "or",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) | m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x7, 0x00, "and",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) & m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x1, 0x00, "sll",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) << m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x00, "srl",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr<u32>(rs1) >> m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x20, "sra",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) >> m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x2, 0x00, "slt",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) < m.rr(rs2) ? 1 : 0);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x3, 0x00, "sltu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr<u32>(rs1) < m.rr<u32>(rs2) ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x0, 0x00, "addi",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) + imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x4, 0x00, "xori",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) ^ imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x6, 0x00, "ori",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) | imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x7, 0x00, "andi",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) & imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x1, 0x00, "slli",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                imm &= 0b11111;
                m.wr(rd, m.rr(rs1) << imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x5, 0x00, "srli",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                imm &= 0b11111;
                m.wr(rd, m.rr<u32>(rs1) >> imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x5, 0x00, "srai",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                imm &= 0b11111;
                m.wr(rd, m.rr(rs1) >> imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x2, 0x00, "slti",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) < imm ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x3, 0x00, "sltiu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr<u32>(rs1) < static_cast<u32>(imm) ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x0, 0x00, "lb",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rm<s8>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x1, 0x00, "lh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rm<s16>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x2, 0x00, "lw",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rm<s32>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x4, 0x00, "lbu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rm<u8>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x5, 0x00, "lhu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rm<u16>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x0, 0x00, "sb",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wm<s8>(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x1, 0x00, "sh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wm<s16>(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x2, 0x00, "sw",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wm<s32>(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x0, 0x00, "beq",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr(rs1) == m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x1, 0x00, "bne",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr(rs1) != m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x4, 0x00, "blt",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr(rs1) < m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x5, 0x00, "bge",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr(rs1) >= m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x6, 0x00, "bltu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr<u32>(rs1) < m.rr<u32>(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x7, 0x00, "bgeu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                if (m.rr<u32>(rs1) >= m.rr<u32>(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::J, 0b1101111, 0x0, 0x00, "jal",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.ri());
                m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::I, 0b1100111, 0x0, 0x00, "jalr",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.ri());
                m.wi((m.rr(rs1) + imm) & static_cast<s32>(-2));
              }),
    InstrDesc(InstrType::U, 0b0110111, 0x0, 0x00, "lui",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, imm << 12);
              }),
    InstrDesc(InstrType::U, 0b0010111, 0x0, 0x00, "auipc",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.ri() - 4 + (imm << 12));
              }),
    // clang-format off
    InstrDesc(InstrType::I, 0b1110011, 0x0, 0x00, "ecall",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) { m.ecall(); }),
    InstrDesc(InstrType::I, 0b1110011, 0x0, 0x00, "ebreak",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) { m.ebreak(); }),
    // clang-format on
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x01, "mul",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) * m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x1, 0x01, "mulh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, (m.rr<s64>(rs1) * m.rr<s64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x2, 0x01, "mulhsu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, (m.rr<s64>(rs1) * m.rr<u64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x3, 0x01, "mulhu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, (m.rr<u64>(rs1) * m.rr<u64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x4, 0x01, "div",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) / m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x01, "divu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr<u32>(rs1) / m.rr<u32>(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x6, 0x01, "rem",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr(rs1) % m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x7, 0x01, "remu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, s32 imm) {
                m.wr(rd, m.rr<u32>(rs1) % m.rr<u32>(rs2));
              })};

std::map<std::string, const InstrDesc *> name2InstrDesc;

void initInstructionInfo() {
  for (auto &desc : instrDescs) {
    assert(!name2InstrDesc.contains(desc.name));
    name2InstrDesc[desc.name] = &desc;
  }
}

const InstrDesc *getInstrDesc(const std::string &name) {
  if (name2InstrDesc.contains(name))
    return name2InstrDesc[name];
  UNKNOWN_INSTRUCTION(name);
}

std::ostream &operator<<(std::ostream &os, const Instr &instr) {
  os << instr.getName();
  bool first = true;
  for (const auto &operand : instr.getOperands()) {
    if (first) {
      os << " ";
      first = false;
    } else {
      os << ", ";
    }

    if (std::holds_alternative<s32>(operand))
      os << std::get<s32>(operand);
    else
      os << std::get<std::string>(operand);
  }
  return os;
}

u32 encode(const Instr &instr) {
  std::string format;
  for (const auto &operand : instr.operands)
    format.push_back(std::get_if<std::string>(&operand) ? 'r' : 'i');

  u32 rd = 0, rs1 = 0, rs2 = 0;
  s32 imm = 0;

#define _REG(i) (getReg(std::get<std::string>(instr.operands[i])))
#define _IMM(i) (std::get<s32>(instr.operands[i]))

  const std::string &name = instr.name;
  const InstrDesc *desc = getInstrDesc(name);
  switch (desc->type) {
  case InstrType::R:
    if (format != "rrr")
      UNKNOWN_INSTRUCTION(name, format);
    rd = _REG(0);
    rs1 = _REG(1);
    rs2 = _REG(2);
    break;
  case InstrType::I:
    if (isOneOf(name, {"ecall", "ebreak"})) {
      if (format != "")
        UNKNOWN_INSTRUCTION(name, format);
      rd = Reg::x0;
      rs1 = Reg::x0;
      imm = name == "ecall" ? 0 : 1;
      break;
    }

    if (format != "rri")
      UNKNOWN_INSTRUCTION(name, format);
    rd = _REG(0);
    rs1 = _REG(1);
    imm = _IMM(2);
    if (isOneOf(name, {"slli", "srli", "srai"})) {
      imm &= 0b11111;
      if (name == "srai")
        imm |= 0x20 << 5;
    }
    break;
  case InstrType::S:
  case InstrType::B:
    if (format != "rri")
      UNKNOWN_INSTRUCTION(name, format);
    rs1 = _REG(0);
    rs2 = _REG(1);
    imm = _IMM(2);
    break;
  case InstrType::U:
  case InstrType::J:
    if (format != "ri")
      UNKNOWN_INSTRUCTION(name, format);
    rd = _REG(0);
    imm = _IMM(1);
    break;
  default:
    UNREACHABLE();
  }

#undef _IMM
#undef _REG

  u32 opcode = desc->opcode, funct3 = desc->funct3, funct7 = desc->funct7;
  u32 b = 0;
  switch (desc->type) {
  case InstrType::R:
    b |= funct3 << 12;
    b |= funct7 << 25;
    b |= rd << 7;
    b |= rs1 << 15;
    b |= rs2 << 20;
    break;
  case InstrType::I:
    b |= funct3 << 12;
    b |= imm << 20;
    b |= rd << 7;
    b |= rs1 << 15;
    break;
  case InstrType::S:
  case InstrType::B:
    b |= funct3 << 12;
    if (desc->type == InstrType::B) {
      imm >>= 1;
      imm = (imm >> 10 & 1) | ((imm & 0x3ff) << 1) | (imm >> 11 << 11);
    }
    b |= (imm & 0b11111) << 7;
    b |= imm >> 5 << 25;
    b |= rs1 << 15;
    b |= rs2 << 20;
    break;
  case InstrType::U:
  case InstrType::J:
    b |= rd << 7;
    if (desc->type == InstrType::J) {
      imm >>= 1;
      imm = (imm >> 11 & 0xff) | ((imm >> 10 & 1) << 8) | ((imm & 0x3ff) << 9) |
            (imm >> 19 << 19);
    }
    b |= imm << 12;
    break;
  default:
    UNREACHABLE();
  }
  b |= desc->opcode;
  return b;
}

u32 encode(const std::string &name,
           const std::vector<Instr::Operand> &operands) {
  return encode(Instr(name, operands));
}

void decode(u32 b, Instr &instr, const InstrDesc *&id) {
  u32 opcode = b & 0x7f;
  InstrType type = InstrType::Invalid;
  for (const InstrDesc &desc : instrDescs)
    if (desc.opcode == opcode) {
      type = desc.type;
      break;
    }

  u32 funct3 = 0, funct7 = 0;
  u32 rd = 0, rs1 = 0, rs2 = 0;
  s32 imm = 0;
  switch (type) {
  case InstrType::R:
    funct3 = b >> 12 & 0b111;
    funct7 = b >> 25 & 0x7f;
    rd = b >> 7 & 0b11111;
    rs1 = b >> 15 & 0b11111;
    rs2 = b >> 20 & 0b11111;
    break;
  case InstrType::I:
    funct3 = b >> 12 & 0b111;
    imm = b >> 20 & 0xfff;
    imm = signExt<s32, 12>(imm);
    rd = b >> 7 & 0b11111;
    rs1 = b >> 15 & 0b11111;
    break;
  case InstrType::S:
  case InstrType::B:
    funct3 = b >> 12 & 0b111;
    imm = (b >> 7 & 0b11111) | ((b >> 25 & 0x7f) << 5);
    if (type == InstrType::S) {
      imm = signExt<s32, 12>(imm);
    } else {
      imm = (imm >> 1 & 0x3ff) | ((imm & 1) << 10) | ((imm >> 11 & 1) << 11);
      imm = signExt<s32, 12>(imm);
      imm <<= 1;
    }
    rs1 = b >> 15 & 0b11111;
    rs2 = b >> 20 & 0b11111;
    break;
  case InstrType::U:
  case InstrType::J:
    rd = b >> 7 & 0b11111;
    imm = b >> 12;
    if (type == InstrType::U) {
      imm = signExt<s32, 20>(imm);
    } else {
      imm = (imm >> 9 & 0x3ff) | ((imm >> 8 & 1) << 10) | ((imm & 0xff) << 11) |
            ((imm >> 19 & 1) << 19);
      imm = signExt<s32, 20>(imm);
      imm <<= 1;
    }
    break;

  default:
    ILLEGAL_INSTRUCTION(toHexStr(b, false, false));
  }

  id = nullptr;
  for (const InstrDesc &desc : instrDescs) {
    if (desc.opcode != opcode || desc.funct3 != funct3 || desc.funct7 != funct7)
      continue;

    auto check = [&desc, &imm]() {
      switch (desc.opcode) {
      case 0b0010011: {
        u32 funct7 = (imm >> 5) & 0x7f;
        if (desc.name == "slli")
          return funct7 == 0;
        if (desc.name == "srli")
          return funct7 == 0;
        if (desc.name == "srai") {
          if (funct7 == 0x20) {
            imm &= 0b11111;
            return true;
          }
          return false;
        }
        return true;
      }
      case 0b1110011:
        if (desc.name == "ecall")
          return imm == 0;
        if (desc.name == "ebreak")
          return imm == 1;
        return false;
      default:
        return true;
      }
    };

    if (check()) {
      id = &desc;
      break;
    }
  }
  if (!id)
    ILLEGAL_INSTRUCTION(toHexStr(b, false, false));

  instr = Instr(id->name);
  if (isOneOf(id->name, {"ecall", "ebreak"}))
    return;

  switch (id->type) {
  case InstrType::R:
    instr.addOperand(getRegName(rd));
    instr.addOperand(getRegName(rs1));
    instr.addOperand(getRegName(rs2));
    break;
  case InstrType::I:
    instr.addOperand(getRegName(rd));
    instr.addOperand(getRegName(rs1));
    instr.addOperand(imm);
    break;
  case InstrType::S:
  case InstrType::B:
    instr.addOperand(getRegName(rs1));
    instr.addOperand(getRegName(rs2));
    instr.addOperand(imm);
    break;
  case InstrType::U:
  case InstrType::J:
    instr.addOperand(getRegName(rd));
    instr.addOperand(imm);
    break;
  default:
    UNREACHABLE();
  }
}

void decode(u32 b, Instr &instr) {
  const InstrDesc *desc;
  decode(b, instr, desc);
}

namespace test {
u32 encode(const std::string &name, const std::string &operands) {
  Instr instr(name);
  for (const std::string &s : split(operands, ",")) {
    if ('a' <= s[0] && s[0] <= 'z')
      instr.addOperand(s);
    else
      instr.addOperand(parseInt(s));
  }
  return encode(instr);
}

u32 encode(std::string s) {
  s = trim(s);
  size_t i = find(s, " ");
  std::string name = substr(s, 0, i);
  std::string operands = substr(s, i);
  return encode(name, operands);
}

std::string decode(u32 b) {
  Instr instr;
  decode(b, instr);
  return toString(instr);
}
} // namespace test

struct RRisc32Init {
  RRisc32Init() {
    initRegisterInfo();
    initInstructionInfo();
  }
};

static RRisc32Init init;

} // namespace rrisc32
