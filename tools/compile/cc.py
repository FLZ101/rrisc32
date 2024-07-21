import sys
import io
import traceback

from abc import ABC

from pycparser import c_ast


class CCError(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


class CCNotImplemented(CCError):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


def ccWarn(*args):
    print("[Warning]", *args, file=sys.stderr)


def unreachable(*args):
    raise CCError("unreachable", *args)


def log2(i: int):
    arr = [1, 2, 4, 8]
    return arr.index(i)


# https://en.cppreference.com/w/c/language/type
class Type(ABC):
    def name(self) -> str:
        return getattr(self, "_name", "")

    def isComplete(self) -> bool:
        return getattr(self, "_size", None) is not None

    def checkComplete(self):
        if not self.isComplete():
            raise CCError("incomplete type", self)

    def size(self) -> int:
        assert self.isComplete()
        return self._size

    def alignment(self) -> int:
        return self._alignment

    def fill(self) -> int:
        return getattr(self, "_fill", 0)

    def __repr__(self) -> str:
        if self._name:
            return self._name
        raise NotImplementedError()


class VoidType(Type):
    def __init__(self):
        self._name = "void"


class IntType(Type):
    def __init__(self, name: str, size: int, unsigned=False, alignment: int = 0):
        self._name = name
        self._size = size
        self._alignment = alignment or size
        self._unsigned = unsigned

    def convert(self, i: int):
        n = self._size * 8
        mins = (1 << (n - 1)) - (1 << n)
        minu = 0
        maxs = (1 << (n - 1)) - 1
        maxu = (1 << n) - 1

        if self._unsigned:
            if minu <= i <= maxu:
                return i
            if mins <= i < minu:
                return i + (1 << n)
        else:
            if mins <= i <= maxs:
                return i
            if maxs < i <= maxu:
                return i - (1 << n)

        ccWarn("out of range", self, i)
        i %= 1 << n
        if not self._unsigned:
            i -= 1 << n
        return i


class FloatType(Type):
    def __init__(self, name: str, size: int, alignment: int):
        self._name = name
        self._size = size
        self._alignment = alignment


class ArrayType(Type):
    def __init__(self, base: Type, dim: int = None):
        self._base = base
        self._alignment = base.alignment()
        self.setDim(dim)

    def setDim(self, dim: int):
        self._dim = dim
        self._layout()

    def _layout(self):
        if self._dim is None:
            return
        self._size = self._base.size() * self._dim

    def __repr__(self) -> str:
        return "%r[%s]" % (self._base, self._dim)


class Field:
    def __init__(self, ty: Type, name: str) -> None:
        assert name

        self._type = ty
        self._name = name
        self._offset = 0

    def __repr__(self) -> str:
        return "(%r, %r)" % (self._type, self._name)


def align(offset: int, m: int) -> int:
    n = offset % m
    if n:
        offset += m - n
    return offset


"""
struct Foo {
	int i;
	char c;
};

struct Foo arr[4];

sizeof(struct Foo) // 8
sizeof(arr) // 32
sizeof(arr[0]) // 4
"""


class StructType(Type):
    def __init__(self, fields: list[Field], name: str = ""):
        self._name = name
        self.setFields(fields)

    def setFields(self, fields: list[Field]):
        self._fields = fields
        self._layout()

    def _layout(self):
        if self._fields is None:
            return

        if len(self._fields) == 0:
            raise CCNotImplemented("structure with no field")

        self._fieldByName: dict[str, Field] = {}

        offset = 0
        alignment = 1

        for field in self._fields:
            if field._name in self._fieldByName:
                raise CCError("redefined field", field)
            self._fieldByName[field._name] = field

            m = field._type.alignment()
            alignment = max(alignment, m)

            offset = align(offset, m)
            field._offset = offset

            offset += field._type.size()

        self._alignment = alignment
        self._fill = align(offset, alignment) - offset
        self._size = offset + self._fill

    def __repr__(self) -> str:
        if self._name:
            return self._name
        return "%r" % self._fields


class PointerType(Type):
    def __init__(self, base: Type) -> None:
        self._base = base
        self._size = 4
        self._alignment = 4

    def __repr__(self) -> str:
        return "%r*" % self._base


class FunctionType(Type):
    def __init__(self, ret: Type, args: list[Type], ellipsis: bool) -> None:

        def _cook(ty: Type):
            if isinstance(ty, ArrayType):
                return PointerType(_cook(ty._base))
            if isinstance(ty, StructType):
                raise CCNotImplemented("pass/return struct")
            if isinstance(ty, FunctionType):
                return PointerType(ty)
            return ty

        self._ret = _cook(ret)
        self._args = [_cook(_) for _ in args]
        self._ellipsis = ellipsis

        self._size = 0
        self._alignment = 4

    def __repr__(self) -> str:
        return "(%r => %r)" % (self._args, self._ret)


# https://en.cppreference.com/w/c/language/value_category
class Value(ABC):
    def getType(self) -> Type:
        return self._type


"""
The following expressions are lvalues:

* identifiers
* string literals
* the result of a member access (dot) operator if its left-hand argument is lvalue
* the result of a member access through pointer -> operator
* the result of the indirection (unary *) operator applied to a pointer to object
* the result of the subscription operator ([])
"""


class LValue(Value):
    pass


class RValue(Value):
    pass


"""
A function designator (the identifier introduced by a function declaration) is
an expression of function type. When used in any context other than the
address-of operator, sizeof, and _Alignof (the last two generate compile
errors when applied to functions), the function designator is always converted
to a non-lvalue pointer to function. Note that the function-call operator is
defined for pointers to functions and not for function designators themselves.
"""


class Function(Value):
    def __init__(self, name: str, ty: FunctionType) -> None:
        self._name = name
        self._type = ty


class Variable(LValue):
    def __init__(self, name: str, ty: Type) -> None:
        self._name = name
        self._type = ty

    def __repr__(self) -> str:
        return "(%s, %r)" % (self._name, self._type)

    def load(self, asm: "Asm", r1: str = "a0", r2: str = "a1"):
        _label = getattr(self, "_label", None)
        _offset = getattr(self, "_offset", None)

        ty = self._type
        sz = ty.size()
        if _label is not None:
            match sz:
                case 8:
                    asm.emit(f"lw {r1}, ${_label}")
                    asm.emit(f"lw {r2}, +(${_label} 4)")
                case 4:
                    asm.emit(f"lw {r1}, ${_label}")
                case 2:
                    assert isinstance(ty, IntType)
                    mnemonic = "lhu" if ty._unsigned else "lh"
                    asm.emit(f"{mnemonic} {r1}, ${_label}")
                case 1:
                    assert isinstance(ty, IntType)
                    mnemonic = "lbu" if ty._unsigned else "lb"
                    asm.emit(f"{mnemonic} {r1}, ${_label}")
                case _:
                    unreachable()
        else:
            assert _offset is not None

            match sz:
                case 8:
                    asm.emitFormatI("lw", r1, "fp", _offset)
                    asm.emitFormatI("lw", r2, "fp", _offset + 4)
                case 4:
                    asm.emitFormatI("lw", r1, "fp", _offset)
                case 2:
                    assert isinstance(ty, IntType)
                    mnemonic = "lhu" if ty._unsigned else "lh"
                    asm.emitFormatI(mnemonic, r1, "fp", _offset)
                case 1:
                    assert isinstance(ty, IntType)
                    mnemonic = "lbu" if ty._unsigned else "lb"
                    asm.emitFormatI(mnemonic, r1, "fp", _offset)
                case _:
                    unreachable()

    def push(self, asm: "Asm"):
        self.load(asm, "t0", "t1")
        if self._type.size() == 8:
            asm.emit("push t1")
        asm.emit("push t0")


class GlobalVariable(Variable):
    def __init__(self, name: str, ty: Type, _static=False) -> None:
        super().__init__(name, ty)
        self._label = name
        self._static = _static


class StaticVariable(Variable):
    def __init__(self, name: str, ty: Type, label: str) -> None:
        super().__init__(name, ty)
        self._label = label


class ExternVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)
        self._label = name


class LocalVariable(Variable):
    def __init__(self, name: str, ty: Type, offset: int) -> None:
        super().__init__(name, ty)
        self._offset = offset


class Argument(Variable):
    def __init__(self, name: str, ty: Type, offset: int) -> None:
        super().__init__(name, ty)
        self._offset = offset


class StrLiteral(LValue):
    def __init__(self, s: str, sOrig: str, ty: Type) -> None:
        super().__init__()
        self._s = s
        self._sOrig = sOrig
        self._type = ArrayType(ty, len(s))

        self._label = ""


# https://en.cppreference.com/w/c/language/constant_expression
class Constant(RValue):

    def load(self, asm: "Asm", r1: str = "a0", r2: str = "a1"):
        sz = self._type.size()
        assert sz in [1, 2, 4, 8]
        i = getattr(self, "_i", None)
        if i is not None:
            if sz == 8:
                asm.emit([f"li {r2}, {i >> 32}", f"li {r1}, { i & 0xffffffff}"])
            else:
                asm.emit(f"li {r1}, {i}")
        else:
            raise NotImplementedError()

    def push(self, asm: "Asm"):
        self.load(asm, "t0", "t1")
        if self._type.size() == 8:
            asm.emit("push t1")
        asm.emit("push t0")


class IntConstant(Constant):
    def __init__(self, i: int, ty: IntType) -> None:
        self._i = ty.convert(i)
        self._type = ty


class PtrConstant(Constant):
    def __init__(self, i: int, ty: PointerType) -> None:
        self._i = i
        self._type = ty


class SymConstant(Constant):
    def __init__(self, name: str, ty: PointerType, offset: int = 0) -> None:
        super().__init__()
        self._name = name
        self._type = ty
        self._offset = offset

        if offset:
            assert ty.isComplete()

    def load(self, asm: "Asm", r1: str = "a0", _: str = "a1"):
        if self._offset:
            asm.emit(f"li {r1}, +(${self._name} {self._offset})")
        else:
            asm.emit(f"li {r1}, ${self._name}")


class TemporaryValue(RValue):
    def __init__(self, ty: Type) -> None:
        self._type = ty

    def load(self, asm: "Asm", r1: str = "a0", r2: str = "a1"):
        sz = self._type.size()
        assert sz in [1, 2, 4, 8]
        if sz == 8:
            if r2 != "a1":
                asm.emit(f"mv {r2}, a1")
        if r1 != "a0":
            asm.emit(f"mv {r1}, a0")

    def push(self, asm: "Asm"):
        sz = self._type.size()
        assert sz in [1, 2, 4, 8]
        if sz == 8:
            asm.emit("push a1")
        asm.emit("push a0")


"""
int i;
i = 1;
(int)i = 2; // error: assignment to cast is illegal
"""


class CastedLValue(RValue):
    def __init__(self, v: LValue) -> None:
        self._type = v.getType()
        self._v = v

    def load(self, asm: "Asm", r1: str = "a0", r2: str = "a1"):
        match self._v:
            case Variable() | TemporaryValue():
                self._v.load(asm, r1, r2)
            case _:
                unreachable()

    def push(self, asm: "Asm"):
        match self._v:
            case Constant() | Variable() | TemporaryValue():
                self._v.push(asm)
            case _:
                unreachable()


class Scope(ABC):
    def __init__(self, prev=None) -> None:
        self._symbolTable: dict[str, Type | Variable | Function] = {}
        self._prev: Scope = prev

    def addSymbol(self, name: str, value: Type | Variable | Function):
        if name in self._symbolTable:
            raise CCError("redefined", name)
        self._symbolTable[name] = value

    def findSymbol(self, name: str) -> Type | Variable | Function:
        if name in self._symbolTable:
            return self._symbolTable[name]
        if self._prev:
            return self._prev.getSymbol(name)
        return None

    def getSymbol(self, name: str):
        res = self.findSymbol(name)
        if res is None:
            raise CCError("undefined", name)
        return res

    def getType(self, name: str):
        ty = self.getSymbol(name)
        if not isinstance(ty, Type):
            raise CCError("not a type", name)
        return ty

    def getStructType(self, name: str):
        ty = self.getType(name)
        if not isinstance(ty, StructType):
            raise CCError("not a struct type", name)
        return ty

    def getVariable(self, name: str):
        var = self.getSymbol(name)
        if not isinstance(var, Variable):
            raise CCError("not a variable", name)
        return var

    def getFunction(self, name: str):
        func = self.getSymbol(name)
        if not isinstance(func, Function):
            raise CCError("not a function", name)
        return func


class GlobalScope(Scope):
    def __init__(self) -> None:
        super().__init__()


class LocalScope(Scope):
    def __init__(self, prev: Scope) -> None:
        super().__init__(prev)

        self._offset: int = 0
        if isinstance(prev, LocalScope):
            self._offset = prev._offset


class Section:
    def __init__(self, name: str) -> None:
        assert name in [".text", ".rodata", ".data", ".bss"]
        self.name = name
        self.lines: list[str] = []

        self.add(name)

    def addEmptyLine(self):
        self.lines.append("")

    def add(self, s: str | list[str], *, indent="    "):
        if type(s) is list:
            for _ in s:
                self.add(_, indent=indent)
        else:
            self.lines.append(indent + s)

    def addRaw(self, s: str, *, indent="    "):
        for line in s.splitlines():
            line = line.strip()
            if line == "":
                continue
            if line.endswith(":"):
                self.lines.append(line)
            else:
                self.add(line, indent=indent)

    def addInt(self, ic: IntConstant):
        c = "bhwq"[log2(ic.getType().size())]
        self.add(f".d{c} {ic._i}")

    def addPtr(self, pc: PtrConstant):
        self.add(f".dw {pc._i}")

    def addSym(self, sc: SymConstant):
        if sc._offset == 0:
            self.add(f".dw ${sc._name}")
        else:
            self.add(f".dw +(${sc._name} {sc._offset})")

    def addConstant(self, c: Constant):
        match c:
            case IntConstant():
                self.addInt(c)
            case PtrConstant():
                self.addPtr(c)
            case SymConstant():
                self.addSym(c)
            case _:
                unreachable()

    def addLabel(self, s: str):
        self.add(s + ":", indent="")
        return s

    def addStaticLabel(self, s: str):
        return self.addLabel("%s_%s%d" % (s, self.name[1], self._i_static))

    def addLocalLabel(self):
        return self.addLabel(".L_%s%d" % (self.name[1], self._i))

    @property
    def _i(self, *, _d={"i": 0}):
        _d["i"] += 1
        return _d["i"]

    @property
    def _i_static(self, *, _d={"i": 0}):
        _d["i"] += 1
        return _d["i"]

    def save(self, o: io.StringIO):
        for line in self.lines:
            print(line, file=o)
        print("", file=o)


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

    def emitFormatI(self, mnemonic: str, rd: str, rs1: str, i: int):
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

    def emitFormatS(self, mnemonic: str, rs1: str, rs2: str, i: int):
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

    def save(self, o: io.StringIO):
        self._secText.save(o)
        self._secRodata.save(o)
        self._secData.save(o)
        self._secBss.save(o)


def unescapeStr(s: str) -> str:
    arr = []

    for c in s:
        x = ord(c)
        if x > 255:
            raise SyntaxError("invalid character", c)

    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\":
            i += 1
            match s[i : i + 1]:
                case "n":
                    arr.append("\n")
                case "t":
                    arr.append("\t")
                case "0":
                    arr.append("\0")
                case '"':
                    arr.append('"')
                case "\\":
                    arr.append("\\")
                case "x":
                    i += 2
                    arr.append(chr(int(s[i - 1 : i + 1], base=16)))
                case _:
                    raise SyntaxError("invalid escape sequence", repr(s[i - 1 : i + 1]))
        else:
            arr.append(c)

        i += 1

    return "".join(arr)


def formatNode(node: c_ast.Node):
    return "%s %s" % (node.__class__.__qualname__, node.coord)


class NodeRecord:
    def __init__(self, *, ty: Type | None = None, v: Value | None = None) -> None:
        self._type: Type = ty
        self._value: Value = v


class Node(c_ast.Node):
    def __init__(self, *, ty: Type | None = None, v: Value | None = None) -> None:
        self._type = ty
        self._value = v


class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []

        self._gScope = GlobalScope()
        self._scopes: list[Scope] = [self._gScope]

        self._func: Function = None

        self._asm = Asm()

        self._skipCodegen = False

        def _addType(ty: Type, names: list[str] | None = None):
            if names is None:
                names = []
            if ty.name():
                names.append(ty.name())
            assert len(names) > 0
            for name in names:
                self._gScope.addSymbol(name, ty)

        def _addBasicTypes():
            _addType(VoidType())

            _addType(IntType("char", 1), ["signed char"])
            _addType(
                IntType("short", 2), ["short int", "signed short", "signed short int"]
            )
            _addType(IntType("int", 4), ["signed int"])
            _addType(
                IntType("long", 4),
                ["long int", "signed long", "signed long int", "ssize_t"],
            )
            _addType(
                IntType("long long", 8, False, 4),
                ["long long int", "signed long long", "signed long long int"],
            )

            _addType(IntType("unsigned char", 1, True))
            _addType(IntType("unsigned short", 2, True), ["unsigned short int"])
            _addType(IntType("unsigned int", 4, True), ["unsigned"])
            _addType(IntType("unsigned long", 4, True), ["unsigned long int", "size_t"])
            _addType(
                IntType("unsigned long long", 8, True, 4), ["unsigned long long int"]
            )

            # _addType(FloatType("float", 4))
            # _addType(FloatType("double", 4))

        _addBasicTypes()

        """
        User-defined classes have __eq__() and __hash__() methods by default;
        with them, all objects compare unequal (except with themselves) and
        x.__hash__() returns an appropriate value such that x == y implies
        both that x is y and hash(x) == hash(y).
        """
        self._nodeRecords: dict[c_ast.Node, NodeRecord] = {}

    def getNodeRecord(self, node: c_ast.Node) -> NodeRecord | Node:
        if isinstance(node, Node):
            return node

        self.visit(node)
        if node not in self._nodeRecords:
            pass
        return self._nodeRecords[node]

    def setNodeRecord(
        self, node: c_ast.Node, *, ty: Type | None = None, v: Value | None = None
    ):
        assert node not in self._nodeRecords and not isinstance(node, Node)
        self._nodeRecords[node] = NodeRecord(ty=ty, v=v)

    def setNodeType(self, node: c_ast.Node, ty: Type):
        self.setNodeRecord(node, ty=ty)

    def setNodeValue(self, node: c_ast.Node, v: Value):
        self.setNodeRecord(node, v=v)

    def getNodeType(self, node: c_ast.Node) -> Type:
        r = self.getNodeRecord(node)
        if r._value:
            return r._value.getType()
        assert r._type
        return r._type

    def getNodeValue(self, node: c_ast.Node, ty: Type | None = None) -> Value:
        r = self.getNodeRecord(node)
        return r._value if ty is None else self.convert(ty, r._value)

    def loadValue(self, v: Value, r1: str = "a0", r2: str = "a1"):
        match v:
            case Constant() | Variable() | TemporaryValue() | CastedLValue():
                v.load(self._asm, r1, r2)
            case _:
                unreachable()

    def loadNodeValue(
        self,
        node: c_ast.Node,
        ty: Type | None = None,
        r1: str = "a0",
        r2: str = "a1",
    ):
        v = self.getNodeValue(node, ty)
        self.loadValue(v, r1, r2)

    def getNodeIntConstant(self, node: c_ast.Node) -> int:
        ic = self.getNodeValue(node)
        if not isinstance(ic, IntConstant):
            raise CCError("not an integral constant expression")
        return ic._i

    def getNodeStrLiteral(self, node: c_ast.Node) -> StrLiteral:
        sLit = self.getNodeValue(node)
        if not isinstance(sLit, StrLiteral):
            raise CCError("not a string literal")
        return sLit

    def parent(self, i=1):
        n = len(self._path)
        assert n >= 2
        if i >= n:
            return None
        return self._path[-(i + 1)]

    def getType(self, name: str) -> Type:
        return self._gScope.getType(name)

    def getIC(self, i: int, ty: str = None):
        ty = ty or "int"
        return IntConstant(i, self.getType(ty))

    def visit(self, node: c_ast.Node):
        if isinstance(node, Node):
            return
        if node in self._nodeRecords:
            return

        try:
            self._path.append(node)

            _sizeof = isinstance(node, c_ast.UnaryOp) and node.op == "sizeof"
            if _sizeof:
                _skipCodegen = self._skipCodegen
                self._skipCodegen = True

            super().visit(node)

            if _sizeof:
                self._skipCodegen = _skipCodegen

            self._path.pop()
        except CCError as ex:
            print(traceback.format_exc())
            print("%s : %s" % (formatNode(self._path[-1]), ex))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s" % ("  " * (i + 1), formatNode(_)))
            sys.exit(1)

    def enterScope(self):
        self._scopes.append(LocalScope(self.curScope))

    def exitScope(self):
        self._scopes.pop()

    @property
    def curScope(self):
        return self._scopes[-1]

    def save(self, o: io.StringIO):
        self._asm.save(o)

    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.setNodeType(node, self.getNodeType(node.type))

    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        self.setNodeType(node, self.curScope.getType(name))

    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        dim = node.dim
        if dim:
            dim = self.getNodeIntConstant(node.dim)
            if dim < 1:
                raise CCError("not greater than zero")

        self.setNodeType(node, ArrayType(self.getNodeType(node.type), dim))

    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        self.setNodeType(node, PointerType(self.getNodeType(node.type)))

    def visit_Typedef(self, node: c_ast.Typedef):
        self.curScope.addSymbol(node.name, self.getNodeType(node.type))

    def visit_Struct(self, node: c_ast.Struct):
        if node.decls is None:  # declaration
            if not self.curScope.findSymbol(node.name):
                self.curScope.addSymbol(node.name, StructType(None, node.name))
            self.setNodeType(node, self.curScope.getStructType(node.name))
            return

        fields: list[Field] = []
        for decl in node.decls:
            decl: c_ast.Decl
            if not decl.name:
                raise CCNotImplemented("anonymous field")
            fields.append(Field(self.getNodeType(decl), decl.name))

        ty = self.curScope.findSymbol(node.name)
        if isinstance(ty, StructType) and not ty.isComplete():  # declared
            ty.setFields(fields)
            return

        self.setNodeType(node, StructType(fields, node.name))
        if node.name:
            self.curScope.addSymbol(node.name, self.getNodeType(node))

    def visit_Union(self, _: c_ast.Union):
        raise CCNotImplemented("union")

    def visit_Enum(self, _: c_ast.Enum):
        raise CCNotImplemented("enum")

    # https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html
    def visit_NamedInitializer(self, _: c_ast.NamedInitializer):
        raise CCNotImplemented("designated initializer")

    # https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
    def visit_CompoundLiteral(self, _: c_ast.CompoundLiteral):
        raise CCNotImplemented("compound literal")

    # https://en.cppreference.com/w/c/language/type#Compatible_types
    def isCompatible(self, t1: Type, t2: Type):
        if t1 is t2:
            return True

        if not t1.isComplete() or not t2.isComplete():
            return False

        if isinstance(t1, PointerType) and isinstance(t2, PointerType):
            return self.isCompatible(t1._base, t2._base)

        if isinstance(t1, ArrayType) and isinstance(t2, ArrayType):
            return t1.size() == t2.size() and self.isCompatible(t1._base, t2._base)

        if isinstance(t1, StructType) and isinstance(t2, StructType):
            if len(t1._fields) != len(t2._fields):
                return False
            for i in range(len(t1._fields)):
                f1 = t1._fields[i]
                f2 = t2._fields[i]
                if f1._name != f2._name or not self.isCompatible(f1._type, f2._type):
                    return False
            return True

        if isinstance(t1, FunctionType) and isinstance(t2, FunctionType):
            if t1._ellipsis != t2._ellipsis:
                return False
            if len(t1._args) != len(t2._args):
                return False
            for i in range(t1._args):
                at1 = t1._args[i]
                at2 = t2._args[i]
                if self.isCompatible(at1, at2):
                    continue

                def _isArrayToPointerCompatible(_t1: Type, _t2: Type):
                    return (
                        isinstance(_t1, ArrayType)
                        and isinstance(_t2, PointerType)
                        and self.isCompatible(_t1._base, _t2._base)
                    )

                def _isFunctionToPointerCompatible(_t1: Type, _t2: Type):
                    return (
                        isinstance(_t1, FunctionType)
                        and isinstance(_t2, PointerType)
                        and isinstance(_t2._base, FunctionType)
                        and self.isCompatible(_t1, _t2._base)
                    )

                if _isArrayToPointerCompatible(at1, at2):
                    continue
                if _isArrayToPointerCompatible(at2, at1):
                    continue

                if _isFunctionToPointerCompatible(at1, at2):
                    continue
                if _isFunctionToPointerCompatible(at2, at1):
                    continue

                return False

            return True

        return False

    # https://en.cppreference.com/w/c/language/conversion
    def tryConvert(self, t1: Type, v: Value) -> Value:
        t2 = v.getType()

        if self.isCompatible(t1, t2):
            v._type = t1
            return v

        # array to pointer conversion
        """
        int arr[] = {1, 2, 3};
        int *p1 = arr; // &arr[0]

        int *p2 = &arr;
        // error: initialization of 'int *' from incompatible pointer type 'int (*)[3]'

        int **p3 = &arr;
        // error: initialization of 'int **' from incompatible pointer type 'int (*)[3]'
        """
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, ArrayType)
            and isinstance(v, LValue)
            and self.isCompatible(t1._base, t2._base)
        ):
            # TODO remove
            if isinstance(v, StrLiteral):
                assert v._label
                return SymConstant(v._label, PointerType(v._type._base))

            # &arr[0]
            node = c_ast.UnaryOp(
                "&", c_ast.ArrayRef(Node(v=v), Node(v=self.getIC(0, "size_t")))
            )
            return self.getNodeValue(node, t1)

        # function to pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, FunctionType)
            and self.isCompatible(t1._base, t2)
        ):
            assert isinstance(v, Function)
            return SymConstant(v._name, t1)

        # integer conversion
        if isinstance(t1, IntType) and isinstance(t2, IntType):
            if isinstance(v, IntConstant):
                return IntConstant(v._i, t1)

            sz1 = t1.size()
            sz2 = t2.size()
            if sz1 == sz2:
                v._type = t1
                return v

            if sz1 > sz2:
                if sz1 < 8:
                    v._type = t1
                    return v
                self.loadValue(v)
                self._asm.emit("srai a1, a0, 31")
                return TemporaryValue(t1)

            self.loadValue(v)
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

        # pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, PointerType)
            and t2._base is self.getType("void")
        ):
            v._type = t1
            return v
        if (
            isinstance(t1, PointerType)
            and t1._base is self.getType("void")
            and isinstance(v, IntConstant)
            and v._i == 0
        ):
            return PtrConstant(0, t1)

        return None

    def convert(self, t1: Type, v: Value):
        res = self.tryConvert(t1, v)
        if res is None:
            raise CCError(f"can not convert {v.getType()} to {t1}")
        return res

    # https://en.cppreference.com/w/c/language/cast
    def cast(self, t1: Type, v: Value):
        res = self.tryConvert(t1, v)
        if res is None:
            t2 = v.getType()
            match t1, t2:
                # Any integer can be cast to any pointer type.
                case PointerType(), IntType():
                    res = self.tryConvert(self.getType("unsigned long"), v)
                    match res:
                        case IntConstant():
                            res = PtrConstant(res._i, t1)
                        case Variable() | TemporaryValue():
                            res.load(self._asm)
                            res = TemporaryValue(t1)
                        case _:
                            unreachable()

                # Any pointer type can be cast to any integer type.
                case IntType(), PointerType():
                    match v:
                        case PtrConstant():
                            res = IntConstant(v._i, t1)
                        case Constant() | Variable() | TemporaryValue():
                            v.load(self._asm)
                            res = TemporaryValue(self.getType("unsigned long"))
                            res = self.tryConvert(t1, res)
                        case _:
                            unreachable()

                # Any pointer to object can be cast to any other pointer to object.
                # Any pointer to function can be cast to a pointer to any other function type.
                case PointerType(), PointerType():
                    _1 = isinstance(t1._base, FunctionType)
                    _2 = isinstance(t2._base, FunctionType)
                    if _1 == _2:
                        v._type = t1
                        res = v

        if res is None:
            raise CCError(f"can not cast {v.getType()} to {t1}")

        # The value category of the cast expression is always non-lvalue.
        if isinstance(res, LValue):
            res = CastedLValue(res)
        return res

    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise CCNotImplemented("bitfield")

        ty = self.getNodeType(node.type)
        if ty is self.getType("void"):
            ty.checkComplete()  # trigger an exception

        self.setNodeType(node, ty)

        # struct Foo { int i; };
        if not node.name:
            return

        if isinstance(ty, FunctionType):
            self.curScope.addSymbol(node.name, Function(node.name, ty))
            return

        if "extern" in node.quals:
            self.curScope.addSymbol(node.name, ExternVariable(node.name, ty))
            return

        if isinstance(self.parent(), c_ast.Struct):  # fields
            return

        if isinstance(self.parent(), c_ast.FuncDecl):  # arguments
            return

        _global = self.curScope is self._gScope
        _static = "static" in node.quals
        _local = not _global and not _static

        _path = []

        def _genRef():
            res = c_ast.ID(node.name)
            for x in _path:
                match x:
                    case int():
                        res = c_ast.ArrayRef(res, c_ast.Constant("int", f"{x}"))
                    case str():
                        res = c_ast.StructRef(res, ".", c_ast.ID(x))
                    case _:
                        unreachable()
            return res

        def _gen(ty: Type, init: c_ast.Node):
            if isinstance(ty, ArrayType):
                if ty._base in [
                    self.getType("char"),
                    self.getType("unsigned char"),
                ]:
                    if not isinstance(init, c_ast.InitList):  # char s[] = "hello"
                        sLit = self.getNodeStrLiteral(init)

                        if not _local:
                            sec.add(f".asciz {sLit._sOrig}")
                        else:
                            for i, c in enumerate(sLit._s):
                                _path.append(i)
                                _gen(ty._base, c_ast.Constant("int", f"{ord(c)}"))
                                _path.pop()

                        n = len(sLit._s)
                        if not ty.isComplete():
                            ty.setDim(n)
                        if n > ty._dim:
                            raise CCError("initializer-string is too long")

                        if not _local:
                            left = ty.size() - n
                            if left > 0:
                                sec.add(f".fill {left}")
                        return

                if not isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")

                n = len(init.exprs)
                if not ty.isComplete():
                    ty.setDim(n)
                ty.checkComplete()

                if n > ty._dim:
                    raise CCError("too many elements in array initializer")

                for i in range(n):
                    _gen(ty._base, init.exprs[i])

                left = ty.size() - n * ty._base.size()
                if left > 0:
                    sec.add(f".fill {left}")

            elif isinstance(ty, StructType):
                if not isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")
                ty.checkComplete()

                n = len(init.exprs)
                if n > len(ty._fields):
                    raise CCError("too many elements in struct initializer")

                for i in range(n):
                    field = ty._fields[i]

                    if not _local:
                        _gen(field._type, init.exprs[i])
                        left = (
                            (ty._fields[i + 1]._offset if i < n - 1 else ty.size())
                            - field._offset
                            - field._type.size()
                        )
                        if left > 0:
                            sec.add(f".fill {left}")
                    else:
                        _path.append(field._name)
                        _gen(field._type, init.exprs[i])
                        _path.pop()

            else:
                if isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")

                if not _local:
                    v = self.getNodeValue(init, ty)
                    if not isinstance(v, Constant):
                        raise CCError("initializer element is not constant")
                    sec.addConstant(v)
                else:
                    self.visit(c_ast.Assignment("=", _genRef(), init))

        if _global or _static:  # global/static variables
            sec = self._asm._secData if node.init else self._asm._secBss

            sec.addEmptyLine()
            _ = log2(ty.alignment())
            if _ > 0:
                sec.add(f".align {_}")

            if _global:
                label = sec.addLabel(node.name)
                self.curScope.addSymbol(
                    node.name, GlobalVariable(node.name, ty, _static)
                )
            else:
                label = sec.addStaticLabel(node.name)
                self.curScope.addSymbol(node.name, StaticVariable(node.name, ty, label))

            if not node.init:
                ty.checkComplete()
                sec.add(f".fill {ty.size()}")
            else:
                _gen(ty, node.init)

            if _static:
                sec.add(f".local ${label}")
            else:
                sec.add(f".global ${label}")

            sec.add(f'.type ${label}, "object"')
            sec.add(f".size ${label}, -($. ${label})")

        else:  # local variables
            scope: LocalScope = self.curScope
            scope._offset += align(ty.size(), 4)
            scope.addSymbol(node.name, LocalVariable(node.name, ty, scope._offset))
            if node.init:
                self.emitBuiltinCall(
                    "memset",
                    c_ast.UnaryOp("&", c_ast.ID(node.name)),  # s
                    self.getIC(0),  # c
                    self.getIC(ty.size(), "size_t"),  # n
                )
                # translate the initialization into assignments
                _gen(ty, node.init)

    def visit_Constant(self, node: c_ast.Constant):
        s: str = node.value
        assert isinstance(s, str)

        try:
            if node.type == "char":
                # https://en.cppreference.com/w/c/language/character_constant
                assert s.startswith("'") and s.endswith("'")
                s = unescapeStr(s[1:-1])
                assert len(s) == 1
                self.setNodeValue(node, self.getIC(ord(s[0]), "char"))
                return

            if node.type == "string":
                # https://en.cppreference.com/w/c/language/string_literal
                assert s.startswith('"') and s.endswith('"')
                s = unescapeStr(s[1:-1] + "\0")
                sLit = StrLiteral(s, node.value, self.getType("char"))
                self.setNodeValue(node, sLit)

                def _shouldAdd():
                    # char s[] = "hello";
                    p = self.parent()
                    if isinstance(p, c_ast.Decl) and isinstance(
                        self.getNodeType(p), ArrayType
                    ):
                        return False
                    return True

                if _shouldAdd():
                    self._asm.addStr(sLit)

                return

            if "int" in node.type:
                # https://en.cppreference.com/w/c/language/integer_constant
                ty = self.getType(node.type)

                s = s.lower()
                while s.endswith("u") or s.endswith("l"):
                    s = s[:-1]
                if s.startswith("0x"):
                    i = int(s[2:], base=16)
                else:
                    i = int(s, base=10)
                self.setNodeValue(node, IntConstant(i, ty))
                return

            raise CCError("%s is unsupported" % node.type)

        except Exception as ex:
            raise CCError("invalid literal", str(ex))

    def visit_ID(self, node: c_ast.ID):
        _ = self.curScope.getSymbol(node.name)
        if isinstance(_, Type):
            self.setNodeType(node, _)
        else:
            self.setNodeValue(node, _)

    def visit_FuncDef(self, node: c_ast.FuncDef):
        if node.param_decls is not None:
            raise CCNotImplemented("old-style (K&R) function definition")

        self.enterScope()

        decl: c_ast.Decl = node.decl
        self.visit(decl)

        funcName = decl.name
        self._func = self.curScope.getFunction(funcName)
        funcType: FunctionType = self._func.getType()
        funcDecl: c_ast.FuncDecl = decl.type

        sec = self._asm._secText
        sec.addEmptyLine()

        if "static" in decl.quals:
            sec.add(f".local ${funcName}")
        else:
            sec.add(f".global ${funcName}")

        sec.add(f'.type ${funcName}, "function"')

        sec.add(".align 2")
        sec.addLabel(funcName)

        # declare arguments
        offset = 8
        for i, argTy in enumerate(funcType._args):
            if isinstance(funcDecl.args[i], c_ast.Decl):
                argName = funcDecl.args[i].name
                offset = align(offset, 4)
                self.curScope.addSymbol(argName, Argument(argName, argTy, offset))
                offset += argTy.size()

        self.visit(node.body)

        sec.add(f".size ${funcName}, -($. ${funcName})")
        self._func = None

        self.exitScope()

    # https://en.cppreference.com/w/c/language/function_declaration
    def visit_FuncDecl(self, node: c_ast.FuncDecl):
        ret = self.getNodeType(node.type)

        args: list[Type] = []
        ellipsis = False

        if node.args is None:
            node.args = []
        else:
            node.args = node.args.params

        n = len(node.args)
        for i, arg in enumerate(node.args):
            arg: c_ast.Decl | c_ast.Typename
            if isinstance(arg, c_ast.EllipsisParam):
                assert i == n - 1
                ellipsis = True
                break

            ty = self.getNodeType(arg)
            if isinstance(ty, VoidType):
                if n != 1:
                    raise CCError("'void' must be the only argument")
                # ignore the difference between f(void) and f()
                break
            args.append(ty)

        self.setNodeType(
            node,
            FunctionType(ret, args, ellipsis),
        )

    def visit_Typename(self, node: c_ast.Typename):
        self.setNodeType(node, self.getNodeType(node.type))

    def visit_Compound(self, node: c_ast.Compound):
        isFuncBody = isinstance(self.parent(), c_ast.FuncDef)
        if isFuncBody:
            self._asm.emitPrelogue()
        else:
            self.enterScope()

        block_items = node.block_items or []
        for _ in block_items:
            self.visit(_)

        if isFuncBody:
            if len(block_items) == 0 or not isinstance(block_items[-1], c_ast.Return):
                self._asm.emitRet()
        else:
            self.exitScope()

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        self.visit(node.expr)
        match node.op:
            case "sizeof":
                match node.expr:
                    case c_ast.Typename() | c_ast.ID():
                        ty = self.getNodeType(node.expr)
                        ty.checkComplete()
                        self.setNodeValue(
                            node,
                            self.getIC(ty.size(), "size_t"),
                        )
                    case _:
                        raise CCNotImplemented()

            case "&":
                v = self.getNodeValue(node.expr)
                if isinstance(v, GlobalVariable) or isinstance(v, StaticVariable):
                    self.setNodeValue(
                        node, SymConstant(v._name, PointerType(v.getType()))
                    )
                # TODO

            case _:
                raise CCError("unknown unary operator", node.op)

    def visit_Cast(self, node: c_ast.Cast):
        v = self.cast(self.getNodeType(node.to_type), self.getNodeValue(node.expr))
        self.setNodeValue(node, v)

    def visit_ArrayRef(self, node: c_ast.ArrayRef):
        pass

    def visit_StructRef(self, node: c_ast.StructRef):
        pass

    def visit_Assignment(self, node: c_ast.Assignment):
        pass

    def visit_BinaryOp(self, node: c_ast.BinaryOp):
        pass

    def visit_TernaryOp(self, node: c_ast.TernaryOp):
        pass

    def visit_FuncCall(self, node: c_ast.FuncCall):
        pass

    def visit_Alignas(self, _: c_ast.Alignas):
        raise CCNotImplemented("alignas")

    def visit_Break(self, node: c_ast.Break):
        pass

    def visit_Case(self, node: c_ast.Case):
        pass

    def visit_Continue(self, node: c_ast.Continue):
        pass

    def visit_Default(self, node: c_ast.Default):
        pass

    def visit_DoWhile(self, node: c_ast.DoWhile):
        pass

    def visit_For(self, node: c_ast.For):
        pass

    def visit_If(self, node: c_ast.If):
        pass

    def visit_Return(self, node: c_ast.Return):
        retTy = self._func._type._ret
        if node.expr:
            if isinstance(retTy, VoidType):
                raise CCError("'return' with a value, in function returning void")
            self.visit(node.expr)
            self.loadNodeValue(node.expr, retTy)
        else:
            if not isinstance(retTy, VoidType):
                raise CCError("'return' with no value, in function returning non-void")
        self._asm.emitRet()

    def visit_Switch(self, node: c_ast.Switch):
        pass

    def visit_While(self, node: c_ast.While):
        pass

    def visit_Goto(self, _: c_ast.Goto):
        raise CCNotImplemented("goto")

    def visit_StaticAssert(self, _: c_ast.StaticAssert):
        raise CCNotImplemented("static assertion")

    def emitBuiltinCall(self, name: str, *args: Value | c_ast.Node):
        if name in self._builtins:
            self._builtins[name] += 1
        self.emitCall(f"__builtin_{name}", *args)

    def emitCall(self, name: str, *args: Value | c_ast.Node):
        n = 0
        # push arguments
        for arg in reversed(args):
            if isinstance(arg, c_ast.Node):
                self.visit(arg)
                arg = self.getNodeValue(arg)
            match arg:
                case Constant() | Variable() | TemporaryValue() | CastedLValue():
                    arg.push(self._asm)
                case _:
                    unreachable()
            n += align(arg.getType().size(), 4)

        self._asm.emit(f"call ${name}")

        # restore sp
        if n > 0:
            self._asm.emit(f"addi sp, sp, -${n}")

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
        sec = self._asm._secText
        sec.add(f".global ${name}")
        sec.add(f'.type ${name}, "function"')
        sec.add(".align 2")
        sec.addLabel(f"{name}")

        # load arguments
        Argument("s", PointerType(self.getType("void")), 8).load(self._asm, "a0")
        Argument("c", self.getType("int"), 12).load(self._asm, "a1")
        Argument("n", self.getType("size_t"), 16).load(self._asm, "a2")

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
                getattr(self, f"_emitBuiltin{name[0].upper() + name[1:]}")()
