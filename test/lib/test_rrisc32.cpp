#include <gtest/gtest.h>

#include "rrisc32.h"

using namespace rrisc32;

namespace {

u32 encode(const std::string &name, const std::string &operands) {
  Instr instr(name);
  for (const std::string &s : split(operands, ",")) {
    if (isalpha(s[0]))
      instr.addOperand(s);
    else
      instr.addOperand(parseInt(s));
  }
  return encode(instr);
}

u32 encode(std::string s) {
  s = trim(s);
  size_t i = s.find(' ');
  std::string name = substr(s, 0, i);
  std::string operands = substr(s, i);
  return encode(name, operands);
}

#define CHECK(b, s) EXPECT_EQ(b, encode(s))

TEST(EncodeTest, R) {
  CHECK(0x003100b3, " add x1, x2, x3");
  CHECK(0x403100b3, " sub x1, x2, x3");
  CHECK(0x003140b3, " xor x1, x2, x3");
  CHECK(0x003160b3, "  or x1, x2, x3");
  CHECK(0x003170b3, " and x1, x2, x3");
  CHECK(0x003110b3, " sll x1, x2, x3");
  CHECK(0x003150b3, " srl x1, x2, x3");
  CHECK(0x403150b3, " sra x1, x2, x3");
  CHECK(0x003120b3, " slt x1, x2, x3");
  CHECK(0x003130b3, "sltu x1, x2, x3");
}

TEST(EncodeTest, I) {
  CHECK(0x00010093, "addi x1, x2, 0");
  CHECK(0x00d10093, "addi x1, x2, 13");
  CHECK(0xfff10093, "addi x1, x2, -1");
  CHECK(0x00014093, "xori x1, x2, 0");
  CHECK(0x00d14093, "xori x1, x2, 13");
  CHECK(0xfff14093, "xori x1, x2, -1");
  CHECK(0x00016093, " ori x1, x2, 0");
  CHECK(0x00d16093, " ori x1, x2, 13");
  CHECK(0xfff16093, " ori x1, x2, -1");
  CHECK(0x00017093, "andi x1, x2, 0");
  CHECK(0x00d17093, "andi x1, x2, 13");
  CHECK(0xfff17093, "andi x1, x2, -1");

  CHECK(0x00011093, "slli x1, x2, 0");
  CHECK(0x00d11093, "slli x1, x2, 13");
  CHECK(0x00015093, "srli x1, x2, 0");
  CHECK(0x00d15093, "srli x1, x2, 13");
  CHECK(0x40015093, "srai x1, x2, 0");
  CHECK(0x40d15093, "srai x1, x2, 13");

  CHECK(0x00012093, " slti x1, x2, 0");
  CHECK(0x00d12093, " slti x1, x2, 13");
  CHECK(0xfff12093, " slti x1, x2, -1");
  CHECK(0x00013093, "sltiu x1, x2, 0");
  CHECK(0x00d13093, "sltiu x1, x2, 13");
  CHECK(0xfff13093, "sltiu x1, x2, -1");

  CHECK(0x00010083, " lb x1,x2,0");
  CHECK(0x00d10083, " lb x1,x2,13");
  CHECK(0xfff10083, " lb x1,x2,-1");
  CHECK(0x00011083, " lh x1,x2,0");
  CHECK(0x00d11083, " lh x1,x2,13");
  CHECK(0xfff11083, " lh x1,x2,-1");
  CHECK(0x00012083, " lw x1,x2,0");
  CHECK(0x00d12083, " lw x1,x2,13");
  CHECK(0xfff12083, " lw x1,x2,-1");
  CHECK(0x00014083, "lbu x1,x2,0");
  CHECK(0x00d14083, "lbu x1,x2,13");
  CHECK(0xfff14083, "lbu x1,x2,-1");
  CHECK(0x00015083, "lhu x1,x2,0");
  CHECK(0x00d15083, "lhu x1,x2,13");
  CHECK(0xfff15083, "lhu x1,x2,-1");
}

TEST(EncodeTest, Register) {}

} // namespace
