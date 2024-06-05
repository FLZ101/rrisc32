#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <initializer_list>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

#include "CLI/CLI.hpp"

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

template <typename T>
void __join(std::ostringstream &os, std::string sep, T x) {
  os << x;
}

template <typename T, typename... Args>
void __join(std::ostringstream &os, std::string sep, T x, Args... args) {
  os << x << sep;
  __join(os, sep, args...);
}

template <typename T, typename... Args>
std::string join(std::string sep, T x, Args... args) {
  std::ostringstream os;
  __join(os, sep, x, args...);
  return os.str();
}

template <typename T> std::string joinArr(std::string sep, T arr) {
  std::ostringstream os;
  bool first = true;
  for (auto &x : arr) {
    if (first)
      first = false;
    else
      os << sep;
    os << x;
  }
  return os.str();
}

template <typename T> std::string toString(T x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

template <typename T>
std::string toHexStr(T x, bool ox = false, bool smart = true) {
  std::ostringstream os;

  unsigned n = sizeof(T);
  if (smart) {
    u64 y = 0;
    switch (sizeof(x)) {
    case 1:
      y = u8(x);
      break;
    case 2:
      y = u16(x);
      break;
    case 4:
      y = u32(x);
      break;
    case 8:
      y = u64(x);
    }

    n = 0;
    do {
      ++n;
    } while (y >>= 8);
  }

  if (ox)
    os << "0x";
  os << std::hex << std::setw(n * 2) << std::setfill('0') << x;
  return os.str();
}

std::string substr(const std::string &s, int i);
std::string substr(const std::string &s, int i, int j);

std::string trim(const std::string &s);

class Exception : public std::exception {
public:
  Exception(const std::string &msg) : std::exception(), msg(msg) {}

  const char *what() const noexcept override { return msg.c_str(); }

private:
  std::string msg;
};

#define DEFINE_EXCEPTION(NAME)                                                 \
  class NAME : public Exception {                                              \
  public:                                                                      \
    NAME(const std::string &msg) : Exception(msg) {}                           \
  };

// https://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html
//
// if the variable arguments are omitted or empty, the ‘##’ operator
// causes the preprocessor to remove the comma before it
#ifndef NDEBUG
#define THROW(NAME, ...)                                                       \
  do {                                                                         \
    throw NAME(join(" : ", #NAME, __FILE__ + (":" + std::to_string(__LINE__)), \
                    ##__VA_ARGS__));                                           \
  } while (false)
#else
#define THROW(NAME, ...)                                                       \
  do {                                                                         \
    throw NAME(toString(" : ", #NAME, ##__VA_ARGS__));                         \
  } while (false)
#endif

DEFINE_EXCEPTION(Unreachable)

#define UNREACHABLE(...)                                                       \
  do {                                                                         \
    THROW(Unreachable, ##__VA_ARGS__);                                         \
    __builtin_unreachable();                                                   \
  } while (false)

#ifndef NDEBUG
extern std::string debugType;

void setDebugType(const char *);

bool chkDebugType(const char *);

#define DEBUG_WITH_TYPE(TYPE, X)                                               \
  do {                                                                         \
    if (chkDebugType(TYPE)) {                                                  \
      X;                                                                       \
    }                                                                          \
  } while (false)

#define ADD_DEBUG_OPT(app)                                                     \
  do {                                                                         \
    (app).add_option("--debug", debugType);                                    \
  } while (false)
#else
#define setDebugType(X)                                                        \
  do {                                                                         \
    (void)(X);                                                                 \
  } while (false)

#define chkDebugType(X) (false)

#define DEBUG_WITH_TYPE(TYPE, X)                                               \
  do {                                                                         \
  } while (false)

#define ADD_DEBUG_OPT(app)                                                     \
  do {                                                                         \
  } while (false)
#endif

#define DEBUG(X) DEBUG_WITH_TYPE(DEBUG_TYPE, X)
#endif
