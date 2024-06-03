#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>

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

#define CATCH(X)                                                               \
  do {                                                                         \
    try {                                                                      \
      X;                                                                       \
    } catch (Exception & ex) {                                                 \
      std::cerr << "!!! " << ex.what() << "\n";                                \
    }                                                                          \
  } while (false)

DEFINE_EXCEPTION(Unreachable)

#define UNREACHABLE(...) THROW(NAME, __VA_ARGS__)

#endif
