#include "cli.h"
#include "emulation.h"

using namespace emulation;

class Formatter : public CLI::Formatter {
public:
  Formatter() : CLI::Formatter() {}

  std::string make_usage(const CLI::App *app, std::string name) const override {
    return trim(CLI::Formatter::make_usage(app, name)) + " [arg]...\n";
  }
};

int main(int argc, char *argv[]) {
  EmulatorOpts o;

  CLI::App app;
  app.add_option("<exe_file>", o.exeFile)->required()->type_name("");
  ADD_DEBUG_OPT(app);
  app.formatter(std::make_shared<Formatter>());
  CLI11_PARSE(app, argc, argv);

  for (const std::string &s : app.remaining())
    o.args.push_back(s);

  TRY()
  emulate(o);
  CATCH()
}
