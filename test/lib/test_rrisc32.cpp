#include <gtest/gtest.h>

#include "rrisc32.h"

#include <regex>

using namespace rrisc32;

namespace {

#define CHECK(b, s) EXPECT_EQ(b, test::encode(s))

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
  CHECK(0x00010093, "addi x1, x2,  0");
  CHECK(0x00d10093, "addi x1, x2, 13");
  CHECK(0xfff10093, "addi x1, x2, -1");
  CHECK(0x00014093, "xori x1, x2,  0");
  CHECK(0x00d14093, "xori x1, x2, 13");
  CHECK(0xfff14093, "xori x1, x2, -1");
  CHECK(0x00016093, " ori x1, x2,  0");
  CHECK(0x00d16093, " ori x1, x2, 13");
  CHECK(0xfff16093, " ori x1, x2, -1");
  CHECK(0x00017093, "andi x1, x2,  0");
  CHECK(0x00d17093, "andi x1, x2, 13");
  CHECK(0xfff17093, "andi x1, x2, -1");

  CHECK(0x00011093, "slli x1, x2,  0");
  CHECK(0x00d11093, "slli x1, x2, 13");
  CHECK(0x00015093, "srli x1, x2,  0");
  CHECK(0x00d15093, "srli x1, x2, 13");
  CHECK(0x40015093, "srai x1, x2,  0");
  CHECK(0x40d15093, "srai x1, x2, 13");

  CHECK(0x00012093, " slti x1, x2,  0");
  CHECK(0x00d12093, " slti x1, x2, 13");
  CHECK(0xfff12093, " slti x1, x2, -1");
  CHECK(0x00013093, "sltiu x1, x2,  0");
  CHECK(0x00d13093, "sltiu x1, x2, 13");
  CHECK(0xfff13093, "sltiu x1, x2, -1");

  CHECK(0x00010083, " lb x1, x2,  0"); //  lb x1,  0(x2)
  CHECK(0x00d10083, " lb x1, x2, 13"); //  lb x1, 13(x2)
  CHECK(0xfff10083, " lb x1, x2, -1"); //  lb x1, -1(x2)
  CHECK(0x00011083, " lh x1, x2,  0"); //  lh x1,  0(x2)
  CHECK(0x00d11083, " lh x1, x2, 13"); //  lh x1, 13(x2)
  CHECK(0xfff11083, " lh x1, x2, -1"); //  lh x1, -1(x2)
  CHECK(0x00012083, " lw x1, x2,  0"); //  lw x1,  0(x2)
  CHECK(0x00d12083, " lw x1, x2, 13"); //  lw x1, 13(x2)
  CHECK(0xfff12083, " lw x1, x2, -1"); //  lw x1, -1(x2)
  CHECK(0x00014083, "lbu x1, x2,  0"); // lbu x1,  0(x2)
  CHECK(0x00d14083, "lbu x1, x2, 13"); // lbu x1, 13(x2)
  CHECK(0xfff14083, "lbu x1, x2, -1"); // lbu x1, -1(x2)
  CHECK(0x00015083, "lhu x1, x2,  0"); // lhu x1,  0(x2)
  CHECK(0x00d15083, "lhu x1, x2, 13"); // lhu x1, 13(x2)
  CHECK(0xfff15083, "lhu x1, x2, -1"); // lhu x1, -1(x2)

  CHECK(0x00000073, "ecall");
  CHECK(0x00100073, "ebreak");

  CHECK(0x000100e7, "jalr x1, x2,  0"); // jalr x1,  0(x2)
  CHECK(0x00d100e7, "jalr x1, x2, 13"); // jalr x1, 13(x2)
  CHECK(0xfff100e7, "jalr x1, x2, -1"); // jalr x1, -7(x2)
}

TEST(EncodeTest, S) {
  CHECK(0x00208023, "sb x1, x2,  0"); // sb x2,  0(x1)
  CHECK(0x002086a3, "sb x1, x2, 13"); // sb x2, 13(x1)
  CHECK(0xfe208fa3, "sb x1, x2, -1"); // sb x2, -1(x1)
  CHECK(0x00209023, "sh x1, x2,  0"); // sh x2,  0(x1)
  CHECK(0x002096a3, "sh x1, x2, 13"); // sh x2, 13(x1)
  CHECK(0xfe209fa3, "sh x1, x2, -1"); // sh x2, -1(x1)
  CHECK(0x0020a023, "sw x1, x2,  0"); // sw x2,  0(x1)
  CHECK(0x0020a6a3, "sw x1, x2, 13"); // sw x2, 13(x1)
  CHECK(0xfe20afa3, "sw x1, x2, -1"); // sw x2, -1(x1)
}

TEST(EncodeTest, B) {
  CHECK(0x00208063, "beq x1, x2,  0");
  CHECK(0x00208663, "beq x1, x2, 12");
  CHECK(0x00208663, "beq x1, x2, 13");
  CHECK(0xfe208ce3, "beq x1, x2, -8");
  CHECK(0xfe208ce3, "beq x1, x2, -7");
  CHECK(0x00209063, "bne x1, x2,  0");
  CHECK(0x00209663, "bne x1, x2, 12");
  CHECK(0x00209663, "bne x1, x2, 13");
  CHECK(0xfe209ce3, "bne x1, x2, -8");
  CHECK(0xfe209ce3, "bne x1, x2, -7");
  CHECK(0x0020c063, "blt x1, x2,  0");
  CHECK(0x0020c663, "blt x1, x2, 12");
  CHECK(0x0020c663, "blt x1, x2, 13");
  CHECK(0xfe20cce3, "blt x1, x2, -8");
  CHECK(0xfe20cce3, "blt x1, x2, -7");
  CHECK(0x0020d063, "bge x1, x2,  0");
  CHECK(0x0020d663, "bge x1, x2, 12");
  CHECK(0x0020d663, "bge x1, x2, 13");
  CHECK(0xfe20dce3, "bge x1, x2, -8");
  CHECK(0xfe20dce3, "bge x1, x2, -7");

  CHECK(0x0020e063, "bltu x1, x2,  0");
  CHECK(0x0020e663, "bltu x1, x2, 12");
  CHECK(0x0020e663, "bltu x1, x2, 13");
  CHECK(0xfe20ece3, "bltu x1, x2, -8");
  CHECK(0xfe20ece3, "bltu x1, x2, -7");
  CHECK(0x0020f063, "bgeu x1, x2,  0");
  CHECK(0x0020f663, "bgeu x1, x2, 12");
  CHECK(0x0020f663, "bgeu x1, x2, 13");
  CHECK(0xfe20fce3, "bgeu x1, x2, -8");
  CHECK(0xfe20fce3, "bgeu x1, x2, -7");
}

TEST(EncodeTest, J) {
  CHECK(0x000000ef, "jal x1,  0");
  CHECK(0x00c000ef, "jal x1, 12");
  CHECK(0x00c000ef, "jal x1, 13");
  CHECK(0xff9ff0ef, "jal x1, -8");
  CHECK(0xff9ff0ef, "jal x1, -7");
}

TEST(EncodeTest, U) {
  CHECK(0x000000b7, "lui x1,  0");
  CHECK(0x0000d0b7, "lui x1, 13");
  CHECK(0x00000097, "auipc x1,  0");
  CHECK(0x0000d097, "auipc x1, 13");
}

TEST(EncodeTest, Register) {
  CHECK(0x000100b3, "add x1, x2, x0");
  CHECK(0x00418133, "add sp, gp, tp");
  CHECK(0x01ee8e33, "add t3, t4, t5");
  CHECK(0x00d605b3, "add a1, a2, a3");
  CHECK(0x000c8433, "add fp, s9, zero");
}

#undef CHECK

std::string normalize(const std::string &s) {
  return std::regex_replace(trim(s), std::regex(" +"), " ");
}

#define CHECK(b, s) EXPECT_EQ(test::decode(b), normalize(s))

TEST(DecodeTest, R) {
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

TEST(DecodeTest, I) {
  CHECK(0x00010093, "addi x1, x2,  0");
  CHECK(0x00d10093, "addi x1, x2, 13");
  CHECK(0xfff10093, "addi x1, x2, -1");
  CHECK(0x00014093, "xori x1, x2,  0");
  CHECK(0x00d14093, "xori x1, x2, 13");
  CHECK(0xfff14093, "xori x1, x2, -1");
  CHECK(0x00016093, " ori x1, x2,  0");
  CHECK(0x00d16093, " ori x1, x2, 13");
  CHECK(0xfff16093, " ori x1, x2, -1");
  CHECK(0x00017093, "andi x1, x2,  0");
  CHECK(0x00d17093, "andi x1, x2, 13");
  CHECK(0xfff17093, "andi x1, x2, -1");

  CHECK(0x00011093, "slli x1, x2,  0");
  CHECK(0x00d11093, "slli x1, x2, 13");
  CHECK(0x00015093, "srli x1, x2,  0");
  CHECK(0x00d15093, "srli x1, x2, 13");
  CHECK(0x40015093, "srai x1, x2,  0");
  CHECK(0x40d15093, "srai x1, x2, 13");

  CHECK(0x00012093, " slti x1, x2,  0");
  CHECK(0x00d12093, " slti x1, x2, 13");
  CHECK(0xfff12093, " slti x1, x2, -1");
  CHECK(0x00013093, "sltiu x1, x2,  0");
  CHECK(0x00d13093, "sltiu x1, x2, 13");
  CHECK(0xfff13093, "sltiu x1, x2, -1");

  CHECK(0x00010083, " lb x1, x2,  0"); //  lb x1,  0(x2)
  CHECK(0x00d10083, " lb x1, x2, 13"); //  lb x1, 13(x2)
  CHECK(0xfff10083, " lb x1, x2, -1"); //  lb x1, -1(x2)
  CHECK(0x00011083, " lh x1, x2,  0"); //  lh x1,  0(x2)
  CHECK(0x00d11083, " lh x1, x2, 13"); //  lh x1, 13(x2)
  CHECK(0xfff11083, " lh x1, x2, -1"); //  lh x1, -1(x2)
  CHECK(0x00012083, " lw x1, x2,  0"); //  lw x1,  0(x2)
  CHECK(0x00d12083, " lw x1, x2, 13"); //  lw x1, 13(x2)
  CHECK(0xfff12083, " lw x1, x2, -1"); //  lw x1, -1(x2)
  CHECK(0x00014083, "lbu x1, x2,  0"); // lbu x1,  0(x2)
  CHECK(0x00d14083, "lbu x1, x2, 13"); // lbu x1, 13(x2)
  CHECK(0xfff14083, "lbu x1, x2, -1"); // lbu x1, -1(x2)
  CHECK(0x00015083, "lhu x1, x2,  0"); // lhu x1,  0(x2)
  CHECK(0x00d15083, "lhu x1, x2, 13"); // lhu x1, 13(x2)
  CHECK(0xfff15083, "lhu x1, x2, -1"); // lhu x1, -1(x2)

  CHECK(0x00000073, "ecall");
  CHECK(0x00100073, "ebreak");

  CHECK(0x000100e7, "jalr x1, x2,  0"); // jalr x1,  0(x2)
  CHECK(0x00d100e7, "jalr x1, x2, 13"); // jalr x1, 13(x2)
  CHECK(0xfff100e7, "jalr x1, x2, -1"); // jalr x1, -7(x2)
}

TEST(DecodeTest, S) {
  CHECK(0x00208023, "sb x1, x2,  0"); // sb x2,  0(x1)
  CHECK(0x002086a3, "sb x1, x2, 13"); // sb x2, 13(x1)
  CHECK(0xfe208fa3, "sb x1, x2, -1"); // sb x2, -1(x1)
  CHECK(0x00209023, "sh x1, x2,  0"); // sh x2,  0(x1)
  CHECK(0x002096a3, "sh x1, x2, 13"); // sh x2, 13(x1)
  CHECK(0xfe209fa3, "sh x1, x2, -1"); // sh x2, -1(x1)
  CHECK(0x0020a023, "sw x1, x2,  0"); // sw x2,  0(x1)
  CHECK(0x0020a6a3, "sw x1, x2, 13"); // sw x2, 13(x1)
  CHECK(0xfe20afa3, "sw x1, x2, -1"); // sw x2, -1(x1)
}

TEST(DecodeTest, B) {
  CHECK(0x00208063, "beq x1, x2,  0");
  CHECK(0x00208663, "beq x1, x2, 12");
  CHECK(0xfe208ce3, "beq x1, x2, -8");
  CHECK(0x00209063, "bne x1, x2,  0");
  CHECK(0x00209663, "bne x1, x2, 12");
  CHECK(0xfe209ce3, "bne x1, x2, -8");
  CHECK(0x0020c063, "blt x1, x2,  0");
  CHECK(0x0020c663, "blt x1, x2, 12");
  CHECK(0xfe20cce3, "blt x1, x2, -8");
  CHECK(0x0020d063, "bge x1, x2,  0");
  CHECK(0x0020d663, "bge x1, x2, 12");
  CHECK(0xfe20dce3, "bge x1, x2, -8");

  CHECK(0x0020e063, "bltu x1, x2,  0");
  CHECK(0x0020e663, "bltu x1, x2, 12");
  CHECK(0xfe20ece3, "bltu x1, x2, -8");
  CHECK(0x0020f063, "bgeu x1, x2,  0");
  CHECK(0x0020f663, "bgeu x1, x2, 12");
  CHECK(0xfe20fce3, "bgeu x1, x2, -8");
}

TEST(DecodeTest, J) {
  CHECK(0x000000ef, "jal x1,  0");
  CHECK(0x00c000ef, "jal x1, 12");
  CHECK(0xff9ff0ef, "jal x1, -8");
}

TEST(DecodeTest, U) {
  CHECK(0x000000b7, "lui x1,  0");
  CHECK(0x0000d0b7, "lui x1, 13");
  CHECK(0x00000097, "auipc x1,  0");
  CHECK(0x0000d097, "auipc x1, 13");
}

#undef check

} // namespace
