#include "assembly.h"

#include <algorithm>
#include <fstream>

#include "elf.h"

#define DEBUG_TYPE "assembly"

namespace assembly {

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
    UNREACHABLE();
  }
  return os;
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

bool Lexer::tryEat(const std::string &str) {
  unsigned k = i + 1;
  if (s.size() - k < str.size())
    return false;
  for (unsigned j = 0; j < str.size(); ++j)
    if (s[k + j] != str[j])
      return false;
  return true;
}

bool Lexer::eat(const std::string &str) {
  if (tryEat(str)) {
    i += str.size();
    return true;
  }
  return false;
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
  THROW(AssemblyError, "unexpected character", i, escape(c))

#define SKIP_SPACE                                                             \
  if (isSpace(c))                                                              \
    continue;

#define TRY_COMMENT                                                            \
  if (c == '#') {                                                              \
    state = Comment;                                                           \
    continue;                                                                  \
  }

#define TRY_END                                                                \
  if (c == '\n') {                                                             \
    state = End;                                                               \
    continue;                                                                  \
  }

#define ADD_TOKEN(T) tokens.push_back({.type = T, .s = substr(s, j, i)})

#define ADD_TOKEN_T(T) tokens.push_back({.type = T})

    switch (state) {
    case Begin:
      SKIP_SPACE
      if (isStatement(c)) {
        j = i;
        state = Statement;
        continue;
      }
      TRY_COMMENT
      TRY_END
      UNEXPECTED_CHAR();
      break;
    case Comment:
      TRY_END
      break;
    case End:
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
      SKIP_SPACE
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
      TRY_COMMENT
      TRY_END
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
      case '\\':
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
    if (tk.s.empty() ||
        (isDec(tk.s[0]) && (tk.s.size() != 2 || !isOneOf(tk.s[1], "bf"))))
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
  if (tk.s.back() == ':') {
    tk.type = Token::Label;
    tk.s = substr(tk.s, 0, -1);
    if (!checkLabel(tk.s))
      THROW(AssemblyError, "invalid Label", tk.s);
  } else if (tk.s.front() == '.') {
    tk.type = Token::Directive;
    if (!checkDirective(tk.s))
      THROW(AssemblyError, "invalid Directive", tk.s);
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
  if (str.empty())
    return false;
  if (str == ".")
    return false;
  if (str.size() > 1 && isDec(str[0]))
    return false;
  for (char c : str)
    if (!isLabel(c))
      return false;
  return true;
}

void Lexer::cookStr(Token &tk) { tk.s = unescape(tk.s); }

void Lexer::cookInt(Token &tk) {
  if (tk.s.starts_with("0x")) {
    tk.i = parseInt(substr(tk.s, 2), true);
  } else {
    tk.i = parseInt(tk.s);
  }
}

std::vector<Token> tokenize(const std::string &s) {
  std::vector<Token> tokens;

  Lexer lexer(s, tokens);
  lexer.tokenize();

  return std::move(tokens);
}

std::unique_ptr<Statement> Parser::parse() {
  if (tokens.empty())
    return nullptr;

  tokens.push_back({.type = Token::End});

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

Expr Parser::parseFunc() {
  const Token *tk = expect(Token::Func);
  auto expr = Expr(tk->s, Expr::Func);
  expect(Token::LPar);
  while (tk = eat({Token::Int, Token::Sym, Token::Func})) {
    switch (tk->type) {
    case Token::Int:
      expr.operands.emplace_back(tk->i);
      break;
    case Token::Sym:
      expr.operands.emplace_back(tk->s, Expr::Sym);
      break;
    case Token::Func:
      --i;
      expr.operands.emplace_back(parseFunc());
      break;
    default:
      UNREACHABLE();
    }
  }
  expect(Token::RPar);
  return expr;
}

std::vector<Expr> Parser::parseArguments() {
  std::vector<Expr> args;

#define ADD_ARG_I()                                                            \
  do {                                                                         \
    args.emplace_back(tk.i);                                                   \
    ++i;                                                                       \
  } while (false)

#define ADD_ARG_S(T)                                                           \
  do {                                                                         \
    args.emplace_back(tk.s, T);                                                \
    ++i;                                                                       \
  } while (false)

#define UNEXPECTED_TOKEN() THROW(AssemblyError, "unexpected token", tk)

  bool comma = false;
  while (i < tokens.size()) {
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
      args.emplace_back(parseFunc());
      break;
    default:
      if (comma)
        UNEXPECTED_TOKEN();
      expect(Token::End);
      return std::move(args);
    }
    comma = eat(Token::Comma);
  }

#undef UNEXPECTED_TOKEN
#undef ADD_ARG_S
#undef ADD_ARG_I

  UNREACHABLE();
}

std::ostream &operator<<(std::ostream &os, Expr::Type type) {
  switch (type) {
  case Expr::Reg:
    os << "Reg";
    break;
  case Expr::Str:
    os << "Str";
    break;
  case Expr::Int:
    os << "Int";
    break;
  case Expr::Sym:
    os << "Sym";
    break;
  case Expr::Func:
    os << "Func";
    break;
  default:
    UNREACHABLE();
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Expr &expr) {
  switch (expr.type) {
  case Expr::Reg:
    os << expr.s;
    break;
  case Expr::Str:
    os << escape(expr.s);
    break;
  case Expr::Int:
    os << toHexStr(expr.i);
    break;
  case Expr::Sym:
    os << "$" << expr.s;
    break;
  case Expr::Func:
    if (!Lexer::isOperator(expr.s[0]))
      os << "%";
    os << expr.s << "(" << joinSeq(" ", expr.operands) << ")";
    break;
  default:
    UNREACHABLE();
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Statement &stmt) {
  switch (stmt.type) {
  case Statement::Directive:
  case Statement::Instr:
    os << stmt.s;
    break;
  case Statement::Label:
    os << stmt.s << ":";
    break;
  default:
    UNREACHABLE();
  }
  if (!stmt.arguments.empty()) {
    assert(stmt.type != Statement::Label);
    os << " " << joinSeq(", ", stmt.arguments);
  }
  return os;
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
    THROW(AssemblyError, joinSeq("|", types) + " expected",
          toString(tokens[i]));
  return tk;
}

const Token *Parser::expect(Token::Type type) { return expect({type}); }

std::unique_ptr<Statement> parse(const std::string &s) {
  return std::move(Parser(tokenize(s)).parse());
}

struct Section {
  explicit Section(const std::string &name) : name(name) {}

  Section(const Section &) = delete;
  Section &operator=(const Section &) = delete;

  const Statement *findLabel(const std::string &name);

  std::string name;
  s64 offset = 0;

  static const u8 Alignment = 12;

  std::vector<std::unique_ptr<Statement>> stmts;

  ByteBuffer bb;
};

const Statement *Section::findLabel(const std::string &name) {
  if (stmts.empty())
    return nullptr;

  auto it = std::lower_bound(stmts.cbegin(), stmts.cend(), offset,
                             [](const std::unique_ptr<Statement> &stmt,
                                s64 offset) { return stmt->offset < offset; });
  if (name.ends_with('b')) {
    while (++it >= stmts.cbegin())
      if ((*it)->s[0] == name[0])
        return it->get();
  } else {
    assert(name.ends_with('f'));
    while (it != stmts.cend())
      if ((*it)->s[0] == name[0])
        return it->get();
  }
  return nullptr;
}

struct Symbol {
  Symbol(const std::string &name) {
    sym.name = name;
    if (!sym.name.starts_with(".L"))
      sym.bind = elf::STB_GLOBAL;
  }

  void set(s64 offset) { set(nullptr, offset); }

  void set(Section *sec, s64 offset) {
    this->sec = sec;
    this->offset = offset;
    if (!this->sec)
      sym.sec = elf::SHN_ABS;
  }

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

class ExprVal {
public:
  ExprVal() {}

  explicit ExprVal(s64 i) : i(i), valid(true) {}

  ExprVal(const std::string &name) : name(name), valid(true) {}

  ExprVal(Section *sec, s64 offset) : sec(sec), offset(offset), valid(true) {}

  bool isValid() const { return valid; }

  bool isInt() const { return valid && !isSym(); }
  s64 getI() const { return i; }

  bool isSym() const { return valid && !!sec; }
  Section *getSec() const { return sec; }
  s64 getOffset() const { return offset; }

  bool isUndef() const { return valid && !name.empty(); }
  const std::string &getName() const { return name; }

private:
  bool valid = false;

  s64 i = 0;

  Section *sec = nullptr;
  s64 offset = 0;

  std::string name;
};

std::ostream &operator<<(std::ostream &os, const ExprVal &v) {
  if (v.isInt()) {
    os << "Int|" << v.getI();
  } else if (v.isSym()) {
    os << "Sym|" << v.getSec()->name << "|" << v.getOffset();
  } else {
    os << "!";
  }
  return os;
}

class AssemblerImpl {
public:
  AssemblerImpl(const AssemblerOpts &opts) : opts(opts) {}

  void run();

private:
  void handleDirective(std::unique_ptr<Statement> stmt);
  void expandInstr(std::unique_ptr<Statement> stmt);
  void handleInstr(std::unique_ptr<Statement> stmt);
  void handleLabel(std::unique_ptr<Statement> stmt);

  void addInstr(std::unique_ptr<Statement> stmt) {
    stmt->offset = curSec->offset;
    curSec->offset += 4;
    curSec->stmts.emplace_back(std::move(stmt));
  }

  void addInstr(const std::string &s, const std::vector<Expr> &arguments) {
    std::unique_ptr<Statement> stmt = std::make_unique<Statement>();
    stmt->type = Statement::Instr;
    stmt->s = s;
    stmt->arguments = arguments;
    addInstr(std::move(stmt));
  }

  template <typename T> void handleDirectiveD(std::unique_ptr<Statement> stmt);
  void handleDirectiveAscii(std::unique_ptr<Statement> stmt);

  void handleDelayedStmts();

  void cookSections();

  void saveToFile();

  ExprVal evalExpr(const Expr &expr);
  ExprVal evalFunc(const Expr &expr);

  Section *getSection(const std::string &name);

  Symbol *getSymbol(const std::string &name);
  Symbol *addSymbol(const std::string &name);

  void setSymbolBind(const std::string &name, unsigned char bind);
  void setSymbolType(const std::string &name, unsigned char type);
  void setSymbolSize(const std::string &name, elf::Elf_Xword size);

  void checkCurSecName(const std::initializer_list<std::string> &names);

  Section secText{".text"};
  Section secRodata{".rodata"};
  Section secData{".data"};
  Section secBss{".bss"};

  Section *sections[4] = {&secText, &secRodata, &secData, &secBss};
  Section *curSec = nullptr;

  std::map<std::string, std::unique_ptr<Symbol>> symTab;

  std::list<std::unique_ptr<Statement>> delayedStmts;

  const AssemblerOpts opts;
};

#define CHECK_CURRENT_SECTION()                                                \
  do {                                                                         \
    if (!curSec)                                                               \
      THROW(AssemblyError, "no current section");                              \
  } while (false)

#define CHECK_ARGUMENTS_SIZE(n)                                                \
  do {                                                                         \
    if (stmt->arguments.size() != n)                                           \
      THROW(AssemblyError, toString(n) + " arguments expected");               \
  } while (false)

#define CHECK_ARGUMENT_TYPE(i, type)                                           \
  do {                                                                         \
    if (stmt->arguments[i].type != type)                                       \
      THROW(AssemblyError, toString(type) + " expected", i,                    \
            stmt->arguments[i]);                                               \
  } while (false)

#define CHECK_OPERANDS_SIZE(n)                                                 \
  do {                                                                         \
    if (expr.operands.size() != n)                                             \
      THROW(AssemblyError, toString(n) + " operands expected");                \
  } while (false)

#define CHECK_DUPLICATED_SYMBOL(name)                                          \
  do {                                                                         \
    if (getSymbol(name))                                                       \
      THROW(AssemblyError, "duplicated symbol", name);                         \
  } while (false)

#define INVALID_STATEMENT() THROW(AssemblyError, "invalid statement", *stmt)

ExprVal AssemblerImpl::evalFunc(const Expr &expr) {
  std::vector<ExprVal> values;
  for (const Expr &e : expr.operands) {
    ExprVal v = evalExpr(e);
    if (!v.isValid() || v.isUndef())
      return ExprVal();
    values.push_back(v);
  }

  const std::string &name = expr.s;
  if (name == "-") {
    if (expr.operands.size() == 1) {
      ExprVal v = values[0];
      if (v.isInt())
        return ExprVal(-v.getI());
      return ExprVal();
    } else {
      CHECK_OPERANDS_SIZE(2);
      ExprVal v0 = values[0];
      ExprVal v1 = values[1];
      if (v0.isSym() && v1.isSym()) {
        if (v0.getSec() == v1.getSec())
          return ExprVal(v0.getOffset() - v1.getOffset());
        return ExprVal();
      }
      if (v0.isSym())
        return ExprVal(v0.getSec(), v0.getOffset() - v1.getI());
      if (v1.isSym())
        return ExprVal();
      return ExprVal(v0.getI() - v1.getI());
    }
  } else if (name == "+") {
    CHECK_OPERANDS_SIZE(2);
    ExprVal v0 = values[0];
    ExprVal v1 = values[1];
    if (v0.isInt() && v1.isInt())
      return ExprVal(v0.getI() + v1.getI());
    if (v0.isInt() && v1.isSym())
      return ExprVal(v1.getSec(), v0.getI() + v1.getOffset());
    if (v0.isSym() && v1.isInt())
      return ExprVal(v0.getSec(), v0.getOffset() + v1.getI());
    return ExprVal();
  } else {
    THROW(AssemblyError, "unimplemented Func " + name);
  }
}

ExprVal AssemblerImpl::evalExpr(const Expr &expr) {
  switch (expr.type) {
  case Expr::Reg:
  case Expr::Str:
    THROW(AssemblyError, "can not eval " + toString(expr.type));
    break;
  case Expr::Int:
    return ExprVal(expr.i);
    break;
  case Expr::Sym: {
    if (expr.s == ".") {
      if (!curSec)
        return ExprVal();
      return ExprVal(curSec, curSec->offset);
    }

    if (Lexer::isDec(expr.s[0])) {
      assert(expr.s.size() == 2 && isOneOf(expr.s[1], "bf"));
      if (!curSec)
        return ExprVal();
      const Statement *stmt = curSec->findLabel(expr.s);
      if (!stmt)
        return ExprVal();
      return ExprVal(curSec, stmt->offset);
    }

    Symbol *sym = addSymbol(expr.s);
    if (sym->sec)
      return ExprVal(sym->sec, sym->offset);
    if (sym->sym.sec == elf::SHN_ABS)
      return ExprVal(sym->offset);
    return ExprVal(expr.s);
  }
  case Expr::Func:
    return evalFunc(expr);
  default:
    UNREACHABLE();
  }
}

void AssemblerImpl::run() {
  std::vector<std::string> lines;
  {
    std::fstream ifs(opts.inFile, std::ios::in | std::ios::binary);
    if (!ifs)
      THROW(AssemblyError, "read", opts.inFile);
    for (std::string line; std::getline(ifs, line);)
      lines.push_back(std::move(line));
  }

  for (unsigned i = 0; i < lines.size(); ++i) {
    TRY()
    std::unique_ptr<Statement> stmt = std::move(parse(lines[i]));
    switch (stmt->type) {
    case Statement::Directive:
      handleDirective(std::move(stmt));
      break;
    case Statement::Instr:
      checkCurSecName({".text"});
      expandInstr(std::move(stmt));
      break;
    case Statement::Label:
      handleLabel(std::move(stmt));
      break;
    default:
      UNREACHABLE();
    }
    RETHROW(AssemblyError, i + 1, escape(lines[i]))
  }

  handleDelayedStmts();

  cookSections();

  saveToFile();
}

void AssemblerImpl::expandInstr(std::unique_ptr<Statement> stmt) {
  const std::string &name = stmt->s;

  std::string format;
  for (const auto &expr : stmt->arguments)
    format.push_back(expr.type == Expr::Reg ? 'r' : 'i');

  if (isOneOf(name, {"ecall", "ebreak"}) && format == "") {
    addInstr("ecb", {Expr(name == "ecall" ? 0 : 1)});
  } else if (name == "li" && format == "ri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("lui", {e0, Expr("hi", {e1})});
    addInstr("addi", {e0, e0, Expr("lo", {e1})});
  } else if (isOneOf(name, {"lb", "lh", "lw", "lbu", "lhu", "lwu"}) &&
             format == "ri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("lui", {e0, Expr("hi", {e1})});
    addInstr(name, {e0, e0, Expr("lo", {e1})});
  } else if (isOneOf(name, {"sb", "sh", "sw", "sbu", "shu", "swu"}) &&
             format == "rri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    const Expr &e2 = stmt->arguments[2];
    addInstr("lui", {e1, Expr("hi", {e2})});
    addInstr(name, {e0, e1, Expr("lo", {e2})});
  } else if (name == "nop" && format == "") {
    addInstr("addi", {Expr("x0", Expr::Reg), Expr("x0", Expr::Reg), Expr(0)});
  } else if (name == "mv" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("addi", {e0, e1, Expr(0)});
  } else if (name == "not" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("xori", {e0, e1, Expr(-1)});
  } else if (name == "neg" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("sub", {e0, Expr("x0", Expr::Reg), e1});
  } else if (name == "sext.b" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("slli", {e0, e1, Expr(24)});
    addInstr("srai", {e0, e1, Expr(24)});
  } else if (name == "sext.h" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("slli", {e0, e1, Expr(16)});
    addInstr("srai", {e0, e1, Expr(16)});
  } else if (name == "zext.b" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("andi", {e0, e1, Expr(255)});
  } else if (name == "zext.b" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("slli", {e0, e1, 16});
    addInstr("srli", {e0, e1, 16});
  } else if (name == "seqz" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("sltiu", {e0, e1, Expr(1)});
  } else if (name == "snez" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("sltu", {e0, Expr("x0", Expr::Reg), e1});
  } else if (name == "sltz" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("slt", {e0, e1, Expr("x0", Expr::Reg)});
  } else if (name == "sgtz" && format == "rr") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr("slt", {e0, Expr("x0", Expr::Reg), e1});
  } else if (isOneOf(name, {"beqz", "bnez", "bgez", "bltz"}) &&
             format == "ri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    addInstr(substr(name, -1), {e0, Expr("x0", Expr::Reg), e1});
  } else if (isOneOf(name, {"blez", "bgtz"}) && format == "ri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    std::string s = substr(name, 0, -1);
    s[1] = s[1] == 'l' ? 'g' : 'l';
    addInstr(s, {Expr("x0", Expr::Reg), e0, e1});
  } else if (isOneOf(name, {"bgt", "ble", "bgtu", "bleu"}) && format == "rri") {
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    const Expr &e2 = stmt->arguments[2];
    std::string s = name;
    s[1] = s[1] == 'l' ? 'g' : 'l';
    addInstr(s, {e1, e0, e2});
  } else if (name == "j" && format == "i") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("jal", {Expr("x0", Expr::Reg), e0});
  } else if (name == "jal" && format == "i") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("jal", {Expr("x1", Expr::Reg), e0});
  } else if (name == "jr" && format == "r") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("jalr", {Expr("x0", Expr::Reg), e0, Expr(0)});
  } else if (name == "jalr" && format == "r") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("jalr", {Expr("x1", Expr::Reg), e0, Expr(0)});
  } else if (name == "ret" && format == "") {
    addInstr("jalr", {Expr("x0", Expr::Reg), Expr("x1", Expr::Reg), Expr(0)});
  } else if (name == "call" && format == "i") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("lui", {Expr("x1", Expr::Reg), Expr("hi", {e0})});
    addInstr("jalr",
             {Expr("x1", Expr::Reg), Expr("x1", Expr::Reg), Expr("lo", {e0})});
  } else if (name == "tail" && format == "i") {
    const Expr &e0 = stmt->arguments[0];
    addInstr("lui", {Expr("x6", Expr::Reg), Expr("hi", {e0})});
    addInstr("jalr",
             {Expr("x0", Expr::Reg), Expr("x6", Expr::Reg), Expr("lo", {e0})});
  } else {
    addInstr(std::move(stmt));
  }
}

void AssemblerImpl::setSymbolBind(const std::string &name, unsigned char bind) {
  Symbol *sym = addSymbol(name);
  sym->sym.bind = bind;
}

void AssemblerImpl::setSymbolType(const std::string &name, unsigned char type) {
  Symbol *sym = addSymbol(name);
  sym->sym.type = type;
}

void AssemblerImpl::setSymbolSize(const std::string &name,
                                  elf::Elf_Xword size) {
  Symbol *sym = addSymbol(name);
  sym->sym.size = size;
}

void AssemblerImpl::handleDelayedStmts() {
  curSec = nullptr;
  for (std::unique_ptr<Statement> &stmt : delayedStmts) {
    const std::string &name = stmt->s;
    if (name == ".global") {
      setSymbolBind(stmt->arguments[0].s, elf::STB_GLOBAL);
    } else if (name == ".local") {
      setSymbolBind(stmt->arguments[0].s, elf::STB_LOCAL);
    } else if (name == ".weak") {
      setSymbolBind(stmt->arguments[0].s, elf::STB_WEAK);
    } else if (name == ".type") {
      const Expr &e0 = stmt->arguments[0];
      const Expr &e1 = stmt->arguments[1];
      setSymbolType(e0.s, e1.s == "function" ? elf::STT_FUNC : elf::STT_OBJECT);
    } else if (name == ".size") {
      const Expr &e0 = stmt->arguments[0];
      const Expr &e1 = stmt->arguments[1];
      ExprVal v1 = evalExpr(e1);
      if (!v1.isInt() || v1.getI() < 0)
        INVALID_STATEMENT();
      setSymbolSize(e0.s, v1.getI());
    } else if (name == ".equ") {
      const Expr &e0 = stmt->arguments[0];
      const Expr &e1 = stmt->arguments[1];

      ExprVal v1 = evalExpr(e1);
      if (!v1.isValid() || v1.isUndef())
        INVALID_STATEMENT();

      Symbol *sym = addSymbol(e0.s);
      if (v1.isInt()) {
        sym->set(v1.getI());
      } else {
        assert(v1.isSym());
        sym->set(v1.getSec(), v1.getOffset());
      }
    } else {
      UNREACHABLE();
    }
  }
}

void AssemblerImpl::handleInstr(std::unique_ptr<Statement> stmt) {}

template <typename T>
void AssemblerImpl::handleDirectiveD(std::unique_ptr<Statement> stmt) {
  if (curSec->name == ".bss")
    return;
  for (const auto &expr : stmt->arguments) {
    ExprVal v = evalExpr(expr);
    if (!v.isInt())
      INVALID_STATEMENT();
    curSec->bb.appendInt(static_cast<T>(v.getI()));
  }
}

void AssemblerImpl::handleDirectiveAscii(std::unique_ptr<Statement> stmt) {
  if (curSec->name == ".bss")
    return;
  for (const auto &expr : stmt->arguments) {
    curSec->bb.append(expr.s);
    if (stmt->s == ".asciz")
      curSec->bb.append('\0');
  }
}

void AssemblerImpl::cookSections() {
  for (Section *sec : sections) {
    curSec = sec;
    for (std::unique_ptr<Statement> &stmt : sec->stmts) {
      curSec->offset = stmt->offset;
      if (stmt->type == Statement::Instr) {
        handleInstr(std::move(stmt));
      } else if (stmt->type == Statement::Directive) {
        const std::string &name = stmt->s;
        if (name == ".db") {
          handleDirectiveD<u8>(std::move(stmt));
        } else if (name == ".dh") {
          handleDirectiveD<u16>(std::move(stmt));
        } else if (name == ".dw") {
          handleDirectiveD<u32>(std::move(stmt));
        } else if (name == ".dq") {
          handleDirectiveD<u64>(std::move(stmt));
        } else if (name == ".ascii" || name == ".asciz") {
          handleDirectiveAscii(std::move(stmt));
        } else if (name == ".fill") {
          if (curSec->name == ".bss")
            return;

          unsigned n = stmt->arguments.size();

          unsigned repeat = evalExpr(stmt->arguments[0]).getI();
          unsigned size = 1;
          s64 value = 0;

          if (n >= 2)
            size = evalExpr(stmt->arguments[1]).getI();

          if (n == 3)
            value = evalExpr(stmt->arguments[2]).getI();

          for (unsigned i = 0; i < repeat; ++i)
            curSec->bb.append(value, size);
        } else if (name == ".align") {
          if (curSec->name == ".bss")
            return;

          unsigned repeat =
              P2ALIGN(stmt->offset, evalExpr(stmt->arguments[0]).getI()) -
              stmt->offset;

          for (unsigned i = 0; i < repeat; ++i)
            curSec->bb.append('\0');
        } else {
          UNREACHABLE();
        }
      }
    }
  }
}

void AssemblerImpl::saveToFile() {
  // TODO: writer(filename + ".o", elf::ET_REL)
}

void AssemblerImpl::checkCurSecName(
    const std::initializer_list<std::string> &names) {
  CHECK_CURRENT_SECTION();
  for (const std::string &name : names)
    if (name == curSec->name)
      return;
  THROW(AssemblyError, joinSeq("|", names) + " expected",
        toString(curSec->name));
}

void AssemblerImpl::handleDirective(std::unique_ptr<Statement> stmt) {
  const std::string &name = stmt->s;
  if (name == ".global" || name == ".local" || name == ".weak") {
    CHECK_ARGUMENTS_SIZE(1);
    CHECK_ARGUMENT_TYPE(0, Expr::Sym);
    delayedStmts.emplace_back(std::move(stmt));
    return;
  }
  if (name == ".type") {
    CHECK_ARGUMENTS_SIZE(2);
    CHECK_ARGUMENT_TYPE(0, Expr::Sym);
    CHECK_ARGUMENT_TYPE(1, Expr::Str);

    const Expr &e1 = stmt->arguments[1];
    if (e1.s != "function" && e1.s != "object")
      THROW(AssemblyError, "symbol type must be function/object", e1.s);

    delayedStmts.emplace_back(std::move(stmt));
    return;
  }
  if (name == ".size") {
    CHECK_ARGUMENTS_SIZE(2);
    CHECK_ARGUMENT_TYPE(0, Expr::Sym);
    delayedStmts.emplace_back(std::move(stmt));
    return;
  }
  if (name == ".equ") {
    CHECK_ARGUMENTS_SIZE(2);
    CHECK_ARGUMENT_TYPE(0, Expr::Sym);
    const Expr &e0 = stmt->arguments[0];
    const Expr &e1 = stmt->arguments[1];
    if (e0.s == ".")
      THROW(AssemblyError, "can not define .");

    ExprVal v1 = evalExpr(e1);
    if (!v1.isValid() || v1.isUndef()) {
      delayedStmts.emplace_front(std::move(stmt));
      return;
    }

    Symbol *sym = addSymbol(e0.s);
    if (v1.isInt()) {
      sym->set(v1.getI());
    } else {
      assert(v1.isSym());
      sym->set(v1.getSec(), v1.getOffset());
    }
    return;
  }

  if (name == ".section") {
    CHECK_ARGUMENTS_SIZE(1);
    CHECK_ARGUMENT_TYPE(0, Expr::Str);
    const std::string &secName = stmt->arguments[0].s;
    Section *sec = getSection(secName);
    if (!sec)
      THROW(AssemblyError, "unknown section", secName);
    curSec = sec;
    return;
  }

  if (name == ".db" || name == ".dh" || name == ".dw" || name == ".dq") {
    checkCurSecName({".rodata", ".data"});
    unsigned n = stmt->arguments.size();
    if (name == ".dh")
      n *= 2;
    else if (name == ".dw")
      n *= 4;
    else if (name == ".dq")
      n *= 8;
    curSec->offset += n;
    curSec->stmts.emplace_back(std::move(stmt));
    return;
  }

  if (name == ".ascii" || name == ".asciz") {
    checkCurSecName({".rodata", ".data"});
    unsigned n = 0;
    for (unsigned i = 0; i < stmt->arguments.size(); ++i) {
      CHECK_ARGUMENT_TYPE(i, Expr::Str);
      n += stmt->arguments[i].s.size();
      if (name == ".asciz")
        ++n;
    }
    curSec->offset += n;
    curSec->stmts.emplace_back(std::move(stmt));
    return;
  }

  if (name == ".fill") {
    checkCurSecName({".rodata", ".data", ".bss"});
    unsigned n = stmt->arguments.size();
    if (n < 1 || n > 3)
      THROW(AssemblyError, "1/2/3 arguments expected", *stmt);

    ExprVal v0 = evalExpr(stmt->arguments[0]);
    if (!v0.isInt() || v0.getI() < 0)
      INVALID_STATEMENT();
    unsigned repeat = v0.getI();
    if (!repeat)
      return;

    unsigned size = 1;
    if (n >= 2) {
      ExprVal v1 = evalExpr(stmt->arguments[1]);
      if (!v1.isInt() || !v1.getI())
        INVALID_STATEMENT();
      size = v1.getI();
      if (size != 1 && size != 2 && size != 4 && size != 8)
        INVALID_STATEMENT();
    }

    curSec->offset += repeat * size;
    curSec->stmts.emplace_back(std::move(stmt));
    return;
  }

  if (name == ".align") {
    CHECK_CURRENT_SECTION();
    CHECK_ARGUMENTS_SIZE(1);
    ExprVal v = evalExpr(stmt->arguments[0]);
    if (!v.isInt() || v.getI() > Section::Alignment || v.getI() < 0)
      INVALID_STATEMENT();
    if (v.getI() == 0)
      return;
    stmt->offset = curSec->offset;
    curSec->offset = P2ALIGN(curSec->offset, v.getI());
    curSec->stmts.emplace_back(std::move(stmt));
    return;
  }

  THROW(AssemblyError, "unknown directive", name);
}

void AssemblerImpl::handleLabel(std::unique_ptr<Statement> stmt) {
  CHECK_CURRENT_SECTION();

  stmt->offset = curSec->offset;

  const std::string &name = stmt->s;
  if (name.size() == 1 && Lexer::isDec(name[0])) {
    curSec->stmts.emplace_back(std::move(stmt));
    return;
  }

  CHECK_DUPLICATED_SYMBOL(name);

  Symbol *sym = addSymbol(name);
  sym->set(curSec, curSec->offset);
}

Section *AssemblerImpl::getSection(const std::string &name) {
  for (Section *sec : sections)
    if (sec->name == name)
      return sec;
  return nullptr;
}

Symbol *AssemblerImpl::getSymbol(const std::string &name) {
  if (symTab.contains(name))
    return symTab[name].get();
  return nullptr;
}

Symbol *AssemblerImpl::addSymbol(const std::string &name) {
  assert(name != ".");
  if (!symTab.contains(name))
    symTab[name].reset(new Symbol(name));
  return symTab[name].get();
}

#undef INVALID_STATEMENT
#undef CHECK_DUPLICATED_SYMBOL
#undef CHECK_OPERANDS_SIZE
#undef CHECK_ARGUMENT_TYPE
#undef CHECK_ARGUMENTS_SIZE
#undef CHECK_CURRENT_SECTION

void Assembler::run() { AssemblerImpl(opts).run(); }

} // namespace assembly
