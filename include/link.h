#ifndef LINK_H
#define LINK_H

#include "util.h"

namespace link {

DEFINE_EXCEPTION(LinkError)

struct LinkerOpts {
  std::vector<std::string> inFiles;
  std::string outFiles;
};

void link(const LinkerOpts &o);

}

#endif
