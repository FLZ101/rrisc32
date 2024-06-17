#ifndef CLI_H
#define CLI_H

#include "CLI/CLI.hpp"

#ifndef NDEBUG
#define ADD_DEBUG_OPT(app)                                                     \
  do {                                                                         \
    (app)                                                                      \
        .add_option("--debug", debugType, "Set debug types")                   \
        ->type_name("TYPE[,...]");                                             \
  } while (false)
#else
#define ADD_DEBUG_OPT(app)                                                     \
  do {                                                                         \
  } while (false)
#endif

#endif
