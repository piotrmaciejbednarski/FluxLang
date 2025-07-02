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
from typing import List, Optional, Union, Any
from enum import Enum, auto

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

# ============================================================================
# Function Definitions
# ============================================================================

class FunctionModifier(Enum):
    VOLATILE = "volatile"
    CONST = "const"
    COMPT = "compt"

@dataclass
class Parameter(ASTNode):
    """Function parameter"""
    name: str = ""
    type: 'Type' = None

@dataclass
class TemplateParameter(ASTNode):
    """Template parameter"""
    name: str = ""

@dataclass
class FunctionDef(GlobalItem):
    """Function definition"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    return_type: 'Type' = None
    body: List['Statement'] = field(default_factory=list)
    modifiers: List[FunctionModifier] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)

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
class ObjectFunctionDef(ObjectMember):
    """Function definition inside an object"""
    function: 'FunctionDef' = None

@dataclass
class ObjectVariableDecl(ObjectMember):
    """Variable declaration inside an object"""
    declaration: 'VariableDeclaration' = None

@dataclass
class ObjectObjectDef(ObjectMember):
    """Nested object definition"""
    object_def: 'ObjectDef' = None

@dataclass
class ObjectStructDef(ObjectMember):
    """Nested struct definition"""
    struct_def: 'StructDef' = None

@dataclass
class ObjectDef(GlobalItem):
    """Object definition"""
    name: str = ""
    members: List[ObjectMember] = field(default_factory=list)
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
class NamespaceFunctionDef(NamespaceMember):
    """Function definition in namespace"""
    function: 'FunctionDef' = None

@dataclass
class NamespaceObjectDef(NamespaceMember):
    """Object definition in namespace"""
    object_def: 'ObjectDef' = None

@dataclass
class NamespaceStructDef(NamespaceMember):
    """Struct definition in namespace"""
    struct_def: 'StructDef' = None

@dataclass
class NamespaceNamespaceDef(NamespaceMember):
    """Nested namespace definition"""
    namespace_def: 'NamespaceDef' = None

@dataclass
class NamespaceVariableDecl(NamespaceMember):
    """Variable declaration in namespace"""
    declaration: 'VariableDeclaration' = None

@dataclass
class NamespaceDef(GlobalItem):
    """Namespace definition"""
    name: str = ""
    members: List[NamespaceMember] = field(default_factory=list)

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
    names: List['QualifiedName'] = field(default_factory=list)

# ============================================================================
# External FFI
# ============================================================================

@dataclass
class ExternDecl(ASTNode):
    """External function declaration"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    return_type: 'Type' = None

@dataclass
class ExternBlock(GlobalItem):
    """External function block"""
    language: str = ""
    declarations: List[ExternDecl] = field(default_factory=list)

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
    value: Optional['Expression'] = None

# ============================================================================
# Type System
# ============================================================================

@dataclass
class Type(ASTNode):
    """Base class for all types"""
    pass

class TypeQualifier(Enum):
    VOLATILE = "volatile"
    CONST = "const"

class PrimitiveTypeKind(Enum):
    INT = "int"
    FLOAT = "float"
    BOOL = "bool"
    CHAR = "char"
    VOID = "void"

@dataclass
class PrimitiveType(Type):
    """Primitive type (int, float, bool, char, void)"""
    kind: PrimitiveTypeKind = PrimitiveTypeKind.VOID
    qualifiers: List[TypeQualifier] = field(default_factory=list)

class DataSignedness(Enum):
    SIGNED = "signed"
    UNSIGNED = "unsigned"

@dataclass
class DataType(Type):
    """Data type with bit width and optional alignment"""
    bit_width: int = 0
    alignment: Optional[int] = None
    signedness: Optional[DataSignedness] = None
    qualifiers: List[TypeQualifier] = field(default_factory=list)

@dataclass
class NamedType(Type):
    """Named type (user-defined types, templates)"""
    name: 'QualifiedName' = None
    template_args: List[Union['Type', 'Expression']] = field(default_factory=list)
    qualifiers: List[TypeQualifier] = field(default_factory=list)

@dataclass
class PointerType(Type):
    """Pointer type"""
    pointee_type: 'Type' = None
    qualifiers: List[TypeQualifier] = field(default_factory=list)

@dataclass
class ArrayType(Type):
    """Array type"""
    element_type: 'Type' = None
    qualifiers: List[TypeQualifier] = field(default_factory=list)

@dataclass
class FunctionPointerType(Type):
    """Function pointer type"""
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    return_type: 'Type' = None
    template_params: List[TemplateParameter] = field(default_factory=list)
    qualifiers: List[TypeQualifier] = field(default_factory=list)

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
class ObjectInstantiationDeclarator(VariableDeclarator):
    """Object instantiation declarator"""
    arguments: List['Expression'] = field(default_factory=list)

@dataclass
class VariableDeclaration(ASTNode):
    """Variable declaration"""
    type: 'Type' = None
    declarators: List[VariableDeclarator] = field(default_factory=list)

@dataclass
class FunctionPointerDeclaration(ASTNode):
    """Function pointer declaration"""
    return_type: 'Type' = None
    name: str = ""
    parameters: List[Parameter] = field(default_factory=list)
    template_params: List[TemplateParameter] = field(default_factory=list)
    initializer: Optional['Expression'] = None
    is_array: bool = False
    array_initializer: Optional['ArrayInitializer'] = None

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
class CompoundStmt(Statement):
    """Compound statement (block)"""
    statements: List['Statement'] = field(default_factory=list)

@dataclass
class IfStmt(Statement):
    """If statement"""
    condition: 'Expression' = None
    then_stmt: 'Statement' = None
    elif_parts: List[tuple['Expression', 'Statement']] = field(default_factory=list)
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
class MatchCase(ASTNode):
    """Match case with pattern"""
    pattern: 'Pattern' = None
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
class MatchStmt(Statement):
    """Match statement"""
    expression: 'Expression' = None
    cases: List[MatchCase] = field(default_factory=list)
    default_case: Optional[DefaultCase] = None

@dataclass
class CatchClause(ASTNode):
    """Catch clause for try-catch"""
    type: 'Type' = None
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
class FunctionPointerDeclStmt(Statement):
    """Function pointer declaration statement"""
    declaration: 'FunctionPointerDeclaration' = None

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

@dataclass
class SimplePattern(Pattern):
    """Simple expression pattern"""
    expression: 'Expression' = None

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
    LOGICAL_AND = "&&"
    LOGICAL_OR = "||"
    LOGICAL_NAND = "!&"
    LOGICAL_NOR = "!|"
    AND_KEYWORD = "and"
    OR_KEYWORD = "or"
    
    # Bitwise
    BITWISE_AND = "&"
    BITWISE_OR = "|"
    BITWISE_XOR = "^^"
    XOR_KEYWORD = "xor"
    
    # Bitwise with backtick prefix
    BITWISE_B_AND = "`&"
    BITWISE_B_NAND = "`!&"
    BITWISE_B_OR = "`|"
    BITWISE_B_NOR = "`!|"
    BITWISE_B_XOR = "`^^"
    BITWISE_B_XNOR = "`^!|"
    BITWISE_B_XAND = "`^&"
    BITWISE_B_XNAND = "`^!&"
    
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
    BITWISE_NOT = "~"
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
    B_AND_ASSIGN = "`&="
    B_NAND_ASSIGN = "`!&="
    B_OR_ASSIGN = "`|="
    B_NOR_ASSIGN = "`!|="
    B_XOR_ASSIGN = "`^="
    B_NOT_ASSIGN = "`!="

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
    type: 'Type' = None
    expression: 'Expression' = None

@dataclass
class FunctionCall(Expression):
    """Function call expression"""
    function: 'Expression' = None
    arguments: List['Expression'] = field(default_factory=list)

@dataclass
class FunctionPointerCall(Expression):
    """Function pointer call (*ptr)(args)"""
    pointer: 'Expression' = None
    arguments: List['Expression'] = field(default_factory=list)

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

# Array literals and comprehensions
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
    """Array initializer [1, 2, 3] or {x = 1, y = 2}"""
    elements: List['Expression'] = field(default_factory=list)
    struct_items: List[StructInitItem] = field(default_factory=list)
    is_struct_style: bool = False

# Object instantiation
@dataclass
class ObjectInstantiation(Expression):
    """Object instantiation (Type name(args) or super.Type name(args))"""
    type_name: Union['QualifiedName', str] = ""
    instance_name: str = ""
    arguments: List['Expression'] = field(default_factory=list)
    is_super: bool = False
    is_virtual: bool = False

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

def create_function_def(name: str, return_type: 'Type' = None) -> FunctionDef:
    """Create a function definition node"""
    return FunctionDef(name=name, return_type=return_type or PrimitiveType(PrimitiveTypeKind.VOID))

def create_object_def(name: str) -> ObjectDef:
    """Create an object definition node"""
    return ObjectDef(name=name)

def create_struct_def(name: str) -> StructDef:
    """Create a struct definition node"""
    return StructDef(name=name)

def create_primitive_type(kind: PrimitiveTypeKind, qualifiers: List[TypeQualifier] = None) -> PrimitiveType:
    """Create a primitive type node"""
    return PrimitiveType(kind=kind, qualifiers=qualifiers or [])

def create_data_type(bit_width: int, alignment: int = None, signedness: DataSignedness = None) -> DataType:
    """Create a data type node"""
    return DataType(bit_width=bit_width, alignment=alignment, signedness=signedness)

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
    import sys
    
    def main():
        print("Flux AST Module - fast.py")
        print("Representing test.fx as AST structure")
        print("=" * 50)
        
        # Create the main program
        program = create_program()
        
        # 1. Import statements
        import1 = ImportStmt(path="standard.fx", alias="std")
        import2 = ImportStmt(path="math.fx")
        program.global_items.extend([import1, import2])
        
        # 2. Using statement
        using_stmt = UsingStmt(names=[
            create_qualified_name(["std", "io"]),
            create_qualified_name(["std", "types"])
        ])
        program.global_items.append(using_stmt)
        
        # 3. Data type definitions
        i32_type = DataType(bit_width=32, signedness=DataSignedness.SIGNED)
        i32_decl = VariableDeclaration(
            type=i32_type,
            declarators=[SimpleVariableDeclarator(name="i32")]
        )
        program.global_items.append(i32_decl)
        
        ui16_aligned_type = DataType(bit_width=16, alignment=32, signedness=DataSignedness.UNSIGNED)
        ui16_decl = VariableDeclaration(
            type=ui16_aligned_type,
            declarators=[SimpleVariableDeclarator(name="ui16_aligned")]
        )
        program.global_items.append(ui16_decl)
        
        # String type (unsigned data{8}[])
        string_type = ArrayType(
            element_type=DataType(bit_width=8, signedness=DataSignedness.UNSIGNED)
        )
        string_decl = VariableDeclaration(
            type=string_type,
            declarators=[SimpleVariableDeclarator(name="string")]
        )
        program.global_items.append(string_decl)
        
        # 4. Macro definitions
        debug_macro = MacroDef(name="DEBUG_MODE", value=BooleanLiteral(value=True))
        max_size_macro = MacroDef(name="MAX_SIZE", value=IntegerLiteral(value="1024"))
        program.global_items.extend([debug_macro, max_size_macro])
        
        # 5. External FFI block
        extern_block = ExternBlock(
            language="C",
            declarations=[
                ExternDecl(
                    name="malloc",
                    parameters=[Parameter(name="size", type=NamedType(name=create_qualified_name(["ui64"])))],
                    return_type=PointerType(pointee_type=PrimitiveType(PrimitiveTypeKind.VOID))
                ),
                ExternDecl(
                    name="free",
                    parameters=[Parameter(name="ptr", type=PointerType(pointee_type=PrimitiveType(PrimitiveTypeKind.VOID)))],
                    return_type=PrimitiveType(PrimitiveTypeKind.VOID)
                ),
                ExternDecl(
                    name="printf",
                    parameters=[Parameter(name="format", type=NamedType(name=create_qualified_name(["string"])))],
                    return_type=PrimitiveType(PrimitiveTypeKind.INT)
                )
            ]
        )
        program.global_items.append(extern_block)
        
        # 6. Namespace definition
        test_namespace = NamespaceDef(
            name="TestNamespace",
            members=[
                NamespaceVariableDecl(
                    declaration=VariableDeclaration(
                        type=PrimitiveType(
                            PrimitiveTypeKind.INT,
                            qualifiers=[TypeQualifier.VOLATILE, TypeQualifier.CONST]
                        ),
                        declarators=[SimpleVariableDeclarator(
                            name="global_counter",
                            initializer=IntegerLiteral(value="42")
                        )]
                    )
                ),
                NamespaceFunctionDef(
                    function=FunctionDef(
                        name="utility_func",
                        parameters=[
                            Parameter(name="x", type=PrimitiveType(PrimitiveTypeKind.INT)),
                            Parameter(name="y", type=PrimitiveType(PrimitiveTypeKind.FLOAT))
                        ],
                        return_type=PrimitiveType(PrimitiveTypeKind.BOOL),
                        body=[
                            ReturnStmt(
                                value=BinaryExpression(
                                    left=Identifier(name="x"),
                                    operator=BinaryOperator.GREATER_THAN,
                                    right=CastExpression(
                                        type=PrimitiveType(PrimitiveTypeKind.INT),
                                        expression=Identifier(name="y")
                                    )
                                )
                            )
                        ]
                    )
                ),
                NamespaceNamespaceDef(
                    namespace_def=NamespaceDef(
                        name="NestedNamespace",
                        members=[
                            NamespaceVariableDecl(
                                declaration=VariableDeclaration(
                                    type=NamedType(name=create_qualified_name(["string"])),
                                    declarators=[SimpleVariableDeclarator(
                                        name="nested_message",
                                        initializer=StringLiteral(value="Hello from nested namespace")
                                    )]
                                )
                            )
                        ]
                    )
                )
            ]
        )
        program.global_items.append(test_namespace)
        
        # 7. Base object definition
        base_object = ObjectDef(
            name="BaseObject",
            members=[
                ObjectVariableDecl(
                    access=AccessSpecifier.PRIVATE,
                    declaration=VariableDeclaration(
                        type=PrimitiveType(PrimitiveTypeKind.INT),
                        declarators=[SimpleVariableDeclarator(name="base_value")]
                    )
                ),
                ObjectFunctionDef(
                    access=AccessSpecifier.PUBLIC,
                    function=FunctionDef(
                        name="__init",
                        parameters=[Parameter(name="val", type=PrimitiveType(PrimitiveTypeKind.INT))],
                        return_type=NamedType(name=create_qualified_name(["this"])),
                        body=[
                            ExpressionStmt(
                                expression=AssignmentExpression(
                                    left=MemberAccess(object=ThisExpression(), member="base_value"),
                                    operator=AssignmentOperator.ASSIGN,
                                    right=Identifier(name="val")
                                )
                            ),
                            ReturnStmt(value=ThisExpression())
                        ]
                    )
                ),
                ObjectFunctionDef(
                    access=AccessSpecifier.PUBLIC,
                    function=FunctionDef(
                        name="__exit",
                        parameters=[],
                        return_type=PrimitiveType(PrimitiveTypeKind.VOID),
                        body=[
                            ReturnStmt(value=Identifier(name="void"))
                        ]
                    )
                )
            ]
        )
        program.global_items.append(base_object)
        
        # 8. Template object with inheritance
        template_object = ObjectDef(
            name="TemplateObject",
            template_params=[
                TemplateParameter(name="T"),
                TemplateParameter(name="K")
            ],
            inheritance=[create_qualified_name(["BaseObject"])],
            members=[
                ObjectVariableDecl(
                    access=AccessSpecifier.PRIVATE,
                    declaration=VariableDeclaration(
                        type=NamedType(name=create_qualified_name(["T"])),
                        declarators=[SimpleVariableDeclarator(name="template_data")]
                    )
                ),
                ObjectVariableDecl(
                    access=AccessSpecifier.PRIVATE,
                    declaration=VariableDeclaration(
                        type=NamedType(name=create_qualified_name(["K"])),
                        declarators=[SimpleVariableDeclarator(name="secondary_data")]
                    )
                )
            ]
        )
        program.global_items.append(template_object)
        
        # 9. Struct definitions
        point_struct = StructDef(
            name="Point",
            members=[
                StructMember(
                    access=AccessSpecifier.PUBLIC,
                    declaration=VariableDeclaration(
                        type=PrimitiveType(PrimitiveTypeKind.FLOAT),
                        declarators=[
                            SimpleVariableDeclarator(name="x"),
                            SimpleVariableDeclarator(name="y"),
                            SimpleVariableDeclarator(name="z")
                        ]
                    )
                ),
                StructMember(
                    access=AccessSpecifier.PRIVATE,
                    declaration=VariableDeclaration(
                        type=PrimitiveType(PrimitiveTypeKind.BOOL),
                        declarators=[SimpleVariableDeclarator(name="is_valid")]
                    )
                )
            ]
        )
        program.global_items.append(point_struct)
        
        vector3d_struct = StructDef(
            name="Vector3D",
            inheritance=[create_qualified_name(["Point"])],
            members=[
                StructMember(
                    declaration=VariableDeclaration(
                        type=PrimitiveType(PrimitiveTypeKind.FLOAT),
                        declarators=[SimpleVariableDeclarator(name="magnitude")]
                    )
                ),
                StructMember(
                    declaration=VariableDeclaration(
                        type=DataType(bit_width=1, signedness=DataSignedness.UNSIGNED),
                        declarators=[SimpleVariableDeclarator(name="normalized")]
                    )
                )
            ]
        )
        program.global_items.append(vector3d_struct)
        
        # 10. Compile-time block
        compt_block = ComptBlock(
            statements=[
                IfStmt(
                    condition=FunctionCall(
                        function=Identifier(name="def"),
                        arguments=[Identifier(name="DEBUG_MODE")]
                    ),
                    then_stmt=CompoundStmt(
                        statements=[
                            ExpressionStmt(
                                expression=MacroDef(name="LOG_LEVEL", value=IntegerLiteral(value="3"))
                            )
                        ]
                    ),
                    elif_parts=[
                        (
                            UnaryExpression(
                                operator=UnaryOperator.LOGICAL_NOT,
                                operand=FunctionCall(
                                    function=Identifier(name="def"),
                                    arguments=[Identifier(name="DEBUG_MODE")]
                                )
                            ),
                            CompoundStmt(
                                statements=[
                                    ExpressionStmt(
                                        expression=MacroDef(name="LOG_LEVEL", value=IntegerLiteral(value="0"))
                                    )
                                ]
                            )
                        )
                    ]
                )
            ]
        )
        program.global_items.append(compt_block)
        
        # 11. Main function with comprehensive features
        main_func = FunctionDef(
            name="main",
            parameters=[],
            return_type=PrimitiveType(PrimitiveTypeKind.INT),
            body=[
                # Variable declaration
                VariableDeclStmt(
                    declaration=VariableDeclaration(
                        type=NamedType(name=create_qualified_name(["string"])),
                        declarators=[SimpleVariableDeclarator(
                            name="test_msg",
                            initializer=StringLiteral(value="Testing all features")
                        )]
                    )
                ),
                
                # Array literal
                VariableDeclStmt(
                    declaration=VariableDeclaration(
                        type=ArrayType(element_type=PrimitiveType(PrimitiveTypeKind.INT)),
                        declarators=[ArrayVariableDeclarator(
                            name="numbers",
                            initializer=ArrayInitializer(elements=[
                                IntegerLiteral(value="1"),
                                IntegerLiteral(value="2"),
                                IntegerLiteral(value="3"),
                                IntegerLiteral(value="4"),
                                IntegerLiteral(value="5")
                            ])
                        )]
                    )
                ),
                
                # For loop
                CStyleForStmt(
                    init=VariableDeclaration(
                        type=PrimitiveType(PrimitiveTypeKind.INT),
                        declarators=[SimpleVariableDeclarator(
                            name="i",
                            initializer=IntegerLiteral(value="0")
                        )]
                    ),
                    condition=BinaryExpression(
                        left=Identifier(name="i"),
                        operator=BinaryOperator.LESS_THAN,
                        right=Identifier(name="MAX_SIZE")
                    ),
                    update=UnaryExpression(
                        operator=UnaryOperator.POST_INCREMENT,
                        operand=Identifier(name="i"),
                        is_postfix=True
                    ),
                    body=CompoundStmt(
                        statements=[
                            IfStmt(
                                condition=BinaryExpression(
                                    left=BinaryExpression(
                                        left=Identifier(name="i"),
                                        operator=BinaryOperator.MODULO,
                                        right=IntegerLiteral(value="2")
                                    ),
                                    operator=BinaryOperator.EQUAL,
                                    right=IntegerLiteral(value="0")
                                ),
                                then_stmt=CompoundStmt(statements=[ContinueStmt()])
                            ),
                            IfStmt(
                                condition=BinaryExpression(
                                    left=Identifier(name="i"),
                                    operator=BinaryOperator.GREATER_THAN,
                                    right=IntegerLiteral(value="100")
                                ),
                                then_stmt=CompoundStmt(statements=[BreakStmt()])
                            )
                        ]
                    )
                ),
                
                # Python-style for loop
                PythonStyleForStmt(
                    variables=["val"],
                    iterable=RangeExpression(
                        start=IntegerLiteral(value="1"),
                        end=IntegerLiteral(value="10")
                    ),
                    body=CompoundStmt(statements=[])
                ),
                
                # Try-catch block
                TryCatchStmt(
                    try_body=CompoundStmt(
                        statements=[
                            AssertStmt(
                                condition=BinaryExpression(
                                    left=Identifier(name="i"),
                                    operator=BinaryOperator.GREATER_THAN,
                                    right=IntegerLiteral(value="0")
                                ),
                                message=StringLiteral(value="Value must be positive")
                            )
                        ]
                    ),
                    catch_clauses=[
                        CatchClause(
                            type=NamedType(name=create_qualified_name(["string"])),
                            variable="error_msg",
                            body=CompoundStmt(
                                statements=[ReturnStmt(value=UnaryExpression(
                                    operator=UnaryOperator.MINUS,
                                    operand=IntegerLiteral(value="1")
                                ))]
                            )
                        )
                    ]
                ),
                
                # Return statement
                ReturnStmt(value=IntegerLiteral(value="0"))
            ]
        )
        program.global_items.append(main_func)
        
        # Validate the AST
        validator = ASTValidator()
        errors = validator.validate(program)
        
        if errors:
            print("AST Validation Errors:")
            for error in errors:
                print(f"  - {error}")
        else:
            print("AST validation passed!")
            print(f"Program has {len(program.global_items)} global items")
            
            # Count different types of items
            item_counts = {}
            for item in program.global_items:
                item_type = type(item).__name__
                item_counts[item_type] = item_counts.get(item_type, 0) + 1
            
            print("Global item breakdown:")
            for item_type, count in sorted(item_counts.items()):
                print(f"  {item_type}: {count}")
    
    main()