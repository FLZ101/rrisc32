#include "util.h"

std::string substr(const std::string &s, int i) {
  return substr(s, i, s.size());
}

std::string substr(const std::string &s, int i, int j) {
  int n = s.size();
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
