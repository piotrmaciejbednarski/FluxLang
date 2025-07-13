#!/usr/bin/env python3
"""
Flux Abstract Syntax Tree (AST) - fast.py

Complete AST node definitions for the Flux programming language.
Represents all language constructs as defined in the language specification.

Usage:
    from fast import *
    # Create AST nodes to represent Flux programs
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import List, Optional, Union, Any, Tuple
from enum import Enum, auto
import sys

# ============================================================================
# Base AST Node
# ============================================================================

@dataclass
class ASTNode(ABC):
    """Base class for all AST nodes"""
    line: int = 0
    column: int = 0

# ============================================================================
# Program Structure
# ============================================================================

@dataclass
class Program(ASTNode):
    """Root node representing an entire Flux program"""
    global_items: List['GlobalItem'] = field(default_factory=list)

@dataclass
class GlobalItem(ASTNode):
    """Base class for top-level program items"""
    pass

class CVModifier(Enum):
    VOLATILE = "volatile"
    CONST = "const"

@dataclass
class Parameter(ASTNode):
    """Function parameter"""
    name: str = ""
    type: 'Type' = None

@dataclass
class TemplateParameter(ASTNode):
    """Template parameter"""
    name: str = ""
    constraints: Optional['Type'] = None  # Added constraint support

# ============================================================================
# Object Definitions
# ============================================================================

class AccessSpecifier(Enum):
    PUBLIC = "public"
    PRIVATE = "private"

@dataclass
class ObjectMember(ASTNode):
    """Base class for object members"""
    access: Optional[AccessSpecifier] = None

@dataclass
class MagicMethod(ASTNode):
    """Magic method in objects"""
    name: str = ""  # __init, __exit, etc.
    parameters: List[Parameter] = field(default_factory=list)
    return_type: 'Type' = None
    body: List['Statement'] = field(default_factory=list)

@dataclass
class ObjectDef(GlobalItem):
    """Object definition"""
    name: str = ""
    members: List[Union['FunctionDef', 'VariableDeclaration', MagicMethod, 'ObjectDef']] = field(default_factory=list)
    inheritance: List['QualifiedName'] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)
    is_forward_decl: bool = False

# ============================================================================
# Struct Definitions
# ============================================================================

@dataclass
class StructMember(ASTNode):
    """Struct member (variable declaration with optional access specifier)"""
    declaration: 'VariableDeclaration' = None
    access: Optional[AccessSpecifier] = None

@dataclass
class StructDef(GlobalItem):
    """Struct definition"""
    name: str = ""
    members: List[StructMember] = field(default_factory=list)
    inheritance: List['QualifiedName'] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)
    is_forward_decl: bool = False

# ============================================================================
# Namespace Definitions
# ============================================================================

@dataclass
class NamespaceMember(ASTNode):
    """Base class for namespace members"""
    pass

@dataclass
class NamespaceDef(GlobalItem):
    """Namespace definition"""
    name: str = ""
    members: List[Union['FunctionDef', 'ObjectDef', 'StructDef', 'VariableDeclaration', 'NamespaceDef']] = field(default_factory=list)

# ============================================================================
# Import and Using Statements
# ============================================================================

@dataclass
class ImportStmt(GlobalItem):
    """Import statement"""
    path: str = ""
    alias: Optional[str] = None

@dataclass
class UsingStmt(GlobalItem):
    """Using statement"""
    names: List[Union['QualifiedName', 'Identifier']] = field(default_factory=list)

# ============================================================================
# External FFI
# ============================================================================

@dataclass
class ExternBlock(GlobalItem):
    """External function block"""
    language: str = ""
    declarations: List['FunctionDecl'] = field(default_factory=list)

# ============================================================================
# Compile-time Blocks
# ============================================================================

@dataclass
class ComptBlock(GlobalItem):
    """Compile-time block"""
    statements: List['Statement'] = field(default_factory=list)

# ============================================================================
# Macro Definitions
# ============================================================================

@dataclass
class MacroDef(GlobalItem):
    """Macro definition"""
    name: str = ""
    value: Optional[Union['Expression', str]] = None  # Can be expression or operator string

# ============================================================================
# Type System
# ============================================================================

class TypeQualifier(Enum):
    VOLATILE = "volatile"
    CONST = "const"

class PrimitiveTypeKind(Enum):
    INT = "int"
    FLOAT = "float"
    BOOL = "bool"
    CHAR = "char"
    VOID = "void"

class Endianness(Enum):
    LITTLE = 0
    BIG = 1

@dataclass
class Type(ASTNode):
    """Base class for all types"""
    qualifiers: List[TypeQualifier] = field(default_factory=list)

@dataclass
class PrimitiveType(Type):
    """Primitive type (int, float, bool, char, void)"""
    kind: PrimitiveTypeKind = PrimitiveTypeKind.VOID

@dataclass
class DataType(Type):
    """Data type with bit width and optional alignment"""
    bit_width: int = 0
    alignment: Optional[int] = None
    endianness: Endianness = Endianness.BIG
    is_signed: bool = False
    is_array: bool = False
    array_size: Optional[int] = None

@dataclass
class NamedType(Type):
    """Named type (user-defined types, templates)"""
    name: 'QualifiedName' = None
    template_args: List[Union['Type', 'Expression']] = field(default_factory=list)

@dataclass
class PointerType(Type):
    """Pointer type"""
    pointee_type: 'Type' = None

@dataclass
class FunctionPointerType(Type):
    """Function pointer type"""
    return_type: 'Type' = None
    parameters: List[Parameter] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)

# ============================================================================
# Variable Declarations
# ============================================================================

@dataclass
class VariableDeclarator(ASTNode):
    """Base class for variable declarators"""
    name: str = ""

@dataclass
class SimpleVariableDeclarator(VariableDeclarator):
    """Simple variable declarator with optional initializer"""
    initializer: Optional['Expression'] = None

@dataclass
class ArrayVariableDeclarator(VariableDeclarator):
    """Array variable declarator"""
    size: Optional['Expression'] = None
    initializer: Optional['ArrayInitializer'] = None

@dataclass
class VariableDeclaration(ASTNode):
    """Variable declaration"""
    type: Type = None
    declarators: List[VariableDeclarator] = field(default_factory=list)

@dataclass
class TypeAlias(ASTNode):
    """Type alias (using 'as' keyword)"""
    original_type: Type = None
    alias: str = ""

# ============================================================================
# Statements
# ============================================================================

@dataclass
class Statement(ASTNode):
    """Base class for all statements"""
    pass

@dataclass
class ExpressionStmt(Statement):
    """Expression statement"""
    expression: Optional['Expression'] = None

@dataclass
class BlockStmt(Statement):
    """Compound statement (block)"""
    statements: List[Statement] = field(default_factory=list)

@dataclass
class IfStmt(Statement):
    """If statement"""
    condition: 'Expression' = None
    then_stmt: 'Statement' = None
    elif_parts: List[Tuple['Expression', 'Statement']] = field(default_factory=list)
    else_stmt: Optional['Statement'] = None

@dataclass
class WhileStmt(Statement):
    """While statement"""
    condition: 'Expression' = None
    body: 'Statement' = None

@dataclass
class DoWhileStmt(Statement):
    """Do-while statement"""
    body: 'Statement' = None
    condition: 'Expression' = None

@dataclass
class ForStmt(Statement):
    """Base class for for statements"""
    pass

@dataclass
class CStyleForStmt(ForStmt):
    """C-style for statement"""
    init: Optional['Expression'] = None
    condition: Optional['Expression'] = None
    update: Optional['Expression'] = None
    body: 'Statement' = None

@dataclass
class PythonStyleForStmt(ForStmt):
    """Python-style for statement"""
    variables: List[str] = field(default_factory=list)
    iterable: 'Expression' = None
    body: 'Statement' = None

@dataclass
class SwitchCase(ASTNode):
    """Switch case"""
    value: 'Expression' = None
    body: 'Statement' = None

@dataclass
class DefaultCase(ASTNode):
    """Default case for switch/match"""
    body: 'Statement' = None

@dataclass
class SwitchStmt(Statement):
    """Switch statement"""
    expression: 'Expression' = None
    cases: List[SwitchCase] = field(default_factory=list)
    default_case: Optional[DefaultCase] = None

@dataclass
class CatchClause(ASTNode):
    """Catch clause for try-catch"""
    type: Type = None
    variable: str = ""
    body: 'Statement' = None

@dataclass
class TryCatchStmt(Statement):
    """Try-catch statement"""
    try_body: 'Statement' = None
    catch_clauses: List[CatchClause] = field(default_factory=list)

@dataclass
class ReturnStmt(Statement):
    """Return statement"""
    value: Optional['Expression'] = None

@dataclass
class BreakStmt(Statement):
    """Break statement"""
    pass

@dataclass
class ContinueStmt(Statement):
    """Continue statement"""
    pass

@dataclass
class ThrowStmt(Statement):
    """Throw statement"""
    expression: 'Expression' = None

@dataclass
class AssertStmt(Statement):
    """Assert statement"""
    condition: 'Expression' = None
    message: Optional['Expression'] = None

@dataclass
class AsmStmt(Statement):
    """Inline assembly statement"""
    content: str = ""

@dataclass
class VariableDeclStmt(Statement):
    """Variable declaration statement"""
    declaration: 'VariableDeclaration' = None

@dataclass
class DestructuringStmt(Statement):
    """Destructuring assignment statement"""
    target_vars: List[str] = field(default_factory=list)
    source_expr: 'Expression' = None
    source_fields: List[str] = field(default_factory=list)

# ============================================================================
# Patterns (for match statements)
# ============================================================================

@dataclass
class Pattern(ASTNode):
    """Base class for patterns"""
    pass

@dataclass
class InPattern(Pattern):
    """Pattern: expression in expression"""
    expr: 'Expression' = None
    container: 'Expression' = None

@dataclass
class RangePattern(Pattern):
    """Pattern: expression in range"""
    expr: 'Expression' = None
    range_expr: 'RangeExpression' = None

# ============================================================================
# Expressions
# ============================================================================

@dataclass
class Expression(ASTNode):
    """Base class for all expressions"""
    pass

# Literals
@dataclass
class IntegerLiteral(Expression):
    """Integer literal"""
    value: str = ""
    radix: int = 10  # 10 for decimal, 16 for hex, 2 for binary

@dataclass
class FloatLiteral(Expression):
    """Float literal"""
    value: str = ""

@dataclass
class CharacterLiteral(Expression):
    """Character literal"""
    value: str = ""

@dataclass
class StringLiteral(Expression):
    """String literal"""
    value: str = ""

@dataclass
class BooleanLiteral(Expression):
    """Boolean literal"""
    value: bool = False

@dataclass
class VoidLiteral(Expression):
    """Void literal"""
    pass

# Identifiers and names
@dataclass
class Identifier(Expression):
    """Simple identifier"""
    name: str = ""

@dataclass
class QualifiedName(Expression):
    """Qualified name (namespace::name)"""
    parts: List[str] = field(default_factory=list)

@dataclass
class ThisExpression(Expression):
    """'this' keyword"""
    pass

@dataclass
class SuperExpression(Expression):
    """'super' keyword"""
    pass

@dataclass
class VirtualQualifiedName(Expression):
    """virtual::qualified_name"""
    name: 'QualifiedName' = None

# Binary operators
class BinaryOperator(Enum):
    # Arithmetic
    ADD = "+"
    SUBTRACT = "-"
    MULTIPLY = "*"
    DIVIDE = "/"
    MODULO = "%"
    POWER = "^"
    
    # Comparison
    EQUAL = "=="
    NOT_EQUAL = "!="
    LESS_THAN = "<"
    LESS_EQUAL = "<="
    GREATER_THAN = ">"
    GREATER_EQUAL = ">="
    
    # Logical
    LOGICAL_AND = "&"
    LOGICAL_OR = "|"
    AND_KEYWORD = "and"
    OR_KEYWORD = "or"
    
    # Bitwise
    BITWISE_AND = "&"
    BITWISE_OR = "|"
    BITWISE_XOR = "^^"
    XOR_KEYWORD = "xor"
    
    # Shift
    LEFT_SHIFT = "<<"
    RIGHT_SHIFT = ">>"
    
    # Identity
    IS = "is"
    NOT = "not"
    
    # Membership
    IN = "in"
    
    # Range
    RANGE = ".."

# Unary operators
class UnaryOperator(Enum):
    PLUS = "+"
    MINUS = "-"
    LOGICAL_NOT = "!"
    NOT_KEYWORD = "not"
    ADDRESS_OF = "@"
    DEREFERENCE = "*"
    PRE_INCREMENT = "++"
    PRE_DECREMENT = "--"
    POST_INCREMENT = "++"
    POST_DECREMENT = "--"
    SIZEOF = "sizeof"
    TYPEOF = "typeof"
    ALIGNOF = "alignof"

# Assignment operators
class AssignmentOperator(Enum):
    ASSIGN = "="
    PLUS_ASSIGN = "+="
    MINUS_ASSIGN = "-="
    MULTIPLY_ASSIGN = "*="
    DIVIDE_ASSIGN = "/="
    MODULO_ASSIGN = "%="
    POWER_ASSIGN = "^="
    AND_ASSIGN = "&="
    OR_ASSIGN = "|="
    XOR_ASSIGN = "^^="
    LEFT_SHIFT_ASSIGN = "<<="
    RIGHT_SHIFT_ASSIGN = ">>="

@dataclass
class BinaryExpression(Expression):
    """Binary expression"""
    left: 'Expression' = None
    operator: BinaryOperator = BinaryOperator.ADD
    right: 'Expression' = None

@dataclass
class UnaryExpression(Expression):
    """Unary expression"""
    operator: UnaryOperator = UnaryOperator.PLUS
    operand: 'Expression' = None
    is_postfix: bool = False

@dataclass
class AssignmentExpression(Expression):
    """Assignment expression"""
    left: 'Expression' = None
    operator: AssignmentOperator = AssignmentOperator.ASSIGN
    right: 'Expression' = None

@dataclass
class ConditionalExpression(Expression):
    """Ternary conditional expression (condition ? true_expr : false_expr)"""
    condition: 'Expression' = None
    true_expr: 'Expression' = None
    false_expr: 'Expression' = None

@dataclass
class CastExpression(Expression):
    """Type cast expression"""
    type: Type = None
    expression: 'Expression' = None

@dataclass
class FunctionCall(Expression):
    """Function call expression"""
    function: 'Expression' = None
    arguments: List['Expression'] = field(default_factory=list)
    template_args: List[Union[Type, 'Expression']] = field(default_factory=list)

@dataclass
class MemberAccess(Expression):
    """Member access (obj.member)"""
    object: 'Expression' = None
    member: str = ""

@dataclass
class ScopeAccess(Expression):
    """Scope access (namespace::member)"""
    scope: 'Expression' = None
    member: str = ""

@dataclass
class ArrayAccess(Expression):
    """Array access (array[index])"""
    array: 'Expression' = None
    index: 'Expression' = None

@dataclass
class ArrayLiteral(Expression):
    """Array literal [1, 2, 3]"""
    elements: List['Expression'] = field(default_factory=list)

@dataclass
class ArrayComprehension(Expression):
    """Array comprehension [expr for (var in iterable) if (condition)]"""
    expression: 'Expression' = None
    variable: str = ""
    iterable: 'Expression' = None
    condition: Optional['Expression'] = None

@dataclass
class RangeExpression(Expression):
    """Range expression (start..end)"""
    start: 'Expression' = None
    end: 'Expression' = None

# String interpolation
@dataclass
class IStringLiteral(Expression):
    """i-string literal with interpolation"""
    template: str = ""
    expressions: List['Expression'] = field(default_factory=list)

@dataclass
class FStringLiteral(Expression):
    """f-string literal with embedded expressions"""
    content: str = ""

# Struct initialization
@dataclass
class StructInitItem(ASTNode):
    """Struct initialization item"""
    name: str = ""
    value: Optional['Expression'] = None

@dataclass
class StructInitializer(Expression):
    """Struct initializer {x = 1, y = 2}"""
    items: List[StructInitItem] = field(default_factory=list)

@dataclass
class ArrayInitializer(Expression):
    """Array initializer [1, 2, 3]"""
    elements: List['Expression'] = field(default_factory=list)

# Object instantiation
@dataclass
class ObjectInstantiation(Expression):
    """Object instantiation"""
    type_name: Union['QualifiedName', 'Identifier'] = None
    arguments: List['Expression'] = field(default_factory=list)

# ============================================================================
# Function Definition
# ============================================================================

@dataclass
class FunctionDecl(ASTNode):
    """Function declaration"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    return_type: Type = None
    template_params: List[TemplateParameter] = field(default_factory=list)

@dataclass
class FunctionContract(ASTNode):
    """Function contract"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    body: List[AssertStmt] = field(default_factory=list)

@dataclass
class FunctionDef(GlobalItem):
    """Function definition"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    return_type: Type = None
    body: List[Statement] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)
    contracts: List[FunctionContract] = field(default_factory=list)
    is_compt: bool = False  # Compile-time function

@dataclass
class OperatorOverload(ASTNode):
    """Operator overload definition"""
    operator: str = ""
    left_type: Type = None
    right_type: Optional[Type] = None
    return_type: Type = None
    body: List[Statement] = field(default_factory=list)

# ============================================================================
# AST Visitor Pattern (for traversal)
# ============================================================================

class ASTVisitor(ABC):
    """Abstract base class for AST visitors"""
    
    @abstractmethod
    def visit(self, node: ASTNode) -> Any:
        """Visit an AST node"""
        pass
    
    def generic_visit(self, node: ASTNode) -> Any:
        """Generic visit method - calls visit on all child nodes"""
        for field_name, field_value in node.__dict__.items():
            if isinstance(field_value, ASTNode):
                self.visit(field_value)
            elif isinstance(field_value, list):
                for item in field_value:
                    if isinstance(item, ASTNode):
                        self.visit(item)
        return None

class ASTTraverser(ASTVisitor):
    """Basic AST traverser that visits all nodes"""
    
    def visit(self, node: ASTNode) -> Any:
        method_name = f'visit_{type(node).__name__}'
        visitor = getattr(self, method_name, self.generic_visit)
        return visitor(node)

# ============================================================================
# Utility Functions
# ============================================================================

def create_program() -> Program:
    """Create an empty program node"""
    return Program()

def create_function_def(name: str, return_type: Type = None) -> FunctionDef:
    """Create a function definition node"""
    return FunctionDef(name=name, return_type=return_type or PrimitiveType(PrimitiveTypeKind.VOID))

def create_object_def(name: str) -> ObjectDef:
    """Create an object definition node"""
    return ObjectDef(name=name)

def create_struct_def(name: str) -> StructDef:
    """Create a struct definition node"""
    return StructDef(name=name)

def create_primitive_type(kind: PrimitiveTypeKind) -> PrimitiveType:
    """Create a primitive type node"""
    return PrimitiveType(kind=kind)

def create_data_type(bit_width: int, alignment: int = None, is_signed: bool = False) -> DataType:
    """Create a data type node"""
    return DataType(bit_width=bit_width, alignment=alignment, is_signed=is_signed)

def create_identifier(name: str) -> Identifier:
    """Create an identifier expression"""
    return Identifier(name=name)

def create_qualified_name(parts: List[str]) -> QualifiedName:
    """Create a qualified name expression"""
    return QualifiedName(parts=parts)

def create_binary_expr(left: Expression, op: BinaryOperator, right: Expression) -> BinaryExpression:
    """Create a binary expression"""
    return BinaryExpression(left=left, operator=op, right=right)

def create_integer_literal(value: str, radix: int = 10) -> IntegerLiteral:
    """Create an integer literal"""
    return IntegerLiteral(value=value, radix=radix)

def create_string_literal(value: str) -> StringLiteral:
    """Create a string literal"""
    return StringLiteral(value=value)

# ============================================================================
# AST Validation
# ============================================================================

class ASTValidator(ASTVisitor):
    """Validates AST structure for correctness"""
    
    def __init__(self):
        self.errors = []
    
    def validate(self, node: ASTNode) -> List[str]:
        """Validate an AST node and return list of errors"""
        self.errors = []
        self.visit(node)
        return self.errors
    
    def visit(self, node: ASTNode) -> None:
        """Visit and validate a node"""
        method_name = f'validate_{type(node).__name__}'
        validator = getattr(self, method_name, self.generic_visit)
        validator(node)
    
    def validate_FunctionDef(self, node: FunctionDef) -> None:
        """Validate function definition"""
        if not node.name:
            self.errors.append("Function definition must have a name")
        if not node.return_type:
            self.errors.append(f"Function '{node.name}' must have a return type")
        self.generic_visit(node)
    
    def validate_ObjectDef(self, node: ObjectDef) -> None:
        """Validate object definition"""
        if not node.name:
            self.errors.append("Object definition must have a name")
        self.generic_visit(node)
    
    def validate_StructDef(self, node: StructDef) -> None:
        """Validate struct definition"""
        if not node.name:
            self.errors.append("Struct definition must have a name")
        self.generic_visit(node)

if __name__ == "__main__":
    # Example usage and testing - Representing test.fx as AST
    def main():
        print("Flux AST Module - fast.py")
    
    main()