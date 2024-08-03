import os
import sys
import argparse
import subprocess

from tempfile import mktemp

from pycparser import parse_file

from sema import Sema, NodeVisitorCtx
from codegen import Codegen


def get_sysroot():
    sr = os.path.abspath(sys.argv[0])
    sr = os.path.dirname(sr)
    return os.path.dirname(sr)


sysroot = get_sysroot()
binDir = os.path.join(sysroot, "bin")
includeDir = os.path.join(sysroot, "include")
libDir = os.path.join(sysroot, "lib")


class Action:
    def __init__(self, outfile: str) -> None:
        self._outfile = outfile

    def run(self) -> None:
        raise NotImplementedError()

    def getOutfile(self):
        self.run()
        return self._outfile


class InputAction(Action):
    def __init__(self, outfile: str) -> None:
        super().__init__(outfile)

    def run(self):
        pass


def once(func):
    def _f(*args, **kwargs):
        if not _f.done:
            _f.done = True
            func(*args, **kwargs)

    _f.done = False
    return _f


class CompileAction(Action):
    def __init__(self, inact: Action, outfile: str = None) -> None:
        super().__init__(outfile)
        self._inact = inact

    @once
    def run(self):
        infile = self._inact.getOutfile()
        if not self._outfile:
            assert infile.endswith(".c")
            self._outfile = infile[:-2] + ".s"

        ast = parse_file(infile, use_cpp=True, cpp_args=f"-nostdinc -I{includeDir}")

        ctx = NodeVisitorCtx()
        sm = Sema(ctx)
        sm.visit(ast)

        cg = Codegen(ctx)
        cg.visit(ast)

        with open(self._outfile, "w") as ofs:
            cg.save(ofs)


class AssembleAction(Action):
    def __init__(self, inact: Action, outfile: str = None) -> None:
        super().__init__(outfile)
        self._inact = inact

    @once
    def run(self):
        infile = self._inact.getOutfile()
        if not self._outfile:
            assert infile.endswith(".s")
            self._outfile = infile[:-2] + ".o"

        exe = os.path.join(binDir, "rrisc32-as")
        subprocess.run([exe, "-o", self._outfile, infile])


class LinkAction(Action):
    def __init__(self, inacts: list[Action], outfile: str) -> None:
        super().__init__(outfile)
        self._inacts = inacts

    @once
    def run(self):
        infiles = [inact.getOutfile() for inact in self._inacts]
        exe = os.path.join(binDir, "rrisc32-link")
        subprocess.run([exe, "-o", self._outfile, " ".join(infiles)])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-S", action="store_true", help="Compile only; do not assemble or link.")
    parser.add_argument("-c", action="store_true", help="Compile and assemble, but do not link.")
    parser.add_argument("-o", metavar="<outfile>")
    parser.add_argument("infiles", metavar="<infile>", nargs="+")

    args = parser.parse_args()
    infiles: list[str] = args.infiles

    for infile in infiles:
        if infile.endswith(".c") or infile.endswith(".s") or infile.endswith(".o"):
            continue
        raise Exception("input files should be *.c, *.s or *.o")

    actions: list[Action] = []
    if args.S:
        if args.o:
            if len(infiles) > 0:
                raise Exception("expect a single input file when both -S and -o are specified")

            infile = infiles[0]
            if infile.endswith(".c"):
                actions.append(CompileAction(InputAction(infile), args.o))
        else:
            for infile in infiles[0]:
                if infile.endswith(".c"):
                    actions.append(CompileAction(InputAction(infile)))

    elif args.c:
        if args.o:
            if len(infiles) > 0:
                raise Exception("expect a single input file when both -c and -o are specified")

            infile = infiles[0]
            if infile.endswith(".c"):
                actions.append(
                    AssembleAction(CompileAction(InputAction(infile), mktemp(".s")), args.o)
                )
            elif infile.endswith(".s"):
                actions.append(AssembleAction(InputAction(infile), args.o))
        else:
            for infile in infiles:
                if infile.endswith(".c"):
                    actions.append(AssembleAction(CompileAction(InputAction(infile), mktemp(".s"))))
                elif infile.endswith(".s"):
                    actions.append(AssembleAction(InputAction(infile)))

    else:
        if not args.o:
            raise Exception("expect an output file")

        arr = []
        for infile in infiles:
            if infile.endswith(".c"):
                arr.append(
                    AssembleAction(
                        CompileAction(InputAction(infile), mktemp(".s")),
                        mktemp(".o"),
                    )
                )
            elif infile.endswith(".s"):
                arr.append(AssembleAction(InputAction(infile), mktemp(".o")))
            elif infile.endswith(".o"):
                arr.append(InputAction(infile))

        actions.append(LinkAction(arr, args.o))

    for act in actions:
        act.run()


if __name__ == "__main__":
    main()
