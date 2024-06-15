#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "util.h"

namespace assembly {

DEFINE_EXCEPTION(AssemblyError)

struct Token {
  enum Type {
    Directive,
    Instr,
    Label,
    Int,
    Str,
    Reg,
    Sym,
    Func,
    LPar,
    RPar,
    Comma,
    End
  };

  Type type;

  s64 i;
  std::string s;
};

std::ostream &operator<<(std::ostream &os, Token::Type type);

std::ostream &operator<<(std::ostream &os, const Token &tk);

class Lexer {
public:
  Lexer(const std::string &s, std::vector<Token> &tokens)
      : s(s + "\n\n"), i(0), tokens(tokens) {}

  void tokenize();

  static bool isDec(char c) { return '0' <= c && c <= '9'; }
  static bool isHex(char c) { return isDec(c) || ('a' <= c && c <= 'f'); }
  static bool isReg(char c) { return isDec(c) || ('a' <= c && c <= 'z'); }

  static bool isSpace(char c) { return isOneOf(c, "\t "); }

  static bool isLower(char c) { return 'a' <= c && c <= 'z'; }
  static bool isUpper(char c) { return 'A' <= c && c <= 'Z'; }
  static bool isAlpha(char c) { return isLower(c) || isUpper(c); }

  static bool isDirective(char c) { return isInstr(c); }
  static bool isInstr(char c) { return isLower(c) || c == '.'; }
  static bool isLabel(char c) {
    return isAlpha(c) || isDec(c) || isOneOf(c, "_.");
  }
  static bool isStatement(char c) {
    return isDirective(c) || isInstr(c) || isLabel(c) || c == ':';
  }

  static bool isOperator(char c) { return isOneOf(c, "+-*/%<>|&^~"); }
  static bool isFunc(char c) { return isOperator(c) || isLower(c) || c == '_'; }

  static bool isSym(char c) { return isLabel(c); }

private:
  bool tryEat(const std::string &str);
  bool eat(const std::string &str);

  void cookToken(Token &tk);
  void cookStatement(Token &tk);
  void cookStr(Token &tk);
  void cookInt(Token &tk);

  bool checkDirective(const std::string &str);
  bool checkInstr(const std::string &str);
  bool checkLabel(const std::string &str);

  std::string s;
  unsigned i;

  std::vector<Token> &tokens;
};

std::vector<Token> tokenize(const std::string &s);

struct Expr {
  enum Type { Reg, Str, Int, Sym, Func };

  Expr(s64 i) : i(i), type(Int) {}
  Expr(const std::string &s, Type type = Str) : s(s), type(type) {}
  Expr(const std::string &s, const std::vector<Expr> &operands)
      : s(s), type(Func), operands(operands) {}

  Type type;

  s64 i;
  std::string s;
  std::vector<Expr> operands;
};

std::ostream &operator<<(std::ostream &os, Expr::Type type);

std::ostream &operator<<(std::ostream &os, const Expr &expr);

struct Statement {
  enum Type { Directive, Instr, Label };

  Type type;

  std::string s;
  std::vector<Expr> arguments;

  s64 offset = 0;
};

std::ostream &operator<<(std::ostream &os, const Statement &stmt);

class Parser {
public:
  Parser(const std::vector<Token> &tokens) : tokens(tokens), i(0) {}

  std::unique_ptr<Statement> parse();

private:
  std::vector<Expr> parseArguments();
  Expr parseFunc();

  const Token *eat(const std::initializer_list<Token::Type> &types);
  const Token *eat(Token::Type type);
  const Token *expect(const std::initializer_list<Token::Type> &types);
  const Token *expect(Token::Type type);

  std::vector<Token> tokens;
  unsigned i;
};

std::unique_ptr<Statement> parse(const std::string &s);

struct AssemblerOpts {
  std::string inFile;
  std::string outFile;
};

class Assembler {
public:
  explicit Assembler(const AssemblerOpts &opts) : opts(opts) {}

  Assembler(const Assembler &) = delete;
  Assembler &operator=(const Assembler &) = delete;

  void run();

private:
  const AssemblerOpts opts;
};

} // namespace assembly

#endif
