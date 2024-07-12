import sys
import io

from abc import ABC

from pycparser import c_ast


class SemaError(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


# https://en.cppreference.com/w/c/language/type
class Type(ABC):
    def name(self) -> str:
        return getattr(self, "_name", "")

    def isComplete(self) -> bool:
        return getattr(self, "_size") is not None

    def checkComplete(self):
        if not self.isComplete():
            raise SemaError("incomplete type", self)

    def size(self) -> int:
        assert self.isComplete()
        return self._size

    def alignment(self) -> int:
        return self._alignment

    def p2align(self) -> int:
        arr = [1, 2, 4]
        assert self._alignment in arr
        return arr.index(self._alignment)

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

    def convert(self, i: int) -> int:
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

        raise SemaError("out of range", self, i)


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
        if self._dim == 0:
            raise SemaError("array of length 0 is not supported")
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


def _align(offset: int, m: int) -> int:
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
            raise SemaError("structure with no field is not supported")

        self._fieldByName: dict[str, Field] = {}

        offset = 0
        alignment = 1

        for field in self._fields:
            if field._name in self._fieldByName:
                raise SemaError("redefined field", field)
            self._fieldByName[field._name] = field

            m = field._type.alignment()
            alignment = max(alignment, m)

            offset = _align(offset, m)
            field._offset = offset

            offset += field._type.size()

        self._alignment = alignment
        self._fill = _align(offset, alignment) - offset
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
    def __init__(self, ret: Type, args: list[Type]) -> None:
        self._ret = ret
        self._args = args

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
    def __init__(self, label: str, ty: ArrayType, s: str) -> None:
        super().__init__()
        self._label = label
        self._type = ty
        self._s = s


# o.x
class DotExpr(LValue):
    def __init__(self, o: LValue, x: Field) -> None:
        self._o = o
        self._x = x
        self._type = x._type


# p->x
class ArrowExpr(LValue):
    def __init__(self, p: Value, x: Field) -> None:
        self._p = p
        self._x = x
        self._type = x._type


# *p
class StarExpr(LValue):
    def __init__(self, p: Value) -> None:
        self._p = p

        ty: PointerType = p.getType()
        self._type = ty._base


# p[i]
class BrackeExpr(LValue):
    def __init__(self, p: Value, i: Value) -> None:
        self._p = p
        self._i = i

        ty: ArrayType | PointerType = p.getType()
        self._type = ty._base


# https://en.cppreference.com/w/c/language/constant_expression
class Constant(RValue):
    pass


class IntConstant(Constant):
    def __init__(self, i: int, ty: IntType) -> None:
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
            raise SemaError("redefined", name)
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
            raise SemaError("undefined", name)
        return res

    def getType(self, name: str):
        ty = self.getSymbol(name)
        if not isinstance(ty, Type):
            raise SemaError("not a type", name)
        return ty

    def getStructType(self, name: str):
        ty = self.getType(name)
        if not isinstance(ty, StructType):
            raise SemaError("not a struct type", name)
        return ty

    def getVariable(self, name: str):
        var = self.getSymbol(name)
        if not isinstance(var, Variable):
            raise SemaError("not a variable", name)
        return var

    def getFunction(self, name: str):
        func = self.getSymbol(name)
        if not isinstance(func, Function):
            raise SemaError("not a function", name)
        return func


class GlobalScope(Scope):
    def __init__(self) -> None:
        super().__init__()


class LocalScope(Scope):
    def __init__(self, offset: int = 0) -> None:
        super().__init__()

        self._offset = offset


_gScope = GlobalScope()


def _addType(ty: Type, names: list[str] = None):
    if names is None:
        names = []
    if ty.name():
        names.append(ty.name())
    assert len(names) > 0
    for name in names:
        _gScope.addSymbol(name, ty)


def _addBasicTypes():
    _addType(VoidType())

    _addType(IntType("char", 1), ["signed char"])
    _addType(IntType("short", 2), ["short int", "signed short", "signed short int"])
    _addType(IntType("int", 4), ["signed int"])
    _addType(
        IntType("long", 4), ["long int", "signed long", "signed long int", "ssize_t"]
    )
    _addType(
        IntType("long long", 8, False, 4),
        ["long long int", "signed long long", "signed long long int"],
    )

    _addType(IntType("unsigned char", 1, True))
    _addType(IntType("unsigned short", 2, True), ["unsigned short int"])
    _addType(IntType("unsigned int", 4, True), ["unsigned"])
    _addType(IntType("unsigned long", 4, True), ["unsigned long int", "size_t"])
    _addType(IntType("unsigned long long", 8, True, 4), ["unsigned long long int"])

    # _addType(FloatType("float", 4))
    # _addType(FloatType("double", 4))


_addBasicTypes()


class Section:
    def __init__(self, name: str) -> None:
        assert name in [".text", ".rodata", ".data", ".bss"]
        self.name = name
        self.lines: list[str] = []

    def add(self, s: str, *, indent="    "):
        self.lines.append(indent + s)

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


class Asm:
    def __init__(self) -> None:
        self._secText = Section(".text")
        self._secRodata = Section(".rodata")
        self._secData = Section(".data")
        self._secBss = Section(".bss")

        self._strPool: dict[str, str] = {}

    def addStr(self, s: str) -> str:
        if s not in self._strPool:
            label = self._secRodata.addLocalLabel()
            self._secRodata.add(".asciz %s" % s)
            self._strPool[s] = label
        return self._strPool[s]

    def save(self, o: io.StringIO):
        pass


class NodeRecord:
    def __init__(self) -> None:
        self._type: Type = None
        self._value: Value = None


"""
User-defined classes have __eq__() and __hash__() methods by default;
with them, all objects compare unequal (except with themselves) and
x.__hash__() returns an appropriate value such that x == y implies
both that x is y and hash(x) == hash(y).
"""
_nodeRecords = {}


def _getNodeRecord(node: c_ast.Node) -> NodeRecord:
    if node not in _nodeRecords:
        _nodeRecords[node] = NodeRecord()
    return _nodeRecords[node]


def _setType(node: c_ast.Node, ty: Type):
    _getNodeRecord(node)._type = ty


def _getType(node: c_ast.Node) -> Type:
    r = _getNodeRecord(node)
    if r._value:
        return r._value.getType()
    return r._type


def _setValue(node: c_ast.Node, value: Value):
    _getNodeRecord(node)._value = value


def _getValue(node: c_ast.Node) -> Value:
    return _getNodeRecord(node)._value


def _getIntConstant(node: c_ast.Node) -> int:
    ci = _getValue(node)
    if not isinstance(ci, IntConstant):
        raise SemaError("not an integral constant expression")
    return ci._i


def _unescapeStr(s: str) -> str:
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


def _formatNode(node: c_ast.Node):
    return "%s %s" % (node.__class__.__qualname__, node.coord)


class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []
        self._scopes: list[Scope] = [_gScope]
        self._func: Function = None
        self._asm = Asm()

    def visit(self, node: c_ast.Node):
        try:
            self._path.append(node)
            super().visit(node)
            self._path.pop()
        except SemaError as ex:
            print("%s : %s" % (_formatNode(self._path[-1]), ex))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s" % ("  " * (i + 1), _formatNode(_)))
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
        _setType(node, _getType(node.type))

    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        _setType(node, self.curScope.getType(name))

    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        dim = node.dim
        if dim:
            self.visit(node.dim)
            dim = _getIntConstant(node.dim)
            if dim < 1:
                raise SemaError("not greater than zero")

        self.visit(node.type)
        _setType(node, ArrayType(_getType(node.type), dim))

    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        self.visit(node.type)
        _setType(node, PointerType(_getType(node.type)))

    def visit_Typedef(self, node: c_ast.Typedef):
        self.visit(node.type)
        self.curScope.addSymbol(node.name, _getType(node.type))

    def visit_Struct(self, node: c_ast.Struct):
        if node.decls is None:  # declaration
            if not self.curScope.findSymbol(node.name):
                self.curScope.addSymbol(node.name, StructType(None, node.name))
            else:
                self.curScope.getStructType(node.name)
            return

        fields: list[Field] = []
        for decl in node.decls:
            decl: c_ast.Decl
            if not decl.name:
                raise SemaError("anonymous field is not supported")
            self.visit(decl)
            fields.append(Field(_getType(decl), decl.name))

        ty = self.curScope.findSymbol(node.name)
        if isinstance(ty, StructType) and not ty.isComplete():  # declared
            ty.setFields(fields)
            return

        _setType(node, StructType(fields, node.name))
        if node.name:
            self.curScope.addSymbol(node.name, _getType(node))

    def visit_Union(self, _: c_ast.Union):
        raise SemaError("union is not supported")

    def visit_Enum(self, _: c_ast.Enum):
        raise SemaError("enum is not supported")

    def visit_NamedInitializer(self, node: c_ast.NamedInitializer):
        # https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html
        raise SemaError("designated initializer is not supported")

    def visit_CompoundLiteral(self, node: c_ast.CompoundLiteral):
        # https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
        raise SemaError("compound literal is not supported")

    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise SemaError("bitfield is not supported")

        self.visit(node.type)
        ty = _getType(node.type)

        assert node.name
        _global = self.curScope is _gScope
        _static = "static" in node.quals
        if _global or _static:
            sec = self._asm._secData if node.init else self._asm._secBss

            _ = ty.p2align()
            if _ > 0:
                sec.add(f".align {_}")

            if _global:
                label = sec.addLabel(node.name)
                self.curScope.addSymbol(node.name, GlobalVariable(node.name, ty, _static))
            else:
                label = sec.addStaticLabel(node.name)
                self.curScope.addSymbol(node.name, StaticVariable(node.name, ty, label))

            if _static:
                sec.add(f".local ${label}")
            else:
                sec.add(f".global ${label}")

            sec.add(f'.type ${label}, "object"')

            if not node.init:
                ty.checkComplete()
                sec.add(f".fill {ty.size()}")
            else:
                def _gen(ty: Type, init: c_ast.Node):
                    if isinstance(ty, ArrayType):
                        if not isinstance(init, c_ast.InitList):
                            raise SemaError("invalid initializer")

                        if not ty.isComplete():
                            ty.setDim(n)
                        ty.checkComplete()

                        n = len(init.exprs)
                        if n > ty._dim:
                            raise SemaError("too many elements in array initializer")

                        for i in range(n):
                            _gen(ty._base, init.exprs[i])

                        left = ty.size() - n * ty._base.size()
                        if left > 0:
                            sec.add(f".fill {left}")

                    elif isinstance(ty, StructType):
                        if not isinstance(init, c_ast.InitList):
                            raise SemaError("invalid initializer")
                        ty.checkComplete()

                        n = len(init.exprs)
                        if n > len(ty._fields):
                            raise SemaError("too many elements in struct initializer")
                    else:
                        if isinstance(init, c_ast.InitList):
                            raise SemaError("invalid initializer")

                _gen(ty, node.init)

            sec.add(f".size ${node.name}, {ty.size()}")
            return

        if node.init:
            self.visit(node.init)
        # TODO: def(addr, type, expr)
        # global/static
        # local assign

    def visit_Constant(self, node: c_ast.Constant):
        s: str = node.value
        assert isinstance(s, str)

        try:
            if node.type == "char":
                # https://en.cppreference.com/w/c/language/character_constant
                assert s.startswith("'") and s.endswith("'")
                s = _unescapeStr(s[1:-1])
                assert len(s) == 1
                _setValue(node, IntConstant(ord(s[0]), _gScope.getType("char")))
                return

            if node.type == "string":
                # https://en.cppreference.com/w/c/language/string_literal
                assert s.startswith('"') and s.endswith('"')
                s = _unescapeStr(s[1:-1] + "\0")
                label = self._asm.addStr(s)
                _setValue(
                    node,
                    StrLiteral(label, ArrayType(_gScope.getType("char"), len(s)), s),
                )
                return

            if "int" in node.type:
                # https://en.cppreference.com/w/c/language/integer_constant
                ty = _gScope.getType(node.type)

                s = s.lower()
                while s.endswith("u") or s.endswith("l"):
                    s = s[:-1]
                if s.startswith("0x"):
                    i = int(s[2:], base=16)
                else:
                    i = int(s, base=10)
                _setValue(node, IntConstant(i, ty))
                return

            raise SemaError("%s is unsupported" % node.type)

        except Exception as ex:
            raise SemaError("invalid literal", str(ex))

    def visit_ID(self, node: c_ast.ID):
        _ = self.curScope.getSymbol()
        if isinstance(_, Type):
            _setType(node, _)
        else:
            _setValue(node, _)

    def _skipCodegen(self) -> bool:
        for node in reversed(self._path):
            if isinstance(node, c_ast.UnaryOp) and node.op == "sizeof":
                return True
        return False

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        op = node.op
        if op == "sizeof":
            self.visit(node.expr)
            ty = _getType(node.expr)
            ty.checkComplete()
            _setValue(
                node,
                IntConstant(ty.size(), _gScope.getType("size_t")),
            )
        elif op == "-":
            pass
        else:
            raise SemaError("unknown unary operator", op)

    def visit_Cast(self, node: c_ast.Cast):
        pass

    def visit_Goto(self, node: c_ast.Goto):
        raise SemaError("goto is not supported")
