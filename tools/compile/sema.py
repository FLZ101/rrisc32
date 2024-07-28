import sys
import traceback

from abc import ABC
from typing import Callable, Optional

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
        return hasattr(self, "_size")

    def checkComplete(self):
        if not self.isComplete():
            raise CCError("incomplete type", self)

    def size(self) -> int:
        self.checkComplete()
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

    def getField(self, name: str):
        if name not in self._fieldByName:
            raise CCError("invalid field", name)
        return self._fieldByName[name]

    def __repr__(self) -> str:
        if self._name:
            return self._name
        return "%r" % self._fields


class PointerType(Type):
    def __init__(self, base: Type) -> None:
        self._base = base
        self._size = 4
        self._alignment = 4

    def toFunction(self):
        return isinstance(self._base, FunctionType)

    def toObject(self):
        return not self.toFunction()

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
    def __init__(self, ty: Type) -> None:
        self._type = ty

    def getType(self) -> Type:
        return self._type

    @property
    def node(self) -> Optional[c_ast.Node]:
        return getattr(self, "_node", None)


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
    def __init__(self) -> None:
        super().__init__()


def checkLValue(v: Value):
    if not isinstance(v, LValue):
        raise CCError("not a lvalue")


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

    def toSymConstant(self) -> "SymConstant":
        return SymConstant(self._name, PointerType(self._type))


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
    def __init__(self, name: str, ty: Type, label: str = '') -> None:
        super().__init__(name, ty)
        self._label = label


class ExternVariable(Variable):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(name, ty)


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
    pass


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


class TmpValue(RValue):
    def __init__(self, ty: Type) -> None:
        self._type = ty


"""
int i;
i = 1;
(int)i = 2; // error: assignment to cast is illegal
"""


class CastedLValue(RValue):
    def __init__(self, v: LValue) -> None:
        self._type = v.getType()
        self._v = v


class Scope(ABC):
    def __init__(self, prev: Optional["Scope"] = None) -> None:
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
    def __init__(self, prev: Scope) -> None:
        super().__init__(prev)


class LocalScope(Scope):
    def __init__(self, prev: Scope) -> None:
        super().__init__(prev)

        self._offset: int = 0
        if isinstance(prev, LocalScope):
            self._offset = prev._offset


builtinScope = Scope()


def addBuiltinType(ty: Type, names: Optional[list[str]] = None):
    if names is None:
        names = []
    if ty.name():
        names.append(ty.name())
    assert len(names) > 0
    for name in names:
        builtinScope.addSymbol(name, ty)


def addBuiltinTypes():
    addBuiltinType(VoidType())

    addBuiltinType(IntType("char", 1), ["signed char"])
    addBuiltinType(
        IntType("short", 2), ["short int", "signed short", "signed short int"]
    )
    addBuiltinType(IntType("int", 4), ["signed int"])
    addBuiltinType(
        IntType("long", 4),
        ["long int", "signed long", "signed long int", "ssize_t"],
    )
    addBuiltinType(
        IntType("long long", 8, False, 4),
        ["long long int", "signed long long", "signed long long int"],
    )

    addBuiltinType(IntType("unsigned char", 1, True))
    addBuiltinType(IntType("unsigned short", 2, True), ["unsigned short int"])
    addBuiltinType(IntType("unsigned int", 4, True), ["unsigned"])
    addBuiltinType(IntType("unsigned long", 4, True), ["unsigned long int", "size_t"])
    addBuiltinType(
        IntType("unsigned long long", 8, True, 4), ["unsigned long long int"]
    )

    # addBuiltinType(FloatType("float", 4))
    # addBuiltinType(FloatType("double", 4))


addBuiltinTypes()


def getBuiltinType(name: str) -> Type:
    return builtinScope.getType(name)


def getIntConstant(i: int, ty: str = ""):
    ty = ty or "int"
    return IntConstant(i, getBuiltinType(ty))


# https://en.cppreference.com/w/c/language/type#Compatible_types
def isCompatible(t1: Type, t2: Type):
    if t1 is t2:
        return True

    if not t1.isComplete() or not t2.isComplete():
        return False

    if isinstance(t1, PointerType) and isinstance(t2, PointerType):
        return isCompatible(t1._base, t2._base)

    if isinstance(t1, ArrayType) and isinstance(t2, ArrayType):
        return t1.size() == t2.size() and isCompatible(t1._base, t2._base)

    if isinstance(t1, StructType) and isinstance(t2, StructType):
        if len(t1._fields) != len(t2._fields):
            return False
        for i in range(len(t1._fields)):
            f1 = t1._fields[i]
            f2 = t2._fields[i]
            if f1._name != f2._name or not isCompatible(f1._type, f2._type):
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
            if isCompatible(at1, at2):
                continue

            def _isArrayToPointerCompatible(_t1: Type, _t2: Type):
                return (
                    isinstance(_t1, ArrayType)
                    and isinstance(_t2, PointerType)
                    and isCompatible(_t1._base, _t2._base)
                )

            def _isFunctionToPointerCompatible(_t1: Type, _t2: Type):
                return (
                    isinstance(_t1, FunctionType)
                    and isinstance(_t2, PointerType)
                    and isinstance(_t2._base, FunctionType)
                    and isCompatible(_t1, _t2._base)
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


class Node(c_ast.Node):
    def __init__(self, v: Value) -> None:
        match v:
            case Constant() | Variable():
                self._value = v
            case _:
                unreachable()


def unescapeStr(s: str) -> str:
    arr = []

    for c in s:
        x = ord(c)
        if x > 255:
            raise CCError("invalid character", c)

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
                    raise CCError("invalid escape sequence", repr(s[i - 1 : i + 1]))
        else:
            arr.append(c)

        i += 1

    return "".join(arr)


class NodeRecord:
    def __init__(self, v: Value) -> None:
        self._value = v


class NodeVisitorCtx:
    def __init__(self) -> None:
        self.records: dict[c_ast.Node, NodeRecord] = {}
        self.gScope = GlobalScope(builtinScope)

class NodeVisitor(c_ast.NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        self._records = ctx.records
        self._scope: Scope = ctx.gScope
        self._func: Function = None
        self._path: list[c_ast.Node] = []

    def getParent(self, i=1):
        n = len(self._path)
        if i >= n:
            return None
        return self._path[-(i + 1)]

    def visit(self, node: c_ast.Node):
        try:
            assert not isinstance(node, Node)

            self._path.append(node)
            super().visit(node)
            self._path.pop()
        except CCError as ex:

            def formatNode(node: c_ast.Node):
                return "%s %s" % (node.__class__.__qualname__, getattr(node, 'coord', ''))

            print(traceback.format_exc())
            print("%s : %s" % (formatNode(self._path[-1]), ex))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s" % ("  " * (i + 1), formatNode(_)))
            sys.exit(1)

    def getNodeRecord(self, node: c_ast.Node) -> NodeRecord | Node:
        if isinstance(node, Node):
            return node

        if node not in self._records:
            self._records[node] = NodeRecord()
        return self._records[node]

    def setNodeValue(self, node: c_ast.Node, v: Value):
        r = self.getNodeRecord(node)
        r._value = v

    def getNodeValue(self, node: c_ast.Node) -> Value:
        r = self.getNodeRecord(node)
        if r._value is None:
            self.visit(node)
        assert r._value
        return r._value

    def setNodeType(self, node: c_ast.Node, ty: Type):
        self.setNodeValue(node, Value(ty))

    def getNodeType(self, node: c_ast.Node) -> Type:
        return self.getNodeValue(node).getType()

    def getNodeIntConstant(self, node: c_ast.Node) -> IntConstant:
        v = self.getNodeValue(node)
        if not isinstance(v, IntConstant):
            raise CCError("not an integral constant expression")
        return v

    def getNodeStrLiteral(self, node: c_ast.Node) -> StrLiteral:
        v = self.getNodeValue(node)
        if not isinstance(v, StrLiteral):
            raise CCError("not a string literal")
        return v


class Sema(NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        super().__init__(ctx)

    def enterScope(self):
        self._scope = LocalScope(self._scope)

    def exitScope(self):
        self._scope = self._scope._prev

    # https://en.cppreference.com/w/c/language/conversion
    def tryConvert(self, t1: Type, node: c_ast.Node) -> c_ast.Node:
        v2 = self.getNodeValue(node)
        t2 = v2.getType()

        if isinstance(t1, PointerType) and t1._base is None:
            match t2:
                case ArrayType() | PointerType():
                    t1._base = t2._base
                case FunctionType():
                    t1._base = t2
                case _:
                    return None

        if isCompatible(t1, t2):
            return node

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
            and isinstance(v2, LValue)
            and isCompatible(t1._base, t2._base)
        ):
            # translated to &arr[0]
            node2 = c_ast.Cast(
                Node(Value(t1)),
                c_ast.UnaryOp(
                    "&", c_ast.ArrayRef(node, Node(getIntConstant(0, "size_t")))
                ),
            )
            return node2

        # function to pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, FunctionType)
            and isCompatible(t1._base, t2)
        ):
            if isinstance(v2, Function):
                self.setNodeValue(node, v2.toSymConstant())
                return node
            return c_ast.Cast(Node(Value(t1)), node)

        # integer conversion
        if isinstance(t1, IntType) and isinstance(t2, IntType):
            if isinstance(v2, IntConstant):
                self.setNodeValue(node, IntConstant(v2._i, t1))
                return node
            return c_ast.Cast(Node(Value(t1)), node)

        # pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, PointerType)
            and t2._base is getBuiltinType("void")
        ):
            return c_ast.Cast(Node(Value(t1)), node)

        if (
            isinstance(t1, PointerType)
            and t1._base is self.getType("void")
            and isinstance(v2, IntConstant)
            and v2._i == 0
        ):
            return Node(PtrConstant(0, t1))

        return None

    def convert(self, t1: Type, node: c_ast.Node):
        node2 = self.tryConvert(t1, node)
        if node2 is None:
            raise CCError(f"can not convert {self.getNodeType(node)} to {t1}")
        self.setNodeType(node2, t1)
        return node2

    # https://en.cppreference.com/w/c/language/cast
    def cast(self, t1: Type, node: c_ast.Node):
        node2 = self.tryConvert(t1, node)
        if node2 is None:
            v2 = self.getNodeValue(node)
            t2 = v2.getType()
            match t1, t2:
                # Any integer can be cast to any pointer type.
                case PointerType(), IntType():
                    if isinstance(v2, IntConstant):
                        self.setNodeValue(node, PtrConstant(v2._i, t1))
                    node2 = node

                # Any pointer type can be cast to any integer type.
                case IntType(), PointerType():
                    if isinstance(v2, PtrConstant):
                        self.setNodeValue(node, IntConstant(v2._i, t1))
                    node2 = node

                # Any pointer to object can be cast to any other pointer to object.
                # Any pointer to function can be cast to a pointer to any other function type.
                case PointerType(), PointerType():
                    if t1.toFunction() == t2.toFunction():
                        node2 = node

        if node2 is None:
            raise CCError(f"can not cast {t2} to {t1}")

        self.setNodeType(node2, t1)
        return node2

    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.setNodeType(node, self.getNodeType(node.type))

    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        self.setNodeType(node, self._scope.getType(name))

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
        self._scope.addSymbol(node.name, self.getNodeType(node.type))

    def visit_Struct(self, node: c_ast.Struct):
        if node.decls is None:  # declaration
            if not self._scope.findSymbol(node.name):
                self._scope.addSymbol(node.name, StructType(None, node.name))
            self.setNodeType(node, self._scope.getStructType(node.name))
            return

        fields: list[Field] = []
        for decl in node.decls:
            decl: c_ast.Decl
            if not decl.name:
                raise CCNotImplemented("anonymous field")
            fields.append(Field(self.getNodeType(decl), decl.name))

        if len(fields) == 0:
            raise CCNotImplemented("empty struct")

        ty = self._scope.findSymbol(node.name)
        if isinstance(ty, StructType) and not ty.isComplete():  # declared
            ty.setFields(fields)
            self.setNodeType(node, ty)
            return

        self.setNodeType(node, StructType(fields, node.name))
        if node.name:
            self._scope.addSymbol(node.name, self.getNodeType(node))

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

    def visit_Decl(self, node: c_ast.Decl):
        if node.bitsize:
            raise CCNotImplemented("bitfield")

        ty = self.getNodeType(node.type)
        if ty is getBuiltinType("void"):
            ty.checkComplete()  # trigger an exception

        self.setNodeType(node, ty)

        # struct Foo { int i; };
        if not node.name:
            return

        if isinstance(ty, FunctionType):
            self._scope.addSymbol(node.name, Function(node.name, ty))
            return

        if "extern" in node.quals:
            self._scope.addSymbol(node.name, ExternVariable(node.name, ty))
            return

        if isinstance(self.getParent(), c_ast.Struct):  # fields
            return

        if isinstance(self.getParent(), c_ast.FuncDecl):  # arguments
            return

        _global = isinstance(self._scope, GlobalScope)
        _static = "static" in node.storage
        _local = not _global and not _static

        def _check(ty: Type, init: c_ast.Node):
            if isinstance(ty, ArrayType):
                ty._base.checkComplete()

                if ty._base in [
                    self.getType("char"),
                    self.getType("unsigned char"),
                ]:
                    if not isinstance(init, c_ast.InitList):  # char s[] = "hello"
                        sLit = self.getNodeStrLiteral(init)
                        n = len(sLit._s)
                        if not ty.isComplete():
                            ty.setDim(n)
                        if n > ty._dim:
                            raise CCError("initializer-string is too long")
                        return init

                if not isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")

                n = len(init.exprs)
                if not ty.isComplete():
                    ty.setDim(n)
                ty.checkComplete()

                if n > ty._dim:
                    raise CCError("too many elements in array initializer")

                for i in range(n):
                    init.exprs[i] = _check(ty._base, init.exprs[i])
                return init

            elif isinstance(ty, StructType):
                if not isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")
                ty.checkComplete()

                n = len(init.exprs)
                if n > len(ty._fields):
                    raise CCError("too many elements in struct initializer")

                for i in range(n):
                    field = ty._fields[i]
                    init.exprs[i] = _check(field._type, init.exprs[i])
                return init

            else:
                if isinstance(init, c_ast.InitList):
                    raise CCError("invalid initializer")

                if not _local:
                    v = self.getNodeValue(init)
                    if not isinstance(v, Constant):
                        raise CCError("initializer element is not constant")
                return self.convert(ty, init)

        if _global or _static:  # global/static variables
            if _global:
                self._scope.addSymbol(
                    node.name, GlobalVariable(node.name, ty, _static)
                )
            else:
                self._scope.addSymbol(node.name, StaticVariable(node.name, ty))

            if node.init:
                _check(ty, node.init)

        else:  # local variables
            scope: LocalScope = self._scope
            scope.addSymbol(node.name, LocalVariable(node.name, ty, scope._offset))

            if node.init:
                _check(ty, node.init)

            sz = align(ty.size(), 4)
            scope._offset += sz

    def visit_Constant(self, node: c_ast.Constant):
        s: str = node.value
        assert isinstance(s, str)

        try:
            if node.type == "char":
                # https://en.cppreference.com/w/c/language/character_constant
                assert s.startswith("'") and s.endswith("'")
                s = unescapeStr(s[1:-1])
                assert len(s) == 1
                self.setNodeValue(node, getIntConstant(ord(s[0]), "char"))
                return

            if node.type == "string":
                # https://en.cppreference.com/w/c/language/string_literal
                assert s.startswith('"') and s.endswith('"')
                s = unescapeStr(s[1:-1] + "\0")
                sLit = StrLiteral(s, node.value, self.getType("char"))
                self.setNodeValue(node, sLit)
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
        # foo.x
        assert not isinstance(self.getParent(), c_ast.StructRef)

        _ = self._scope.getSymbol(node.name)
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
        self._func = self._scope.getFunction(funcName)
        funcType: FunctionType = self._func.getType()
        funcDecl: c_ast.FuncDecl = decl.type

        sec = self._asm._secText
        sec.addEmptyLine()

        if "static" in decl.storage:
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
                self._scope.addSymbol(argName, Argument(argName, argTy, offset))
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
        match node.op:
            case "sizeof":
                match node.expr:
                    case c_ast.Typename() | c_ast.ID() | c_ast.Constant():
                        ty = self.getNodeType(node.expr)
                        self.setNodeValue(
                            node,
                            self.getIC(ty.size(), "size_t"),
                        )
                    case _:
                        raise CCNotImplemented()

            case "&":
                v = self.getNodeValue(node.expr)
                ty = v.getType()
                if isinstance(ty, FunctionType()):
                    match v:
                        case Function():
                            self.setNodeValue(node, v.toSymConstant())
                        case TemporaryValue():
                            v._type = PointerType(ty)
                            self.setNodeValue(node, v)
                        case _:
                            unreachable()
                else:
                    match v:
                        case LValue():
                            self.setNodeValue(node, v.addressOf())
                        case _:
                            raise CCError(f"can not take address of rvalue")

            case "*":
                v = self.getNodeValue(node.expr, PointerType())
                ty: PointerType = v.getType()
                if ty.toFunction():
                    match v:
                        case SymConstant():
                            self.setNodeValue(node, self._scope.getFunction(v._name))
                        case _:
                            v.load()
                            self.setNodeValue(node, TemporaryValue(ty._base))
                else:
                    match v:
                        case SymConstant():
                            self.setNodeValue(node, self._scope.getVariable(v._name))
                        case _:
                            self.setNodeValue(node, MemoryAccess(v))

            case _:
                raise CCError("unknown unary operator", node.op)

    def visit_Cast(self, node: c_ast.Cast):
        v = self.cast(self.getNodeType(node.to_type), self.getNodeValue(node.expr))
        self.setNodeValue(node, v)

    def visit_ArrayRef(self, node: c_ast.ArrayRef):
        # translate arr[i] to *(arr + i)
        node2 = c_ast.UnaryOp("*", c_ast.BinaryOp("+", node.name, node.subscript))
        self.setNodeValue(node, self.getNodeValue(node2))

    def visit_StructRef(self, node: c_ast.StructRef):
        v = self.getNodeValue(node.name)
        ty = v.getType()
        match node.type:
            # translate foo.x to *(X*)((void *)&foo + offset(x))
            case ".":
                match ty:
                    case StructType():
                        checkLValue(v)
                        field = ty.getField(node.field.name)
                        node2 = c_ast.UnaryOp(
                            "*",
                            c_ast.Cast(
                                Node(ty=PointerType(field._type)),
                                c_ast.BinaryOp(
                                    "+",
                                    c_ast.Cast(
                                        Node(ty=PointerType(self.getType("void"))),
                                        c_ast.UnaryOp("&", Node(v=v)),
                                    ),
                                    Node(v=self.getIC(field._offset)),
                                ),
                            ),
                        )
                        self.setNodeValue(node, self.getNodeValue(node2))
                    case _:
                        raise CCError("not a struct")

            # translate foo->x to *(X*)((void *)foo + offset(x))
            case "->":
                match ty:
                    case PointerType(StructType()):
                        field = ty._base.getField(node.field.name)
                        node2 = c_ast.UnaryOp(
                            "*",
                            c_ast.Cast(
                                Node(ty=PointerType(field._type)),
                                c_ast.BinaryOp(
                                    "+",
                                    c_ast.Cast(
                                        Node(ty=PointerType(self.getType("void"))),
                                        Node(v=v),
                                    ),
                                    Node(v=self.getIC(field._offset)),
                                ),
                            ),
                        )
                    case _:
                        raise CCError("not a struct pointer")
            case _:
                unreachable()

    # https://en.cppreference.com/w/c/language/operator_assignment
    def visit_Assignment(self, node: c_ast.Assignment):
        a: LValue = self.getNodeValue(node.lvalue)
        checkLValue(a)

        ty = a.getType()

        addr = a.addressOf()
        addr.push(self._asm)

        op = node.op

        match ty:
            case StructType():
                if op != "=":
                    raise CCError(f"can not {op} a struct")

                b: LValue = self.getNodeValue(node.lvalue)
                if not self.isCompatible(ty, b.getType()):
                    raise CCError(f"can not assign from {b.getType()} to {ty}")

                assert isinstance(b, LValue)
                addr.pop(self._asm, "a2", "a3")
                b.addressOf().push(self._asm)
                self.emitBuiltinCall()
            case IntType() | PointerType():
                pass
            case _:
                unreachable()

        match node.op:
            case "=":
                self.loadNodeValue(node.rvalue, ty, "a2", "a3")

                addr.pop(self._asm)
                ma = MemoryAccess(TemporaryValue(addr.getType()))
                ma.store(self._asm, "a2", "a3")

                self._asm.emit("mv a0, a2")
                self._asm.emit("mv a1, a3")
                self.setNodeValue(TemporaryValue(ty))

            case _:
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
        assert name in self._builtins
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
        sec.add(f".local ${name}")
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
                _f = getattr(self, f"_emitBuiltin{name[0].upper() + name[1:]}")
                _f()
