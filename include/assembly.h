#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "elf.h"
#include "rrisc32.h"
#include "util.h"

namespace assembly {

DEFINE_EXCEPTION(AssemblyError)

struct Label {
  char c;
  s64 offset;
  unsigned line;
};

struct Instr {
  std::vector<std::string> tokens;
  unsigned line;
};

struct Directive {
  std::vector<std::string> tokens;
  unsigned line;
};

struct Section {
  Section(std::string name) : name(name) {}

  Section(const Section &) = delete;
  Section &operator=(const Section &) = delete;

  const Label *findLabelB(char c, unsigned line);
  const Label *findLabelF(char c, unsigned line);

  std::string name;
  s64 offset = 0;
  u8 align = 3;

  std::vector<Label> labels;
  std::list<Instr> instrs;
  std::list<Directive> directives;
};

struct Symbol {
  Symbol() {}
  Symbol(const std::string &name) { sym.name = name; }

  elf::Symbol sym = {.name = "",
                     .value = 0,
                     .size = 0,
                     .type = elf::STT_NOTYPE,
                     .bind = elf::STB_LOCAL,
                     .other = elf::STV_DEFAULT,
                     .sec = elf::SHN_UNDEF};

  Section *sec = nullptr;
  s64 offset = 0;
};

struct ExprVal {
  explicit ExprVal(s64 i) : i(i) {}
  ExprVal(Section *sec, s64 offset) : sec(sec), offset(offset) {}

  s64 i = 0;
  Section *sec = nullptr;
  s64 offset = 0;
};

struct Expr {
  enum Type { Reg, Str, Int, Sym, Func };

  Expr(s64 i) : i(i), type(Int) {}
  Expr(const std::string &s, Type type = Str) : s(s), type(type) {}

  Type type;

  s64 i;
  std::string s;
  std::vector<std::unique_ptr<Expr>> operands;
};

struct Statement {
  enum Type { Directive, Instr, Label };

  Type type;

  std::string s;
  std::vector<std::unique_ptr<Expr>> arguments;
};

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

  const std::vector<Token> &tokens;
  unsigned i;
};

class Lexer {
public:
  Lexer(const std::string &s) : s(s + "\n\n"), i(0) {}

  void tokenize();

  const std::vector<Token> &getTokens() { return tokens; }

private:
  bool startsWith(const std::string &str) {
    if (s.size() - i < str.size())
      return false;
    for (unsigned j = 0; j < str.size(); ++j)
      if (s[i + j] != str[j])
        return false;
    return true;
  }
  bool eat(const std::string &str) {
    if (startsWith(str)) {
      i += str.size();
      return true;
    }
    return false;
  }
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

  s64 parseInt(const std::string &str, bool hex = false);

  std::string s;
  unsigned i;

  std::vector<Token> tokens;
};

class Assembler {
public:
  explicit Assembler(std::string filename)
      : filename(filename), writer(filename + ".o", elf::ET_REL) {}

  Assembler(const Assembler &) = delete;
  Assembler &operator=(const Assembler &) = delete;

  void run();

private:
  void parseDirective(const std::vector<std::string> &tokens);
  void parseLabel(const std::vector<std::string> &tokens);
  void parseInstr(const std::vector<std::string> &tokens);

  ExprVal evalExpr(const Expr *expr);

  Section *getSection(const std::string &name);

  Symbol *getSymbol(const std::string &name);
  Symbol *addSymbol(const std::string &name);

  bool isLocalSymbolName(const std::string &name) {
    return name.starts_with(".L");
  }

  std::vector<std::string> lines;
  unsigned curLine = 0;

  Section sections[4] = {Section(".text"), Section(".rodata"), Section(".data"),
                         Section(".bss")};
  Section *curSec = nullptr;

  std::map<std::string, std::unique_ptr<Symbol>> symbols;

  std::list<Directive> directives;

  std::string filename;
  elf::RRisc32Writer writer;
};

} // namespace assembly

#endif
