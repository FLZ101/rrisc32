import io

from sema import *


class TemporaryValue(RValue):
    def __init__(self, ty: Type) -> None:
        super().__init__(ty)


class StackFrameOffset(Constant):
    def __init__(self, i: int, ty: PointerType) -> None:
        super().__init__(ty)
        self._i = i


# https://en.cppreference.com/w/c/language/operator_member_access
class MemoryAccess(LValue):
    def __init__(self, addr: Value) -> None:
        ty = addr.getType()
        assert isinstance(ty, PointerType) and ty.toObject()

        super().__init__(ty._base)
        self._addr = addr


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
        self._fragments.append(Fragment(f"{self._name[1]}{len(self._fragments)}"))
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

        self._builtins = {"memset": 0, "memcpy": 0}

    def addStr(self, sLit: StrLiteral) -> str:
        self._secRodata.addLabel(sLit._label)
        self._secRodata.add(f".asciz {sLit._sOrig}")

    def getLocalLabel(self, name: str, *, _i=[0]):
        _i[0] += 1
        return f".LL{_i[0]}.{name}"

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

    def emitFormatI(self, mnemonic: str, rd: str, rs1: str, i: int = 0, rt: str = "t0"):
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

        if i == 0 and rd == rs1 and mnemonic in ["addi", "xori", "ori"]:
            return

        if self.checkImmI(i):
            self.emit(f"{mnemonic} {rd}, {rs1}, {i}")
        else:
            assert rs1 != rt

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
                        f"li {rt}, {i}",
                        f"{_getRMnemonic()} {rd}, {rs1}, {rt}",
                    ]
                )
            else:
                self.emit(
                    [
                        f"lui {rt}, %hi({i})",
                        f"add {rt}, {rt}, {rs1}",
                        f"{mnemonic} {rd}, {rt}, %lo({i})",
                    ]
                )

    # sb/h/w rs2, i(rs1)
    def emitFormatS(self, mnemonic: str, rs2: str, rs1: str, i: int = 0, rt: str = "t0"):
        assert mnemonic in ["sb", "sh", "sw"]

        if self.checkImmS(i):
            self.emit(f"{mnemonic} {rs1}, {rs2}, {i}")
        else:
            self.emit(
                [
                    f"lui {rt}, %hi({i})",
                    f"add {rt}, {rt}, {rs1}",
                    f"{mnemonic} {rt}, {rs2}, %lo({i})",
                ]
            )

    def addressOf(self, v: LValue):
        match v:
            case GlobalVariable() | StaticVariable() | ExternVariable():
                return SymConstant(v._label, PointerType(v._type))

            case LocalVariable() | Argument():
                return StackFrameOffset(v._offset, PointerType(v._type))

            case StrLiteral():
                assert v._label
                return SymConstant(v._label, PointerType(v.getType()))

            case MemoryAccess():
                return v._addr

            case _:
                unreachable()

    def load(self, v: Value, r1: str = "a0", r2: str = "a1"):
        match v:
            case IntConstant() | PtrConstant():
                sz = v._type.size()
                i = v._i
                if sz == 8:
                    self.emit([f"li {r2}, {i >> 32}", f"li {r1}, {i & 0xffffffff}"])
                else:
                    self.emit(f"li {r1}, {i}")

            case SymConstant():
                if v._offset:
                    self.emit(f"li {r1}, +(${v._name} {v._offset})")
                else:
                    self.emit(f"li {r1}, ${v._name}")

            case StackFrameOffset():
                self.emitFormatI("addi", "a0", "fp", v._i)

            case TemporaryValue():
                sz = v._type.size()
                if sz == 8:
                    if r2 != "a1":
                        self.emit(f"mv {r2}, a1")
                if r1 != "a0":
                    self.emit(f"mv {r1}, a0")

            case (
                GlobalVariable()
                | StaticVariable()
                | ExternVariable()
                | LocalVariable()
                | Argument()
            ):
                addr = self.addressOf(v)
                self.load(MemoryAccess(addr), r1, r2)

            case MemoryAccess():
                ty = v._type
                sz = ty.size()
                addr = v._addr
                match addr:
                    case SymConstant():  # global/static variables
                        _name = addr._name
                        _offset = addr._offset
                        match sz:
                            case 8:
                                self.emit(f"lw {r1}, +(${_name} {_offset})")
                                self.emit(f"lw {r2}, +(${_name} {_offset + 4})")
                            case 4:
                                self.emit(f"lw {r1}, +(${_name} {_offset})")
                            case 2:
                                assert isinstance(ty, IntType)
                                instr = "lhu" if ty._unsigned else "lh"
                                self.emit(f"{instr} {r1}, +(${_name} {_offset})")
                            case 1:
                                assert isinstance(ty, IntType)
                                instr = "lbu" if ty._unsigned else "lb"
                                self.emit(f"{instr} {r1}, +(${_name} {_offset})")
                            case _:
                                unreachable()

                    case StackFrameOffset():  # local variables
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emitFormatI("lw", r1, "fp", _offset)
                                self.emitFormatI("lw", r2, "fp", _offset + 4)
                            case 4:
                                self.emitFormatI("lw", r1, "fp", _offset)
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emitFormatI("lhu" if ty._unsigned else "lh", r1, "fp", _offset)
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emitFormatI("lbu" if ty._unsigned else "lb", r1, "fp", _offset)
                            case _:
                                unreachable()

                    case PtrConstant():
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emit(f"lw {r1}, {_offset}")
                                self.emit(f"lw {r2}, {_offset + 4}")
                            case 4:
                                self.emit(f"lw {r1}, {_offset}")
                            case 2:
                                assert isinstance(ty, IntType)
                                instr = "lhu" if ty._unsigned else "lh"
                                self.emit(f"{instr} {r1}, {_offset}")
                            case 1:
                                assert isinstance(ty, IntType)
                                instr = "lbu" if ty._unsigned else "lb"
                                self.emit(f"{instr} {r1}, {_offset}")
                            case _:
                                unreachable()

                    case TemporaryValue():
                        match sz:
                            case 8:
                                assert r2 != "a0"
                                self.emitFormatI("lw", r2, "a0", 4)
                                self.emitFormatI("lw", r1, "a0")
                            case 4:
                                self.emitFormatI("lw", r1, "a0")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emitFormatI("lhu" if ty._unsigned else "lh", r1, "a0")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emitFormatI("lbu" if ty._unsigned else "lb", r1, "a0")
                            case _:
                                unreachable()

                    case _:
                        pass

    def store(self, v: Value, r1: str = "a0", r2: str = "a1"):
        match v:
            case (
                GlobalVariable()
                | StaticVariable()
                | ExternVariable()
                | LocalVariable()
                | Argument()
            ):
                addr = self.addressOf(v)
                self.store(MemoryAccess(addr), r1, r2)

            case MemoryAccess():
                ty = v._type
                sz = ty.size()
                addr = v._addr
                match addr:
                    case SymConstant():  # global/static variables
                        _name = addr._name
                        _offset = addr._offset
                        match sz:
                            case 8:
                                self.emit(f"sw {r1}, +(${_name} {_offset})")
                                self.emit(f"sw {r2}, +(${_name} {_offset + 4})")
                            case 4:
                                self.emit(f"sw {r1}, +(${_name} {_offset})")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f"sh {r1}, +(${_name} {_offset})")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f"sb {r1}, +(${_name} {_offset})")
                            case _:
                                unreachable()

                    case StackFrameOffset():  # local variables
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emitFormatS("sw", r1, "fp", _offset)
                                self.emitFormatS("sw", r2, "fp", _offset + 4)
                            case 4:
                                self.emitFormatS("sw", r1, "fp", _offset)
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emitFormatS("sh", r1, "fp", _offset)
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emitFormatS("sb", r1, "fp", _offset)
                            case _:
                                unreachable()

                    case PtrConstant():
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emit(f"sw {r1}, {_offset}")
                                self.emit(f"sw {r2}, {_offset + 4}")
                            case 4:
                                self.emit(f"sw {r1}, {_offset}")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f"sh {r1}, {_offset}")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f"sb {r1}, {_offset}")
                            case _:
                                unreachable()

                    case TemporaryValue():
                        match sz:
                            case 8:
                                assert r2 != "a0"
                                self.emitFormatS("sw", r2, "a0", 4)
                                self.emitFormatS("sw", r1, "a0")
                            case 4:
                                self.emitFormatS("sw", r1, "a0")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emitFormatS("sh", r1, "a0")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emitFormatS("sb", r1, "a0")
                            case _:
                                unreachable()

                    case _:
                        pass

    def push(self, v: Value):
        self.load(v)
        if v.getType().size() == 8:
            self.emit("push a1")
        self.emit("push a0")

    def pop(self, ty: Type, r1: str = "a0", r2: str = "a1"):
        self.emit(f"pop {r1}")
        if ty.size() == 8:
            self.emit(f"pop {r2}")

    def emitBuiltinCall(self, name: str, *args: Value):
        assert name in self._builtins
        self._builtins[name] += 1
        self.emitCall(f"__builtin_{name}", *args)

    def emitCall(self, name: str, *args: Value):
        n = 0
        # push arguments
        for arg in reversed(args):
            self.push(arg)
            n += align(arg.getType().size(), 4)

        self.emit(f"call ${name}")

        # restore sp
        if n > 0:
            self.emit(f"addi sp, sp, {n}")

    """
    void *__builtin_memset(void *s, int c, size_t n) {
        char *p1 = s, *p2 = s + n;
        while (p1 < p2)
            *(p1++) = c;
        return s;
    }
    """

    def _emitBuiltinMemset(self):
        name = "__builtin_memset"
        sec = self._secText

        sec.add(f".local ${name}")
        sec.add(f'.type ${name}, "function"')
        sec.add(".align 2")
        sec.addLabel(f"{name}")

        # load arguments
        self.load(Argument("s", PointerType(getBuiltinType("void")), 8), "a0")
        self.load(Argument("c", getBuiltinType("int"), 12), "a1")
        self.load(Argument("n", getBuiltinType("size_t"), 16), "a2")

        sec.addRaw(
            """
            blez    a2, $2f
            add     a2, a2, a0
            mv      a3, a0
        1:
            addi    a4, a3, 1
            sb      a3, a1, 0
            mv      a3, a4
            bltu    a4, a2, $1b
        2:
            ret
            """
        )
        sec.add(f".size ${name}, -($. ${name})")

    def emitBuiltins(self):
        for name, n in self._builtins.items():
            if n > 0:
                _f = getattr(self, f"_emitBuiltin{name.capitalize()}")
                _f()

    def save(self, o: io.StringIO):
        self._secText.save(o)
        self._secRodata.save(o)
        self._secData.save(o)
        self._secBss.save(o)


class Codegen(NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        super().__init__(ctx)

        self._asm = Asm()

        for sLit in ctx._strPool.values():
            self._asm.addStr(sLit)

    def save(self, o: io.StringIO):
        self._asm.save(o)

    def getNodeValue(self, node: c_ast.Node) -> Value:
        r = self.getNodeRecord(node)

        def _shouldVisit():
            if not r._value:
                return True
            # current value is just a Type wrapper
            return type(r._value) in [
                Value,
                LValue,
                RValue,
            ]

        if _shouldVisit():
            self.visit(node)

        assert not _shouldVisit()
        return r._value

    def getNodeType(self, node: c_ast.Node) -> Type:
        return super().getNodeValue(node).getType()

    def loadNodeValue(
        self,
        node: c_ast.Node,
        r1: str = "a0",
        r2: str = "a1",
    ):
        v = self.getNodeValue(node)
        self._asm.load(v, r1, r2)

    def visit_Decl(self, node: c_ast.Decl):
        if not node.name:
            return

        def _gen(init: c_ast.Node, offset: int, _local: bool):
            ty = self.getNodeType(init)
            assert offset == align(offset, ty.alignment())

            sec = self._asm._secData
            match ty:
                case ArrayType():
                    match init:
                        case c_ast.InitList():
                            for i, expr in enumerate(init.exprs):
                                _gen(expr, offset, _local)
                                offset += ty._base.size()
                            if not _local:
                                left = ty.size() - offset
                                if left > 0:
                                    sec.add(f".fill {left}")
                        case _:
                            sLit = self.getNodeStrLiteral(init)

                            if not _local:
                                sec.add(f".asciz {sLit._sOrig}")
                                left = ty.size() - len(sLit._s)
                                if left > 0:
                                    sec.add(f".fill {left}")
                            else:
                                for i, c in enumerate(sLit._s):
                                    _gen(Node(IntConstant(ord(c), ty._base)), offset + i, _local)

                case StructType():
                    n = len(init.exprs)
                    for i in range(n):
                        field = ty._fields[i]
                        _gen(init.exprs[i], offset + field._offset, _local)
                        if not _local:
                            left = (
                                (ty._fields[i + 1]._offset if i < n - 1 else ty.size())
                                - field._offset
                                - field._type.size()
                            )
                            if left > 0:
                                sec.add(f".fill {left}")

                case _:
                    if not _local:
                        sec.addConstant(self.getNodeValue(init))
                    else:
                        self.loadNodeValue(init)
                        self._asm.store(MemoryAccess(StackFrameOffset(offset, PointerType(ty))))

        v, ty = self.getNodeValueType(node)
        match v:
            case Function() | ExternVariable():
                pass
            case GlobalVariable() | StaticVariable():
                sec = self._asm._secData if node.init else self._asm._secBss
                sec.addEmptyLine()
                _ = log2(ty.alignment())
                if _ > 0:
                    sec.add(f".align {_}")

                label = v._label
                sec.addLabel(label)

                if not node.init:
                    sec.add(f".fill {ty.size()}")
                else:
                    _gen(node.init, 0, False)

                if isinstance(v, StaticVariable):
                    sec.add(f".local ${label}")
                else:
                    sec.add(f".global ${label}")

                sec.add(f'.type ${label}, "object"')
                sec.add(f".size ${label}, -($. ${label})")

            case LocalVariable():
                if node.init:
                    match ty:
                        case ArrayType() | StructType():
                            self._asm.emitBuiltinCall(
                                "memset",
                                self._asm.addressOf(v),  # s
                                getIntConstant(0),  # c
                                getIntConstant(ty.size(), "size_t"),  # n
                            )
                    _gen(node.init, v._offset, True)
            case _:
                unreachable()

    def emitPrelogue(self):
        self._asm.emit(["push ra", "push fp", "mv fp, sp"])
        self._asm.emitFormatI("addi", "sp", "sp", -self._func._maxOffset)

    def emitEpilogue(self):
        self._asm.emit(["mv sp, fp", "pop fp", "pop ra"])

    def emitRet(self):
        self.emitEpilogue()
        self._asm.emit("ret")

    def visit_FuncDef(self, node: c_ast.FuncDef):
        self._func = self.getNodeValue(node)

        name = self._func._name

        sec = self._asm._secText
        sec.addEmptyLine()

        decl: c_ast.Decl = node.decl
        if "static" in decl.storage:
            sec.add(f".local ${name}")
        else:
            sec.add(f".global ${name}")

        sec.add(f'.type ${name}, "function"')

        sec.add(".align 2")
        sec.addLabel(name)

        self.visit(node.body)

        sec.add(f".size ${name}, -($. ${name})")

        self._func = None

    def visit_Compound(self, node: c_ast.Compound):
        isFuncBody = isinstance(self.getParent(), c_ast.FuncDef)
        if isFuncBody:
            self.emitPrelogue()

        block_items = node.block_items or []
        for _ in block_items:
            self.visit(_)

        if isFuncBody:
            if len(block_items) == 0 or not isinstance(block_items[-1], c_ast.Return):
                self.emitRet()

    def visit_Return(self, node: c_ast.Return):
        if node.expr:
            self.loadNodeValue(node.expr)
        self.emitRet()

    def visit_Cast(self, node: c_ast.Cast):
        t1 = self.getNodeType(node.to_type)
        v2, t2 = self.getNodeValueType(node.expr)

        match t1, t2:
            # array to pointer conversion
            case PointerType(), ArrayType():
                match v2:
                    case LocalVariable():  # p = arr
                        self.setNodeValue(node, StackFrameOffset(v2._offset, t1))

                    case MemoryAccess():  # p = *pArr
                        self.setNodeValue(node, self._asm.addressOf(v2))

                    case _:
                        unreachable()

            # function to pointer conversion
            case PointerType(), FunctionType():
                match v2:
                    case MemoryAccess():  # p = *pFunc
                        self.setNodeValue(node, self._asm.addressOf(v2))

                    case _:
                        unreachable()

            # integer conversion
            case IntType(), IntType():

                def _getV():
                    sz1 = t1.size()
                    sz2 = t2.size()
                    if sz1 == sz2:
                        v2._type = t1
                        return v2

                    if sz1 > sz2:
                        if sz1 < 8:
                            v2._type = t1
                            return v2
                        self._asm.load(v2)
                        self._asm.emit("srai a1, a0, 31")
                        return TemporaryValue(t1)

                    self._asm.load(v2)
                    match sz1:
                        case 4:
                            pass
                        case 2:
                            if t1._unsigned:
                                self._asm.emit("zext.h a0, a0")
                            else:
                                self._asm.emit("sext.h a0, a0")
                        case 1:
                            if t1._unsigned:
                                self._asm.emit("zext.b a0, a0")
                            else:
                                self._asm.emit("sext.b a0, a0")
                        case _:
                            unreachable()
                    return TemporaryValue(t1)

                self.setNodeValue(node, _getV())

            case PointerType(), PointerType():
                v2._type = t1
                self.setNodeValue(node, v2)

            # Any integer can be cast to any pointer type.
            case PointerType(), IntType():
                if t2.size() >= 4:
                    v2._type = t1
                    self.setNodeValue(node, v2)
                else:
                    self._asm.load(v2)
                    self.setNodeValue(node, TemporaryValue(t1))

            # Any pointer type can be cast to any integer type.
            case IntType(), PointerType():
                if t1.size() == t2.size():
                    v2._type = t1
                    self.setNodeValue(node, v2)
                else:
                    node.expr = c_ast.Cast(Node(getBuiltinType("unsigned long")), node.expr)
                    self.visit_Cast(node)

            case _:
                unreachable()

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        match node.op:
            case "&":
                pass

            case '*':
                v = self.getNodeValue(node.expr)
                self.setNodeValue(node, MemoryAccess(v))

            case _:
                unreachable()
