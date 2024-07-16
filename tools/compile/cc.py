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
        return "%r[%d]" % (self._base, self._dim)


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


class FunctionSignature:
    def __init__(self, ret: Type, args: list[Type], ellipsis: bool) -> None:
        self._ret = ret
        self._args = args
        self._ellipsis = ellipsis

    def __repr__(self) -> str:
        return "(%r => %r)" % (self._args, self._ret)


class FunctionType(Type):
    def __init__(self, sig: FunctionSignature) -> None:
        self._sig = sig
        self._size = 4
        self._alignment = 4

    def __repr__(self) -> str:
        return "%r" % self._sig


# https://en.cppreference.com/w/c/language/value_category
class Value(ABC):
    def getType(self) -> Type:
        return self._type


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


class GlobalVariable(Variable):
    def __init__(self, name: str, ty: Type, _static=False) -> None:
        super().__init__(name, ty)
        self._static = _static


class StaticVariable(Variable):
    def __init__(self, name: str, ty: Type, label: str) -> None:
        super().__init__(name, ty)
        self._label = label


class ExternVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class LocalVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class Argument(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class StrLiteral(LValue):
    def __init__(self, s: str, sOrig: str, ty: Type) -> None:
        super().__init__()
        self._s = s
        self._sOrig = sOrig
        self._type = ArrayType(ty, len(s))

        self._label = ""


# o.x
class MemberAccessExpr(LValue):
    def __init__(self, o: LValue, x: Field) -> None:
        self._o = o
        self._x = x
        self._type = x._type


# p->x
class MemberAccessThroughPointerExpr(LValue):
    def __init__(self, p: Value, x: Field) -> None:
        self._p = p
        self._x = x
        self._type = x._type


# *p
class PointerDereferenceExpr(LValue):
    def __init__(self, p: Value) -> None:
        self._p = p

        ty: PointerType = p.getType()
        self._type = ty._base


# p[i]
class ArraySubscriptExpr(LValue):
    def __init__(self, p: Value, i: Value) -> None:
        self._p = p
        self._i = i

        ty: ArrayType | PointerType = p.getType()
        self._type = ty._base


# &o
class AddressOfExpr(RValue):
    def __init__(self, o: LValue, ty: PointerType = None):
        self._o = o
        self._type = ty or PointerType(o.getType())


class CastExpr(RValue):
    def __init__(self, o: Value, ty: Type) -> None:
        self._o = 0
        self._type = ty


# https://en.cppreference.com/w/c/language/constant_expression
class Constant(RValue):
    pass


class IntConstant(Constant):
    def __init__(self, i: int, ty: IntType) -> None:
        self._i = i
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


class TemporaryValue(RValue):
    def __init__(self, ty: Type) -> None:
        self._type = ty


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
    def __init__(self, offset: int = 0) -> None:
        super().__init__()

        self._offset = offset


class Section:
    def __init__(self, name: str) -> None:
        assert name in [".text", ".rodata", ".data", ".bss"]
        self.name = name
        self.lines: list[str] = []

        self.add(name)

    def addEmptyLine(self):
        self.lines.append("")

    def add(self, s: str, *, indent="    "):
        self.lines.append(indent + s)

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

    def save(self, o: io.StringIO):
        self._secText.save(o)
        self._secRodata.save(o)
        self._secData.save(o)
        self._secBss.save(o)


class NodeRecord:
    def __init__(self) -> None:
        self._type: Type = None
        self._value: Value = None


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


class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []

        self._gScope = GlobalScope()
        self._scopes: list[Scope] = [self._gScope]

        self._func: Function = None

        self._asm = Asm()

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

    def getNodeRecord(self, node: c_ast.Node) -> NodeRecord:
        if node not in self._nodeRecords:
            self._nodeRecords[node] = NodeRecord()
        return self._nodeRecords[node]

    def setNodeType(self, node: c_ast.Node, ty: Type):
        self.getNodeRecord(node)._type = ty

    def getNodeType(self, node: c_ast.Node) -> Type:
        r = self.getNodeRecord(node)
        if r._value:
            return r._value.getType()
        assert r._type
        return r._type

    def setNodeValue(self, node: c_ast.Node, value: Value):
        self.getNodeRecord(node)._value = value

    def getNodeValue(self, node: c_ast.Node) -> Value:
        r = self.getNodeRecord(node)
        assert r._value is not None
        return r._value

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

    def parent(self):
        assert len(self._path) >= 2
        return self._path[-2]

    def getType(self, name: str) -> Type:
        return self._gScope.getType(name)

    def visit(self, node: c_ast.Node):
        try:
            self._path.append(node)
            super().visit(node)
            self._path.pop()
        except CCError as ex:
            print(traceback.format_exc())
            print("%s : %s" % (formatNode(self._path[-1]), ex))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s" % ("  " * (i + 1), formatNode(_)))
            sys.exit(1)

    def enterScope(self):
        self.scopes.append(LocalScope(self.curScope))

    def exitScope(self):
        self.scopes.pop()

    @property
    def curScope(self):
        return self._scopes[-1]

    def save(self, o: io.StringIO):
        self._asm.save(o)

    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.visit(node.type)
        self.setNodeType(node, self.getNodeType(node.type))

    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        self.setNodeType(node, self.curScope.getType(name))

    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        dim = node.dim
        if dim:
            self.visit(node.dim)
            dim = self.getNodeIntConstant(node.dim)
            if dim < 1:
                raise CCError("not greater than zero")

        self.visit(node.type)
        self.setNodeType(node, ArrayType(self.getNodeType(node.type), dim))

    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        self.visit(node.type)
        self.setNodeType(node, PointerType(self.getNodeType(node.type)))

    def visit_Typedef(self, node: c_ast.Typedef):
        self.visit(node.type)
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
            self.visit(decl)
            fields.append(Field(self.getNodeType(decl.type), decl.name))

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

    def visit_NamedInitializer(self, _: c_ast.NamedInitializer):
        # https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html
        raise CCNotImplemented("designated initializer")

    def visit_CompoundLiteral(self, _: c_ast.CompoundLiteral):
        # https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
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
            sig1 = t1._sig
            sig2 = t2._sig
            if sig1._ellipsis != sig2._ellipsis:
                return False
            if len(sig1._args) != len(sig2._args):
                return False
            for i in range(sig1._args):
                at1 = sig1._args[i]
                at2 = sig2._args[i]
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
    def tryConvert(self, t1: Type, v: Value):
        t2 = v.getType()

        if self.isCompatible(t1, t2):
            v._type = t1
            return v

        # array to pointer conversion
        """
        int arr[] = {1, 2, 3};
        int *p1 = arr; // &arr[0]
        int *p2 = &arr; // error: initialization of 'int *' from incompatible pointer type 'int (*)[3]'
        int **p3 = &arr; // error: initialization of 'int **' from incompatible pointer type 'int (*)[3]'
        """
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, ArrayType)
            and isinstance(v, LValue)
            and self.isCompatible(t1._base, t2._base)
        ):
            if isinstance(v, StrLiteral):
                assert v._label
                return SymConstant(v._label, PointerType(v._type._base))
            return AddressOfExpr(
                ArraySubscriptExpr(v, IntConstant(0, self.getType("size_t")))
            )

        # function to pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, FunctionType)
            and self.isCompatible(t1._base, t2)
        ):
            return AddressOfExpr(v, t1)

        # integer conversion
        if isinstance(t1, IntType) and isinstance(t2, IntType):
            if isinstance(v, IntConstant):
                return IntConstant(t1.convert(v._i), t1)
            return CastExpr(v, t1)

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
            return PtrConstant(0, PointerType(self.getType("void")))

        return None

    def convert(self, t1: Type, v: Value):
        res = self.tryConvert(t1, v)
        if res is None:
            raise CCError(f"can not convert {res.getType()} to {t1}")
        return res

    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise CCNotImplemented("bitfield")

        self.visit(node.type)
        ty = self.getNodeType(node.type)

        # struct Foo { int i; };
        if not node.name:
            return

        _global = self.curScope is self._gScope
        _static = "static" in node.quals
        if _global or _static:
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

                def _gen(ty: Type, init: c_ast.Node):
                    if isinstance(ty, ArrayType):
                        if ty._base in [
                            self.getType("char"),
                            self.getType("unsigned char"),
                        ]:
                            if not isinstance(init, c_ast.InitList):
                                self.visit(init)
                                sLit = self.getNodeStrLiteral(init)
                                sec.add(f".asciz {sLit._sOrig}")
                                n = len(sLit._s)
                                if not ty.isComplete():
                                    ty.setDim(n)
                                if n > ty._dim:
                                    raise CCError("initializer-string is too long")
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
                            _gen(field._type, init.exprs[i])
                            left = (
                                (ty._fields[i + 1]._offset if i < n - 1 else ty.size())
                                - field._offset
                                - field._type.size()
                            )
                            if left > 0:
                                sec.add(f".fill {left}")
                    else:
                        if isinstance(init, c_ast.InitList):
                            raise CCError("invalid initializer")

                        self.visit(init)
                        v = self.convert(ty, self.getNodeValue(init))

                        if not isinstance(v, Constant):
                            raise CCError("initializer element is not constant")

                        if isinstance(v, IntConstant):
                            sec.addInt(v)
                        elif isinstance(v, SymConstant):
                            sec.addSym(v)
                        elif isinstance(v, PtrConstant):
                            sec.addPtr(v)
                        else:
                            unreachable()

                _gen(ty, node.init)

            if _static:
                sec.add(f".local ${label}")
            else:
                sec.add(f".global ${label}")

            sec.add(f'.type ${label}, "object"')
            sec.add(f".size ${label}, {ty.size()}")

            return

    def visit_Constant(self, node: c_ast.Constant):
        s: str = node.value
        assert isinstance(s, str)

        try:
            if node.type == "char":
                # https://en.cppreference.com/w/c/language/character_constant
                assert s.startswith("'") and s.endswith("'")
                s = unescapeStr(s[1:-1])
                assert len(s) == 1
                self.setNodeValue(node, IntConstant(ord(s[0]), self.getType("char")))
                return

            if node.type == "string":
                # https://en.cppreference.com/w/c/language/string_literal
                assert s.startswith('"') and s.endswith('"')
                s = unescapeStr(s[1:-1] + "\0")
                sLit = StrLiteral(s, node.value, self.getType("char"))
                self.setNodeValue(node, sLit)

                def _shouldAdd():
                    p = self.parent()
                    if isinstance(p, c_ast.Decl) and isinstance(
                        self.getNodeType(p.type), ArrayType
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

    def visit_FuncDecl(self, node: c_ast.FuncDecl):
        pass

    def visit_FuncDef(self, node: c_ast.FuncDef):
        pass

    @property
    def _skipCodegen(self) -> bool:
        for node in reversed(self._path):
            if isinstance(node, c_ast.UnaryOp) and node.op == "sizeof":
                return True
        return False

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        self.visit(node.expr)
        match node.op:
            case "sizeof":
                ty = self.getNodeType(node.expr)
                ty.checkComplete()
                self.setNodeValue(
                    node,
                    IntConstant(ty.size(), self.getType("size_t")),
                )

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
        pass

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

    def visit_Compound(self, node: c_ast.Compound):
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
        pass

    def visit_Switch(self, node: c_ast.Switch):
        pass

    def visit_While(self, node: c_ast.While):
        pass

    def visit_Goto(self, _: c_ast.Goto):
        raise CCNotImplemented("goto")

    def visit_StaticAssert(self, _: c_ast.StaticAssert):
        raise CCNotImplemented("static assertion")

    def visit_Typename(self, _: c_ast.Typename):
        unreachable()
