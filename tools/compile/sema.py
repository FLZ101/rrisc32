import sys
import traceback

from abc import ABC
from typing import Optional

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
    if i in arr:
        return arr.index(i)
    if i > arr[-1]:
        assert i % 2 == 0
        return 1 + log2(i // 2)
    unreachable()


# https://en.cppreference.com/w/c/language/type
class Type(ABC):
    def name(self) -> str:
        return getattr(self, "_name", "")

    def isVoid(self):
        return self.name() == "void"

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
    def __init__(self, ty: Type) -> None:
        super().__init__(ty)


def checkLValue(v: Value):
    if not isinstance(v, LValue):
        raise CCError("not a lvalue")


class RValue(Value):
    def __init__(self, ty: Type) -> None:
        super().__init__(ty)


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
        super().__init__(ty)
        self._name = name

        self._maxOffset = 0

        self._labels = set()
        self._gotos = set()

    def getStaticLabel(self, name: str, *, _i=[0]):
        _i[0] += 1
        return f"{self._name}.{name}.{_i[0]}"

    def getLocalLabel(self, name: str):
        return f".LF.{self._name}.{name}"

    def getTempVarName(self, *, _i=[0]):
        _i[0] += 1
        return f"tmp.{_i[0]}"

    def updateMaxOffset(self, offset: int):
        if offset > self._maxOffset:
            self._maxOffset = offset

    def addLabel(self, name: str):
        if name in self._labels:
            raise CCError("duplicated label", name)
        self._labels.add(name)
        return self.getLocalLabel(name)

    def addGoto(self, name: str):
        self._gotos.add(name)
        return self.getLocalLabel(name)

    def checkLabels(self):
        s = self._gotos - self._labels
        if len(s) > 0:
            raise CCError("unknown labels", s)


class Variable(LValue):
    def __init__(self, name: str, ty: Type) -> None:
        super().__init__(ty)
        self._name = name

    def __repr__(self) -> str:
        return "(%s, %r)" % (self._name, self._type)


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
        super().__init__(ArrayType(ty, len(s)))
        self._s = s
        self._sOrig = sOrig

        self._label = ""


# https://en.cppreference.com/w/c/language/constant_expression
class Constant(RValue):
    def __init__(self, ty: Type) -> None:
        super().__init__(ty)


class IntConstant(Constant):
    def __init__(self, i: int, ty: IntType) -> None:
        super().__init__(ty)
        self._i = ty.convert(i)


class PtrConstant(Constant):
    def __init__(self, i: int, ty: PointerType) -> None:
        super().__init__(ty)
        self._i = i


class SymConstant(Constant):
    def __init__(self, name: str, ty: PointerType, offset: int = 0) -> None:
        super().__init__(ty)
        self._name = name
        self._offset = offset

        if offset:
            assert ty.isComplete()


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
            return self._prev.findSymbol(name)
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
    addBuiltinType(IntType("short", 2), ["short int", "signed short", "signed short int"])
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
    addBuiltinType(IntType("unsigned long long", 8, True, 4), ["unsigned long long int"])

    # addBuiltinType(FloatType("float", 4))
    # addBuiltinType(FloatType("double", 4))


addBuiltinTypes()


def getBuiltinType(name: str) -> Type:
    return builtinScope.getType(name)


voidTy = getBuiltinType("void")


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
        for i in range(len(t1._args)):
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


def promoteIntType(ty: IntType) -> IntType:
    if ty.size() >= 4:
        return ty
    return getBuiltinType("int")


# https://en.cppreference.com/w/c/language/conversion#Usual_arithmetic_conversions
def getArithmeticCommonType(t1: IntType, t2: IntType):
    t1 = promoteIntType(t1)
    t2 = promoteIntType(t2)

    if t1.size() == t2.size():
        if t1._unsigned:
            return t1
        return t2
    if t1.size() > t2.size():
        return t1
    return t2


class Node(c_ast.Node):
    def __init__(self, v: Value | Type) -> None:
        match v:
            case Constant() | Variable():
                self._value = v
            case _:
                if type(v) is Value:  # a Type wrapper
                    assert v._type
                    self._value = v
                else:
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
    __slots__ = ("_value", "_translated", "_visited", "_loop", "_labels", "_cases")

    def __init__(self, v: Value = None) -> None:
        self._value = v
        self._translated: c_ast.Node = None
        self._visited = False
        self._labels: list[str] = None
        self._cases: list[tuple[Optional[int], str]] = None


class NodeVisitorCtx:
    def __init__(self) -> None:
        self.records: dict[c_ast.Node, NodeRecord] = {}
        self.gScope = GlobalScope(builtinScope)

        self._strPool: dict[str, StrLiteral] = {}

    def addStr(self, sLit: StrLiteral):
        s: str = sLit._s
        if s not in self._strPool:
            sLit._label = self.getStrLabel()
            self._strPool[s] = sLit
        else:
            sLit._label = self._strPool[s]._label

    def getStrLabel(self, *, _i=[0]):
        _i[0] += 1
        return f".LS_{_i[0]}"

    def getLocalLabel(self, name: str, *, _i=[0]):
        _i[0] += 1
        return f".LL_{_i[0]}.{name}"


class NodeVisitor(c_ast.NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        self._ctx = ctx
        self._records = ctx.records
        self._scope: Scope = ctx.gScope
        self._func: Function = None
        self._path: list[c_ast.Node] = []

    def getParent(self, i=1):
        n = len(self._path)
        if i >= n:
            return None
        return self._path[-(i + 1)]

    def visit(self, node: Optional[c_ast.Node]):
        if not node:
            return
        try:
            assert not isinstance(node, Node)

            self._path.append(node)
            super().visit(node)
            self._path.pop()
        except CCError as ex:

            def formatNode(node: c_ast.Node):
                return "%s %s" % (
                    node.__class__.__qualname__,
                    getattr(node, "coord", ""),
                )

            print(traceback.format_exc())
            print("%s : %s" % (formatNode(self._path[-1]), ex))
            for i, _ in enumerate(reversed(self._path[1:-1])):
                print("%s%s" % ("  " * (i + 1), formatNode(_)))
            raise

    def getNodeRecord(self, node: c_ast.Node) -> NodeRecord | Node:
        assert not isinstance(node, Node)

        if node not in self._records:
            self._records[node] = NodeRecord()
        return self._records[node]

    def setNodeValue(self, node: c_ast.Node, v: Value):
        r = self.getNodeRecord(node)
        r._value = v

    def getNodeValue(self, node: c_ast.Node) -> Value:
        if isinstance(node, Node):
            return node._value

        r = self.getNodeRecord(node)
        if r._value is None:
            self.visit(node)
        assert r._value
        return r._value

    def getNodeValueType(self, node: c_ast.Node) -> tuple[Value, Type]:
        v = self.getNodeValue(node)
        ty = v.getType()
        return v, ty

    def setNodeType(self, node: c_ast.Node, ty: Type):
        self.setNodeValue(node, Value(ty))

    def setNodeTypeL(self, node: c_ast.Node, ty: Type):
        self.setNodeValue(node, LValue(ty))

    def setNodeTypeR(self, node: c_ast.Node, ty: Type):
        self.setNodeValue(node, RValue(ty))

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

    def setNodeTranslated(self, node: c_ast.Node, translated: c_ast.Node):
        r = self.getNodeRecord(node)
        r._translated = translated
        r._value = self.getNodeValue(translated)

    def getNodeTranslated(self, node: c_ast.Node) -> c_ast.Node:
        r = self.getNodeRecord(node)
        return r._translated

    def setNodeLabels(self, node: c_ast.Node, labels: list[str]):
        r = self.getNodeRecord(node)
        r._labels = [self._ctx.getLocalLabel(_) for _ in labels]

    def getNodeLabels(self, node: c_ast.Node) -> list[str]:
        r = self.getNodeRecord(node)
        return r._labels


class Sema(NodeVisitor):
    def __init__(self, ctx: NodeVisitorCtx) -> None:
        super().__init__(ctx)

    def enterScope(self):
        self._scope = LocalScope(self._scope)

    def exitScope(self):
        self._scope = self._scope._prev

    # https://en.cppreference.com/w/c/language/conversion
    def tryConvert(self, t1: Type, node: c_ast.Node) -> c_ast.Node:
        v2, t2 = self.getNodeValueType(node)

        if isinstance(t1, PointerType) and t1._base is None:
            match t2:
                case ArrayType() | PointerType():
                    t1._base = t2._base
                case FunctionType():
                    t1._base = t2
                case _:
                    t1._base = getBuiltinType("void")

        if isCompatible(t1, t2):
            v2._type = t1
            return node

        res = c_ast.Cast(Node(Value(t1)), node)
        self.setNodeTypeR(res, t1)

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
            and isCompatible(t1._base, t2._base)
            and isinstance(v2, LValue)
        ):
            match v2:
                case GlobalVariable() | StaticVariable():
                    return Node(SymConstant(v2._label, t1))
                case StrLiteral():
                    self._ctx.addStr(v2)
                    return Node(SymConstant(v2._label, PointerType(t2._base)))
                case _:
                    return res

        # function to pointer conversion
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, FunctionType)
            and isCompatible(t1._base, t2)
        ):
            if isinstance(v2, Function):
                return Node(SymConstant(v2._name, t1))
            return res

        # integer conversion
        if isinstance(t1, IntType) and isinstance(t2, IntType):
            if isinstance(v2, IntConstant):
                return Node(IntConstant(v2._i, t1))
            return res

        # pointer conversion

        # a pointer to void can be implicitly converted to and from any pointer to object type
        if (
            isinstance(t1, PointerType)
            and isinstance(t2, PointerType)
            and ((t1._base is voidTy and t2.toObject()) or (t1.toObject() and t2._base is voidTy))
        ):
            match v2:
                case PtrConstant():
                    return Node(PtrConstant(v2._i, t1))
                case SymConstant():
                    return Node(SymConstant(v2._name, t1))
                case _:
                    return res

        # any integer constant expression with value ​0​ as well as
        # integer pointer expression with value zero cast to the type void*
        # can be implicitly converted to any pointer type
        # (both pointer to object and pointer to function)
        if (
            isinstance(t1, PointerType)
            and (
                isinstance(v2, IntConstant) or (isinstance(v2, PtrConstant) and t2._base is voidTy)
            )
            and v2._i == 0
        ):
            return Node(PtrConstant(0, t1))

        return None

    def convert(self, t1: Type, node: c_ast.Node):
        nodeNew = self.tryConvert(t1, node)
        if nodeNew:
            return nodeNew
        raise CCError(f"can not convert {self.getNodeType(node)} to {t1}")

    # https://en.cppreference.com/w/c/language/conversion
    def tryConvertToPointer(
        self, o: c_ast.Node, attr: str, skipIfInt=True, pointerTy: PointerType = None
    ) -> tuple[Value, Type]:
        node: c_ast.Node = getattr(o, attr)

        v, ty = self.getNodeValueType(node)
        if skipIfInt and isinstance(ty, IntType):
            return v, ty

        nodeNew = self.tryConvert(pointerTy or PointerType(None), node)
        if nodeNew:
            setattr(o, attr, nodeNew)
            v, ty = self.getNodeValueType(nodeNew)

        return v, ty

    # https://en.cppreference.com/w/c/language/cast
    def visit_Cast(self, node: c_ast.Cast):
        t1 = self.getNodeType(node.to_type)

        nodeNew = self.tryConvert(t1, node.expr)
        if nodeNew:
            if isinstance(nodeNew, Node):
                vNew = nodeNew._value
                if isinstance(vNew, Constant):
                    self.setNodeValue(node, vNew)
                    return
            self.setNodeTypeR(node, t1)
            return

        v2, t2 = self.getNodeValueType(node.expr)
        match t1, t2:
            # Any integer can be cast to any pointer type.
            case PointerType(), IntType():
                if isinstance(v2, IntConstant):
                    self.setNodeValue(node, PtrConstant(v2._i, t1))
                else:
                    self.setNodeTypeR(node, t1)

            # Any pointer type can be cast to any integer type.
            case IntType(), PointerType():
                if isinstance(v2, PtrConstant):
                    self.setNodeValue(node, IntConstant(v2._i, t1))
                else:
                    self.setNodeTypeR(node, t1)

            # Any pointer to object can be cast to any other pointer to object.
            # Any pointer to function can be cast to a pointer to any other function type.
            case PointerType(), PointerType():
                if t1.toFunction() == t2.toFunction():
                    match v2:
                        case PtrConstant():
                            self.setNodeValue(node, PtrConstant(v2._i, t1))
                        case SymConstant():
                            self.setNodeValue(node, SymConstant(v2._name, t1))
                        case _:
                            self.setNodeTypeR(node, t1)
            case _:
                raise CCError(f"can not cast {t2} to {t1}")

    def visit_TypeDecl(self, node: c_ast.TypeDecl):
        self.setNodeType(node, self.getNodeType(node.type))

    def visit_IdentifierType(self, node: c_ast.IdentifierType):
        name = " ".join(node.names)
        self.setNodeType(node, self._scope.getType(name))

    def visit_ArrayDecl(self, node: c_ast.ArrayDecl):
        dim = node.dim
        if dim:
            dim = self.getNodeIntConstant(node.dim)._i
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
        if ty is voidTy:
            ty.checkComplete()  # trigger an exception

        self.setNodeType(node, ty)

        # struct Foo { int i; };
        if not node.name:
            return

        if isinstance(self.getParent(), c_ast.Struct):  # fields
            return

        def _addSymbol(v: Value):
            self._scope.addSymbol(node.name, v)
            self.setNodeValue(node, v)

        if isinstance(ty, FunctionType):
            _addSymbol(Function(node.name, ty))
            return

        if "extern" in node.quals:
            _addSymbol(ExternVariable(node.name, ty))
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
                    getBuiltinType("char"),
                    getBuiltinType("unsigned char"),
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
                self.setNodeType(init, ty)

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
                    vR, tyR = self.getNodeValueType(init)
                    if isCompatible(ty, tyR) and isinstance(vR, LValue):
                        return init

                    raise CCError("invalid initializer")

                self.setNodeType(init, ty)

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

                init = self.convert(ty, init)
                if not _local:
                    v = self.getNodeValue(init)
                    if not isinstance(v, Constant):
                        raise CCError("initializer element is not constant")
                return init

        if _global or _static:  # global/static variables
            if node.init:
                node.init = _check(ty, node.init)

            if _global:
                _addSymbol(GlobalVariable(node.name, ty, _static))
            else:
                _addSymbol(StaticVariable(node.name, ty, self._func.getStaticLabel(node.name)))

        else:  # local variables
            if node.init:
                node.init = _check(ty, node.init)

            sz = align(ty.size(), 4)
            scope: LocalScope = self._scope
            scope._offset += sz

            _addSymbol(LocalVariable(node.name, ty, -scope._offset))

            self._func.updateMaxOffset(scope._offset)

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
                sLit = StrLiteral(s, node.value, getBuiltinType("char"))
                self.setNodeValue(node, sLit)
                return

            if "int" in node.type:
                # https://en.cppreference.com/w/c/language/integer_constant
                ty = getBuiltinType(node.type)

                s = s.lower()
                while s.endswith("u") or s.endswith("l"):
                    s = s[:-1]
                if s.startswith("0x"):
                    i = int(s[2:], base=16)
                else:
                    i = int(s, base=10)
                self.setNodeValue(node, IntConstant(i, ty))
                return

            raise CCError("unrecognized constant", node.type)

        except Exception as ex:
            print(traceback.format_exc())
            raise CCError("invalid literal", str(ex))

    def visit_ID(self, node: c_ast.ID):
        _ = self._scope.getSymbol(node.name)
        if isinstance(_, Type):
            self.setNodeType(node, _)
        else:
            self.setNodeValue(node, _)

    def visit_FuncDef(self, node: c_ast.FuncDef):
        if node.param_decls is not None:
            raise CCNotImplemented("old-style (K&R) function definition")

        decl: c_ast.Decl = node.decl
        self.visit(decl)

        self._func = self._scope.getFunction(decl.name)
        self.setNodeValue(node, self._func)

        funcType: FunctionType = self._func.getType()
        funcDecl: c_ast.FuncDecl = decl.type

        self.enterScope()

        # declare arguments
        offset = 8
        for i, argTy in enumerate(funcType._args):
            offset = align(offset, 4)
            if isinstance(funcDecl.args[i], c_ast.Decl):
                argName = funcDecl.args[i].name
                self._scope.addSymbol(argName, Argument(argName, argTy, offset))
            offset += argTy.size()

        self.visit(node.body)

        self.exitScope()
        self._func.checkLabels()
        self._func = None

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
        shouldEnter = True
        match self.getParent():
            case c_ast.FuncDef() | c_ast.For():
                shouldEnter = False

        if shouldEnter:
            self.enterScope()

        block_items = node.block_items or []
        for _ in block_items:
            self.visit(_)

        if shouldEnter:
            self.exitScope()

    def visit_ArrayRef(self, node: c_ast.ArrayRef):
        _, arrTy = self.tryConvertToPointer(node, "name")
        subTy = self.getNodeType(node.subscript)
        match arrTy, subTy:
            case PointerType(), IntType():
                # translate arr[i] to *(arr + i)
                self.setNodeTranslated(
                    node, c_ast.UnaryOp("*", c_ast.BinaryOp("+", node.name, node.subscript))
                )
                assert isCompatible(arrTy._base, self.getNodeType(node))
            case _:
                raise CCError("array subscript expression")

    def visit_StructRef(self, node: c_ast.StructRef):
        v, ty = self.getNodeValueType(node.name)
        match node.type:
            case ".":
                match ty:
                    case StructType():
                        checkLValue(v)
                        field = ty.getField(node.field.name)
                        # translate foo.x to *(__type(x)*)((void *)&foo + __offset(x))
                        addr = c_ast.UnaryOp("&", node.name)
                    case _:
                        raise CCError("not a struct")
            case "->":
                match ty:
                    case PointerType(StructType()):
                        field = ty._base.getField(node.field.name)
                        # translate foo->x to *(__type(x)*)((void *)foo + __offset(x))
                        addr = node.name
                    case _:
                        raise CCError("not a pointer to struct")
            case _:
                unreachable()

        self.setNodeTranslated(
            node,
            c_ast.UnaryOp(
                "*",
                c_ast.Cast(
                    Node(Value(PointerType(field._type))),
                    c_ast.BinaryOp(
                        "+",
                        c_ast.Cast(Node(Value(PointerType(getBuiltinType("void")))), addr),
                        Node(getIntConstant(field._offset)),
                    ),
                ),
            ),
        )

    # whether evaluation of @node involve no computations (which could produce temporary values)
    def isValueStable(self, node: c_ast.Node):
        v = self.getNodeValue(node)
        match v:
            case Constant() | Variable():
                return True
            case _:
                match node:
                    case c_ast.UnaryOp():
                        match node.op:
                            case "&" | "*":
                                return self.isValueStable(node.expr)
        return False

    # @node is evaluated once and its value is stored in a temporary local variable
    def makeValueStable(self, node: c_ast.Node):
        ty = self.getNodeType(node)
        return c_ast.Decl(
            self._func.getTempVarName(),
            [],
            [],
            [],
            [],
            Node(Value(ty)),
            node,
            None,
        )

    # https://en.cppreference.com/w/c/language/operator_assignment
    def visit_Assignment(self, node: c_ast.Assignment):
        vL, tyL = self.getNodeValueType(node.lvalue)
        checkLValue(vL)

        """
        int i;
        i = 1;
        (int)i = 2; // error: assignment to cast is illegal
        """

        match tyL:
            case StructType():
                if node.op != "=":
                    raise CCError(f"can not {node.op} a struct")

                vR, tyR = self.getNodeValueType(node.rvalue)
                if not isinstance(vR, LValue):
                    raise CCNotImplemented("struct rvalue")
                if not isCompatible(tyL, tyR):
                    raise CCError(f"can not assign {tyL} {tyR}")
                self.setNodeTypeR(node, tyL)

            case IntType() | PointerType():
                match node.op:
                    case "=":
                        node.rvalue = self.convert(tyL, node.rvalue)
                        self.setNodeTypeR(node, tyL)

                    case "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "<<=" | ">>=":
                        if self.isValueStable(node.lvalue):
                            # convert "a += b" into "a = a + b"
                            node.rvalue = c_ast.BinaryOp(node.op[:-1], node.lvalue, node.rvalue)
                        else:
                            # convert "a += b" into "p = &a; *p = *p + b"
                            p = self.makeValueStable(c_ast.UnaryOp("&", node.lvalue))
                            node.lvalue = c_ast.UnaryOp("*", p)
                            node.rvalue = c_ast.BinaryOp(
                                node.op[:-1], c_ast.UnaryOp("*", p), node.rvalue
                            )
                        node.op = "="
                        self.visit_Assignment(node)

                    case _:
                        raise CCError(f"unknown assignment operator {node.op}")
            case _:
                raise CCError(f"can not assign {tyL}")

    # https://en.cppreference.com/w/c/language/expressions
    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        match node.op:
            case "sizeof":
                ty = self.getNodeType(node.expr)
                self.setNodeValue(
                    node,
                    getIntConstant(ty.size(), "size_t"),
                )

            case "&":
                v, ty = self.getNodeValueType(node.expr)
                if isinstance(ty, FunctionType):
                    match v:
                        case Function():
                            self.setNodeValue(node, SymConstant(v._name, PointerType(ty)))
                        case _:
                            self.setNodeTypeR(node, PointerType(ty))
                else:
                    match v:
                        case GlobalVariable() | StaticVariable():
                            self.setNodeValue(node, SymConstant(v._label, PointerType(ty)))
                        case LValue():
                            self.setNodeTypeR(node, PointerType(ty))
                        case _:
                            raise CCError(f"can not take address of rvalue")

            case "*":
                v, ty = self.tryConvertToPointer(node, "expr")
                ty: PointerType
                match v:
                    case SymConstant():
                        if ty.toFunction():
                            self.setNodeValue(node, self._scope.getFunction(v._name))
                        else:
                            self.setNodeValue(node, self._scope.getVariable(v._name))
                    case _:
                        self.setNodeTypeL(node, ty._base)

            case "+" | "-" | "~":
                ty = self.getNodeType(node.expr)
                if not isinstance(ty, IntType):
                    raise CCError("not an integer")

                ty = promoteIntType(ty)
                node.expr = self.convert(ty, node.expr)

                v = self.getNodeValue(node.expr)
                if isinstance(v, IntConstant):
                    i = v._i
                    match node.op:
                        case "+":
                            pass
                        case "-":
                            i = -i
                        case "~":
                            i = ~i
                        case _:
                            unreachable()
                    self.setNodeValue(node, IntConstant(i, ty))
                else:
                    if ty.size() == 8:
                        # translate -x into ~x + 1
                        if node.op == "-":
                            self.setNodeTranslated(
                                node,
                                c_ast.BinaryOp(
                                    "+", c_ast.UnaryOp("~", node.expr), Node(getIntConstant(1))
                                ),
                            )
                            return

                    self.setNodeTypeR(node, ty)

            case "!":
                v, ty = self.tryConvertToPointer(node, "expr")
                match ty:
                    case IntType() | PointerType():
                        match v:
                            case IntConstant() | PtrConstant():
                                self.setNodeValue(
                                    node, getIntConstant(0 if v._i == 0 else 1, "int")
                                )
                            case _:
                                self.setNodeTypeR(node, getBuiltinType("int"))
                    case _:
                        raise CCError("not an integer or a pointer")

            case "++" | "--" | "p++" | "p--":
                v, ty = self.getNodeValueType(node.expr)
                checkLValue(v)

                match ty:
                    case IntType():
                        pass
                    case PointerType() if ty.toObject():
                        pass
                    case _:
                        raise CCError("not an integer or a pointer to object")

                c = node.op[-1]
                if node.op.startswith("p"):
                    # translate x++ into p = &x; *p += 1, *p - 1
                    p = self.makeValueStable(c_ast.UnaryOp("&", node.expr))
                    self.setNodeTranslated(
                        node,
                        c_ast.ExprList(
                            [
                                c_ast.Assignment(
                                    f"{c}=", c_ast.UnaryOp("*", p), Node(getIntConstant(1))
                                ),
                                c_ast.BinaryOp(
                                    "-" if c == "+" else "+",
                                    c_ast.UnaryOp("*", p),
                                    Node(getIntConstant(1)),
                                ),
                            ]
                        ),
                    )
                else:
                    # translate ++x into x += 1
                    self.setNodeTranslated(
                        node, c_ast.Assignment(f"{c}=", node.expr, Node(getIntConstant(1)))
                    )

            case _:
                raise CCError("unknown unary operator", node.op)

    def visit_BinaryOp(self, node: c_ast.BinaryOp):
        match node.op:
            case "+":
                vL, tyL = self.tryConvertToPointer(node, "left")
                vR, tyR = self.tryConvertToPointer(node, "right")
                match tyL, tyR:
                    case IntType(), IntType():
                        ty = getArithmeticCommonType(tyL, tyR)
                        node.left = self.convert(ty, node.left)
                        node.right = self.convert(ty, node.right)

                        match vL, vR:
                            case IntConstant(), IntConstant():
                                self.setNodeValue(node, IntConstant(vL._i + vR._i, ty))
                            case _:
                                self.setNodeTypeR(node, ty)

                    case IntType(), PointerType():
                        match vL, vR:
                            case IntConstant(), PtrConstant():
                                self.setNodeValue(node, IntConstant(vL._i + vR._i, tyR))
                            case _:
                                self.setNodeTypeR(node, tyR)

                    case PointerType(), IntType():
                        match vL, vR:
                            case PtrConstant(), IntConstant():
                                self.setNodeValue(node, IntConstant(vL._i + vR._i, tyL))
                            case _:
                                self.setNodeTypeR(node, tyL)

                    case _:
                        raise CCError(f"can not + {tyL} and {tyR}")

            case "-":
                vL, tyL = self.tryConvertToPointer(node, "left")
                vR, tyR = self.tryConvertToPointer(node, "right")
                match tyL, tyR:
                    case IntType(), IntType():
                        ty = getArithmeticCommonType(tyL, tyR)
                        node.left = self.convert(ty, node.left)
                        node.right = self.convert(ty, node.right)

                        match vL, vR:
                            case IntConstant(), IntConstant():
                                self.setNodeValue(node, IntConstant(vL._i - vR._i, ty))
                            case _:
                                self.setNodeTypeR(node, ty)

                    case PointerType(), IntType() if tyL.toObject():
                        self.setNodeTypeR(node, tyL)
                    case (
                        PointerType(),
                        PointerType(),
                    ) if tyL.toObject() and isCompatible(tyL, tyR):
                        self.setNodeTypeR(node, getBuiltinType("ssize_t"))
                    case _:
                        raise CCError(f"can not - {tyL} and {tyR}")

            case "*" | "/" | "%" | "&" | "|" | "^":
                vL, tyL = self.getNodeValueType(node.left)
                vR, tyR = self.getNodeValueType(node.right)
                match tyL, tyR:
                    case IntType(), IntType():
                        ty = getArithmeticCommonType(tyL, tyR)
                        node.left = self.convert(ty, node.left)
                        node.right = self.convert(ty, node.right)

                        match vL, vR:
                            case IntConstant(), IntConstant():
                                iL = vL._i
                                iR = vR._i
                                match node.op:
                                    case "*":
                                        i = iL * iR
                                    case "/":
                                        i = iL // iR
                                    case "%":
                                        i = iL % iR
                                    case "&":
                                        i = iL & iR
                                    case "|":
                                        i = iL | iR
                                    case "^":
                                        i = iL ^ iR
                                    case _:
                                        unreachable()
                                self.setNodeValue(node, IntConstant(i, ty))
                            case _:
                                self.setNodeTypeR(node, ty)

                    case _:
                        raise CCError(f"can not {node.op} {tyL} and {tyR}")

            case "<<" | ">>":
                vL, tyL = self.getNodeValueType(node.left)
                vR, tyR = self.getNodeValueType(node.right)
                match tyL, tyR:
                    case IntType(), IntType():
                        tyL = promoteIntType(tyL)
                        node.left = self.convert(tyL, node.left)
                        tyR = promoteIntType(tyR)
                        node.right = self.convert(tyR, node.right)

                        match vL, vR:
                            case IntConstant(), IntConstant():
                                iL = vL._i
                                iR = vR._i
                                match node.op:
                                    case "<<":
                                        i = iL << iR
                                    case ">>":
                                        i = iL >> iR
                                    case _:
                                        unreachable()
                                self.setNodeValue(node, IntConstant(i, tyL))
                            case _:
                                self.setNodeTypeR(node, tyL)
                    case _:
                        raise CCError(f"can not {node.op} {tyL} and {tyR}")

            case "&&" | "||":
                vL, tyL = self.tryConvertToPointer(node, "left")
                vR, tyR = self.tryConvertToPointer(node, "right")
                match tyL, tyR:
                    case (IntType() | PointerType()), (IntType() | PointerType()):
                        match vL, vR:
                            case (
                                (IntConstant() | PtrConstant()),
                                (IntConstant() | PtrConstant()),
                            ):
                                iL = 0 if vL._i == 0 else 1
                                iR = 0 if vR._i == 0 else 1
                                match node.op:
                                    case "&&":
                                        i = iL & iR
                                    case "||":
                                        i = iL | iR
                                    case _:
                                        unreachable()
                                self.setNodeValue(node, getIntConstant(i, "int"))
                            case _:
                                self.setNodeTypeR(node, getBuiltinType("int"))
                    case _:
                        raise CCError(f"can not {node.op} {tyL} and {tyR}")

            case "==" | "!=":
                vL, tyL = self.tryConvertToPointer(node, "left")
                vR, tyR = self.tryConvertToPointer(node, "right")
                match tyL, tyR:
                    case IntType(), IntType():
                        ty = getArithmeticCommonType(tyL, tyR)
                        node.left = self.convert(ty, node.left)
                        node.right = self.convert(ty, node.right)

                        match vL, vR:
                            case IntConstant(), IntConstant():
                                iL = vL._i
                                iR = vR._i
                                match node.op:
                                    case "==":
                                        i = 1 if iL == iR else 0
                                    case "!=":
                                        i = 1 if iL != iR else 0
                                    case _:
                                        unreachable()
                                self.setNodeValue(node, IntConstant(i, "int"))
                            case _:
                                self.setNodeTypeR(node, getBuiltinType("int"))
                        return

                    case IntType(), PointerType():
                        vL, tyL = self.tryConvertToPointer(node, "left", False, tyR)

                    case PointerType(), IntType():
                        vR, tyR = self.tryConvertToPointer(node, "right", False, tyL)

                match tyL, tyR:

                    case PointerType(), PointerType():
                        if not (
                            isCompatible(tyL, tyR)
                            or (tyL.toObject() and tyR._base is voidTy)
                            or (tyR.toObject() and tyL._base is voidTy)
                        ):
                            ccWarn("comparison of distinct pointer types", tyL, tyR)

                        match vL, vR:
                            case PtrConstant(), PtrConstant():
                                iL = vL._i
                                iR = vR._i
                                match node.op:
                                    case "==":
                                        i = 1 if iL == iR else 0
                                    case "!=":
                                        i = 1 if iL != iR else 0
                                    case _:
                                        unreachable()
                                self.setNodeValue(node, IntConstant(i, "int"))
                            case _:
                                self.setNodeTypeR(node, getBuiltinType("int"))

                    case _:
                        raise CCError(f"can not {node.op} {tyL} and {tyR}")

            case "<" | ">=":
                vL, tyL = self.tryConvertToPointer(node, "left")
                vR, tyR = self.tryConvertToPointer(node, "right")

                def _f():
                    match vL, vR:
                        case (IntConstant() | PtrConstant()), (IntConstant() | PtrConstant()):
                            iL = vL._i
                            iR = vR._i
                            match node.op:
                                case "<":
                                    i = 1 if iL < iR else 0
                                case ">=":
                                    i = 1 if iL >= iR else 0
                                case _:
                                    unreachable()
                            self.setNodeValue(node, getIntConstant(i, "int"))
                        case _:
                            self.setNodeTypeR(node, getBuiltinType("int"))

                match tyL, tyR:
                    case IntType(), IntType():
                        _f()
                    case PointerType(), PointerType() if tyL.toObject() and tyR.toObject():
                        _f()
                    case _:
                        raise CCError(f"can not {node.op} {tyL} and {tyR}")

            case ">" | "<=":
                # translate 'a > b' to 'b < a'
                self.setNodeTranslated(
                    node, c_ast.BinaryOp("<" if node.op == ">" else ">=", node.right, node.left)
                )

            case _:
                raise CCError("unknown binary operator", node.op)

    # https://en.cppreference.com/w/c/language/operator_other#Conditional_operator
    def visit_TernaryOp(self, node: c_ast.TernaryOp):
        self.setNodeLabels(
            node,
            [
                "false",
                "end",
            ],
        )

        vC, tyC = self.tryConvertToPointer(node, "cond")
        vT, tyT = self.tryConvertToPointer(node, "iftrue")
        vF, tyF = self.tryConvertToPointer(node, "iffalse")

        match tyC:
            case IntType() | PointerType():
                pass
            case _:
                raise CCError("not an integer or a pointer")

        match tyT, tyF:
            case VoidType(), VoidType():
                self.setNodeTypeR(node, getBuiltinType("void"))
                return

            case IntType(), IntType():
                ty = getArithmeticCommonType(tyT, tyF)
                node.iftrue = self.convert(ty, node.iftrue)
                node.iffalse = self.convert(ty, node.iffalse)

                match vC, vT, vF:
                    case (IntConstant() | PtrConstant()), IntConstant(), IntConstant():
                        i = vT._i if vC._i else vF._i
                        self.setNodeValue(node, IntConstant(i, ty))
                    case _:
                        self.setNodeTypeR(node, ty)
                return

            case IntType(), PointerType():
                vT, tyT = self.tryConvertToPointer(node, "iftrue", False, tyF)

            case PointerType(), IntType():
                vF, tyF = self.tryConvertToPointer(node, "iffalse", False, tyT)

        def _f(ty):
            match vC, vT, vF:
                case (IntConstant() | PtrConstant()), PtrConstant(), PtrConstant():
                    i = vT._i if vC._i else vF._i
                    self.setNodeValue(node, PtrConstant(i, ty))
                case _:
                    self.setNodeTypeR(node, ty)

        match tyT, tyF:
            case PointerType(), PointerType() if isCompatible(tyT, tyF):
                _f(tyT)
            case PointerType(_base=VoidType()), PointerType():
                _f(tyT)
            case PointerType(), PointerType(_base=VoidType()):
                _f(tyF)
            case _:
                raise CCError("incompatible operand types", tyT, tyF)

    # https://en.cppreference.com/w/c/language/operator_other#Comma_operator
    def visit_ExprList(self, node: c_ast.ExprList):
        assert len(node.exprs) > 0
        for expr in node.exprs:
            ty = self.getNodeType(expr)
        self.setNodeTypeR(node, ty)

    def visit_Alignas(self, _: c_ast.Alignas):
        raise CCNotImplemented("alignas")

    def visit_FuncCall(self, node: c_ast.FuncCall):
        _, ty = self.tryConvertToPointer(node, "name")
        match ty:
            case PointerType(_base=FunctionType()):
                tyF: FunctionType = ty._base
                args: list[c_ast.Node] = node.args.exprs

                if len(args) < len(tyF._args):
                    raise CCError("too few arguments to function call")

                if len(args) > len(tyF._args) and not tyF._ellipsis:
                    raise CCError("too many arguments to function call")

                for i in range(len(args)):
                    if i < len(tyF._args):
                        args[i] = self.convert(tyF._args[i], args[i])
                    else:
                        self.visit(args[i])

                self.setNodeTypeR(node, tyF._ret)
            case _:
                raise CCError("not a pointer to function")

    def visit_Return(self, node: c_ast.Return):
        ret = self._func._type._ret
        if node.expr:
            if isinstance(ret, VoidType):
                raise CCError("'return' with a value, in function returning void")
            node.expr = self.convert(ret, node.expr)
        else:
            if not isinstance(ret, VoidType):
                raise CCError("'return' with no value, in function returning non-void")

    def visit_If(self, node: c_ast.If):
        self.setNodeLabels(
            node,
            [
                "if.false",
                "if.end",
            ],
        )

        ty = self.getNodeType(node.cond)
        match ty:
            case IntType() | PointerType():
                self.visit(node.iftrue)
                self.visit(node.iffalse)
            case _:
                raise CCError("not an integer or a pointer")

    def visit_While(self, node: c_ast.While):
        self.setNodeLabels(node, ["while.start", "while.end"])
        ty = self.getNodeType(node.cond)
        match ty:
            case IntType() | PointerType():
                self.visit(node.stmt)
            case _:
                raise CCError("not an integer or a pointer")

    def visit_DoWhile(self, node: c_ast.DoWhile):
        self.setNodeLabels(node, ["do.start", "do.next", "do.end"])

        ty = self.getNodeType(node.cond)
        match ty:
            case IntType() | PointerType():
                self.visit(node.stmt)
            case _:
                raise CCError("not an integer or a pointer")

    def visit_For(self, node: c_ast.For):
        self.setNodeLabels(node, ["for.start", "for.next", "for.end"])

        self.enterScope()

        self.visit(node.init)

        if node.cond:
            ty = self.getNodeType(node.cond)
            match ty:
                case IntType() | PointerType():
                    pass
                case _:
                    raise CCError("not an integer or a pointer")

        self.visit(node.next)
        self.visit(node.stmt)

        self.exitScope()

    # https://en.cppreference.com/w/c/language/switch
    def visit_Switch(self, node: c_ast.Switch):
        self.setNodeLabels(node, ["switch.end"])

        ty = self.getNodeType(node.cond)
        match ty:
            case IntType():
                self.visit(node.stmt)
            case _:
                raise CCError("not an integer")

        r = self.getNodeRecord(node)
        r._cases = []

    def getLoop(self):
        for node in reversed(self._path[:-1]):
            match node:
                case c_ast.While() | c_ast.DoWhile() | c_ast.For():
                    return node
        return None

    def getSwitch(self):
        for node in reversed(self._path[:-1]):
            if isinstance(node, c_ast.Switch):
                return node
        return None

    def getLoopOrSwitch(self):
        for node in reversed(self._path[:-1]):
            match node:
                case c_ast.While() | c_ast.DoWhile() | c_ast.For():
                    return node
                case c_ast.Switch():
                    return node
        return None

    def addCase(self, switchStmt: Node, _case: tuple[Optional[int], str]):
        switchR = self.getNodeRecord(switchStmt)
        for i in switchR._cases:
            if i == _case[0]:
                raise CCError("duplicated case", _case)
        switchR._cases.append(_case)

    def visit_Case(self, node: c_ast.Case):
        switchStmt = self.getSwitch()
        if not isinstance(switchStmt, c_ast.Switch):
            raise CCError("invalid 'case'")

        v = self.getNodeValue(node.expr)
        match v:
            case IntConstant():
                if v.getType().size() == 8:
                    raise CCNotImplemented("case value of size 8")

                self.setNodeLabels(node, ["switch.case"])
                self.addCase(switchStmt, (v._i, self.getNodeLabels(node)[0]))

                for stmt in node.stmts:
                    self.visit(stmt)
            case _:
                raise CCError("not an integer constant")

    def visit_Default(self, node: c_ast.Default):
        switchStmt = self.getSwitch()
        if not isinstance(switchStmt, c_ast.Switch):
            raise CCError("invalid 'default'")

        self.setNodeLabels(node, ["switch.default"])
        self.addCase(switchStmt, (None, self.getNodeLabels(node)[0]))

        for stmt in node.stmts:
            self.visit(stmt)

    def visit_Break(self, node: c_ast.Break):
        loopOrSwitchStmt = self.getLoopOrSwitch()
        if not loopOrSwitchStmt:
            raise CCError("invalid 'break'")
        labelEnd = self.getNodeLabels(loopOrSwitchStmt)[-1]
        self.setNodeLabels(node, [labelEnd])

    def visit_Continue(self, node: c_ast.Continue):
        loopStmt = self.getLoop()
        if not loopStmt:
            raise CCError("invalid 'continue'")

        if isinstance(loopStmt, c_ast.While):
            labelNext = self.getNodeLabels(loopStmt)[0]
        else:
            labelNext = self.getNodeLabels(loopStmt)[1]
        self.setNodeLabels(node, [labelNext])

    def visit_Label(self, node: c_ast.Label):
        if self._func is None:
            raise CCError("unexpected label")
        self.setNodeLabels(node, [self._func.addLabel(node.name)])

        self.visit(node.stmt)

    def visit_Goto(self, node: c_ast.Goto):
        self.setNodeLabels(node, [self._func.addGoto(node.name)])

    # https://en.cppreference.com/w/c/language/_Static_assert
    def visit_StaticAssert(self, node: c_ast.StaticAssert):
        v = self.getNodeValue(node.cond)
        match v:
            case IntConstant() | PtrConstant():
                if v._i == 0:
                    raise CCError("static assertion failed", node.message)
            case _:
                raise CCError("not an integer constant or a pointer constant")
