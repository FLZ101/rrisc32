#ifndef LINKAGE_H
#define LINKAGE_H

#include "util.h"

namespace linkage {

DEFINE_EXCEPTION(LinkageError)

struct LinkerOpts {
  std::vector<std::string> inFiles;
  std::string outFile;
};

void link(const LinkerOpts &o);

} // namespace linkage

#endif
