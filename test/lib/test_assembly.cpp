#include <gtest/gtest.h>

#include "assembly.h"

using namespace assembly;

namespace {

/*
<expr>
  <int>
  <sym>
  <str>
  <func>
  <reg>

<int>
  100
  0xff

<sym>
  $.
  $abc
  $.L10

<str>
  "a"
  "abc"

<func>
  +(1 2)
  %add(1 %(7 4))

<reg>
  x0
  fp

.global <sym>
.local
.weak

.type <sym>, "function"|"object"
.size <sym>, <expr_int>
.section ".text"|".rodata"|".data"|".bss"
.set <sym>, <expr_int>

.db <expr_int>...
.dh
.dw
.dq

.ascii <str>...
.asciz <str>...

.fill <int_repeat>, <int_size>, <int_value>
.fill <int_repeat>, <int_size>, 0
.fill <int_repeat>, 1, 0

.align <int_align>
  nop
  0
*/

std::string tokenize(const std::string &s) {
  Lexer lexer(s);
  lexer.tokenize();
  return joinSeq(" ", lexer.getTokens());
}

#define CHECK(a, b)                                                            \
  do {                                                                         \
    std::string c = tokenize(a);                                               \
    EXPECT_STREQ(c.c_str(), b);                                                \
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
}

TEST(LexerTest, Label) {
  CHECK(R"---(add:)---", "Label|add");
  CHECK(R"---(.L10:)---", "Label|.L10");
}

TEST(LexerTest, Int) {
  CHECK(R"---(.set $., 0x10)---", "Directive|.set Sym|. , Int|10");
  CHECK(R"---(.set $., 16, 1)---", "Directive|.set Sym|. , Int|10 , Int|01");
}

TEST(LexerTest, Str) {
  CHECK(R"---(.ascii "hello\0\n\t\"\\\x00\xab")---",
        "Directive|.ascii Str|hello\0\n\t\"\\\x00\xab");
}

TEST(LexerTest, Func) {
  CHECK(R"---(.set $., -(10))---", "Directive|.set Sym|. , Func|- ( Int|0a )");
  CHECK(R"---(.fill +(10 $foo))---",
        "Directive|.fill Func|+ ( Int|0a Sym|foo )");
  CHECK(R"---(.fill %(10 %add($foo 20)))---",
        "Directive|.fill Func|% ( Int|0a Func|add ( Sym|foo Int|14 ) )");
}

std::string parse(const std::string &s) {
  Lexer lexer(s);
  lexer.tokenize();

  Parser parser(lexer.getTokens());
  std::unique_ptr<Statement> stmt = std::move(parser.parse());
  return toString(*stmt);
}

#undef CHECK

#define CHECK(a, b)                                                            \
  do {                                                                         \
    std::string c = parse(a);                                                  \
    EXPECT_STREQ(c.c_str(), b);                                                \
  } while (false)

TEST(ParserTest, Directive) { CHECK(R"---(.global $add)---", ".global $add"); }

#undef CHECK

} // namespace
