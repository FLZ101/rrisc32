import re
import os
import argparse

from typing import Callable


def gen(nr: int, line: str, _p: Callable[[str], None]):
    m = re.search(r"\((.*)\);$", line)
    assert m

    n = 0
    for x in m.group(1).split(","):
        x = x.strip()
        if x:
            n += 1

    _p("  #pragma ASM \\")
    _p(f"    li a0, {nr}; \\")
    for i in range(1, n + 1):
        _p(f"    lw a{i}, fp, {4+i*4}; \\")
    _p("    ecall")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("outfile")
    args = parser.parse_args()

    lines = []
    with open("include/syscall.inc") as ifs:
        for line in ifs:
            line = line.strip()
            if line.endswith(";"):
                lines.append(line)

    outfile = args.outfile
    outdir = os.path.dirname(outfile)
    os.makedirs(outdir, exist_ok=True)

    with open(outfile, "w") as ofs:
        def _p(s):
            print(s, file=ofs)

        _p('#include "rrisc32.h"')
        _p("")
        for nr, line in enumerate(lines):
            _p(line[:-1])
            _p("{")
            gen(nr, line, _p)
            _p("}")
            _p("")


if __name__ == "__main__":
    main()
