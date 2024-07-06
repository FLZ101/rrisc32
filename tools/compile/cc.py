import sys
import io

from abc import ABC, abstractmethod

from pycparser import c_ast


class SemaError(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


# https://en.cppreference.com/w/c/language/type
class Type(ABC):
    def name(self) -> str:
        return getattr(self, "_name", "")

    def isComplete(self) -> bool:
        return True

    def size(self) -> int:
        return self._size

    def alignment(self) -> int:
        return self._alignment

    def __repr__(self) -> str:
        if self._name:
            return self._name
        raise NotImplementedError()


class VoidType(Type):
    def __init__(self):
        self._name = "void"
        self._size = 0
        self._alignment = 0


class IntType(Type):
    def __init__(self, name: str, size: int):
        self._name = name
        self._size = size
        self._alignment = size

    def isUnsigned(self):
        return "unsigned" in self._name


class FloatType(Type):
    def __init__(self, name: str, size: int):
        self._name = name
        self._size = size
        self._alignment = size


class ArrayType(Type):
    def __init__(self, base: Type, dim: int = None):
        self._base = base
        self._dim = dim

    def setDim(self, dim: int):
        assert dim > 0
        self._dim = dim

    def isComplete(self) -> bool:
        return self._dim is not None

    def size(self) -> int:
        if self._dim is not None:
            return self._base.size() * self._dim
        return 0

    def alignment(self) -> int:
        return self._base.alignment()

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
        self._size = 0
        self._alignment = 0
        self._fill = 0

        self.setFields(fields)

    def setFields(self, fields: list[Field]):
        self._fields = fields
        if fields is None:
            return

        offset = 0
        alignment = 1

        for field in fields:
            m = field._type.alignment()
            alignment = max(alignment, m)

            offset = _align(offset, m)
            field._offset = offset

            offset += field._type.size()

        self._alignment = alignment
        self._fill = _align(offset, alignment) - offset
        self._size = offset + self._fill

    def isComplete(self) -> bool:
        return self._fields is not None

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
    @abstractmethod
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
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class ExternVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class LocalVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class Argument(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


class StringLiteral(LValue):
    def __init__(self, name: str, ty: ArrayType) -> None:
        super().__init__()
        self._name = name
        self._type = ty


class ConstantInt(RValue):
    def __init__(self, i: int, ty: IntType) -> None:
        self._i = i
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
    _addType(IntType("long", 4), ["long int", "signed long", "signed long int"])
    _addType(
        IntType("long long", 8),
        ["long long int", "signed long long", "signed long long int"],
    )

    _addType(IntType("unsigned char", 1))
    _addType(IntType("unsigned short", 2), ["unsigned short int"])
    _addType(IntType("unsigned int", 4))
    _addType(IntType("unsigned long", 4), ["unsigned long int"])
    _addType(IntType("unsigned long long", 8), ["unsigned long long int"])

    # _addType(FloatType("float", 4))
    # _addType(FloatType("double", 8))


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

    @property
    def _i(self, *, _d={"i": 0}):
        _d["i"] += 1
        return _d["i"]

    def addLocalLabel(self):
        return self.addLabel(".L%s%d" % (self.name[1], self._i))


class Asm:
    def __init__(self) -> None:
        self._secText = Section(".text")
        self._secRodata = Section(".rodata")
        self._secData = Section(".data")
        self._secBss = Section(".bss")

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


def _getConstantInt(node: c_ast.Node) -> int:
    ci = _getValue(node)
    if not isinstance(ci, ConstantInt):
        raise SemaError("not an integral constant expression")
    return ci._i


def wrap(func):
    def __wrap(self, node: c_ast.Node):
        try:
            self._path.append(node)
            func(self, node)
            self._path.pop()
        except SemaError as e:

            def _clsName(_: c_ast.Node):
                return _.__class__.__qualname__

            print("%s %s : %s" % (_clsName(node), node.coord, e))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s %s" % ("  " * (i + 1), _clsName(_), _.coord))
            sys.exit(1)

    return __wrap


class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []
        self._scopes: list[Scope] = [_gScope]
        self._func: Function = None
        self._asm = Asm()
        self._skipCodegen = False

    def enterScope(self):
        self.scopes.append(LocalScope(self.curScope))

    def exitScope(self):
        self.scopes.pop()

    @property
    def curScope(self):
        return self._scopes[-1]

    def save(self, o: io.StringIO):
        self._asm.save(o)

    @wrap
    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.visit(node.type)
        _setType(node, _getType(node.type))

    @wrap
    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        _setType(node, self.curScope.getType(name))

    @wrap
    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        dim = node.dim
        if dim:
            self.visit(node.dim)
            dim = _getConstantInt(node.dim)
            if dim < 1:
                raise SemaError("not greater than zero")

        self.visit(node.type)
        _setType(node, ArrayType(_getType(node.type), dim))

    @wrap
    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        self.visit(node.type)
        _setType(node, PointerType(_getType(node.type)))

    @wrap
    def visit_Typedef(self, node: c_ast.Typedef):
        self.visit(node.type)
        self.curScope.addSymbol(node.name, _getType(node.type))

    @wrap
    def visit_Struct(self, node: c_ast.Struct):
        if node.decls is None:  # declaration
            if not self.curScope.findSymbol(node.name):
                self.curScope.addSymbol(node.name, StructType(None, node.name))
            else:
                self.curScope.getStructType(node.name)
            return
        if len(node.decls) == 0:
            raise SemaError("struct with no field is not supported")

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

    @wrap
    def visit_Union(self, _: c_ast.Union):
        raise SemaError("union is not supported")

    @wrap
    def visit_Enum(self, _: c_ast.Enum):
        raise SemaError("enum is not supported")

    @wrap
    def visit_NamedInitializer(self, node: c_ast.NamedInitializer):
        # https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html
        raise SemaError("designated initializer is not supported")

    @wrap
    def visit_CompoundLiteral(self, node: c_ast.CompoundLiteral):
        # https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
        raise SemaError("compound literal is not supported")

    @wrap
    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise SemaError("bitfield is not supported")

        self.visit(node.type)

        node.quals
        node.name
        if node.init:
            self.visit(node.init)
        # TODO: def(addr, type, expr)
        # global/static
        # local assign

    @wrap
    def visit_Constant(self, node: c_ast.Constant):
        pass

    @wrap
    def visit_ID(self, node: c_ast.ID):
        _ = self.curScope.getSymbol()
        if isinstance(_, Type):
            _setType(node, _)
        else:
            _setValue(node, _)

    @wrap
    def visit_InitList(self, node: c_ast.InitList):
        arr = []
        for expr in node.exprs:
            self.visit(expr)
            arr.append(_getValue(expr))
        _setValue(node, arr)

    @wrap
    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        op = node.op
        if op == "sizeof":
            _1 = self._skipCodegen
            self._skipCodegen = True
            self.visit(node.expr)
            self._skipCodegen = _1

            _setValue(
                node,
                ConstantInt(
                    _getType(node.expr).size(), _gScope.getType("unsigned long")
                ),
            )
        else:
            raise SemaError("unknown unary operator", op)

    @wrap
    def visit_Goto(self, name: c_ast.Goto):
        raise SemaError("goto is not supported")

    @wrap
    def generic_visit(self, node):
        return super().generic_visit(node)
