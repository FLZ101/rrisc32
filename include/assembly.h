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
  std::string name;
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

  Label *findLabelB(const std::string &name, unsigned line) {
    auto r = std::ranges::find_last_if(
        labels.begin(), labels.end(), [&name, line](const Label &label) {
          return label.name == name && label.line < line;
        });
    return r.empty() ? nullptr : &*r.begin();
  }

  Label *findLabelF(const std::string &name, unsigned line) {
    auto r = std::ranges::find_last_if(
        labels.rbegin(), labels.rend(), [&name, line](const Label &label) {
          return label.name == name && label.line > line;
        });
    return r.empty() ? nullptr : &*r.begin();
  }

  std::string name;
  s64 offset = 0;
  u8 align = 3;

  std::list<Label> labels;
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
  Expr() {}

  s64 i;

  std::string sym;

  std::string func;
  std::vector<std::unique_ptr<Expr>> arguments;
};

struct ExprToken {
  enum Type { Int, Sym, LPar, RPar, Func };

  s64 i;
  std::string s;
};

class ExprLexer {
public:
  ExprLexer(const std::string &s) : s(s) {}

  bool hasNext();
  void next();

private:
  std::string s;
};

// ExprParser

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

  bool isOperator(char c) { return isOneOf(c, "+-*/%()<>|&^~"); }

  void tokenize(std::vector<std::string> &tokens);

  void parseDirective(const std::vector<std::string> &tokens);
  void parseLabel(const std::vector<std::string> &tokens);
  void parseInstr(const std::vector<std::string> &tokens);

  Section *getSection(const std::string &name);

  Symbol *getSymbol(const std::string &name);
  Symbol *addSymbol(const std::string &name);

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

/*
register x0
imm .

expression
+ - * / %
| & ^ ~
*/

/*
1:
        auipc	a0, %pcrel_hi(msg + 1)
        addi	a0, a0, %pcrel_lo(1b)
*/
