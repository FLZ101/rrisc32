#ifndef RRISC32_H
#define RRISC32_H

#include "util.h"

#include <cassert>
#include <functional>
#include <list>
#include <regex>
#include <utility>
#include <vector>

namespace rrisc32 {

DEFINE_EXCEPTION(IllegalInstruction)
DEFINE_EXCEPTION(DividedByZero)
DEFINE_EXCEPTION(SegmentFault)
DEFINE_EXCEPTION(UnknownInstructionFormat)
DEFINE_EXCEPTION(UnknownRegister)

u32 getReg(std::string name);

enum class InstrType { Invalid, R, I, S, B, U, J };

class Machine;

struct InstrDesc {

  using executeFn = std::function<void(Machine &, u32, u32, u32, u32)>;

  InstrDesc(InstrType type, u32 opcode, u32 funct3, u32 funct7,
            std::string name, executeFn execute)
      : type(type), opcode(opcode), funct3(funct3), funct7(funct7), name(name),
        execute(execute) {}

  const InstrType type;
  const u32 opcode, funct3, funct7;

  const std::string name;

  const executeFn execute;
};

// Except for the 5-bit immediates used in CSR instructions, immediates are
// always sign-extended.
struct Instr {
  const InstrDesc *desc = nullptr;
  u32 rd = 0, rs1 = 0, rs2 = 0, imm = 0;
};

u32 encodeInstr(const Instr &inst);

void decodeInstr(u32 b, Instr &inst);

struct AsmInstr {
  AsmInstr() {}
  AsmInstr(const std::string &name) : name(name) {}
  AsmInstr(const std::string &name, const std::vector<u32> &operands,
           const std::string &format)
      : name(name), operands(operands), format(format) {
    assert(operands.size() == format.size());
  }

  AsmInstr &operator=(const AsmInstr &other) {
    name = other.name;
    operands = other.operands;
    format = other.format;
    return *this;
  }

  std::string getName() const { return name; }

  u32 getOperand(size_t i) const {
    assert(i < operands.size());
    return operands[i];
  }

  std::string getFormat() const { return format; }

private:
  std::string name;
  std::vector<u32> operands;
  std::string format;
};

struct PseudoInstrDesc {
  using expandFn =
      std::function<bool(const AsmInstr &ai, std::list<Instr> &li)>;
  using combineFn =
      std::function<bool(const std::list<Instr> &li,
                         std::list<Instr>::const_iterator &it, AsmInstr &ai)>;

  std::regex namePattern;
  expandFn expand;
  combineFn combine;
};

void assembleInstr(const AsmInstr &ai, std::list<Instr> &li);

void disassembleInstr(const std::list<Instr> &li,
                      std::list<Instr>::const_iterator &it, AsmInstr &ai);

class Machine {
public:
  Machine(size_t sz = 32 * 1024 * 1024) : sz(sz) { mem = new u8[sz]; }

  Machine(const Machine &) = delete;
  Machine &operator=(const Machine &) = delete;

  ~Machine() { delete mem; }

  u32 ri() { return ip; }
  void wi(u32 value) { ip = value; }

  template <typename T = u32> T rr(unsigned i) {
    assert(i < 32);
    return static_cast<T>(reg[i]);
  }
  void wr(unsigned i, u32 value) {
    assert(i < 32);
    reg[i] = value;
  }

  template <typename T = u32> T rm(u32 addr) {
    if (addr + sizeof(T) >= sz)
      THROW(SegmentFault);
    return *reinterpret_cast<T *>(mem + addr);
  }
  template <typename T = u32> void wm(u32 addr, T value) {
    if (addr + sizeof(T) >= sz)
      THROW(SegmentFault);
    *reinterpret_cast<T *>(mem + addr) = value;
  }

  void ecall() {}

  void ebreak() {}

private:
  u32 ip = 0;
  u32 reg[32] = {};
  u8 *mem = nullptr;
  size_t sz = 0;
};

void initRRisc32();

} // namespace rrisc32
#endif
