#include <gtest/gtest.h>

#include "assembly.h"

using namespace assembly;

namespace {

#define CHECK(a, b)                                                            \
  do {                                                                         \
    EXPECT_STREQ(joinSeq(" ", tokenize(a)).c_str(), b);                        \
  } while (false)

TEST(LexerTest, Directive) {
  CHECK(R"---(.global $add)---", "Directive|.global Sym|add");
  CHECK(R"---(.type $add, "function")---",
        "Directive|.type Sym|add , Str|function");
  CHECK(R"---(.size $foo, 13)---", "Directive|.size Sym|foo , Int|0d");
  CHECK(R"---(.db 1, 2, 3)---", "Directive|.db Int|01 , Int|02 , Int|03");
}

TEST(LexerTest, Instr) {
  CHECK(R"---(add x4, x3, x2)---", "Instr|add Reg|x4 , Reg|x3 , Reg|x2");
  CHECK(R"---(addi x4, x3, 13)---", "Instr|addi Reg|x4 , Reg|x3 , Int|0d");
  CHECK(R"---(beq x4, x3, $.L10)---", "Instr|beq Reg|x4 , Reg|x3 , Sym|.L10");
  CHECK(R"---(jal x4, $add)---", "Instr|jal Reg|x4 , Sym|add");
  CHECK(R"---(jal x4, $1f)---", "Instr|jal Reg|x4 , Sym|1f");
}

TEST(LexerTest, Label) {
  CHECK(R"---(add:)---", "Label|add");
  CHECK(R"---(.L10:)---", "Label|.L10");
  CHECK(R"---(1:)---", "Label|1");
}

TEST(LexerTest, Int) {
  CHECK(R"---(.equ $., 0x10)---", "Directive|.equ Sym|. , Int|10");
  CHECK(R"---(.equ $., 16, 1)---", "Directive|.equ Sym|. , Int|10 , Int|01");
}

TEST(LexerTest, Str) {
  CHECK(R"---(.ascii "hello\0\n\t\"\\\x00\xab")---",
        "Directive|.ascii Str|hello\0\n\t\"\\\x00\xab");
}

TEST(LexerTest, Func) {
  CHECK(R"---(.equ $., -(10))---", "Directive|.equ Sym|. , Func|- ( Int|0a )");
  CHECK(R"---(.fill +(10 $foo))---",
        "Directive|.fill Func|+ ( Int|0a Sym|foo )");
  CHECK(R"---(.fill %(10 %add($foo 20)))---",
        "Directive|.fill Func|% ( Int|0a Func|add ( Sym|foo Int|14 ) )");
}

#undef CHECK

#define CHECK(a, b)                                                            \
  do {                                                                         \
    EXPECT_STREQ(toString(*(parse(a))).c_str(), b);                            \
  } while (false)

TEST(ParserTest, Directive) {
  CHECK(R"---(.global $add)---", ".global $add");
  CHECK(R"---(.type $add, "function")---", R"---(.type $add, "function")---");
  CHECK(R"---(.db 1, 2, 3)---", ".db 01, 02, 03");
}

TEST(ParserTest, Instr) {
  CHECK(R"---(add x4, x3, x2)---", "add x4, x3, x2");
  CHECK(R"---(beq x4, x3, $.L10)---", "beq x4, x3, $.L10");
}

TEST(ParserTest, Label) {
  CHECK(R"---(add:)---", "add:");
  CHECK(R"---(.L10:)---", ".L10:");
}

TEST(ParserTest, Int) {
  CHECK(R"---(.equ $., 0xa0)---", ".equ $., a0");
  CHECK(R"---(.equ $., 0x0123, 0xff01)---", ".equ $., 0123, ff01");
}

TEST(ParserTest, Str) {
  CHECK(R"---(.ascii "hello\0\n\t\"\\\x01\xab")---",
        R"---(.ascii "hello\0\n\t\"\\\x01\xab")---");
}

TEST(ParserTest, Func) {
  CHECK(R"---(.equ $., -(10))---", ".equ $., -(0a)");
  CHECK(R"---(.fill +(10 $foo))---", ".fill +(0a $foo)");
  CHECK(R"---(.fill %(10 %add($foo 20)))---",
        R"---(.fill %(0a %add($foo 14)))---");
}

#undef CHECK

} // namespace
