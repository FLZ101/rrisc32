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
  enum class Type {
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
  using enum Type;

  Type type;

  s64 i;
  std::string s;
};

std::ostream &operator<<(std::ostream &os, Token::Type type);

std::ostream &operator<<(std::ostream &os, const Token &tk);

class Lexer {
public:
  Lexer(const std::string &s) : s(s + "\n\n"), i(0) {}

  void tokenize();

  const std::vector<Token> &getTokens() { return tokens; }

private:
  bool tryEat(const std::string &str);
  bool eat(const std::string &str);

  bool isDec(char c) { return '0' <= c && c <= '9'; }
  bool isHex(char c) { return isDec(c) || ('a' <= c && c <= 'f'); }
  bool isReg(char c) { return isDec(c) || ('a' <= c && c <= 'z'); }

  bool isOneOf(char c, const char *s) {
    while (*s) {
      if (*s == c)
        return true;
      ++s;
    }
    return false;
  }

  bool isSpace(char c) { return isOneOf(c, "\t "); }

  bool isLower(char c) { return 'a' <= c && c <= 'z'; }
  bool isUpper(char c) { return 'A' <= c && c <= 'Z'; }
  bool isAlpha(char c) { return isLower(c) || isUpper(c); }

  bool isDirective(char c) { return isInstr(c); }
  bool isInstr(char c) { return isLower(c) || c == '.'; }
  bool isLabel(char c) { return isAlpha(c) || isDec(c) || isOneOf(c, "_."); }
  bool isStatement(char c) {
    return isDirective(c) || isInstr(c) || isLabel(c) || c == ':';
  }

  bool isOperator(char c) { return isOneOf(c, "+-*/%<>|&^~"); }
  bool isFunc(char c) { return isOperator(c) || isLower(c) || c == '_'; }

  bool isSym(char c) { return isLabel(c); }

  void cookToken(Token &tk);
  void cookStatement(Token &tk);
  void cookStr(Token &tk);
  void cookInt(Token &tk);

  bool checkDirective(const std::string &str);
  bool checkInstr(const std::string &str);
  bool checkLabel(const std::string &str);

  std::string s;
  unsigned i;

  std::vector<Token> tokens;
};

struct Expr {
  enum class Type { Reg, Str, Int, Sym, Func };
  using enum Type;

  Expr(s64 i) : i(i), type(Int) {}
  Expr(const std::string &s, Type type = Str) : s(s), type(type) {}

  Type type;

  s64 i;
  std::string s;
  std::vector<std::unique_ptr<Expr>> operands;
};

std::ostream &operator<<(std::ostream &os, const Expr &expr);

struct Statement {
  enum class Type { Directive, Instr, Label };
  using enum Type;

  Type type;

  std::string s;
  std::vector<std::unique_ptr<Expr>> arguments;
};

std::ostream &operator<<(std::ostream &os, const Statement &stmt);

class Parser {
public:
  Parser(const std::vector<Token> &tokens) : tokens(tokens), i(0) {}

  std::unique_ptr<Statement> parse();

private:
  std::vector<std::unique_ptr<Expr>> parseArguments();
  std::unique_ptr<Expr> parseFunc();

  const Token *eat(const std::initializer_list<Token::Type> &types);
  const Token *eat(Token::Type type);
  const Token *expect(const std::initializer_list<Token::Type> &types);
  const Token *expect(Token::Type type);

  std::vector<Token> tokens;
  unsigned i;
};

struct DriverOpts {
  std::string inFile;
  std::string outFile;
};

class Driver {
public:
  explicit Driver(const DriverOpts &opts) : opts(opts) {}

  Driver(const Driver &) = delete;
  Driver &operator=(const Driver &) = delete;

  void run();

private:
  const DriverOpts opts;
};

} // namespace assembly

#endif
