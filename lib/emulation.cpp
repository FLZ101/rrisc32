#include "emulation.h"

#include "elf.h"
#include "rrisc32.h"

namespace emulation {

#define OUT_OF_MEMOEY_RANGE(...)                                               \
  THROW(EmulationError, "out of memory range", ##__VA_ARGS__)

struct Mapping {
  u64 lo = 0, hi = 0;
  elf::Elf_Word flags = 0;
};

class Emulator : public rrisc32::Machine {
public:
  Emulator(const EmulatorOpts &o)
      : o(o), rrisc32::Machine(o.nPages * elf::RRISC32_PAGE_SIZE) {}

  void run();

  void befWm(u32 addr, unsigned n) override {}
  void aftWm(u32 addr, unsigned n) override {}

  void ecall() override {}
  void ebreak() override {}

private:
  void load();

  u64 getStackTop() { return rr(rrisc32::Reg::sp); }

  u32 heapBot = 0, heapTop = 0;
  u32 stackBot = 0;

  std::vector<Mapping> mappings;
  EmulatorOpts o;
};

void Emulator::load() {
  elf::RRisc32Reader reader(o.exeFile);

  for (elf::segment *seg : reader.getLoadSegments()) {
    elf::Elf64_Addr addr = seg->get_virtual_address();
    Mapping map = {.lo = addr,
                   .hi = addr + seg->get_memory_size() - 1,
                   .flags = seg->get_flags()};
    if (map.hi >= memSize)
      OUT_OF_MEMOEY_RANGE("segment " + toString(seg->get_index()), map.lo,
                          map.hi);
    mappings.push_back(map);

    if (seg->get_file_size() > 0)
      std::memcpy(mem + map.lo, seg->get_data(), seg->get_file_size());
  }

  if (mappings.empty())
    THROW(EmulationError, "no segment");
  assert(mappings[0].lo == elf::RRISC32_ENTRY);

  heapBot = heapTop = ((mappings.back().hi >> elf::RRISC32_PAGE_ALIGN) + 1)
                      << elf::RRISC32_PAGE_ALIGN;
  stackBot = (o.nPages - 1) << elf::RRISC32_PAGE_ALIGN;

  if (heapTop >= stackBot)
    THROW(EmulationError, "no space for heap");

  wi(elf::RRISC32_ENTRY);
  wr(rrisc32::Reg::sp, stackBot);
}

void Emulator::run() {
  load();

  int i = 0;
  while (i++ < 10) {
    u32 b = fetch();
    std::cout << rrisc32::test::decode(b) << "\n";
  }
}

void emulate(const EmulatorOpts &o) {
  if (o.nPages < MinNPages || o.nPages > MaxNPages)
    THROW(EmulationError, "invalid number of pages", o.nPages);
  Emulator(o).run();
}

} // namespace emulation
