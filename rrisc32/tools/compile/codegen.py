import io
import textwrap

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
        assert isinstance(ty, PointerType)

        assert not isinstance(addr, MemoryAccess)

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
        s = textwrap.dedent(s)
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

    def save(self, o: io.StringIO):
        for fragment in self._fragments:
            fragment.save(o)


class Asm:
    def __init__(self, cg: "Codegen") -> None:
        self._cg = cg

        self._secText = Section(".text")
        self._secRodata = Section(".rodata")
        self._secData = Section(".data")
        self._secBss = Section(".bss")

        self._builtins = {"memset": 0, "memcpy": 0}

    def addStr(self, sLit: StrLiteral) -> str:
        self._secRodata.addLabel(sLit._label)
        self._secRodata.add(f".asciz {sLit._sOrig}")

    def emit(self, s: str | list[str]):
        self._secText.add(s)

    def emitLabel(self, s: str):
        self._secText.addLabel(s)

    def emitEmptyLine(self):
        self._secText.addEmptyLine()

    def checkImm(self, i: int, n: int):
        mins = (1 << (n - 1)) - (1 << n)
        maxs = (1 << (n - 1)) - 1
        return mins <= i <= maxs

    def checkImmI(self, i: int):
        return self.checkImm(i, 12)

    def checkImmS(self, i: int):
        return self.checkImm(i, 12)

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

    def getNodeValue(self, v: Value | c_ast.Node):
        match v:
            case c_ast.Node() as node:
                return self._cg.getNodeValue(node)
            case Value():
                return v
            case _:
                unreachable()

    def load(self, v: Value | c_ast.Node, r1: str = "a0", r2: str = "a1"):
        v = self.getNodeValue(v)
        match v:
            case IntConstant() | PtrConstant():
                sz = v._type.size()
                i = v._i
                if sz == 8:
                    self.emit(
                        [
                            f"li {r2}, {hex((i >> 32) & 0xffffffff)}",
                            f"li {r1}, {hex(i & 0xffffffff)}",
                        ]
                    )
                else:
                    self.emit(f"li {r1}, {i}")

            case SymConstant():
                if v._offset:
                    self.emit(f"li {r1}, +(${v._name} {v._offset})")
                else:
                    self.emit(f"li {r1}, ${v._name}")

            case StackFrameOffset():
                self.emit(f"addi a0, fp, {v._i}")

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
                    case (
                        GlobalVariable()
                        | StaticVariable()
                        | ExternVariable()
                        | LocalVariable()
                        | Argument()
                    ):
                        assert r2 != "a0"

                        self.load(addr)
                        self.load(MemoryAccess(TemporaryValue(addr.getType())), r1, r2)

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
                                self.emit(
                                    f'{"lhu" if ty._unsigned else "lh"} {r1}, +(${_name} {_offset})'
                                )
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(
                                    f'{"lbu" if ty._unsigned else "lb"} {r1}, +(${_name} {_offset})'
                                )
                            case _:
                                unreachable()

                    case StackFrameOffset():  # local variables
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emit([f"lw {r1}, fp, {_offset}", f"lw {r2}, fp, {_offset+4}"])
                            case 4:
                                self.emit(f"lw {r1}, fp, {_offset}")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lhu" if ty._unsigned else "lh"} {r1}, fp, {_offset}')
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lbu" if ty._unsigned else "lb"} {r1}, fp, {_offset}')
                            case _:
                                unreachable()

                    case PtrConstant():
                        _offset = addr._i
                        match sz:
                            case 8:
                                self.emit([f"lw {r1}, {_offset}", f"lw {r2}, {_offset + 4}"])
                            case 4:
                                self.emit(f"lw {r1}, {_offset}")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lhu" if ty._unsigned else "lh"} {r1}, {_offset}')
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lbu" if ty._unsigned else "lb"} {r1}, {_offset}')
                            case _:
                                unreachable()

                    case TemporaryValue():
                        match sz:
                            case 8:
                                assert r2 != "a0"
                                self.emit([f"lw {r2}, a0, 4", f"lw {r1}, a0, 0"])
                            case 4:
                                self.emit(f"lw {r1}, a0, 0")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lhu" if ty._unsigned else "lh"} {r1}, a0, 0')
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f'{"lbu" if ty._unsigned else "lb"} {r1}, a0, 0')
                            case _:
                                unreachable()

                    case _:
                        unreachable()

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
                    case (
                        GlobalVariable()
                        | StaticVariable()
                        | ExternVariable()
                        | LocalVariable()
                        | Argument()
                    ):
                        if r1 == "a0":
                            assert r2 == "a1"
                            self.emit(["mv a2, a0", "mv a3, a1"])
                            r1, r2 = "a2", "a3"

                        self.load(addr)
                        self.store(MemoryAccess(TemporaryValue(addr.getType())), r1, r2)

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
                                self.emit([f"sw fp, {r1}, {_offset}", f"sw fp, {r2}, {_offset+4}"])
                            case 4:
                                self.emit(f"sw fp, {r1}, {_offset}")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f"sh fp, {r1}, {_offset}")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f"sb fp, {r1}, {_offset}")
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
                                self.emit([f"sw a0, {r1}, 0", f"sw a0, {r2}, 4"])
                            case 4:
                                self.emit(f"sw a0, {r1}, 0")
                            case 2:
                                assert isinstance(ty, IntType)
                                self.emit(f"sh a0, {r1}, 0")
                            case 1:
                                assert isinstance(ty, IntType)
                                self.emit(f"sb a0, {r1}, 0")
                            case _:
                                unreachable()

                    case _:
                        unreachable()

    def push(self, v: Value | c_ast.Node):
        v = self.getNodeValue(v)
        self.load(v)
        if v.getType().size() == 8:
            self.emit("push a1")
        self.emit("push a0")

    def pop(self, ty: Type, r1: str = "a0", r2: str = "a1"):
        self.emit(f"pop {r1}")
        if ty.size() == 8:
            self.emit(f"pop {r2}")

    def emitCond(self, node: c_ast.Node, label: str, eq: bool = True):
        r = self._cg.getNodeRecord(node)
        v = r._value
        match v:
            case IntConstant():
                c = v._i == 0
                if c == eq:
                    self.emit(f"j ${label}")
            case _:
                self.load(node)

                ty = v.getType()
                if ty.size() == 8:
                    self.emit("or a0, a0, a1")

                if eq:
                    self.emit(f"beqz a0, ${label}")
                else:
                    self.emit(f"bnez a0, ${label}")

    def emitPrelogue(self, szLocal: Optional[int] = None):
        self.emit(["push ra", "push fp", "mv fp, sp"])
        if szLocal is None:
            szLocal = self._cg._func._maxOffset
        if szLocal > 0:
            self.emit(f"addi sp, sp, {-szLocal}")

    def emitEpilogue(self):
        self.emit(["mv sp, fp", "pop fp", "pop ra"])

    def emitRet(self):
        self.emitEpilogue()
        self.emit("ret")

    def emitBuiltinCall(self, name: str, *args: Value | c_ast.Node):
        assert name in self._builtins
        self._builtins[name] += 1
        self.emitCall(f"__builtin_{name}", *args)

    def emitCall(self, name: str | SymConstant | c_ast.Node, *args: Value | c_ast.Node):
        # push arguments
        n = 0
        for arg in reversed(args):
            arg = self.getNodeValue(arg)
            self.push(arg)
            n += align(arg.getType().size(), 4)

        match name:
            case str():
                self.emit(f"call ${name}")
            case SymConstant() as sym:
                self.emit(f"call ${sym._name}")
            case c_ast.Node() as node:
                self.load(node)  # load function address
                self.emit(f"jalr a0")
            case _:
                unreachable()

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

        sec.addEmptyLine()
        sec.add(f".local ${name}")
        sec.add(f'.type ${name}, "function"')
        sec.add(".align 2")
        sec.addLabel(f"{name}")

        self.emitPrelogue(0)

        # load arguments
        self.load(Argument("s", PointerType(getBuiltinType("void")), 8), "a0")
        self.load(Argument("c", getBuiltinType("int"), 12), "a1")
        self.load(Argument("n", getBuiltinType("size_t"), 16), "a2")

        sec.addRaw(
            """
            blez a2, $2f
            add a2, a2, a0
            mv a3, a0
        1:
            addi a4, a3, 1
            sb a3, a1, 0
            mv a3, a4
            bltu a4, a2, $1b
        2:
            """
        )
        self.emitRet()
        sec.add(f".size ${name}, -($. ${name})")

    """
    char *memcpy(char *dest, char *src, unsigned int n) {
        char *end = src + n;
        while (src < end) {
            *(dest++) = *(src++);
        }
        return dest;
    }
    """

    def _emitBuiltinMemcpy(self):
        name = "__builtin_memcpy"
        sec = self._secText

        sec.addEmptyLine()
        sec.add(f".local ${name}")
        sec.add(f'.type ${name}, "function"')
        sec.add(".align 2")
        sec.addLabel(f"{name}")

        self.emitPrelogue(0)

        # load arguments
        self.load(Argument("dest", PointerType(getBuiltinType("void")), 8), "a0")
        self.load(Argument("src", PointerType(getBuiltinType("void")), 12), "a1")
        self.load(Argument("n", getBuiltinType("size_t"), 16), "a2")

        sec.addRaw(
            """
            blez a2, $2f
            add a3, a1, a2
        1:
            lbu a4, a1, 0
            addi a5, a1, 1
            addi a2, a0, 1
            sb a0, a4, 0
            mv a0, a2
            mv a1, a5
            bltu a5, a3, $1b
            mv a0, a2
        2:
            """
        )
        self.emitRet()
        sec.add(f".size ${name}, -($. ${name})")

    def _emitBuiltins(self):
        for name, n in self._builtins.items():
            if n > 0:
                _f = getattr(self, f"_emitBuiltin{name.capitalize()}")
                _f()

    def save(self, o: io.StringIO):
        self._emitBuiltins()

        self._secText.save(o)
        self._secRodata.save(o)
        self._secData.save(o)
        self._secBss.save(o)


class Codegen(NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        super().__init__(ctx)

        self._asm = Asm(self)

        for sLit in ctx._strPool.values():
            self._asm.addStr(sLit)

    def save(self, o: io.StringIO):
        self._asm.save(o)

    def getNodeValue(self, node: c_ast.Node) -> Value:
        if isinstance(node, Node):
            return node._value

        r = self.getNodeRecord(node)

        def _shouldVisit():
            if isinstance(node, c_ast.Decl):
                return not r._visited

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

    def visit_Decl(self, node: c_ast.Decl):
        r = self.getNodeRecord(node)
        if r._visited:
            return
        r._visited = True

        if not node.name:
            return

        if "extern" in node.quals:
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
                    if not isinstance(init, c_ast.InitList):
                        tyR = self.getNodeType(init)
                        self._asm.emitBuiltinCall(
                            "memcpy",
                            StackFrameOffset(offset, PointerType(ty)),
                            c_ast.UnaryOp("&", init),
                            getIntConstant(tyR.size(), "size_t"),
                        )
                        return

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
                        self._asm.load(init)
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
                            if isinstance(node.init, c_ast.InitList):
                                self._asm.emitBuiltinCall(
                                    "memset",
                                    self._asm.addressOf(v),  # s
                                    getIntConstant(0),  # c
                                    getIntConstant(ty.size(), "size_t"),  # n
                                )
                    _gen(node.init, v._offset, True)
            case _:
                unreachable()

    def translate(self, node: c_ast.Node):
        self.setNodeTranslated(node, self.getNodeTranslated(node))

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
            self._asm.emitPrelogue()

        for _ in node.block_items:
            self.visit(_)

        if isFuncBody:
            assert len(node.block_items) > 0
            if not isinstance(node.block_items[-1], c_ast.Return):
                if self._func._name == 'main':
                    self._asm.load(getIntConstant(0))
                self._asm.emitRet()

    def visit_Return(self, node: c_ast.Return):
        if node.expr:
            self._asm.load(node.expr)
        self._asm.emitRet()

    def visit_Cast(self, node: c_ast.Cast):
        t1 = self.getNodeType(node.to_type)
        v2, t2 = self.getNodeValueType(node.expr)
        sz1 = t1.size()
        sz2 = t2.size()

        def _reinterpret():
            self._asm.load(v2)
            self.setNodeValue(node, TemporaryValue(t1))

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
                self._asm.load(v2)
                match sz1:
                    case 8:
                        if sz2 < 8:
                            self._asm.emit("srai a1, a0, 31")
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

                self.setNodeValue(node, TemporaryValue(t1))

            case PointerType(), PointerType():
                _reinterpret()

            # Any integer can be cast to any pointer type.
            # Any pointer type can be cast to any integer type.
            case (PointerType(), IntType()) | (IntType(), PointerType()):
                if sz1 == sz2:
                    _reinterpret()
                else:
                    node.expr = c_ast.Cast(Node(getBuiltinType("unsigned long")), node.expr)
                    self.visit_Cast(node)

            case _:
                unreachable()

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        match node.op:
            case "&":
                v = self.getNodeValue(node.expr)
                self.setNodeValue(node, self._asm.addressOf(v))
                return

            case "*":
                v = self.getNodeValue(node.expr)
                if isinstance(v, MemoryAccess):
                    self._asm.load(v)
                    v = TemporaryValue(v.getType())
                self.setNodeValue(node, MemoryAccess(v))
                return

        ty = self.getNodeType(node.expr)

        if self.getNodeTranslated(node):
            self.translate(node)
            self.setNodeValue(node, TemporaryValue(ty))
            return

        self._asm.load(node.expr)
        match node.op:
            case "+":
                pass

            case "-":
                assert ty.size() != 8
                self._asm.emit("neg a0, a0")

            case "~":
                if ty.size() == 8:
                    self._asm.emit("not a1, a1")
                self._asm.emit("not a0, a0")

            case "!":
                if ty.size() == 8:
                    self._asm.emit("or a0, a0, a1")
                self._asm.emit("seqz a0, a0")

            case "++" | "--" | "p++" | "p--":
                unreachable()

            case _:
                unreachable()

        self.setNodeValue(node, TemporaryValue(self.getNodeType(node)))

    def visit_BinaryOp(self, node: c_ast.BinaryOp):
        ty = self.getNodeType(node)
        tyL = self.getNodeType(node.left)
        tyR = self.getNodeType(node.right)
        rL = self.getNodeRecord(node.left)
        rR = self.getNodeRecord(node.right)

        assert tyL.size() == tyR.size()

        if self.getNodeTranslated(node):
            self.translate(node)
            self.setNodeValue(node, TemporaryValue(ty))
            return

        match node.op:
            case "&&" | "||":
                mnemonic = "beqz" if node.op == "&&" else "bnez"
                labelEnd = self.getNodeLabels(node)[0]
                self._asm.load(node.left)
                if tyL.size() == 8:
                    self._asm.emit("or a0, a0, a1")
                self._asm.emit(f"{mnemonic} a0, ${labelEnd}")
                self._asm.load(node.right)
                if tyL.size() == 8:
                    self._asm.emit("or a0, a0, a1")
                self._asm.emitLabel(labelEnd)
                self._asm.emit("snez a0, a0")

                self.setNodeValue(node, TemporaryValue(ty))
                return

            case "+":
                match rL._value, rR._value:
                    case IntConstant(_i=0) | PtrConstant(_i=0), Value():
                        self._asm.load(node.right)
                        self.setNodeValue(node, TemporaryValue(ty))
                        return
                    case Value(), IntConstant(_i=0) | PtrConstant(_i=0):
                        self._asm.load(node.left)
                        self.setNodeValue(node, TemporaryValue(ty))
                        return

        self._asm.push(node.right)
        self._asm.load(node.left)
        self._asm.pop(tyR, "a2", "a3")

        match node.op:
            case "+":
                if tyL.size() == 8:
                    self._asm.emit(
                        [
                            "add a0, a0, a2",
                            "sltu a2, a0, a2",
                            "add a1, a1, a3",
                            "add a1, a1, a2",
                        ]
                    )
                else:
                    if isinstance(tyL, PointerType):  # p + i
                        sz = 0 if tyL._base.isVoid() else tyL._base.size()
                        if sz > 1:
                            self._asm.emit(f"slli a2, a2, {log2(sz)}")
                    elif isinstance(tyR, PointerType):  # i + p
                        sz = 0 if tyR._base.isVoid() else tyR._base.size()
                        if sz > 1:
                            self._asm.emit(f"slli a0, a0, {log2(sz)}")
                    self._asm.emit("add a0, a0, a2")

            case "-":
                if tyL.size() == 8:
                    self._asm.emit(
                        [
                            "sub a1, a1, a3",
                            "sltu a3, a0, a2",
                            "sub a1, a1, a3",
                            "sub a0, a0, a2",
                        ]
                    )
                else:
                    if isinstance(tyL, PointerType):
                        sz = 0 if tyL._base.isVoid() else tyL._base.size()
                        if sz > 1:
                            self._asm.emit(f"slli a2, a2, {log2(sz)}")
                    self._asm.emit("sub a0, a0, a2")

            case "*":
                if tyL.size() == 8:
                    self._asm.emit(
                        [
                            "mul a1, a2, a1",
                            "mul a3, a3, a0",
                            "add a1, a1, a3",
                            "mulhu a3, a2, a0",
                            "add a1, a1, a3",
                            "mul a0, a2, a0",
                        ]
                    )
                else:
                    self._asm.emit("mul a0, a0, a2")

            case "/":
                if tyL.size() == 8:
                    raise CCNotImplemented("64-bit /")
                else:
                    assert isinstance(tyL, IntType)
                    mnemonic = "divu" if tyL._unsigned else "div"
                    self._asm.emit(f"{mnemonic} a0, a0, a2")

            case "%":
                if tyL.size() == 8:
                    raise CCNotImplemented("64-bit %")
                else:
                    assert isinstance(tyL, IntType)
                    mnemonic = "remu" if tyL._unsigned else "rem"
                    self._asm.emit(f"{mnemonic} a0, a0, a2")

            case "&" | "|" | "^":
                mnemonic = {"&": "and", "|": "or", "^": "xor"}[node.op]
                if tyL.size() == 8:
                    self._asm.emit(f"{mnemonic} a1, a1, a3")
                self._asm.emit(f"{mnemonic} a0, a0, a2")

            case "<<":
                if tyL.size() == 8:
                    raise CCNotImplemented(f"64-bit <<")
                else:
                    self._asm.emit(f"sll a0, a0, a2")
            case ">>":
                if tyL.size() == 8:
                    raise CCNotImplemented(f"64-bit >>")
                else:
                    assert isinstance(tyL, IntType)
                    mnemonic = "srl" if tyL._unsigned else "sra"
                    self._asm.emit(f"{mnemonic} a0, a0, a2")

            case "==" | "!=":
                mnemonic = "seqz" if node.op == "==" else "snez"
                if tyL.size() == 8:
                    self._asm.emit(
                        [
                            "xor a1, a1, a3",
                            "xor a0, a0, a2",
                            "or a0, a0, a1",
                            f"{mnemonic} a0, a0",
                        ]
                    )
                else:
                    self._asm.emit(["xor a0, a0, a2", f"{mnemonic} a0, a0"])

            case "<" | ">=":
                mnemonic = "sltu"
                if isinstance(tyL, IntType) and not tyL._unsigned:
                    mnemonic = "slt"

                if tyL.size() == 8:
                    self._asm._secText.addRaw(
                        f"""
                        beq a1, a3, $1f
                        {mnemonic} a0, a1, a3
                        j $2f
                    1:
                        sltu a0, a0, a2
                    2:
                        """
                    )
                else:
                    self._asm.emit(f"{mnemonic} a0, a0, a2")

                if node.op == ">=":
                    self._asm.emit("xori a0, a0, 1")

            case ">" | "<=":
                unreachable()

            case _:
                unreachable()

        self.setNodeValue(node, TemporaryValue(ty))

    # Assignment also returns the same value as what was stored in lhs
    # (so that expressions such as a = b = c are possible). The value
    # category of the assignment operator is non-lvalue (so that expressions
    # such as (a=b)=c are invalid).
    def visit_Assignment(self, node: c_ast.Assignment):
        tyL = self.getNodeType(node.lvalue)
        match node.op:
            case "=":
                match tyL:
                    case StructType():
                        self._asm.emitBuiltinCall(
                            "memcpy",
                            c_ast.UnaryOp("&", node.lvalue),
                            c_ast.UnaryOp("&", node.rvalue),
                            getIntConstant(tyL.size(), "size_t"),
                        )

                        # there is no struct rvalue

                    case IntType() | PointerType():
                        self._asm.push(node.rvalue)
                        vL = self.getNodeValue(node.lvalue)
                        self._asm.pop(tyL, "a2", "a3")
                        self._asm.store(vL, "a2", "a3")

                        self._asm.emit("mv a0, a2")
                        if tyL.size() == 8:
                            self._asm.emit("mv a1, a3")
                        self.setNodeValue(node, TemporaryValue(tyL))

                    case _:
                        unreachable()
            case _:
                unreachable()

    def visit_ArrayRef(self, node: c_ast.ArrayRef):
        self.translate(node)

    def visit_StructRef(self, node: c_ast.StructRef):
        self.translate(node)

    def visit_ExprList(self, node: c_ast.ExprList):
        for expr in node.exprs:
            v = self.getNodeValue(expr)
        self.setNodeValue(node, v)

    def visit_FuncCall(self, node: c_ast.FuncCall):
        args: list[c_ast.Node] = node.args.exprs if node.args else []
        r = self.getNodeRecord(node.name)
        v = r._value
        if isinstance(r._value, SymConstant):
            self._asm.emitCall(r._value, *args)
        else:
            self._asm.emitCall(node.name, *args)
        self.setNodeValue(node, TemporaryValue(v.getType()))

    def visit_TernaryOp(self, node: c_ast.TernaryOp):
        labelFalse, labelEnd = self.getNodeLabels(node)

        self._asm.emitCond(node.cond, labelFalse)

        self._asm.load(node.iftrue)
        self._asm.emit(f"j ${labelEnd}")

        self._asm.emitLabel(labelFalse)
        self._asm.load(node.iffalse)

        self._asm.emitLabel(labelEnd)

        self.setNodeValue(node, TemporaryValue(self.getNodeType(node)))

    def visit_Label(self, node: c_ast.Label):
        label = self.getNodeLabels(node)[0]
        self._asm.emitLabel(label)

        self.visit(node.stmt)

    def visit_Goto(self, node: c_ast.Goto):
        label = self.getNodeLabels(node)[0]
        self._asm.emit(f"j ${label}")

    def visit_If(self, node: c_ast.If):
        labelFalse, labelEnd = self.getNodeLabels(node)

        self._asm.emitCond(node.cond, labelFalse)

        self.visit(node.iftrue)
        self._asm.emit(f"j ${labelEnd}")

        self._asm.emitLabel(labelFalse)
        self.visit(node.iffalse)

        self._asm.emitLabel(labelEnd)

    def visit_While(self, node: c_ast.While):
        labelStart, labelEnd = self.getNodeLabels(node)

        self._asm.emitLabel(labelStart)
        self._asm.emitCond(node.cond, labelEnd)

        self.visit(node.stmt)
        self._asm.emit(f"j ${labelStart}")

        self._asm.emitLabel(labelEnd)

    def visit_DoWhile(self, node: c_ast.DoWhile):
        labelStart, labelNext, labelEnd = self.getNodeLabels(node)

        self._asm.emitLabel(labelStart)
        self.visit(node.stmt)

        self._asm.emitLabel(labelNext)
        self._asm.emitCond(node.cond, labelStart, False)

        self._asm.emitLabel(labelEnd)

    def visit_For(self, node: c_ast.For):
        labelStart, labelNext, labelEnd = self.getNodeLabels(node)

        self.visit(node.init)

        self._asm.emitLabel(labelStart)
        if node.cond:
            self._asm.emitCond(node.cond, labelEnd)
        self.visit(node.stmt)

        self._asm.emitLabel(labelNext)
        self.visit(node.next)
        self._asm.emit(f"j ${labelStart}")

        self._asm.emitLabel(labelEnd)

    def visit_Switch(self, node: c_ast.Switch):
        self._asm.load(node.cond)

        r = self.getNodeRecord(node)
        labelDefault = None
        for i, label in r._cases:
            if i is not None:
                self._asm.emit([f"li a1, {i}", f"beq a0, a1, ${label}"])
            else:
                labelDefault = label
        if labelDefault is not None:
            self._asm.emit(f"j ${labelDefault}")

        self.visit(node.stmt)

        labelEnd = self.getNodeLabels(node)[0]
        self._asm.emitLabel(labelEnd)

    def visit_Case(self, node: c_ast.Case):
        label = self.getNodeLabels(node)[0]
        self._asm.emitLabel(label)

        for stmt in node.stmts:
            self.visit(stmt)

    def visit_Default(self, node: c_ast.Default):
        label = self.getNodeLabels(node)[0]
        self._asm.emitLabel(label)

        for stmt in node.stmts:
            self.visit(stmt)

    def visit_Break(self, node: c_ast.Break):
        label = self.getNodeLabels(node)[0]
        self._asm.emit(f"j ${label}")

    def visit_Continue(self, node: c_ast.Continue):
        label = self.getNodeLabels(node)[0]
        self._asm.emit(f"j ${label}")

    def visit_Pragma(self, node: c_ast.Pragma):
        r = self.getNodeRecord(node)

        insts: list[str] = r._pragma.get("ASM", [])
        for inst in insts:
            if inst == "":
                self._asm.emitEmptyLine()
            elif inst.endswith(":"):
                self._asm.emitLabel(inst[:-1])
            else:
                self._asm.emit(inst)

    def visit_Typedef(self, _: c_ast.Typedef):
        match self.getParent():
            case c_ast.FileAST():
                pass
            case _:
                unreachable()
