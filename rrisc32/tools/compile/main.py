import os
import sys
import argparse
import subprocess
import tarfile
import glob
import tempfile

from pycparser import parse_file

from sema import Sema, NodeVisitorCtx
from codegen import Codegen


def get_sysroot():
    sr = os.path.abspath(sys.argv[0])
    sr = os.path.dirname(sr)
    return os.path.dirname(sr)


sysroot = get_sysroot()
binDir = os.path.join(sysroot, "bin")
incDir = os.path.join(sysroot, "include")
libDir = os.path.join(sysroot, "lib")
tmpDir = os.path.join(sysroot, "tmp")


def mktemp(suffix: str):
    return tempfile.mktemp(suffix=suffix, dir=tmpDir)


def mkdtemp(suffix: str):
    return tempfile.mkdtemp(suffix=suffix, dir=tmpDir)


class Action:
    def getOutfile(self):
        raise NotImplementedError()


class MOAction:
    def getOutfiles(self) -> list[str]:
        raise NotImplementedError()


class InputAction(Action):
    def __init__(self, outfile: str) -> None:
        self._outfile = outfile

    def getOutfile(self):
        return self._outfile


def once(func):
    def _f(*args, **kwargs):
        if not _f.done:
            _f.done = True
            func(*args, **kwargs)

    _f.done = False
    return _f


class CompileAction(Action):
    def __init__(self, inact: Action, outfile: str = None) -> None:
        self._inact = inact
        self._outfile = outfile

    @once
    def run(self):
        infile = self._inact.getOutfile()
        if not self._outfile:
            assert infile.endswith(".c")
            self._outfile = infile[:-2] + ".s"

        ast = parse_file(infile, use_cpp=True, cpp_args=["-nostdinc", f"-I{incDir}"])

        ctx = NodeVisitorCtx()
        sm = Sema(ctx)
        sm.visit(ast)

        cg = Codegen(ctx)
        cg.visit(ast)

        with open(self._outfile, "w") as ofs:
            cg.save(ofs)

    def getOutfile(self):
        self.run()
        return self._outfile


class AssembleAction(Action):
    def __init__(self, inact: Action, outfile: str = None) -> None:
        self._inact = inact
        self._outfile = outfile

    @once
    def run(self):
        infile = self._inact.getOutfile()
        if not self._outfile:
            assert infile.endswith(".s")
            self._outfile = infile[:-2] + ".o"

        exe = os.path.join(binDir, "rrisc32-as")
        subprocess.run([exe, "-o", self._outfile, infile])

    def getOutfile(self):
        self.run()
        return self._outfile


def archive(infiles: list[str], outfile: str):
    with tarfile.open(outfile, "w|") as tf:
        for infile in infiles:
            name = os.path.basename(infile)
            with open(infile) as ifs:
                tf.addfile(tarfile.TarInfo(name), ifs)


def extract(infile: str) -> list[str]:
    with tarfile.open(infile, "r|") as tf:
        outdir = mkdtemp(".a")
        tf.extractall(outdir)
        outfiles = glob.glob(os.path.join(outdir, "*.o"))
        return outfiles


class ExtractAction(MOAction):
    def __init__(self, inact: Action) -> None:
        self._inact = inact

    def getOutfiles(self) -> list[str]:
        infile = self._inact.getOutfile()
        outfiles = extract(infile)
        return outfiles


class MIAction(Action):
    def __init__(self, inacts: list[Action | MOAction], outfile: str) -> None:
        self._inacts = inacts
        self._outfile = outfile

    def getInfiles(self) -> list[str]:
        infiles = []
        for inact in self._inacts:
            match inact:
                case Action():
                    infiles.append(inact.getOutfile())
                case MOAction():
                    infiles.extend(inact.getOutfiles())
        return infiles


class LinkAction(MIAction):
    def __init__(self, inacts: list[Action | MOAction], outfile: str) -> None:
        super().__init__(inacts, outfile)

    @once
    def run(self):
        infiles = self.getInfiles()
        exe = os.path.join(binDir, "rrisc32-link")
        subprocess.run([exe, "-o", self._outfile, " ".join(infiles)])

    def getOutfile(self):
        self.run()
        return self._outfile


class ArchiveAction(MIAction):
    def __init__(self, inacts: list[Action | MOAction], outfile: str) -> None:
        super().__init__(inacts, outfile)

    @once
    def run(self):
        infiles = self.getInfiles()
        archive(infiles, self._outfile)

    def getOutfile(self):
        self.run()
        return self._outfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--compile", action="store_true", help="Compile only; do not assemble or link."
    )
    parser.add_argument(
        "--assemble", action="store_true", help="Compile and assemble, but do not link."
    )
    parser.add_argument(
        "--archive", action="store_true", help="Create a static library rather than an executable."
    )
    parser.add_argument("--optimize", action="store_true", help="Enable optimizations.")
    parser.add_argument("-o", metavar="<outfile>")
    parser.add_argument("infiles", metavar="<infile>", nargs="+")

    args = parser.parse_args()
    infiles: list[str] = args.infiles

    for infile in infiles:
        if infile[-2:] in [".c", ".s", ".o", ".a"]:
            continue
        raise Exception("input files should be *.c, *.s, *.o or *.a")

    actions: list[Action] = []
    if args.compile:
        if args.o:
            if len(infiles) != 1:
                raise Exception(
                    "expect a single input file when both --compile and -o are specified"
                )

            infile = infiles[0]
            if infile.endswith(".c"):
                actions.append(CompileAction(InputAction(infile), args.o))
        else:
            for infile in infiles:
                if infile.endswith(".c"):
                    actions.append(CompileAction(InputAction(infile)))

    elif args.assemble:
        if args.o:
            if len(infiles) != 1:
                raise Exception(
                    "expect a single input file when both --assemble and -o are specified"
                )

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

        inacts = []
        for infile in infiles:
            if infile.endswith(".c"):
                inacts.append(
                    AssembleAction(
                        CompileAction(InputAction(infile), mktemp(".s")),
                        mktemp(".o"),
                    )
                )
            elif infile.endswith(".s"):
                inacts.append(AssembleAction(InputAction(infile), mktemp(".o")))
            elif infile.endswith(".o"):
                inacts.append(InputAction(infile))
            elif infile.endswith(".a"):
                inacts.append(ExtractAction(InputAction(infile)))

        if args.archive:
            actions.append(ArchiveAction(inacts, args.o))
        else:
            actions.append(LinkAction(inacts, args.o))

    for act in actions:
        act.run()


if __name__ == "__main__":
    main()
