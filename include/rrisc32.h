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

#define SEGMENT_FAULT(...) THROW(RRisc32Err, "segment fault", ##__VA_ARGS__)

class Machine {
public:
  Machine(size_t sz = 32 * 1024 * 1024) : sz(sz) { mem = new u8[sz]; }

  Machine(const Machine &) = delete;
  Machine &operator=(const Machine &) = delete;

  ~Machine() { delete mem; }

  u32 ri();
  void wi(u32 value);

  template <typename T = u32> T rr(unsigned i);
  void wr(unsigned i, u32 value);

  template <typename T = u32> T rm(u32 addr);
  template <typename T = u32> void wm(u32 addr, T value);

  void ecall();
  void ebreak();

protected:
  u32 ip = 0;
  u32 reg[32] = {};
  u8 *mem = nullptr;
  size_t sz = 0;
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

} // namespace Reg

enum class InstrType { Invalid, R, I, S, B, U, J };

struct InstrDesc {

  using exeFn = std::function<void(Machine &, u32, u32, u32, u32)>;

  InstrDesc(InstrType type, u32 opcode, u32 funct3, u32 funct7,
            const std::string &name, exeFn execute)
      : type(type), opcode(opcode), funct3(funct3), funct7(funct7), name(name),
        exe(exe) {}

  const InstrType type;
  const u32 opcode, funct3, funct7;

  const std::string name;

  const exeFn exe;
};

class Instr {
  friend u32 encode(const Instr &instr);
  friend void decode(u32 b, Instr &instr, const InstrDesc *&id);

public:
  using Operand = std::variant<std::string, u32>;

  Instr(const std::string &name) : name(name) {}
  Instr(const std::string &name, const std::vector<Operand> &operands)
      : name(name), operands(operands) {}

  void addOperand(const std::string &s) { operands.push_back(s); }
  void addOperand(u32 i) { operands.push_back(i); }

  const std::string &getName() const { return name; }
  const std::vector<Operand> &getOperands() const { return operands; }

private:
  std::string name;
  std::vector<Operand> operands;
};

std::ostream &operator<<(std::ostream &os, const Instr &instr);

u32 encode(const Instr &instr);
u32 encode(const std::string &name,
           const std::vector<Instr::Operand> &operands);

void decode(u32 b, Instr &instr, const InstrDesc *&desc);

} // namespace rrisc32
#endif
