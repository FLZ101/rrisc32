#include "emulation.h"

#include <cstdio>

#include "elf.h"
#include "rrisc32.h"

#define DEBUG_TYPE "emulation"

namespace Reg = rrisc32::Reg;

namespace emulation {

#define OUT_OF_MEMOEY_RANGE(...)                                               \
  THROW(EmulationError, "out of memory range", ##__VA_ARGS__)

struct Mapping {
  u64 lo = 0, hi = 0;
  elf::Elf_Word flags = 0;
};

void fclose(std::FILE *fp) { std::fclose(fp); }

class Emulator : public rrisc32::Machine {
public:
  Emulator(const EmulatorOpts &o)
      : o(o), rrisc32::Machine(o.nPages * elf::RRISC32_PAGE_SIZE) {
    for (unsigned i = 0; i < o.nFiles + 3; ++i) {
      files.emplace_back(std::move(FilePointer(nullptr, &fclose)));
    }
  }

  void run();

  void befWm(u32 addr, unsigned n) override {}
  void aftWm(u32 addr, unsigned n) override {}

  void ecall() override;
  void ebreak() override {}

private:
  void syscallOpen();
  void syscallClose();
  void syscallRead();
  void syscallWrite();
  void syscallExit();

  std::FILE *checkFD(s32 fd);
  s32 findFD();

  void wRet(s32 value);
  s32 rArg(unsigned i);
  std::string rStr(u32 addr);

  void load();

  u64 getStackTop() { return rr(Reg::sp); }

  // clang-format off
  const std::vector<decltype(std::mem_fn(&Emulator::syscallOpen))> syscalls
  {
    std::mem_fn(&Emulator::syscallOpen),
    std::mem_fn(&Emulator::syscallClose),
    std::mem_fn(&Emulator::syscallRead),
    std::mem_fn(&Emulator::syscallWrite),
    std::mem_fn(&Emulator::syscallExit),
  };
  // clang-format on

  bool exited = false;

  using FilePointer = std::unique_ptr<std::FILE, decltype(&fclose)>;
  std::vector<FilePointer> files;

  u32 heapBot = 0, heapTop = 0;
  u32 stackBot = 0;

  std::vector<Mapping> mappings;
  EmulatorOpts o;
};

void Emulator::ecall() {
  u32 i = rr(Reg::a0);
  if (i < syscalls.size())
    syscalls[i](this);
  else
    THROW(EmulationError, "unknown system call", i);
}

#define RET(x)                                                                 \
  do {                                                                         \
    wRet(x);                                                                   \
    return;                                                                    \
  } while (false)

void Emulator::syscallOpen() {
  s32 fd = findFD();
  if (fd < 0) {
    LOG("too many open files");
    RET(fd);
  }
  std::string filename = o.fsRoot + "/" + rStr(rArg(1));
  std::string mode = rStr(rArg(2));
  FilePointer fp(std::fopen(filename.data(), mode.data()), &fclose);
  if (!fp) {
    LOG("can not open", filename);
    RET(-1);
  }
  files[fd] = std::move(fp);
  RET(fd);
}

void Emulator::syscallClose() {
  s32 fd = rArg(1);
  if (checkFD(fd))
    files[fd].reset();
}

void Emulator::syscallRead() {
  u32 buffer = rArg(1);
  u32 size = rArg(2);
  u32 count = rArg(3);
  s32 fd = rArg(4);
  std::FILE *fp = checkFD(fd);
  if (!size || !count || !fp)
    RET(0);

  u32 i = 0;
  for (; i < size && i < count; ++i) {
    int c = std::fgetc(fp);
    if (c == EOF)
      break;
    wm<u8>(buffer + i, c);
  }
  RET(i);
}

void Emulator::syscallWrite() {
  u32 buffer = rArg(1);
  u32 size = rArg(2);
  u32 count = rArg(3);
  s32 fd = rArg(4);
  std::FILE *fp = checkFD(fd);
  if (!size || !count || !fp)
    RET(0);

  u32 i = 0;
  for (; i < size && i < count; ++i) {
    u8 c = rm<u8>(buffer + i);
    if (EOF == std::fputc(c, fp))
      break;
  }
  RET(i);
}

void Emulator::syscallExit() {
  s32 status = rArg(1);
  exited = true;
  LOG("EXIT", status);
}

std::FILE *Emulator::checkFD(s32 fd) {
  switch (fd) {
  case 0:
    return stdin;
  case 1:
    return stdout;
  case 2:
    return stderr;
  default:
    if (3 <= fd && fd < files.size()) {
      std::FILE *fp = files[fd].get();
      if (fp)
        return fp;
    }
    LOG("invalid FD", fd);
    return nullptr;
  }
}

s32 Emulator::findFD() {
  for (s32 i = 3; i < files.size(); ++i)
    if (!files[i])
      return i;
  return -1;
}

void Emulator::wRet(s32 value) { wr(Reg::a0, value); }

s32 Emulator::rArg(unsigned i) {
  assert(0 < i && i <= 7);
  return rr(Reg::a0 + i);
}

std::string Emulator::rStr(u32 addr) {
  std::string s;
  do {
    s.push_back(rm<char>(addr++));
  } while (s.back());
  return s;
}

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
  wr(Reg::sp, stackBot);
}

void Emulator::run() {
  load();

  while (!exited) {
    u32 b = fetch();
    LOG("FETCH", rrisc32::test::decode(b));
    rrisc32::execute(*this, b);
  }
}

void emulate(EmulatorOpts o) {
  if (o.nPages < MinNPages || o.nPages > MaxNPages)
    THROW(EmulationError, "invalid number of pages", o.nPages);
  Emulator(o).run();
}

} // namespace emulation
