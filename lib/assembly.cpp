#include "assembly.h"

#include <algorithm>
#include <fstream>
#include <functional>

namespace assembly {

#define INVALID_SYMBOL_NAME(name)                                                       \
  THROW(AssemblyError, "invalid symbol", curLine + 1, name)

#define DUPLICATED_SYMBOL_NAME()                                                    \
  THROW(AssemblyError, "duplicated symbol", curLine + 1, lines[curLine])

#define UNKNOWN_SECTION_NAME()                                                 \
  THROW(AssemblyError, "unknown section name", curLine + 1, lines[curLine])

#define UNKNOWN_SECTION_TYPE()                                                 \
  THROW(AssemblyError, "unknown section type", curLine + 1, lines[curLine])

#define CHECK_CURRENT_SECTION()                                                \
  do {                                                                         \
    if (!curSec)                                                               \
      THROW(AssemblyError, "no current section", curLine + 1, lines[curLine]); \
  } while (false)

#define INVALID_DIRECTIVE()                                                    \
  THROW(AssemblyError, "invalid directive", curLine + 1, lines[curLine])

#define UNKNOWN_DIRECTIVE()                                                    \
  THROW(AssemblyError, "unknown directive", curLine + 1, lines[curLine])

const Label *Section::findLabelB(char c, unsigned line) {
  auto it = std::lower_bound(labels.begin(), labels.end(),
    [](const Label &a, const Label &b) { return a.line < b.line; });
  while (--it >= labels.begin())
    if (it->c == c)
      return &*it;
  return nullptr;
}

const Label *Section::findLabelF(char c, unsigned line) {
  auto it = std::lower_bound(labels.begin(), labels.end(),
    [](const Label &a, const Label &b) { return a.line <= b.line; });
  while (it < labels.end())
    if (it->c == c)
      return &*it;
  return nullptr;
}

void Assembler::run() {
  {
    std::fstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs)
      THROW(AssemblyError, "read", filename);
    for (std::string line; std::getline(ifs, line);)
      lines.push_back(line);
  }

  for (curLine = 0; curLine < lines.size(); ++curLine) {
    std::vector<std::string> tokens;
    tokenize(tokens);
    if (tokens.empty())
      continue;
    if (tokens[0].front() == '.')
      parseDirective(tokens);
    else if (tokens[0].back() == ':')
      parseLabel(tokens);
    else
      parseInstr(tokens);
  }
}

s64 Assembler::parseInt(char c, bool hex) {
  if (isDigit(c))
    return c - '0';
  if (hex && isOneOf(c, "abcdef"))
    return c - 'a';
  UNREACHABLE();
}

s64 Assembler::parseInt(std::string s) {
  bool hex = false;
  if (s.starts_with("0x")) {
    s = substr(s, 2);
    hex = true;
  }
  s64 res = 0;
  for (char c : s) {
    res *= hex ? 16 : 10;
    res += parseInt(c, hex);
  }
  return res;
}

void Assembler::tokenizeExpr(std::string s, std::vector<ExprToken> &tokens) {
  s += "\n\n";

  enum {
    Begin,
    Int,
    Hex,
    Sym,
    Func,
    End
  };
  auto state = Begin;
  unsigned j = 0;
  for (unsigned i = 0; i < s.size(); ++i) {
    char c = s[i];

#define UNEXPECTED_CHAR() \
  THROW(AssemblyError, "unexpected character", curLine + 1, s, i, c, toHexStr(c, true))

#define INVALID_SYM_TOKEN(tk) \
  THROW(AssemblyError, "invalid Sym token", curLine + 1, tk)

#define INVALID_INT_TOKEN(tk) \
  THROW(AssemblyError, "invalid Int token", curLine + 1, tk)

    switch (state) {
    case Begin:
      if (isSpace(c))
        continue;
      if (isDigit(c)) {
        j = i;
        if (c == '0' && i + 1 < s.size() && s[i+1] == 'x') {
          state = Hex;
          ++i;
        } else {
          state = Int;
        }
      } else if (c == '$') {
        j = i;
        state = Sym;
      } else if (isAlpha(c) || isOperatorChar(c) || c == '_') {
        j = i;
        state = Func;
      } else if (c == '\n') {
        state = End;
      } else if (c == '(') {
        toke.push_back({.type = ExprToken::LPar});
      } else if (c == ')') {
        toke.push_back({.type = ExprToken::RPar});
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Int:
    case Hex:
      if (isDigit(c))
        continue;
      if (state == Hex && isOneOf("abcdef"))
        continue;
      if (isSpace(c) || isOneOf(c, "()\n")) {
        std::string tk = substr(s, j, i);
        if (tk == "0x")
          INVALID_INT_TOKEN(tk);
        tokens.push_back({.type = ExprToken::Int, .i = parseInt(tk)});
        --i;
        state = Begin;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Sym:
      if (isSymbolChar(c))
        continue;
      if (isSpace(c) || isOneOf(c, "()\n")) {
        std::string tk = substr(s, j, i);
        if (tk.size() < 2)
          INVALID_SYM_TOKEN(tk);
        if (isDigit(tk[1])) {
          if (tk.size() != 3 || !isOneOf(tk[2], "bf"))
            INVALID_SYM_TOKEN(tk);
        }
        tokens.push_back({.type = ExprToken::Sym, .s = substr(tk, 1)});
        --i;
        state = Begin;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Func:
      if (isAlpha(c) || isOperatorChar(c) || c == '_' || isDigit(c))
        continue;
      if (isSpace(c) || isOneOf(c, "()\n")) {
        tokens.push_back({.type = ExprToken::Func, .s =substr(s, j, i)});
        --i;
        state = Begin;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case End:
      tokens.push_back({.type = ExprToken::Err);
      return;
    }
  }
}

std::unique_ptr<Expr> Assembler::parseExpr(const std::string &s) {
  std::vector<ExprToken> tokens;
  tokenizeExpr(s, tokens);
  return parseExpr(tokens);
}

static std::string formatExprToken(const ExprToken &tk) {
  switch (tk.type) {
  case ExprToken::Int:
    return join("|", "Int", tk.i);
  case ExprToken::Sym:
    return join("|", "Sym", tk.s);
  case ExprToken::LPar:
    return "(";
  case ExprToken::RPar:
    return ")";
  case ExprToken::Func:
    return join("|", "Func", tk.s);
  case ExprToken::Err:
    return "Err";
  default:
    UNREACHABLE();
  }
}

std::unique_ptr<Expr> Assembler::parseExpr(const std::vector<ExprToken> &tokens) {
  unsigned i = 0;

#define UNEXPECTED_TOKEN() \
  THROW(AssemblyError, "unexpected token", curLine + 1, lines[curLine], formatExprToken(tk))

#define CUR() (tokens[i])

#define ADVANCE() (tokens[i++])

#define EAT(type) \
  do { const ExprToken &tk = ADVANCE(); if (tk.type != type) UNEXPECTED_TOKEN(); } while (false)

  std::function<std::unique_ptr<Expr>()> parse;

  parse = [&tokens, &i]() -> std::unique_ptr<Expr> {
    const ExprToken &tk = ADVANCE();
    switch (tk.type) {
    case ExprToken::Int:
      return std::move(std::make_unique<Expr>(tk->i));
    case ExprToken::Sym:
      return std::move(std::make_unique<Expr>(tk->s));
    case ExprToken::Func: {
      auto expr = std::make_unique<Expr>(tk->s, Expr::Func);
      EAT(ExprToken::LPar);
      while (true) {
        if (CUR().type == ExprToken::RPar) {
          ADVANCE();
          break;
        }
        expr->arguments.emplace_back(std::move(parse()));
      }
      return std::move(expr);
    }
    default:
      UNEXPECTED_TOKEN();
    }
  };

  std::unique_ptr<Expr> expr = std::move(parse());

  if (i != tokens.size() - 1)
    THROW(AssemblyError, "residual tokens", curLine + 1, lines[curLine], formatExprToken(tokens[i]));

  return std::move(expr);
}

ExprVal Assembler::evalExpr(const Expr *expr) {
}

Symbol *Assembler::getSymbol(const std::string &name) {
  if (symbols.count(name))
    return symbols[name].get();
  return nullptr;
}

Symbol *Assembler::addSymbol(const std::string &name) {
  if (isDigit(name[0]))
    INVALID_SYMBOL_NAME(name);
  for (char c : name) {
    if (!isSymbolChar(c))
      INVALID_SYMBOL_NAME(name);
  }
  if (name == ".")
    INVALID_SYMBOL_NAME(name);

  if (!symbols.count(name))
    symbols[name].reset(new Symbol(name));
  return symbols[name].get();
}

void Assembler::parseDirective(const std::vector<std::string> &tokens) {
  std::string s = tokens[0];

#define SET_SYMBOLS_BIND(BIND)                                                 \
  do {                                                                         \
    if (tokens.size() < 2)                                                     \
      INVALID_DIRECTIVE();                                                     \
    for (unsigned i = 1; i < tokens.size(); ++i) {                             \
      addSymbol(tokens[i])->sym.bind = BIND;                                   \
    }                                                                          \
  } while (false)

  if (s == ".global") {
    SET_SYMBOLS_BIND(elf::STB_GLOBAL);
  } else if (s == ".local") {
    SET_SYMBOLS_BIND(elf::STB_LOCAL);
  } else if (s == ".weak") {
    SET_SYMBOLS_BIND(elf::STB_WEAK);
  } else if (s == ".type") {
    if (tokens.size() != 3)
      INVALID_DIRECTIVE();
    Symbol *sym = addSymbol(tokens[1]);
    std::string type = tokens[2];
    if (type == "func")
      sym->sym.type = elf::STT_FUNC;
    else if (type == "object")
      sym->sym.type = elf::STT_OBJECT;
    else
      UNKNOWN_SECTION_TYPE();
  } else if (s == ".size") {
    if (tokens.size() != 3)
      INVALID_DIRECTIVE();
    Symbol *sym = addSymbol(tokens[1]);
    directives.push_back({.tokens = tokens, .line = curLine});
  } else if (s == ".section") {
    if (tokens.size() != 2)
      INVALID_DIRECTIVE();
    const std::string &name = tokens[1];
    curSec = getSection(name);
    if (!getSymbol(name)) {
      Symbol *sym = addSymbol(name);
      sym->sec = curSec;
      sym->offset = curSec->offset;
    }
  } else if (s == ".set") {
  } else if (s == ".db") {
  } else if (s == ".dh") {
  } else if (s == ".dw") {
  } else if (s == ".dq") {
  } else if (s == ".ascii") {
  } else if (s == ".asciz") {
  } else if (s == ".fill") {
  } else if (s == ".align") {
  } else {
    UNKNOWN_DIRECTIVE();
  }
}

void Assembler::parseLabel(const std::vector<std::string> &tokens) {
  if (tokens.size() != 1)
    INVALID_DIRECTIVE();

  CHECK_CURRENT_SECTION();

  std::string name = substr(tokens[0], 0, -1);
  if (name.size() == 1 && isDigit(name[0])) {
    curSec->labels.push_back(
        {.c = name[0], .offset = curSec->offset, .line = curLine});
  } else {
    if (getSymbol(name))
      DUPLICATED_SYMBOL_NAME();
    Symbol *sym = addSymbol(name);
    sym->sec = curSec;
    sym->offset = curSec->offset;
  }
}

void Assembler::parseInstr(const std::vector<std::string> &tokens) {}

void Assembler::tokenize(std::vector<std::string> &tokens) {
  std::string s = lines[curLine];
  s += "\n\n";

  enum {
    Begin,
    First,
    AfterFirst,
    Operand,
    AfterOperand,
    Str,
    StrEsc,
    StrEscX,
    StrEscXX,
    AfterStr,
    Char,
    CharEsc,
    Char1,
    Comment,
    End
  };
  auto state = Begin;
  unsigned j = 0;
  for (unsigned i = 0; i < s.size(); ++i) {

#define UNEXPECTED_CHAR()                                                      \
  THROW(AssemblyError, "unexpected character", curLine + 1, i + 1, c,          \
        toHexStr(c, true))

    char c = s[i];
    switch (state) {
    case Begin:
      if (isSpace(c))
        continue;
      if (isAlpha(c) || isOneOf(c, "_.")) {
        state = First;
        j = i;
      } else if (c == '#') {
        state = Comment;
      } else if (c == '\n') {
        state = End;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case First:
      if (isAlpha(c) || isOneOf(c, "_.:") || isDigit(c))
        continue;
      if (isSpace(c)) {
        state = AfterFirst;
      } else if (c == '#') {
        state = Comment;
      } else if (c == '\n') {
        state = End;
      } else {
        UNEXPECTED_CHAR();
      }
      tokens.push_back(substr(s, j, i));
      break;
    case AfterFirst:
      if (isSpace(c))
        continue;
      if (isAlpha(c) || isDigit(c) || isOneOf(c, "_.$") || isOperatorChar(c)) {
        state = Operand;
        j = i;
      } else if (c == '"') {
        state = Str;
        j = i;
      } else if (c == '\'') {
        state = Char;
        j = i;
      } else if (c == '#') {
        state = Comment;
      } else if (c == '\n') {
        state = End;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Operand:
      if (isAlpha(c) || isDigit(c) || isOneOf(c, "_.$") || isOperatorChar(c) ||
          isSpace(c))
        continue;
      if (c == ',') {
        state = AfterOperand;
      } else if (c == '#') {
        state = Comment;
      } else if (c == '\n') {
        state = End;
      } else {
        UNEXPECTED_CHAR();
      }
      tokens.push_back(substr(s, j, i));
      break;
    case AfterOperand:
      if (isSpace(c))
        continue;
      if (isAlpha(c) || isDigit(c) || isOneOf(c, "_.$") || isOperatorChar(c)) {
        state = Operand;
        j = i;
      } else if (c == '"') {
        state = Str;
        j = i;
      } else if (c == '\'') {
        state = Char;
        j = i;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Str:
      if (c == '\\') {
        state = StrEsc;
      } else if (c == '"') {
        state = AfterStr;
        tokens.push_back(substr(s, j, i + 1));
      } else if (c == '\n') {
        UNEXPECTED_CHAR();
      }
      break;
    case StrEsc:
      switch (c) {
      case 'n':
      case 't':
      case '0':
      case '"':
        state = Str;
        break;
      case 'x':
        state = StrEscX;
        break;
      default:
        UNEXPECTED_CHAR();
      }
    case StrEscX:
      if (isOneOf(c, "0123456789abcdef"))
        state = StrEscXX;
      else
        UNEXPECTED_CHAR();
      break;
    case StrEscXX:
      if (isOneOf(c, "0123456789abcdef"))
        state = Str;
      else
        UNEXPECTED_CHAR();
      break;
    case AfterStr:
      if (isSpace(c))
        continue;
      if (c == ',') {
        state = AfterOperand;
      } else if (c == '#') {
        state = Comment;
      } else if (c == '\n') {
        state = End;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Char:
      if (c == '\\') {
        state = CharEsc;
      } else if (isOneOf(c, "'\n")) {
        UNEXPECTED_CHAR();
      } else {
        state = Char1;
      }
      break;
    case CharEsc:
      switch (c) {
      case 'n':
      case 't':
      case '0':
      case '\'':
        state = Char1;
        break;
      default:
        UNEXPECTED_CHAR();
      }
    case Char1:
      if (c == '\'') {
        tokens.push_back(substr(s, j, i + 1));
        state = AfterStr;
      } else {
        UNEXPECTED_CHAR();
      }
      break;
    case Comment:
      if (c == '\n')
        state = End;
      break;
    case End:
      for (std::string &s : tokens) {
        s = trim(s);
        assert(!s.empty());
      }
      return;
    }
  }
}

Section *Assembler::getSection(const std::string &name) {
  for (Section &sec : sections)
    if (sec.name == name)
      return &sec;
  UNKNOWN_SECTION_NAME();
}

} // namespace assembly
