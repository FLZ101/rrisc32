#include "link.h"

/*

1. arrange/concatenate sections (.text; .rodata; .data; .bss)

   each file
     each section's offset

2. symbols (global; weak)

3. relocations
*/

namespace link {

class Linker {
public:
  Linker(const LinkerOpts &o) : opts(o) {}

  Linker(const Linker &) = delete;
  Linker &operator=(const Linker &) = delete;

  void run() {}

private:
  LinkerOpts opts;
};

void link(const LinkerOpts &o) { Linker(o).run(); }

} // namespace link
