#include "rrisc32.h"

#include <map>

namespace rrisc32 {

// clang-format off
const std::string reg2Name[] = {
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
// clang-format on

std::map<std::string, u32> name2Reg;

void initRegisterInfo() {
  for (u32 i = 0; i < 31; ++i) {
    name2Reg["x" + toString(i)] = i;
    name2Reg[reg2Name[i]] = i;
    if (reg2Name[i] == "s0")
      name2Reg["fp"] = i;
  }
}

u32 getReg(const std::string &name) {
  if (name2Reg.count(name))
    return name2Reg[name];
  UNKNOWN_REGISTER();
}

// Except for the 5-bit immediates used in CSR instructions, immediates are
// always sign-extended.

const InstrDesc instrDescs[] = {
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x00, "add",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) + m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x20, "sub",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) - m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x4, 0x00, "xor",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) ^ m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x6, 0x00, "or",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) | m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x7, 0x00, "and",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) & m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x1, 0x00, "sll",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) << m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x00, "srl",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) >> m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x20, "sra",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) >> m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x2, 0x00, "slt",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) < m.rr<s32>(rs2) ? 1 : 0);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x3, 0x00, "sltu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) < m.rr(rs2) ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x0, 0x00, "addi",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) + imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x4, 0x00, "xori",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) ^ imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x6, 0x00, "ori",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) | imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x7, 0x00, "andi",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) & imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x1, 0x00, "slli",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) << imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x5, 0x00, "srli",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) >> imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x5, 0x00, "srai",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) >> imm);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x2, 0x00, "slti",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) < static_cast<s32>(imm) ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0010011, 0x3, 0x00, "sltiu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) < imm ? 1 : 0);
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x0, 0x00, "lb",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rm<s8>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x1, 0x00, "lh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rm<s16>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x2, 0x00, "lw",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rm(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x4, 0x00, "lbu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rm<u8>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::I, 0b0000011, 0x5, 0x00, "lhu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rm<u16>(m.rr(rs1) + imm));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x0, 0x00, "sb",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wm<u8>(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x1, 0x00, "sh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wm<u16>(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::S, 0b0100011, 0x2, 0x00, "sw",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wm(m.rr(rs1) + imm, m.rr(rs2));
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x0, 0x00, "beq",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr(rs1) == m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x1, 0x00, "bne",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr(rs1) != m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x4, 0x00, "blt",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr<s32>(rs1) < m.rr<s32>(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x5, 0x00, "bge",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr<s32>(rs1) >= m.rr<s32>(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x6, 0x00, "bltu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr(rs1) < m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::B, 0b1100011, 0x7, 0x00, "bgeu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (m.rr(rs1) >= m.rr(rs2))
                  m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::J, 0b1101111, 0x0, 0x00, "jal",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.ri());
                m.wi(m.ri() - 4 + imm);
              }),
    InstrDesc(InstrType::I, 0b1100111, 0x0, 0x00, "jalr",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.ri());
                m.wi(m.rr(rs1) + imm);
              }),
    InstrDesc(InstrType::U, 0b0110111, 0x0, 0x00, "lui",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, imm << 12);
              }),
    InstrDesc(InstrType::U, 0b0010111, 0x0, 0x00, "auipc",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.ri() - 4 + (imm << 12));
              }),
    InstrDesc(InstrType::I, 0b1110011, 0x0, 0x00, "ecb",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                if (imm == 0)
                  m.ecall();
                else
                  m.ebreak();
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x0, 0x01, "mul",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) * m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x1, 0x01, "mulh",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, (m.rr<s64>(rs1) * m.rr<s64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x2, 0x01, "mulhsu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, (m.rr<s64>(rs1) * m.rr<u64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x3, 0x01, "mulhu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, (m.rr<u64>(rs1) * m.rr<u64>(rs2) >> 32) & 0xffffffff);
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x4, 0x01, "div",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) / m.rr<s32>(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x5, 0x01, "divu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) / m.rr(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x6, 0x01, "rem",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr<s32>(rs1) % m.rr<s32>(rs2));
              }),
    InstrDesc(InstrType::R, 0b0110011, 0x7, 0x01, "remu",
              [](Machine &m, u32 rd, u32 rs1, u32 rs2, u32 imm) {
                m.wr(rd, m.rr(rs1) % m.rr(rs2));
              })};

u32 encodeInstr(const Instr &inst) {
  u32 b = 0;

  const InstrDesc *desc = inst.desc;
  u32 opcode = desc->opcode, funct3 = desc->funct3, funct7 = desc->funct7;
  u32 rd = 0, rs1 = 0, rs2 = 0, imm = inst.imm;

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
    if (desc->type == InstrType::B)
      imm = (imm >> 10 & 1) | ((imm & 0x3ff) << 1) | (imm >> 11 << 11);
    b |= (imm & 0b11111) << 7;
    b |= imm >> 5 << 25;
    b |= rs1 << 15;
    b |= rs2 << 20;
    break;
  case InstrType::U:
  case InstrType::J:
    b |= rd << 7;
    if (desc->type == InstrType::J)
      imm = (imm >> 11 & 0xff) | ((imm >> 10 & 1) << 8) | ((imm & 0x3ff) << 9) |
            (imm >> 19 << 19);
    b |= imm << 12;
    break;
  }
  b |= desc->opcode;
  return b;
}

void decodeInstr(u32 b, Instr &inst) {
  u32 opcode = b & 0x7f;
  InstrType type = InstrType::Invalid;
  for (const InstrDesc &desc : instrDescs)
    if (desc.opcode == opcode)
      type = desc.type;

  u32 funt3 = 0, funt7 = 0;
  u32 rd = 0, rs1 = 0, rs2 = 0, imm = 0;
  switch (type) {
  case InstrType::R:
    funt3 = b >> 12 & 0b111;
    funt7 = b >> 25 & 0x7f;
    rd = b >> 7 & 0b11111;
    rs1 = b >> 15 & 0b11111;
    rs2 = b >> 20 & 0b11111;
    break;
  case InstrType::I:
    funt3 = b >> 12 & 0b111;
    imm = b >> 20 & 0xfff;
    rd = b >> 7 & 0b11111;
    rs1 = b >> 15 & 0b11111;
    break;
  case InstrType::S:
  case InstrType::B:
    funt3 = b >> 12 & 0b111;
    imm = (b >> 7 & 0b11111) | ((b >> 25 & 0x7f) << 5);
    rs1 = b >> 15 & 0b11111;
    rs2 = b >> 20 & 0b11111;
    if (type == InstrType::B)
      imm = (imm >> 1 & 0x3ff) | ((imm & 1) << 10) | ((imm >> 11 & 1) << 11);
    break;
  case InstrType::U:
  case InstrType::J:
    rd = b >> 7 & 0b11111;
    imm = b >> 12;
    if (type == InstrType::J)
      imm = (imm >> 9 & 0x3ff) | ((imm >> 8 & 1) << 10) | ((imm & 0xff) << 11) |
            ((imm >> 19 & 1) << 19);
    break;

  default:
    UNKNOWN_INSTRUCTION();
  }

  for (const InstrDesc &desc : instrDescs) {
    if (desc.opcode == opcode && desc.funct3 == funt3 && desc.funct7 == funt7) {
      inst.desc = &desc;
      inst.rd = rd;
      inst.rs1 = rs1;
      inst.rs2 = rs2;
      inst.imm = imm;
      return;
    }
  }
  UNKNOWN_INSTRUCTION();
}

std::map<std::string, const InstrDesc *> name2InstrDesc;

void initInstructionInfo() {
  for (auto &desc : instrDescs) {
    assert(name2InstrDesc.count(desc.name) == 0);
    name2InstrDesc[desc.name] = &desc;
  }
}

RRisc32Init::RRisc32Init() {
  initRegisterInfo();
  initInstructionInfo();
}

static RRisc32Init rrisc32Init;

} // namespace rrisc32
