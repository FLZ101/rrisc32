import io

from sema import *


class Fragment:
    def __init__(self, name: str) -> None:
        self._lines = []
        self._name = name

    def add(self, s: str | list[str], *, indent="    "):
        if type(s) is list:
            for _ in s:
                self.add(_, indent=indent)
        else:
            self._lines.append(indent + s)

    def addEmptyLine(self):
        self._lines.append("")

    def addRaw(self, s: str, *, indent="    "):
        for line in s.splitlines():
            line = line.strip()
            if line == "":
                continue
            if line.endswith(":"):
                self._lines.append(line)
            else:
                self.add(line, indent=indent)

    def addConstant(self, v: Constant):
        match v:
            case IntConstant():
                c = "bhwq"[log2(v.getType().size())]
                self.add(f".d{c} {v._i}")
            case PtrConstant():
                self.add(f".dw {v._i}")
            case SymConstant():
                if v._offset == 0:
                    self.add(f".dw ${v._name}")
                else:
                    self.add(f".dw +(${v._name} {v._offset})")
            case _:
                unreachable()

    def addLabel(self, s: str):
        self.add(s + ":", indent="")

    def addLocalLabel(self):
        s = f".L_{self._name}_{self._i_local}"
        self.addLabel(s)
        return s

    @property
    def _i_local(self, *, _d={"i": 0}):
        _d["i"] += 1
        return _d["i"]

    def save(self, o: io.StringIO):
        for line in self._lines:
            print(line, file=o)
        print("", file=o)


class Section:
    def __init__(self, name: str) -> None:
        assert name in [".text", ".rodata", ".data", ".bss"]
        self._name = name
        self._fragments: list[Fragment] = []

        self.addFragment()
        self.curFragment.add(name)

    def addFragment(self) -> Fragment:
        self._fragments.append(Fragment(f"{self.name[1]}{len(self._fragments)}"))
        return self._fragments[-1]

    def ownFragment(self) -> Fragment:
        res = self.addFragment()
        self.addFragment()
        return res

    @property
    def curFragment(self):
        return self._fragments[-1]

    def add(self, s: str | list[str], *, indent="    "):
        self.curFragment.add(s, indent=indent)

    def addEmptyLine(self):
        self.curFragment.addEmptyLine()

    def addRaw(self, s: str, *, indent="    "):
        self.curFragment.addRaw(s, indent=indent)

    def addConstant(self, v: Constant):
        self.curFragment.addConstant(v)

    def addLabel(self, s: str):
        self.curFragment.addLabel(s)

    def addLocalLabel(self):
        return self.curFragment.addLocalLabel()

    def save(self, o: io.StringIO):
        for fragment in self._fragments:
            fragment.save(o)

class Asm:
    def __init__(self) -> None:
        self._secText = Section(".text")
        self._secRodata = Section(".rodata")
        self._secData = Section(".data")
        self._secBss = Section(".bss")

        self._strPool: dict[str, str] = {}

    def addStr(self, sLit: StrLiteral) -> str:
        s = sLit._s
        if s not in self._strPool:
            label = self._secRodata.addLocalLabel()
            self._secRodata.add(f".asciz {sLit._sOrig}")
            self._strPool[s] = label
        sLit._label = self._strPool[s]

    def emit(self, s: str | list[str]):
        self._secText.add(s)

    def checkImm(self, i: int, n: int):
        mins = (1 << (n - 1)) - (1 << n)
        maxs = (1 << (n - 1)) - 1
        return mins <= i <= maxs

    def checkImmI(self, i: int):
        return self.checkImm(i, 12)

    def checkImmS(self, i: int):
        return self.checkImm(i, 12)

    def emitFormatI(self, mnemonic: str, rd: str, rs1: str, i: int = 0):
        assert mnemonic in [
            "addi",
            "xori",
            "ori",
            "andi",
            "slli",
            "srli",
            "srai",
            "slti",
            "sltiu",
            "lb",
            "lh",
            "lw",
            "lbu",
            "lhu",
        ]
        if self.checkImmI(i):
            self.emit(f"{mnemonic} {rd}, {rs1}, {i}")
        else:
            assert rs1 not in ["t0", "x5"]

            if mnemonic not in [
                "lb",
                "lh",
                "lw",
                "lbu",
                "lhu",
            ]:

                def _getRMnemonic():
                    _ = str(reversed(mnemonic))
                    _ = _.replace("i", "", 1)
                    _ = str(reversed(_))
                    return _

                self.emit(
                    [
                        f"li t0, {i}",
                        f"{_getRMnemonic()} {rd}, {rs1}, t0",
                    ]
                )
            else:
                self.emit(
                    [
                        f"lui t0, %hi({i})",
                        f"add t0, t0, {rs1}",
                        f"{mnemonic} {rd}, t0, %lo({i})",
                    ]
                )

    # sb/h/w rs2, i(rs1)
    def emitFormatS(self, mnemonic: str, rs2: str, rs1: str, i: int = 0):
        assert mnemonic in ["sb", "sh", "sw"]

        if self.checkImmS(i):
            self.emit(f"{mnemonic} {rs1}, {rs2}, {i}")
        else:
            self.emit(
                [
                    f"lui t0, %hi({i})",
                    f"add t0, t0, {rs1}",
                    f"{mnemonic} t0, {rs2}, %lo({i})",
                ]
            )

    def emitPrelogue(self):
        self.emit(["push ra", "push fp", "mv fp, sp"])

    def emitEpilogue(self):
        self.emit(["mv sp, fp", "pop fp", "pop ra"])

    def emitRet(self):
        self.emitEpilogue()
        self.emit("ret")

    def load(self):
        pass

    def store(self):
        pass

    def save(self, o: io.StringIO):
        self._secText.save(o)
        self._secRodata.save(o)
        self._secData.save(o)
        self._secBss.save(o)


class Codegen(NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        super().__init__(ctx)

        self._asm = Asm()

    def visit_Decl(self, node: c_ast.Decl):
        if not node.name:
            return
        v = self._scope.findSymbol(node.name)
        match v:
            case Function() | ExternVariable():
                pass
            case GlobalVariable() | StaticVariable():
                pass
            case LocalVariable():
                pass
            case _:
                unreachable()
