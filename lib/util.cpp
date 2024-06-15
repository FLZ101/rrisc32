#include "util.h"

#include <cctype>

std::string substr(const std::string &s, ssize_t i) {
  return substr(s, i, s.size());
}

std::string substr(const std::string &s, ssize_t i, ssize_t j) {
  ssize_t n = s.size();
  if (i < 0)
    i += n;
  if (i < 0)
    i = 0;
  if (j < 0)
    j += n;
  if (j > n)
    j = n;
  if (i >= j)
    return "";
  return s.substr(i, j - i);
}

std::string trim(const std::string &s) {
  if (s.empty())
    return "";

  unsigned i = 0, j = s.size() - 1;
  while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n')
    ++i;
  while (s[j] == ' ' || s[j] == '\t' || s[j] == '\n')
    --j;
  return substr(s, i, j + 1);
}

s64 parseInt(const std::string &str, bool hex) {
  if (str.starts_with("-"))
    return -parseInt(substr(str, 1), hex);

  s64 i = 0;
  if (hex) {
    for (char c : str) {
      assert(('0' <= c && c <= '9') || ('a' <= c && c <= 'f'));
      i *= 16;
      i += ('0' <= c && c <= '9') ? (c - '0') : (c - 'a' + 10);
    }
  } else {
    for (char c : str) {
      assert('0' <= c && c <= '9');
      i *= 10;
      i += c - '0';
    }
  }
  return i;
}

std::string escape(char c, bool wrap) { return escape(toString(c), wrap); }

std::string escape(const std::string &s, bool wrap) {
  std::string str;

  if (wrap)
    str.push_back('"');

  for (char c : s) {
    switch (c) {
    case '\n':
      str.append("\\n");
      break;
    case '\t':
      str.append("\\t");
      break;
    case '\0':
      str.append("\\0");
      break;
    case '"':
      str.append("\\\"");
      break;
    case '\\':
      str.append("\\\\");
      break;
    default:
      if (std::isprint(c)) {
        str.push_back(c);
        continue;
      }
      str.append("\\x");
      str.append(toHexStr(c));
      break;
    }
  }

  if (wrap)
    str.push_back('"');

  return str;
}

std::string unescape(const std::string &s) {
  std::string str;
  for (unsigned i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '\\') {
      switch (s[++i]) {
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
      case '\\':
        str.push_back('\\');
        break;
      case 'x':
        i += 2;
        str.push_back(char(parseInt(substr(s, i - 1, i + 1), true)));
        break;
      default:
        UNREACHABLE();
      }
    } else {
      str.push_back(c);
    }
  }
  return str;
}

size_t find(const std::string &s, const std::string &t, size_t pos) {
  size_t res = s.find(t, pos);
  if (res > s.size())
    res = s.size();
  return res;
}

std::vector<std::string> split(const std::string &s, const std::string &sep) {
  std::vector<std::string> v;
  if (sep.empty()) {
    for (char c : s)
      v.push_back(std::string({c}));
    return v;
  }

  size_t i = 0;
  while (i < s.size()) {
    size_t j = find(s, sep, i);
    v.push_back(trim(substr(s, i, j)));
    i = j + sep.size();
  }
  return v;
}

s32 hi20(s32 x) { return x >> 12 & 0xfffff; }

s32 lo12(s32 x) { return x & 0xfff; }

#ifndef NDEBUG
std::string debugType;

void setDebugType(const char *s) { debugType = s; }

bool chkDebugType(const char *s) {
  if (debugType.empty() || !s || !*s)
    return false;
  if (debugType == "all")
    return true;
  return debugType == s ||
         debugType.find(std::string(s) + ",") != std::string::npos ||
         debugType.find("," + std::string(s)) != std::string::npos;
}
#endif
