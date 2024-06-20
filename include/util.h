#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

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
#define _MESSAGE(NAME, ...)                                                    \
  join(" : ", #NAME, __FILE__ + (":" + toString(__LINE__)), ##__VA_ARGS__)
#else
#define _MESSAGE(NAME, ...) join(" : ", #NAME, ##__VA_ARGS__)
#endif

#define THROW(NAME, ...)                                                       \
  do {                                                                         \
    throw NAME(_MESSAGE(NAME, ##__VA_ARGS__));                                 \
  } while (false)

// clang-format off

#define TRY() try {

#define RETHROW(NAME, ...) \
  } catch (Exception &ex) { \
    throw NAME(_MESSAGE(NAME, ##__VA_ARGS__) + "\n" + ex.what()); \
  }

#define CATCH() \
  } catch (Exception &ex) { \
    std::cerr << ex.what() << "\n"; \
  }

// clang-format on

DEFINE_EXCEPTION(Unreachable)

#define UNREACHABLE(...)                                                       \
  do {                                                                         \
    THROW(Unreachable, ##__VA_ARGS__);                                         \
    __builtin_unreachable();                                                   \
  } while (false)

template <typename T>
void __join(std::ostringstream &os, const std::string &sep, const T &x) {
  os << x;
}

template <typename T, typename... Args>
void __join(std::ostringstream &os, const std::string &sep, T x,
            const Args &...args) {
  os << x << sep;
  __join(os, sep, args...);
}

template <typename T, typename... Args>
std::string join(const std::string &sep, T x, const Args &...args) {
  std::ostringstream os;
  __join(os, sep, x, args...);
  return os.str();
}

template <typename It>
std::string joinSeq(const std::string &sep, It beg, It end) {
  std::ostringstream os;
  bool first = true;
  for (; beg != end; ++beg) {
    if (first)
      first = false;
    else
      os << sep;
    os << *beg;
  }
  return os.str();
}

template <typename T>
std::string joinSeq(const std::string &sep, const T &arr) {
  return joinSeq(sep, std::begin(arr), std::end(arr));
}

template <typename T> std::string toString(const T &x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

namespace std {
template <typename T> ostream &operator<<(ostream &os, const unique_ptr<T> &x) {
  os << *x;
  return os;
}
} // namespace std

template <typename T>
std::string toHexStr(T x, bool ox = false, bool smart = true) {
  std::ostringstream os;

  u64 y = 0;
  switch (sizeof(T)) {
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
    break;
  default:
    UNREACHABLE();
  }

  unsigned n = sizeof(T);
  u64 z = y;
  if (smart) {
    n = 0;
    do {
      ++n;
    } while (z >>= 8);
  }

  if (ox)
    os << "0x";
  os << std::hex << std::nouppercase << std::setw(n * 2) << std::setfill('0')
     << y;
  return os.str();
}

std::string substr(const std::string &s, ssize_t i);
std::string substr(const std::string &s, ssize_t i, ssize_t j);

std::string_view subview(const std::string &s, ssize_t i);
std::string_view subview(const std::string &s, ssize_t i, ssize_t j);

std::string trim(const std::string &s);

s64 parseInt(const std::string &str, bool hex = false);

std::string escape(char c, bool wrap = true);
std::string escape(const std::string &s, bool wrap = true);
std::string unescape(const std::string &s);

size_t find(const std::string &s, const std::string &t, size_t pos = 0);

std::vector<std::string> split(const std::string &s, const std::string &sep);

#define ALIGN(x, n) (((x) + ((n) - 1)) & ~((n) - 1))
#define P2ALIGN(x, n) ALIGN((x), (1 << (n)))

bool log2(u64 x, u8 &y);

template <typename T, typename Container = std::initializer_list<T>>
bool isOneOf(const T &x, const Container &elements) {
  for (const auto &element : elements)
    if (element == x)
      return true;
  return false;
}

class ByteBuffer {
public:
  ByteBuffer() {}

  void append(s64 value, unsigned size) {
    switch (size) {
    case 1:
      appendInt<u8>(value & 0xff);
      break;
    case 2:
      appendInt<u16>(value & 0xffff);
      break;
    case 4:
      appendInt<u32>(value & 0xffffffff);
      break;
    case 8:
      appendInt<u64>(value);
      break;
    default:
      UNREACHABLE();
    }
  }

  template <typename T> void appendInt(T x) {
    switch (sizeof(T)) {
    case 1:
    case 2:
    case 4:
    case 8:
      break;
    default:
      UNREACHABLE();
    }
    unsigned i = 0;
    while (i < sizeof(T)) {
      s.push_back(x & 0xff);
      ++i;
      x >>= 8;
    }
  }

  void append(const std::string &str) { s.append(str); }

  void append(char c) { s.push_back(c); }
  void append(size_t n, char c) { s.append(n, c); }
  void append(const char *str, size_t n) { s.append(str, n); }

  const std::string &getData() const { return s; }

  void extend(size_t n) {
    if (n > s.size())
      s.append(n - s.size(), '\0');
  }

  size_t size() const { return s.size(); }

private:
  std::string s;
};

template <typename T, unsigned N> inline T signExt(T x) {
  struct {
    T x : N;
  } s;
  return s.x = x;
}

s32 hi20(s32 x);
s32 lo12(s32 x);

template <typename Key, typename T> class CacheMap {
public:
  CacheMap() {}

  bool contains(const Key &k) const { return m.contains(k); }

  T &operator[](const Key &k) {
    if (!contains(k))
      v.push_back(k);
    return m[k];
  }

  const std::vector<Key> &keys() const { return v; }

private:
  std::vector<Key> v;
  std::map<Key, T> m;
};

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
#else
#define setDebugType(X)                                                        \
  do {                                                                         \
    (void)(X);                                                                 \
  } while (false)

#define chkDebugType(X) (false)

#define DEBUG_WITH_TYPE(TYPE, X)                                               \
  do {                                                                         \
  } while (false)
#endif

#define DEBUG(X) DEBUG_WITH_TYPE(DEBUG_TYPE, X)
#endif
