from dataclasses import dataclass, field
from typing import List, Any, Optional, Union, Tuple
from enum import Enum

# Base classes first
@dataclass
class ASTNode:
    """Base class for all AST nodes"""
    pass

# Enums and simple types
class DataType(Enum):
    INT = "int"
    FLOAT = "float"
    CHAR = "char"
    BOOL = "bool"
    DATA = "data"
    VOID = "void"
    THIS = "this"

class Operator(Enum):
    ADD = "+"
    SUB = "-"
    MUL = "*"
    DIV = "/"
    MOD = "%"
    EQUAL = "=="
    NOT_EQUAL = "!="
    LESS_THAN = "<"
    LESS_EQUAL = "<="
    GREATER_THAN = ">"
    GREATER_EQUAL = ">="
    AND = "and"
    OR = "or"
    NOT = "not"
    XOR = "xor"
    BITSHIFT_LEFT = "<<"
    BITSHIFT_RIGHT = ">>"
    INCREMENT = "++"
    DECREMENT = "--"

# Literal values (no dependencies)
@dataclass
class Literal(ASTNode):
    value: Any
    type: DataType

@dataclass
class Identifier(ASTNode):
    name: str

# Type definitions
@dataclass
class TypeSpec(ASTNode):
    base_type: DataType
    is_signed: bool = True
    is_const: bool = False
    is_volatile: bool = False
    bit_width: Optional[int] = None
    alignment: Optional[int] = None
    is_array: bool = False
    array_size: Optional[int] = None
    is_pointer: bool = False

@dataclass
class CustomType(ASTNode):
    name: str
    type_spec: TypeSpec

# Expressions (built up from simple to complex)
@dataclass
class Expression(ASTNode):
    pass

@dataclass
class QualifiedName(Expression):
    """Represents qualified names with super:: or virtual:: or super::virtual:: for objects or structs"""
    qualifiers: List[str]
    member: Optional[str] = None
    
    def __str__(self):
        qual_str = "::".join(self.qualifiers)
        if self.member:
            return f"{qual_str}.{self.member}"
        return qual_str

@dataclass
class BinaryOp(Expression):
    left: Expression
    operator: Operator
    right: Expression

@dataclass
class UnaryOp(Expression):
    operator: Operator
    operand: Expression
    is_postfix: bool = False

@dataclass
class CastExpression(Expression):
    target_type: TypeSpec
    expression: Expression

@dataclass
class FunctionCall(Expression):
    name: str
    arguments: List[Expression] = field(default_factory=list)

@dataclass
class MemberAccess(Expression):
    object: Expression
    member: str

@dataclass
class ArrayAccess(Expression):
    array: Expression
    index: Expression

@dataclass
class PointerDeref(Expression):
    pointer: Expression

@dataclass
class AddressOf(Expression):
    expression: Expression

@dataclass
class AlignOf(Expression):
	target: Union[TypeSpec, Expression]

@dataclass
class SizeOf(Expression):
	target: Union[TypeSpec, Expression]

# Variable declarations
@dataclass
class VariableDeclaration(ASTNode):
    name: str
    type_spec: TypeSpec
    initial_value: Optional[Expression] = None

# Type declarations
class TypeDeclaration(Expression):
    """AST node for type declarations using AS keyword"""
    def __init__(self, name: str, base_type: TypeSpec, initial_value: Optional[Expression] = None):
        self.name = name
        self.base_type = base_type
        self.initial_value = initial_value
    
    def __repr__(self):
        init_str = f" = {self.initial_value}" if self.initial_value else ""
        return f"TypeDeclaration({self.base_type} as {self.name}{init_str})"

# Statements
@dataclass
class Statement(ASTNode):
    pass

@dataclass
class ExpressionStatement(Statement):
    expression: Expression

@dataclass
class Assignment(Statement):
    target: Expression
    value: Expression

@dataclass
class Block(Statement):
    statements: List[Statement] = field(default_factory=list)

@dataclass
class XorStatement(Statement):
    expressions: List[Expression] = field(default_factory=list)

@dataclass
class IfStatement(Statement):
    condition: Expression
    then_block: Block
    elif_blocks: List[tuple] = field(default_factory=list)  # (condition, block) pairs
    else_block: Optional[Block] = None

@dataclass
class WhileLoop(Statement):
    condition: Expression
    body: Block

@dataclass
class DoWhileLoop(Statement):
    body: Block
    condition: Expression

@dataclass
class ForLoop(Statement):
    init: Optional[Statement]
    condition: Optional[Expression]
    update: Optional[Statement]
    body: Block

@dataclass
class ForInLoop(Statement):
    variables: List[str]
    iterable: Expression
    body: Block

@dataclass
class ReturnStatement(Statement):
    value: Optional[Expression] = None

@dataclass
class BreakStatement(Statement):
    pass

@dataclass
class ContinueStatement(Statement):
    pass

@dataclass
class Case(ASTNode):
    value: Optional[Expression]  # None for default case
    body: Block

@dataclass
class SwitchStatement(Statement):
    expression: Expression
    cases: List[Case] = field(default_factory=list)

@dataclass
class TryBlock(Statement):
    try_body: Block
    catch_blocks: List[tuple] = field(default_factory=list)  # (exception_type, exception_name, body) tuples

@dataclass
class ThrowStatement(Statement):
    expression: Expression

@dataclass
class AssertStatement(Statement):
    condition: Expression
    message: Optional[str] = None

# Function parameter
@dataclass
class Parameter(ASTNode):
    name: str
    type_spec: TypeSpec

@dataclass
class InlineAsm(Expression):
    """Represents inline assembly block"""
    body: str  # The raw assembly code
    is_volatile: bool = False  # New flag for volatile assembly

# Function definition
@dataclass
class FunctionDef(ASTNode):
    name: str
    parameters: List[Parameter]
    return_type: TypeSpec
    body: Block
    is_const: bool = False
    is_volatile: bool = False

@dataclass
class DestructuringAssignment(Statement):
    """Destructuring assignment"""
    variables: List[Union[str, Tuple[str, TypeSpec]]]  # Can be simple names or (name, type) pairs
    source: Expression
    source_type: Optional[Identifier]  # For the "from" clause
    is_explicit: bool  # True if using "as" syntax

# Struct member
@dataclass
class StructMember(ASTNode):
    name: str
    type_spec: TypeSpec
    is_private: bool = False

# Struct definition
@dataclass
class StructDef(ASTNode):
    name: str
    members: List[StructMember] = field(default_factory=list)
    base_structs: List[str] = field(default_factory=list)  # inheritance
    nested_structs: List['StructDef'] = field(default_factory=list)

# Object method
@dataclass
class ObjectMethod(ASTNode):
    name: str
    parameters: List[Parameter]
    return_type: TypeSpec
    body: Block
    is_private: bool = False
    is_const: bool = False
    is_volatile: bool = False

# Object definition
@dataclass
class ObjectDef(ASTNode):
    name: str
    methods: List[ObjectMethod] = field(default_factory=list)
    members: List[StructMember] = field(default_factory=list)
    base_objects: List[str] = field(default_factory=list)
    nested_objects: List['ObjectDef'] = field(default_factory=list)
    nested_structs: List[StructDef] = field(default_factory=list)
    super_calls: List[Tuple[str, str, List[Expression]]] = field(default_factory=list)
    virtual_calls: List[Tuple[str, str, List[Expression]]] = field(default_factory=list)
    virtual_instances: List[Tuple[str, str, List[Expression]]] = field(default_factory=list)

# Namespace definition
@dataclass
class NamespaceDef(ASTNode):
    name: str
    functions: List[FunctionDef] = field(default_factory=list)
    structs: List[StructDef] = field(default_factory=list)
    objects: List[ObjectDef] = field(default_factory=list)
    variables: List[VariableDeclaration] = field(default_factory=list)
    nested_namespaces: List['NamespaceDef'] = field(default_factory=list)
    base_namespaces: List[str] = field(default_factory=list)  # inheritance

# Import statement
@dataclass
class ImportStatement(Statement):
    module_name: str

# Custom type definition
@dataclass
class CustomTypeStatement(Statement):
    name: str
    type_spec: TypeSpec

# Function definition statement
@dataclass
class FunctionDefStatement(Statement):
    function_def: FunctionDef

# Struct definition statement
@dataclass
class StructDefStatement(Statement):
    struct_def: StructDef

# Object definition statement
@dataclass
class ObjectDefStatement(Statement):
    object_def: ObjectDef

# Namespace definition statement
@dataclass
class NamespaceDefStatement(Statement):
    namespace_def: NamespaceDef

# Program root
@dataclass
class Program(ASTNode):
    statements: List[Statement] = field(default_factory=list)

# Example usage
if __name__ == "__main__":
    # Create a simple program AST
    main_func = FunctionDef(
        name="main",
        parameters=[],
        return_type=TypeSpec(base_type=DataType.INT),
        body=Block([
            ReturnStatement(Literal(0, DataType.INT))
        ])
    )
    
    program = Program(
        statements=[
            ImportStatement("standard.fx"),
            FunctionDefStatement(main_func)
        ]
    )
    
    print("AST created successfully!")
    print(f"Program has {len(program.statements)} statements")
    print(f"Main function has {len(main_func.body.statements)} statements")