import sys

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
        self.ret = ret
        self.args = args

    def __repr__(self) -> str:
        return "(%r => %r)" % (self.args, self.ret)


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
        return "(%s, %r)" % (self._name, self._sig)


class Variable:
    def __init__(self, name: str, ty: Type) -> None:
        self._name = name
        self._type = ty

    def __repr__(self) -> str:
        return "(%s, %r)" % (self._name, self._type)


class Label:
    pass


class Scope(ABC):
    def __init__(self) -> None:
        self._symbols: dict[str, Variable|Type|Function|Label] = {}


class LocalScope(Scope):
    def __init__(self) -> None:
        super().__init__()

        self._offset: int = 0


class GlobalScope(Scope):
    def __init__(self) -> None:
        super().__init__()


def wrap(func):
    def f(self: Sema, node: c_ast.Node):
        try:
            self._path.append(node)
            func(self, node)
            self._path.pop()
        except SemaError as e:
            print("%s : %s" % (node.coord, e))
            sys.exit(1)

    return f

class Compiler(c_ast.NodeVisitor):
    def __init__(self) -> None:
        self._path = []
        self._namedTypes: dict[str, Type] = {}

        self.addNamedType(VoidType())

        self.addNamedType(IntType("char", 1), ["signed char"])
        self.addNamedType(
            IntType("short", 2), ["short int", "signed short", "signed short int"]
        )
        self.addNamedType(IntType("int", 4), ["signed int"])
        self.addNamedType(
            IntType("long", 4), ["long int", "signed long", "signed long int"]
        )
        # self.addNamedType(
        #     IntType("long long", 8),
        #     ["long long int", "signed long long", "signed long long int"],
        # )

        self.addNamedType(IntType("unsigned char", 1))
        self.addNamedType(IntType("unsigned short", 2), ["unsigned short int"])
        self.addNamedType(IntType("unsigned int", 4))
        self.addNamedType(IntType("unsigned long", 4), ["unsigned long int"])
        # self.addNamedType(IntType("unsigned long long", 8), ["unsigned long long int"])

        # self.addNamedType(FloatType("float", 4))
        # self.addNamedType(FloatType("double", 8))

        self.scopes = []

    def addNamedType(self, ty: Type, aliases: list[str] = None):
        name = ty.name()
        assert name

        if aliases is None:
            aliases = []
        aliases.append(name)
        for _ in aliases:
            if _ in self._namedTypes:
                raise SemaError("redefined type", _)
            self._namedTypes[_] = ty

    def getNamedType(self, name: str):
        if name not in self._namedTypes:
            raise SemaError("undefined type", name)
        return self._namedTypes[name]

    def enterScope(self):
        self.scopes.append(Scope())

    def exitScope(self):
        self.scopes.pop()

    def findVariable(self, name: str):
        for i in range(len(self.scopes) - 1, -1, -1):
            scope = self.scopes[i]
            if name in scope:
                return scope[name]
        raise SemaError("undefined variable", name)

    def addVariable(self, var: Variable):
        scope = self.scopes[-1]
        if var._name in scope:
            raise SemaError("redefined variable", var._name)
        scope[var._name] = var

    @wrap
    def visit_Typedef(self, node:c_ast.Typedef):
        pass

    @wrap
    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise SemaError("bitfield is not supported")
        self.generic_visit(node.type)
        self.generic_visit(node.init)

    @wrap
    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.generic_visit(node.type)

    @wrap
    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        node._rrisc32_type = self.getNamedType(name)

    @wrap
    def visit_Struct(self, node: c_ast.Struct):
        for decl in node.decls:
            if not decl.name:
                raise SemaError("anonymous field is not supported")
        for decl in node.decls:
            self.generic_visit(decl)

    @wrap
    def visit_PtrDecl(self, node: c_ast.PtrDecl):
        pass

    @wrap
    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        pass

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
    def generic_visit(self, node):
        return super().generic_visit(node)
