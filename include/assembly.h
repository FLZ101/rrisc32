#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#include <list>
#include <map>
#include <memory>
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
  enum Type { Int, Sym, Func};

  Expr(s64 i) : i(i), type(Int) {}
  Expr(const std::string &s, Type type = Sym) : s(s), type(type) {}

  Type type;

  s64 i;
  std::string s;
  std::vector<std::unique_ptr<Expr>> arguments;
};

struct ExprToken {
  enum Type { Int, Sym, LPar, RPar, Func, Err };

  Type type;
  s64 i;
  std::string s;
};

class Assembler {
public:
  explicit Assembler(std::string filename)
      : filename(filename), writer(filename + ".o", elf::ET_REL) {}

  Assembler(const Assembler &) = delete;
  Assembler &operator=(const Assembler &) = delete;

  void run();

private:
  bool isDigit(char c) { return '0' <= c && c <= '9'; }

  bool isAlpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
  }

  bool isOneOf(char c, const char *s) {
    while (*s) {
      if (*s == c)
        return true;
      ++s;
    }
    return false;
  }

  bool isSpace(char c) { return isOneOf(c, "\t "); }

  bool isOperatorChar(char c) { return isOneOf(c, "+-*/%()<>|&^~"); }

  bool isSymbolChar(char c) { return isAlpha(c) || isDigit(c) || isOneOf(c, "_."); }

  void tokenize(std::vector<std::string> &tokens);

  void parseDirective(const std::vector<std::string> &tokens);
  void parseLabel(const std::vector<std::string> &tokens);
  void parseInstr(const std::vector<std::string> &tokens);

  void tokenizeExpr(std::string s, std::vector<ExprToken> &tokens);
  std::unique_ptr<Expr> parseExpr(const std::vector<ExprToken> &tokens);
  std::unique_ptr<Expr> parseExpr(const std::string &s);
  ExprVal evalExpr(const Expr *expr);

  s64 parseInt(std::string s);
  s64 parseInt(char c, bool hex = false);

  Section *getSection(const std::string &name);

  Symbol *getSymbol(const std::string &name);
  Symbol *addSymbol(const std::string &name);

  bool isLocalSymbolName(const std::string &name) { return name.starts_with(".L"); }

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
