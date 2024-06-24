#ifndef RRISC32_H
#define RRISC32_H

#include "util.h"

#include <functional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace rrisc32 {

DEFINE_EXCEPTION(RRisc32Err)

#define UNKNOWN_INSTRUCTION(...)                                               \
  THROW(RRisc32Err, "unknown instruction", ##__VA_ARGS__)

#define UNKNOWN_REGISTER(...)                                                  \
  THROW(RRisc32Err, "unknown register", ##__VA_ARGS__)

#define ILLEGAL_INSTRUCTION(...)                                               \
  THROW(RRisc32Err, "illegal instruction", ##__VA_ARGS__)

#define DIVIDED_BY_ZERO(...) THROW(RRisc32Err, "divided by zero", ##__VA_ARGS__)

#define BUS_ERROR(...) THROW(RRisc32Err, "bus error", ##__VA_ARGS__)

class Machine {
public:
  Machine(size_t memSize) : memSize(memSize) { mem = new s8[memSize]{}; }

  Machine(const Machine &) = delete;
  Machine &operator=(const Machine &) = delete;

  virtual ~Machine() { delete mem; }

  u32 fetch() {
    ip += 4;
    return rm(ip - 4);
  }

  u32 ri() { return ip; }

  void wi(u32 value) { ip = value; }

  template <typename T = s32> T rr(unsigned i) {
    assert(i < 32);
    return static_cast<T>(reg[i]);
  }

  void wr(unsigned i, s32 value) {
    assert(i < 32);
    reg[i] = value;
  }

  template <typename T = s32> T rm(u32 addr) {
    befRm(addr, sizeof(T));

    if (addr + sizeof(T) >= memSize)
      BUS_ERROR(toHexStr(addr, true, false));
    return *reinterpret_cast<T *>(mem + addr);
  }

  template <typename T = s32> void wm(u32 addr, T value) {
    befWm(addr, sizeof(T));

    if (addr + sizeof(T) >= memSize)
      BUS_ERROR(toHexStr(addr, true, false));
    *reinterpret_cast<T *>(mem + addr) = value;

    aftWm(addr, sizeof(T));
  }

  virtual void befRm(u32 addr, unsigned n) {}
  virtual void befWm(u32 addr, unsigned n) {}
  virtual void aftWm(u32 addr, unsigned n) {}

  virtual void ecall() {}
  virtual void ebreak() {}

protected:
  u32 ip = 0;
  s32 reg[32]{};
  s8 *mem = nullptr;
  size_t memSize = 0;
};

namespace Reg {

const u32 x0 = 0;
const u32 x1 = 1;
const u32 x2 = 2;
const u32 x3 = 3;
const u32 x4 = 4;
const u32 x5 = 5;
const u32 x6 = 6;
const u32 x7 = 7;
const u32 x8 = 8;
const u32 x9 = 9;
const u32 x10 = 10;
const u32 x11 = 11;
const u32 x12 = 12;
const u32 x13 = 13;
const u32 x14 = 14;
const u32 x15 = 15;
const u32 x16 = 16;
const u32 x17 = 17;
const u32 x18 = 18;
const u32 x19 = 19;
const u32 x20 = 20;
const u32 x21 = 21;
const u32 x22 = 22;
const u32 x23 = 23;
const u32 x24 = 24;
const u32 x25 = 25;
const u32 x26 = 26;
const u32 x27 = 27;
const u32 x28 = 28;
const u32 x29 = 29;
const u32 x30 = 30;
const u32 x31 = 31;

const u32 zero = 0;
const u32 ra = 1;
const u32 sp = 2;
const u32 gp = 3;
const u32 tp = 4;
const u32 t0 = 5;
const u32 t1 = 6;
const u32 t2 = 7;
const u32 s0 = 8;
const u32 s1 = 9;
const u32 a0 = 10;
const u32 a1 = 11;
const u32 a2 = 12;
const u32 a3 = 13;
const u32 a4 = 14;
const u32 a5 = 15;
const u32 a6 = 16;
const u32 a7 = 17;
const u32 s2 = 18;
const u32 s3 = 19;
const u32 s4 = 20;
const u32 s5 = 21;
const u32 s6 = 22;
const u32 s7 = 23;
const u32 s8 = 24;
const u32 s9 = 25;
const u32 s10 = 26;
const u32 s11 = 27;
const u32 t3 = 28;
const u32 t4 = 29;
const u32 t5 = 30;
const u32 t6 = 31;

const u32 fp = s0;

} // namespace Reg

enum class InstrType { Invalid, R, I, S, B, U, J };

struct InstrDesc {

  using exeFn = std::function<void(Machine &, u32, u32, u32, s32)>;

  InstrDesc(InstrType type, u32 opcode, u32 funct3, u32 funct7,
            const std::string &name, exeFn exe)
      : type(type), opcode(opcode), funct3(funct3), funct7(funct7), name(name),
        exe(exe) {}

  const InstrType type;
  const u32 opcode, funct3, funct7;

  const std::string name;

  const exeFn exe;
};

class Instr {
  friend u32 encode(Instr &instr);
  friend void decode(u32 b, Instr &instr, const InstrDesc *&id);
  friend void execute(Machine &m, const Instr &instr, const InstrDesc *id);

public:
  using Operand = std::variant<std::string, s32>;

  Instr() {}
  Instr(const std::string &name) : name(name) {}
  Instr(const std::string &name, const std::vector<Operand> &operands)
      : name(name), operands(operands) {}

  void addOperand(const std::string &s) { operands.push_back(s); }
  void addOperand(s32 i) { operands.push_back(i); }

  const std::string &getName() const { return name; }
  const std::vector<Operand> &getOperands() const { return operands; }

private:
  std::string name;
  std::vector<Operand> operands;

  u32 rd = 0, rs1 = 0, rs2 = 0;
  s32 imm = 0;
};

std::ostream &operator<<(std::ostream &os, const Instr &instr);

u32 encode(Instr &instr);
u32 encode(const std::string &name,
           const std::vector<Instr::Operand> &operands);

void decode(u32 b, Instr &instr, const InstrDesc *&id);
void decode(u32 b, Instr &instr);

void execute(Machine &m, const Instr &instr, const InstrDesc *id);
void execute(Machine &m, u32 b);

namespace test {
u32 encode(const std::string &name, const std::string &operands);
u32 encode(std::string s);
std::string decode(u32 b);
} // namespace test

} // namespace rrisc32
#endif
