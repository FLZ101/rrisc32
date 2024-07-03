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

    def size(self) -> int:
        return self._size

    def align(self) -> int:
        return self._align

    def __repr__(self) -> str:
        if self._name:
            return self._name
        raise NotImplementedError()


class VoidType(Type):
    def __init__(self):
        self._name = "void"
        self._size = 0
        self._align = 0


class IntType(Type):
    def __init__(self, name: str, size: int):
        self._name = name
        self._size = size
        self._align = size

    def isUnsigned(self):
        return "unsigned" in self._name


class FloatType(Type):
    def __init__(self, name: str, size: int):
        self._name = name
        self._size = size
        self._align = size


class ArrayType(Type):
    def __init__(self, base: Type, dim: int):
        self._base = base
        self._dim = dim

        self._size = base.size() * dim
        self._align = base.align()

    def __repr__(self) -> str:
        return "%r[%d]" % (self._base, self._dim)


class Field:
    def __init__(self, ty: Type, name: str) -> None:
        if name == "":
            raise SemaError("anonymous field is not supported")

        self._type = ty
        self._name = name
        self._offset = 0

    def __repr__(self) -> str:
        return "(%r, %r)" % (self._type, self._name)


class StructType(Type):
    def __init__(self, fields: list[Field], name: str = ""):
        self._fields = fields
        self._name = name

        self._align = 1
        offset = 0
        for field in fields:
            m = field._type.align()
            n = offset % m
            if n:
                offset += m - n
            field._offset = offset
            self._align = max(self._align, m)
        self._size = offset

    def __repr__(self) -> str:
        if self._name:
            return self._name
        return "%r" % self._fields


class PointerType(Type):
    def __init__(self, base: Type) -> None:
        self._base = base
        self._size = 4
        self._align = 4

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
        self._align = 4

    def __repr__(self) -> str:
        return "%r" % self._sig


class Function:
    def __init__(self, name: str, sig: FunctionSignature) -> None:
        self._name = name
        self._sig = sig

    def __repr__(self) -> str:
        return "%s%r" % (self._name, self._sig)


class Variable:
    def __init__(self, name: str, ty: Type, init=None) -> None:
        self._name = name
        self._type = ty

    def __repr__(self) -> str:
        return "(%s, %r)" % (self._name, self._type)


class Scope(ABC):
    def __init__(self, prev=None) -> None:
        self._symbolTable: dict[str, Type | Variable | Function] = {}
        self._prev: Scope = prev

    def addSymbol(self, name: str, value: Type | Variable | Function):
        if name in self._symbolTable:
            raise SemaError("redefined", name)
        self._symbolTable[name] = value

    def findSymbol(self, name: str):
        return self._symbolTable.get(name)

    def getSymbol(self, name: str):
        if name in self._symbolTable:
            return self._symbolTable[name]
        if self._prev:
            return self._prev.getSymbol(name)
        raise SemaError("undefined", name)

    def getType(self, name: str):
        ty = self.getSymbol(name)
        if not isinstance(ty, Type):
            raise SemaError("not a type", name)
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


def wrap(func):
    def f(self, node: c_ast.Node):
        try:
            self._path.append(node)
            func(self, node)
            self._path.pop()
        except SemaError as e:
            print("%s : %s" % (node.coord, e))
            for i, _ in enumerate(reversed(self._path[:-1])):
                print("%s%s" % ("  " * (i + 1), node.coord))
            sys.exit(1)

    return f


class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []

        self._secText = Section(".text")
        self._secRodata = Section(".rodata")
        self._secData = Section(".data")
        self._secBss = Section(".bss")

        self._curFunc: Function = None

        self._scopes: list[Scope] = [GlobalScope()]

        self.addType(VoidType())

        self.addType(IntType("char", 1), ["signed char"])
        self.addType(
            IntType("short", 2), ["short int", "signed short", "signed short int"]
        )
        self.addType(IntType("int", 4), ["signed int"])
        self.addType(IntType("long", 4), ["long int", "signed long", "signed long int"])
        self.addType(
            IntType("long long", 8),
            ["long long int", "signed long long", "signed long long int"],
        )

        self.addType(IntType("unsigned char", 1))
        self.addType(IntType("unsigned short", 2), ["unsigned short int"])
        self.addType(IntType("unsigned int", 4))
        self.addType(IntType("unsigned long", 4), ["unsigned long int"])
        self.addType(IntType("unsigned long long", 8), ["unsigned long long int"])

        # self.addType(FloatType("float", 4))
        # self.addType(FloatType("double", 8))

    def save(self, o: io.StringIO):
        pass

    def enterScope(self):
        self.scopes.append(LocalScope(self.curScope))

    def exitScope(self):
        self.scopes.pop()

    @property
    def curScope(self):
        return self._scopes[-1]

    def addType(self, ty: Type, names: list[str] = None):
        if names is None:
            names = []
        if ty.name():
            names.append(ty.name())
        assert len(names) > 0
        for name in names:
            self.curScope.addSymbol(name, ty)

    @wrap
    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.visit(node.type)

    @wrap
    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        node._o_type = self.curScope.getType(name)

    @wrap
    def visit_Struct(self, node: c_ast.Struct):
        for decl in node.decls:
            if not decl.name:
                raise SemaError("anonymous field is not supported")
        for decl in node.decls:
            self.visit(decl)

    @wrap
    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        if node.dim:
            self.visit(node.dim)
            dim = getattr(node.dim, "_o_int")
            if dim is None or dim < 1:
                raise SemaError(
                    "not an integral constant expression which evaluates to a value greater than zero"
                )
        else:
            pass

    @wrap
    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        self.visit(node.type)
        self._o_type = PointerType(node.type.o_type)

    @wrap
    def visit_Typedef(self, node: c_ast.Typedef):
        self.visit(node.type)
        self.curScope.addSymbol(node.name, node.type._o_type)

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
        self.visit(node.init)

    @wrap
    def visit_Goto(self, name: c_ast.Goto):
        raise SemaError("goto is not supported")

    @wrap
    def generic_visit(self, node):
        return super().generic_visit(node)
