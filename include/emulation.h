#ifndef EMULATION_H
#define EMULATION_H

#include "util.h"

namespace emulation {

DEFINE_EXCEPTION(EmulationError)

const unsigned MinNPages = 1024;
const unsigned MaxNPages = 32 * 1024;

struct EmulatorOpts {
  unsigned nFiles = 0;
  unsigned nPages = 4 * MinNPages;
  std::string exeFile;
  std::vector<std::string> args;
  std::string fsRoot;
};

void emulate(EmulatorOpts o);

} // namespace emulation

#endif
