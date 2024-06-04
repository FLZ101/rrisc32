#include "assembly.h"

#include <algorithm>
#include <fstream>
#include <functional>

namespace assembly {

// const Label *Section::findLabelB(char c, unsigned line) {
//   auto it = std::lower_bound(
//       labels.begin(), labels.end(),
//       [](const Label &a, const Label &b) { return a.line < b.line; });
//   while (--it >= labels.begin())
//     if (it->c == c)
//       return &*it;
//   return nullptr;
// }

// const Label *Section::findLabelF(char c, unsigned line) {
//   auto it = std::lower_bound(
//       labels.begin(), labels.end(),
//       [](const Label &a, const Label &b) { return a.line <= b.line; });
//   while (it < labels.end())
//     if (it->c == c)
//       return &*it;
//   return nullptr;
// }

std::ostream &operator<<(std::ostream &os, Token::Type type) {
  switch (type) {
  case Token::Directive:
    os << "Directive";
    break;
  case Token::Instr:
    os << "Instr";
    break;
  case Token::Label:
    os << "Label";
    break;
  case Token::Reg:
    os << "Reg";
    break;
  case Token::Int:
    os << "Int";
    break;
  case Token::Str:
    os << "Str";
    break;
  case Token::Sym:
    os << "Sym";
    break;
  case Token::LPar:
    os << "(";
    break;
  case Token::RPar:
    os << ")";
    break;
  case Token::Func:
    os << "Func";
    break;
  case Token::Comma:
    os << ",";
    break;
  case Token::End:
    os << "End";
    break;
  default:;
  }
  UNREACHABLE();
}

std::ostream &operator<<(std::ostream &os, const Token &tk) {
  switch (tk.type) {
  case Token::Int:
    os << join("|", toString(tk.type), toHexStr(tk.i));
    break;
  case Token::Directive:
  case Token::Instr:
  case Token::Label:
  case Token::Reg:
  case Token::Str:
  case Token::Sym:
  case Token::Func:
    os << join("|", toString(tk.type), tk.s);
    break;
  case Token::LPar:
  case Token::RPar:
  case Token::Comma:
  case Token::End:
    os << toString(tk.type);
    break;
  default:
    UNREACHABLE();
  }
  return os;
}

void Lexer::tokenize() {

  enum {
    Begin,
    Statement,
    Expr,
    Dec,
    Hex,
    Str,
    StrEsc,
    StrEscX,
    StrEscXX,
    Reg,
    Sym,
    Func,
    Comment,
    End
  };
  auto state = Begin;

  unsigned j = 0;
  for (; i < s.size(); ++i) {
    char c = s[i];

#define UNEXPECTED_CHAR()                                                      \
  THROW(AssemblyError, "unexpected character", s, i, toHexStr(c, true), c)

#define SKIP_SPACE()                                                           \
  do {                                                                         \
    if (isSpace(c))                                                            \
      continue;                                                                \
  } while (false)

#define TRY_COMMENT()                                                          \
  do {                                                                         \
    if (c == '#') {                                                            \
      state = Comment;                                                         \
      continue;                                                                \
    }                                                                          \
  } while (false)

#define TRY_END()                                                              \
  do {                                                                         \
    if (c == '\n') {                                                           \
      state = End;                                                             \
      continue;                                                                \
    }                                                                          \
  } while (false)

#define ADD_TOKEN(T) tokens.push_back({.type = T, .s = substr(s, i, j)})

#define ADD_TOKEN_T(T) tokens.push_back({.type = T})

    switch (state) {
    case Begin:
      SKIP_SPACE();
      if (isStatement(c)) {
        j = i;
        state = Statement;
        continue;
      }
      TRY_COMMENT();
      TRY_END();
      UNEXPECTED_CHAR();
      break;
    case Comment:
      TRY_END();
      break;
    case End:
      ADD_TOKEN(Token::End);
      for (Token &tk : tokens)
        cookToken(tk);
      return;
    case Statement:
      if (isStatement(c))
        continue;
      ADD_TOKEN(Token::Directive);
      --i;
      state = Expr;
      break;
    case Expr:
      SKIP_SPACE();
      if (isDec(c)) {
        j = i;
        if (c == '0' && eat("x"))
          state = Hex;
        else
          state = Dec;
        continue;
      }
      if (c == '"') {
        j = i;
        state = Str;
        continue;
      }
      if (isReg(c)) {
        j = i;
        state = Reg;
        continue;
      }
      if (c == '$') {
        j = i;
        state = Sym;
        continue;
      }
      if (isOperator(c)) {
        j = i;
        state = Func;
        continue;
      }
      if (c == '(') {
        ADD_TOKEN_T(Token::LPar);
        continue;
      }
      if (c == ')') {
        ADD_TOKEN_T(Token::RPar);
        continue;
      }
      if (c == ',') {
        ADD_TOKEN_T(Token::Comma);
        continue;
      }
      TRY_COMMENT();
      TRY_END();
      UNEXPECTED_CHAR();
      break;
    case Str:
      if (c == '\\') {
        state = StrEsc;
        if (eat("x"))
          state = StrEscX;
        continue;
      }
      if (c == '"') {
        state = Expr;
        ++j;
        ADD_TOKEN(Token::Str);
        continue;
      }
      if (c == '\n')
        UNEXPECTED_CHAR();
      break;
    case StrEsc:
      switch (c) {
      case 'n':
      case 't':
      case '0':
      case '"':
        state = Str;
        break;
      default:
        UNEXPECTED_CHAR();
      }
      break;
    case StrEscX:
      if (isHex(c))
        state = StrEscXX;
      else
        UNEXPECTED_CHAR();
      break;
    case StrEscXX:
      if (isHex(c))
        state = Str;
      else
        UNEXPECTED_CHAR();
      break;
    case Dec:
      if (isDec(c))
        continue;
      ADD_TOKEN(Token::Int);
      --i;
      state = Expr;
      break;
    case Hex:
      if (isHex(c))
        continue;
      ADD_TOKEN(Token::Int);
      --i;
      state = Expr;
      break;
    case Reg:
      if (isReg(c))
        continue;
      ADD_TOKEN(Token::Reg);
      --i;
      state = Expr;
      break;
    case Sym:
      if (isSym(c))
        continue;
      ADD_TOKEN(Token::Sym);
      --i;
      state = Expr;
      break;
    case Func:
      if (isFunc(c))
        continue;
      ADD_TOKEN(Token::Func);
      --i;
      state = Expr;
      break;
    default:
      UNREACHABLE();
    }
#undef ADD_TOKEN_T
#undef ADD_TOKEN
#undef TRY_END
#undef TRY_COMMENT
#undef SKIP_SPACE
#undef UNEXPECTED_CHAR
  }

  UNREACHABLE();
}

void Lexer::cookToken(Token &tk) {
  switch (tk.type) {
  case Token::Directive:
    cookStatement(tk);
    break;
  case Token::Str:
    cookStr(tk);
    break;
  case Token::Int:
    cookInt(tk);
    break;
  case Token::Sym:
    tk.s = substr(tk.s, 1);
    if (tk.s.size() < 1 || isDec(tk.s[0]))
      THROW(AssemblyError, "invalid Sym", tk.s);
    break;
  case Token::Func:
    if (tk.s.size() > 1 && tk.s[0] == '%' && !isOperator(tk.s[1]))
      tk.s = substr(tk.s, 1);
    break;
  default:;
  }
}

void Lexer::cookStatement(Token &tk) {
  if (tk.s.front() == '.') {
    tk.type = Token::Directive;
    if (!checkDirective(tk.s))
      THROW(AssemblyError, "invalid Directive", tk.s);
  } else if (tk.s.back() == ':') {
    tk.type = Token::Label;
    tk.s = substr(tk.s, 0, -1);
    if (!checkLabel(tk.s))
      THROW(AssemblyError, "invalid Label", tk.s);
  } else {
    tk.type = Token::Instr;
    if (!checkInstr(tk.s))
      THROW(AssemblyError, "invalid Instr", tk.s);
  }
}

bool Lexer::checkDirective(const std::string &str) {
  if (str.size() < 2)
    return false;
  for (char c : str)
    if (!isDirective(c))
      return false;
  return true;
}

bool Lexer::checkInstr(const std::string &str) {
  for (char c : str)
    if (!isInstr(c))
      return false;
  return true;
}

bool Lexer::checkLabel(const std::string &str) {
  if (str.size() < 2 || isDec(str[0]))
    return false;
  for (char c : str)
    if (!isLabel(c))
      return false;
  return true;
}

void Lexer::cookStr(Token &tk) {
  std::string str;
  for (unsigned i = 1; i < tk.s.size() - 1; ++i) {
    if (tk.s[i] == '\\') {
      switch (tk.s[++i]) {
      case 'n':
        str.push_back('\n');
        break;
      case 't':
        str.push_back('\t');
        break;
      case '0':
        str.push_back('\0');
        break;
      case '"':
        str.push_back('"');
        break;
      case 'x':
        i += 2;
        str.push_back(char(parseInt(substr(tk.s, i - 1, i + 1), true)));
        break;
      default:
        UNREACHABLE();
      }
    }
  }
  tk.s = str;
}

s64 Lexer::parseInt(const std::string &str, bool hex) {
  s64 i = 0;
  if (hex) {
    for (char c : str) {
      i *= 10;
      i += c - '0';
    }
  } else {
    for (char c : str) {
      i *= 16;
      i += isDec(c) ? (c - '0') : (c - 'a' + 10);
    }
  }
  return i;
}

void Lexer::cookInt(Token &tk) {
  if (tk.s.starts_with("0x")) {
    tk.i = parseInt(substr(tk.s, 2), true);
  } else {
    tk.i = parseInt(tk.s);
  }
}

std::unique_ptr<Statement> Parser::parse() {
  if (tokens.empty())
    return nullptr;

  std::unique_ptr<Statement> res = std::make_unique<Statement>();

  const Token &tk = *expect({Token::Directive, Token::Instr, Token::Label});
  switch (tk.type) {
  case Token::Directive:
    res->type = Statement::Directive;
    break;
  case Token::Instr:
    res->type = Statement::Instr;
    break;
  case Token::Label:
    res->type = Statement::Label;
    break;
  default:
    UNREACHABLE();
  }

  res->s = tk.s;
  res->arguments = std::move(parseArguments());
  return std::move(res);
}

std::unique_ptr<Expr> Parser::parseFunc() {
  const Token *tk = expect(Token::Func);
  auto expr = std::make_unique<Expr>(tk->s, Expr::Func);
  expect(Token::LPar);
  while (tk = eat({Token::Int, Token::Sym, Token::Func})) {
    std::unique_ptr<Expr> operand;
    switch (tk->type) {
    case Token::Int:
      operand = std::move(std::make_unique<Expr>(tk->i));
      break;
    case Token::Sym:
      operand = std::move(std::make_unique<Expr>(tk->s, Expr::Sym));
      break;
    case Token::Func:
      --i;
      operand = std::move(parseFunc());
      break;
    default:
      UNREACHABLE();
    }
    expr->operands.emplace_back(std::move(operand));
  }
  expect(Token::RPar);
  return std::move(expr);
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
  std::vector<std::unique_ptr<Expr>> args;

#define ADD_ARG_I()                                                            \
  do {                                                                         \
    args.emplace_back(std::move(std::make_unique<Expr>(tk.i)));                \
  } while (false)

#define ADD_ARG_S(T)                                                           \
  do {                                                                         \
    args.emplace_back(std::move(std::make_unique<Expr>(tk.s, T)));             \
  } while (false)

#define UNEXPECTED_TOKEN() THROW(AssemblyError, "unexpected token", tk)

  bool comma = false;
  for (; i < tokens.size(); ++i) {
    const Token &tk = tokens[i];
    switch (tk.type) {
    case Token::Int:
      ADD_ARG_I();
      break;
    case Token::Str:
      ADD_ARG_S(Expr::Str);
      break;
    case Token::Reg:
      ADD_ARG_S(Expr::Reg);
      break;
    case Token::Sym:
      ADD_ARG_S(Expr::Sym);
      break;
    case Token::Func:
      --i;
      args.emplace_back(std::move(parseFunc()));
      continue;
    default:
      if (comma)
        UNEXPECTED_TOKEN();
      expect(Token::End);
      return std::move(args);
    }
    if (eat(Token::Comma))
      comma = true;
  }

#undef ADD_ARG_S
#undef ADD_ARG_I

  UNREACHABLE();
}

const Token *Parser::eat(const std::initializer_list<Token::Type> &types) {
  for (auto type : types)
    if (tokens[i].type == type)
      return &tokens[i++];
  return nullptr;
}

const Token *Parser::eat(Token::Type type) { return eat({type}); }

const Token *Parser::expect(const std::initializer_list<Token::Type> &types) {
  const Token *tk = eat(types);
  if (!tk)
    THROW(AssemblyError, joinArr("|", types) + " expected",
          toString(tokens[i]));
  return tk;
}

const Token *Parser::expect(Token::Type type) { return expect({type}); }

void Assembler::run() {
  {
    std::fstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs)
      THROW(AssemblyError, "read", filename);
    for (std::string line; std::getline(ifs, line);)
      lines.push_back(line);
  }
}

Symbol *Assembler::getSymbol(const std::string &name) {
  if (symbols.count(name))
    return symbols[name].get();
  return nullptr;
}

// Symbol *Assembler::addSymbol(const std::string &name) {
//   if (isDigit(name[0]))
//     INVALID_SYMBOL_NAME(name);
//   for (char c : name) {
//     if (!isSymbolChar(c))
//       INVALID_SYMBOL_NAME(name);
//   }
//   if (name == ".")
//     INVALID_SYMBOL_NAME(name);

//   if (!symbols.count(name))
//     symbols[name].reset(new Symbol(name));
//   return symbols[name].get();
// }

// void Assembler::parseDirective(const std::vector<std::string> &tokens) {
//   std::string s = tokens[0];

// #define SET_SYMBOLS_BIND(BIND)                                                 \
//   do {                                                                         \
//     if (tokens.size() < 2)                                                     \
//       INVALID_DIRECTIVE();                                                     \
//     for (unsigned i = 1; i < tokens.size(); ++i) {                             \
//       addSymbol(tokens[i])->sym.bind = BIND;                                   \
//     }                                                                          \
//   } while (false)

//   if (s == ".global") {
//     SET_SYMBOLS_BIND(elf::STB_GLOBAL);
//   } else if (s == ".local") {
//     SET_SYMBOLS_BIND(elf::STB_LOCAL);
//   } else if (s == ".weak") {
//     SET_SYMBOLS_BIND(elf::STB_WEAK);
//   } else if (s == ".type") {
//     if (tokens.size() != 3)
//       INVALID_DIRECTIVE();
//     Symbol *sym = addSymbol(tokens[1]);
//     std::string type = tokens[2];
//     if (type == "func")
//       sym->sym.type = elf::STT_FUNC;
//     else if (type == "object")
//       sym->sym.type = elf::STT_OBJECT;
//     else
//       UNKNOWN_SECTION_TYPE();
//   } else if (s == ".size") {
//     if (tokens.size() != 3)
//       INVALID_DIRECTIVE();
//     Symbol *sym = addSymbol(tokens[1]);
//     directives.push_back({.tokens = tokens, .line = curLine});
//   } else if (s == ".section") {
//     if (tokens.size() != 2)
//       INVALID_DIRECTIVE();
//     const std::string &name = tokens[1];
//     curSec = getSection(name);
//     if (!getSymbol(name)) {
//       Symbol *sym = addSymbol(name);
//       sym->sec = curSec;
//       sym->offset = curSec->offset;
//     }
//   } else if (s == ".set") {
//   } else if (s == ".db") {
//   } else if (s == ".dh") {
//   } else if (s == ".dw") {
//   } else if (s == ".dq") {
//   } else if (s == ".ascii") {
//   } else if (s == ".asciz") {
//   } else if (s == ".fill") {
//   } else if (s == ".align") {
//   } else {
//     UNKNOWN_DIRECTIVE();
//   }
// }

// void Assembler::parseLabel(const std::vector<std::string> &tokens) {
//   if (tokens.size() != 1)
//     INVALID_DIRECTIVE();

//   CHECK_CURRENT_SECTION();

//   std::string name = substr(tokens[0], 0, -1);
//   if (name.size() == 1 && isDigit(name[0])) {
//     curSec->labels.push_back(
//         {.c = name[0], .offset = curSec->offset, .line = curLine});
//   } else {
//     if (getSymbol(name))
//       DUPLICATED_SYMBOL_NAME();
//     Symbol *sym = addSymbol(name);
//     sym->sec = curSec;
//     sym->offset = curSec->offset;
//   }
// }

// Section *Assembler::getSection(const std::string &name) {
//   for (Section &sec : sections)
//     if (sec.name == name)
//       return &sec;
//   UNKNOWN_SECTION_NAME();
// }

} // namespace assembly
