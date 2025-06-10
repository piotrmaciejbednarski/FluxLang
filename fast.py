#!/usr/bin/env python3
"""
Flux Language Abstract Syntax Tree (fast.py)
Comprehensive AST node definitions for the Flux programming language.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import List, Optional, Union, Any
from enum import Enum


class ASTNode(ABC):
    """Base class for all AST nodes."""
    
    @abstractmethod
    def accept(self, visitor):
        """Accept a visitor for the visitor pattern."""
        pass


class ASTVisitor(ABC):
    """Base visitor interface for AST traversal."""
    pass


# ===============================================================================
# PROGRAM STRUCTURE
# ===============================================================================

@dataclass
class Program(ASTNode):
    """Root node representing an entire Flux program."""
    global_items: List['GlobalItem']
    main_function: 'MainFunction'
    
    def accept(self, visitor):
        return visitor.visit_program(self)


@dataclass
class MainFunction(ASTNode):
    """Main function definition."""
    statements: List['Statement']
    
    def accept(self, visitor):
        return visitor.visit_main_function(self)


# ===============================================================================
# GLOBAL ITEMS
# ===============================================================================

class GlobalItem(ASTNode):
    """Base class for global-level items."""
    pass


@dataclass
class ImportStatement(GlobalItem):
    """Import statement: import "file.fx" as alias; or import "file.fx";"""
    module_path: str
    alias: Optional[str] = None
    
    def accept(self, visitor):
        return visitor.visit_import_statement(self)


@dataclass
class UsingStatement(GlobalItem):
    """Using statement: using namespace::path, other::path;"""
    namespace_paths: List['NamespacePath']
    
    def accept(self, visitor):
        return visitor.visit_using_statement(self)


@dataclass
class NamespacePath(ASTNode):
    """Namespace path: namespace::member::submember"""
    components: List[str]
    
    def accept(self, visitor):
        return visitor.visit_namespace_path(self)


# ===============================================================================
# TYPE SPECIFIERS
# ===============================================================================

class TypeSpecifier(ASTNode):
    """Base class for type specifiers."""
    pass


@dataclass
class BasicType(TypeSpecifier):
    """Basic types: int, float, char, bool, string, void, auto, this"""
    type_name: str
    
    def accept(self, visitor):
        return visitor.visit_basic_type(self)


@dataclass
class DataType(TypeSpecifier):
    """Data type: [signed|unsigned] data{width}"""
    width: int
    signed: Optional[bool] = None  # None = unspecified, True = signed, False = unsigned
    
    def accept(self, visitor):
        return visitor.visit_data_type(self)


@dataclass
class ArrayType(TypeSpecifier):
    """Array type: type[]"""
    element_type: TypeSpecifier
    
    def accept(self, visitor):
        return visitor.visit_array_type(self)


@dataclass
class PointerType(TypeSpecifier):
    """Pointer type: type* or type[]*"""
    pointed_type: TypeSpecifier
    
    def accept(self, visitor):
        return visitor.visit_pointer_type(self)


@dataclass
class FunctionPointerType(TypeSpecifier):
    """Function pointer type: return_type (*name)(param_types)"""
    return_type: TypeSpecifier
    parameter_types: List[TypeSpecifier]
    name: str
    
    def accept(self, visitor):
        return visitor.visit_function_pointer_type(self)


@dataclass
class TemplatePointerType(TypeSpecifier):
    """Template pointer type: template* name"""
    name: str
    
    def accept(self, visitor):
        return visitor.visit_template_pointer_type(self)


@dataclass
class IdentifierType(TypeSpecifier):
    """User-defined type identified by name."""
    name: str
    
    def accept(self, visitor):
        return visitor.visit_identifier_type(self)


# ===============================================================================
# FUNCTION DEFINITIONS
# ===============================================================================

@dataclass
class Parameter(ASTNode):
    """Function parameter: type name"""
    type_spec: TypeSpecifier
    name: str
    
    def accept(self, visitor):
        return visitor.visit_parameter(self)


@dataclass
class FunctionDefinition(GlobalItem):
    """Function definition: [volatile] def name(params) -> return_type { statements }"""
    name: str
    parameters: List[Parameter]
    return_type: TypeSpecifier
    statements: List['Statement']
    is_volatile: bool = False
    
    def accept(self, visitor):
        return visitor.visit_function_definition(self)


# ===============================================================================
# TEMPLATE DEFINITIONS
# ===============================================================================

@dataclass
class TemplateDefinition(GlobalItem):
    """Template function definition: [volatile] template<T,U> name(params) -> return_type { statements }"""
    name: str
    template_parameters: List[str]
    parameters: List[Parameter]
    return_type: TypeSpecifier
    statements: List['Statement']
    is_volatile: bool = False
    
    def accept(self, visitor):
        return visitor.visit_template_definition(self)


# ===============================================================================
# OBJECT DEFINITIONS
# ===============================================================================

@dataclass
class ObjectDefinition(GlobalItem):
    """Object definition: object name : inheritance { members }"""
    name: str
    inheritance: List[str]  # List of inherited object names
    members: List['ObjectMember']
    
    def accept(self, visitor):
        return visitor.visit_object_definition(self)


class ObjectMember(ASTNode):
    """Base class for object members."""
    pass


@dataclass
class MagicMethodDefinition(ObjectMember):
    """Magic method definition: def __method(params) -> return_type { statements }"""
    method_name: str
    parameters: List[Parameter]
    return_type: TypeSpecifier
    statements: List['Statement']
    
    def accept(self, visitor):
        return visitor.visit_magic_method_definition(self)


# ===============================================================================
# STRUCT DEFINITIONS
# ===============================================================================

@dataclass
class StructDefinition(GlobalItem):
    """Struct definition: struct name { members }"""
    name: str
    members: List['VariableDeclaration']
    
    def accept(self, visitor):
        return visitor.visit_struct_definition(self)


# ===============================================================================
# NAMESPACE DEFINITIONS
# ===============================================================================

@dataclass
class NamespaceDefinition(GlobalItem):
    """Namespace definition: namespace name { members }"""
    name: str
    members: List['NamespaceMember']
    
    def accept(self, visitor):
        return visitor.visit_namespace_definition(self)


class NamespaceMember(ASTNode):
    """Base class for namespace members."""
    pass


# ===============================================================================
# VARIABLE DECLARATIONS
# ===============================================================================

@dataclass
class VariableDeclarator(ASTNode):
    """Variable declarator: name [= initializer | (args) | {fields}]"""
    name: str
    initializer: Optional['Initializer'] = None
    
    def accept(self, visitor):
        return visitor.visit_variable_declarator(self)


@dataclass
class VariableDeclaration(GlobalItem, ObjectMember, NamespaceMember):
    """Variable declaration: type declarator1, declarator2, ...;"""
    type_spec: TypeSpecifier
    declarators: List[VariableDeclarator]
    
    def accept(self, visitor):
        return visitor.visit_variable_declaration(self)


class Initializer(ASTNode):
    """Base class for initializers."""
    pass


@dataclass
class ExpressionInitializer(Initializer):
    """Expression initializer: = expression"""
    expression: 'Expression'
    
    def accept(self, visitor):
        return visitor.visit_expression_initializer(self)


@dataclass
class ConstructorInitializer(Initializer):
    """Constructor initializer: (args)"""
    arguments: List['Expression']
    
    def accept(self, visitor):
        return visitor.visit_constructor_initializer(self)


@dataclass
class FieldInitializer(ASTNode):
    """Field initializer: field = expression"""
    field_name: str
    expression: 'Expression'
    
    def accept(self, visitor):
        return visitor.visit_field_initializer(self)


@dataclass
class StructInitializer(Initializer):
    """Struct initializer: {field1 = expr1, field2 = expr2}"""
    field_initializers: List[FieldInitializer]
    
    def accept(self, visitor):
        return visitor.visit_struct_initializer(self)


# ===============================================================================
# STATEMENTS
# ===============================================================================

class Statement(ASTNode):
    """Base class for statements."""
    pass


@dataclass
class ExpressionStatement(Statement):
    """Expression statement: expression;"""
    expression: Optional['Expression']
    
    def accept(self, visitor):
        return visitor.visit_expression_statement(self)


@dataclass
class CompoundStatement(Statement):
    """Compound statement: { statements }"""
    statements: List[Statement]
    
    def accept(self, visitor):
        return visitor.visit_compound_statement(self)


@dataclass
class IfStatement(Statement):
    """If statement: if (condition) then_stmt [else else_stmt];"""
    condition: 'Expression'
    then_statement: Statement
    else_statement: Optional[Statement] = None
    
    def accept(self, visitor):
        return visitor.visit_if_statement(self)


@dataclass
class CaseClause(ASTNode):
    """Case clause: case (expression) { statements }"""
    expression: 'Expression'
    statements: List[Statement]
    
    def accept(self, visitor):
        return visitor.visit_case_clause(self)


@dataclass
class DefaultClause(ASTNode):
    """Default clause: default { statements }"""
    statements: List[Statement]
    
    def accept(self, visitor):
        return visitor.visit_default_clause(self)


@dataclass
class SwitchStatement(Statement):
    """Switch statement: switch (expression) { cases default };"""
    expression: 'Expression'
    cases: List[CaseClause]
    default_clause: Optional[DefaultClause] = None
    
    def accept(self, visitor):
        return visitor.visit_switch_statement(self)


@dataclass
class WhileStatement(Statement):
    """While statement: while (condition) body;"""
    condition: 'Expression'
    body: Statement
    
    def accept(self, visitor):
        return visitor.visit_while_statement(self)


@dataclass
class ForStatement(Statement):
    """For statement: for (variable in iterable) body;"""
    variable: str
    iterable: 'Expression'
    body: Statement
    
    def accept(self, visitor):
        return visitor.visit_for_statement(self)


@dataclass
class CatchParameter(ASTNode):
    """Catch parameter: type name or auto name"""
    type_spec: TypeSpecifier
    name: str
    
    def accept(self, visitor):
        return visitor.visit_catch_parameter(self)


@dataclass
class CatchClause(ASTNode):
    """Catch clause: catch (parameter) { statements }"""
    parameter: CatchParameter
    statements: List[Statement]
    
    def accept(self, visitor):
        return visitor.visit_catch_clause(self)


@dataclass
class TryStatement(Statement):
    """Try statement: try { statements } catch_clauses"""
    statements: List[Statement]
    catch_clauses: List[CatchClause]
    
    def accept(self, visitor):
        return visitor.visit_try_statement(self)


@dataclass
class ReturnStatement(Statement):
    """Return statement: return [expression];"""
    expression: Optional['Expression'] = None
    
    def accept(self, visitor):
        return visitor.visit_return_statement(self)


@dataclass
class ThrowStatement(Statement):
    """Throw statement: throw(expression);"""
    expression: 'Expression'
    
    def accept(self, visitor):
        return visitor.visit_throw_statement(self)


@dataclass
class BreakStatement(Statement):
    """Break statement: break;"""
    
    def accept(self, visitor):
        return visitor.visit_break_statement(self)


@dataclass
class ContinueStatement(Statement):
    """Continue statement: continue;"""
    
    def accept(self, visitor):
        return visitor.visit_continue_statement(self)


@dataclass
class AssemblyOperand(ASTNode):
    """Assembly operand: identifier, integer, or register"""
    value: str
    
    def accept(self, visitor):
        return visitor.visit_assembly_operand(self)


@dataclass
class AssemblyInstruction(ASTNode):
    """Assembly instruction: mnemonic [operands]"""
    mnemonic: str
    operands: List[AssemblyOperand]
    
    def accept(self, visitor):
        return visitor.visit_assembly_instruction(self)


@dataclass
class AssemblyBlock(Statement):
    """Assembly block: asm { instructions }"""
    instructions: List[AssemblyInstruction]
    
    def accept(self, visitor):
        return visitor.visit_assembly_block(self)


# ===============================================================================
# EXPRESSIONS
# ===============================================================================

class Expression(ASTNode):
    """Base class for expressions."""
    pass


@dataclass
class ConditionalExpression(Expression):
    """Conditional expression: condition ? true_expr : false_expr"""
    condition: Expression
    true_expression: Expression
    false_expression: Expression
    
    def accept(self, visitor):
        return visitor.visit_conditional_expression(self)


@dataclass
class BinaryExpression(Expression):
    """Binary expression: left operator right"""
    left: Expression
    operator: str
    right: Expression
    
    def accept(self, visitor):
        return visitor.visit_binary_expression(self)


@dataclass
class UnaryExpression(Expression):
    """Unary expression: operator operand"""
    operator: str
    operand: Expression
    
    def accept(self, visitor):
        return visitor.visit_unary_expression(self)


@dataclass
class CastExpression(Expression):
    """Cast expression: (type) expression"""
    target_type: TypeSpecifier
    expression: Expression
    
    def accept(self, visitor):
        return visitor.visit_cast_expression(self)


@dataclass
class PostfixExpression(Expression):
    """Base for postfix expressions."""
    expression: Expression


@dataclass
class ArrayAccessExpression(PostfixExpression):
    """Array access: expression[index]"""
    index: Expression
    
    def accept(self, visitor):
        return visitor.visit_array_access_expression(self)


@dataclass
class FunctionCallExpression(PostfixExpression):
    """Function call: expression(arguments)"""
    arguments: List[Expression]
    
    def accept(self, visitor):
        return visitor.visit_function_call_expression(self)


@dataclass
class MemberAccessExpression(PostfixExpression):
    """Member access: expression.member"""
    member: str
    
    def accept(self, visitor):
        return visitor.visit_member_access_expression(self)


@dataclass
class ScopeAccessExpression(PostfixExpression):
    """Scope access: expression::member"""
    member: str
    
    def accept(self, visitor):
        return visitor.visit_scope_access_expression(self)


@dataclass
class PostIncrementExpression(PostfixExpression):
    """Post-increment: expression++"""
    
    def accept(self, visitor):
        return visitor.visit_post_increment_expression(self)


@dataclass
class PostDecrementExpression(PostfixExpression):
    """Post-decrement: expression--"""
    
    def accept(self, visitor):
        return visitor.visit_post_decrement_expression(self)


@dataclass
class TemplateInstantiationExpression(PostfixExpression):
    """Template instantiation: expression<types>"""
    template_arguments: List[TypeSpecifier]
    
    def accept(self, visitor):
        return visitor.visit_template_instantiation_expression(self)


@dataclass
class AssignmentExpression(Expression):
    """Assignment expression: left operator right"""
    left: Expression
    operator: str
    right: Expression
    
    def accept(self, visitor):
        return visitor.visit_assignment_expression(self)


# ===============================================================================
# PRIMARY EXPRESSIONS
# ===============================================================================

@dataclass
class IdentifierExpression(Expression):
    """Identifier expression: name"""
    name: str
    
    def accept(self, visitor):
        return visitor.visit_identifier_expression(self)


@dataclass
class LiteralExpression(Expression):
    """Base class for literal expressions."""
    value: Any


@dataclass
class IntegerLiteralExpression(LiteralExpression):
    """Integer literal: 123"""
    value: int
    
    def accept(self, visitor):
        return visitor.visit_integer_literal_expression(self)


@dataclass
class FloatLiteralExpression(LiteralExpression):
    """Float literal: 3.14"""
    value: float
    
    def accept(self, visitor):
        return visitor.visit_float_literal_expression(self)


@dataclass
class CharLiteralExpression(LiteralExpression):
    """Character literal: "A" """
    value: str  # Single character
    
    def accept(self, visitor):
        return visitor.visit_char_literal_expression(self)


@dataclass
class StringLiteralExpression(LiteralExpression):
    """String literal: "Hello World" """
    value: str
    
    def accept(self, visitor):
        return visitor.visit_string_literal_expression(self)


@dataclass
class BinaryLiteralExpression(LiteralExpression):
    """Binary literal: 1010b"""
    value: str  # Binary string with 'b' suffix
    
    def accept(self, visitor):
        return visitor.visit_binary_literal_expression(self)


@dataclass
class BooleanLiteralExpression(LiteralExpression):
    """Boolean literal: true or false"""
    value: bool
    
    def accept(self, visitor):
        return visitor.visit_boolean_literal_expression(self)


@dataclass
class IStringExpression(Expression):
    """Interpolated string: i"template":{expressions}"""
    template: str
    expressions: List[Expression]
    
    def accept(self, visitor):
        return visitor.visit_i_string_expression(self)


@dataclass
class ArrayLiteralExpression(Expression):
    """Array literal: [expr1, expr2, ...]"""
    elements: List[Expression]
    
    def accept(self, visitor):
        return visitor.visit_array_literal_expression(self)


@dataclass
class ArrayComprehensionExpression(Expression):
    """Array comprehension: [expr for (var in iterable) if (condition)]"""
    expression: Expression
    variable: str
    iterable: Expression
    condition: Optional[Expression] = None
    
    def accept(self, visitor):
        return visitor.visit_array_comprehension_expression(self)


@dataclass
class DestructuringAssignmentExpression(Expression):
    """Destructuring assignment: auto {vars} = object{fields}"""
    variables: List[str]
    object_name: str
    fields: List[str]
    
    def accept(self, visitor):
        return visitor.visit_destructuring_assignment_expression(self)


@dataclass
class ParenthesizedExpression(Expression):
    """Parenthesized expression: (expression)"""
    expression: Expression
    
    def accept(self, visitor):
        return visitor.visit_parenthesized_expression(self)


@dataclass
class ThisExpression(Expression):
    """This expression: this"""
    
    def accept(self, visitor):
        return visitor.visit_this_expression(self)


@dataclass
class SuperExpression(Expression):
    """Super expression: super or super.member"""
    member: Optional[str] = None
    
    def accept(self, visitor):
        return visitor.visit_super_expression(self)


# ===============================================================================
# UTILITY CLASSES
# ===============================================================================

class OperatorType(Enum):
    """Types of operators for precedence and associativity."""
    
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
    LOGICAL_NOT = "!"
    LOGICAL_NAND = "!&"
    LOGICAL_NOR = "!|"
    
    # Bitwise
    BITWISE_AND = "&"
    BITWISE_OR = "|"
    BITWISE_XOR = "^^"
    BITWISE_NOT = "~"
    
    # Special Bitwise (with backtick)
    BBIT_AND = "`&&"
    BBIT_NAND = "`!&"
    BBIT_OR = "`|"
    BBIT_NOR = "`!|"
    BBIT_XOR = "`^^"
    BBIT_XNOR = "`^!|"
    BBIT_XAND = "`^&"
    BBIT_XNAND = "`^!&"
    
    # Shift
    LEFT_SHIFT = "<<"
    RIGHT_SHIFT = ">>"
    
    # Assignment
    ASSIGN = "="
    PLUS_ASSIGN = "+="
    MINUS_ASSIGN = "-="
    MUL_ASSIGN = "*="
    DIV_ASSIGN = "/="
    MOD_ASSIGN = "%="
    POW_ASSIGN = "^="
    AND_ASSIGN = "&="
    OR_ASSIGN = "|="
    XOR_ASSIGN = "^^="
    LSHIFT_ASSIGN = "<<="
    RSHIFT_ASSIGN = ">>="
    
    # Memory
    ADDRESS_OF = "@"
    DEREFERENCE = "*"
    
    # Increment/Decrement
    INCREMENT = "++"
    DECREMENT = "--"
    
    # Other
    IN = "in"
    IS = "is"
    NOT = "not"
    AS = "as"
    XOR = "xor"
    AND = "and"
    OR = "or"


@dataclass
class OperatorInfo:
    """Information about an operator."""
    precedence: int
    associativity: str  # "left", "right"
    is_binary: bool


# Operator precedence table as specified in the language specification
OPERATOR_PRECEDENCE = {
    # Level 1 - Primary expressions (handled separately)
    
    # Level 2 - Postfix increment/decrement
    "++": OperatorInfo(2, "left", False),
    "--": OperatorInfo(2, "left", False),
    
    # Level 3 - Unary operators
    "+": OperatorInfo(3, "right", False),
    "-": OperatorInfo(3, "right", False),
    "!": OperatorInfo(3, "right", False),
    "~": OperatorInfo(3, "right", False),
    "@": OperatorInfo(3, "right", False),
    "*": OperatorInfo(3, "right", False),
    "not": OperatorInfo(3, "right", False),
    "sizeof": OperatorInfo(3, "right", False),
    "typeof": OperatorInfo(3, "right", False),
    
    # Level 4 - Type casting
    # (handled specially)
    
    # Level 5 - Exponentiation
    "^": OperatorInfo(5, "right", True),
    
    # Level 6 - Multiplicative
    # "*": OperatorInfo(6, "left", True),  # Conflict with unary, handled in parser
    "/": OperatorInfo(6, "left", True),
    "%": OperatorInfo(6, "left", True),
    
    # Level 7 - Additive
    # "+": OperatorInfo(7, "left", True),  # Conflict with unary, handled in parser
    # "-": OperatorInfo(7, "left", True),  # Conflict with unary, handled in parser
    
    # Level 8 - Bit shift
    "<<": OperatorInfo(8, "left", True),
    ">>": OperatorInfo(8, "left", True),
    
    # Level 9 - Relational
    "<": OperatorInfo(9, "left", True),
    "<=": OperatorInfo(9, "left", True),
    ">": OperatorInfo(9, "left", True),
    ">=": OperatorInfo(9, "left", True),
    "in": OperatorInfo(9, "left", True),
    
    # Level 10 - Equality
    "==": OperatorInfo(10, "left", True),
    "!=": OperatorInfo(10, "left", True),
    
    # Level 11 - Identity
    "is": OperatorInfo(11, "left", True),
    "not": OperatorInfo(11, "left", True),
    "as": OperatorInfo(11, "left", True),
    
    # Level 12 - Bitwise AND variants
    "&": OperatorInfo(12, "left", True),
    "`&&": OperatorInfo(12, "left", True),
    "`!&": OperatorInfo(12, "left", True),
    "`^&": OperatorInfo(12, "left", True),
    "`^!&": OperatorInfo(12, "left", True),
    
    # Level 13 - Bitwise XOR variants
    "^^": OperatorInfo(13, "left", True),
    "xor": OperatorInfo(13, "left", True),
    "`^^": OperatorInfo(13, "left", True),
    "`^!|": OperatorInfo(13, "left", True),
    
    # Level 14 - Bitwise OR variants
    "|": OperatorInfo(14, "left", True),
    "`|": OperatorInfo(14, "left", True),
    "`!|": OperatorInfo(14, "left", True),
    
    # Level 15 - Logical AND/NAND
    "and": OperatorInfo(15, "left", True),
    "&&": OperatorInfo(15, "left", True),
    "!&": OperatorInfo(15, "left", True),
    
    # Level 16 - Logical OR/NOR
    "or": OperatorInfo(16, "left", True),
    "||": OperatorInfo(16, "left", True),
    "!|": OperatorInfo(16, "left", True),
    
    # Level 17 - Conditional
    "?:": OperatorInfo(17, "right", True),
    
    # Level 18 - Assignment
    "=": OperatorInfo(18, "right", True),
    "+=": OperatorInfo(18, "right", True),
    "-=": OperatorInfo(18, "right", True),
    "*=": OperatorInfo(18, "right", True),
    "/=": OperatorInfo(18, "right", True),
    "%=": OperatorInfo(18, "right", True),
    "^=": OperatorInfo(18, "right", True),
    "&=": OperatorInfo(18, "right", True),
    "|=": OperatorInfo(18, "right", True),
    "^^=": OperatorInfo(18, "right", True),
    "<<=": OperatorInfo(18, "right", True),
    ">>=": OperatorInfo(18, "right", True),
    "`&&=": OperatorInfo(18, "right", True),
    "`!&=": OperatorInfo(18, "right", True),
    "`|=": OperatorInfo(18, "right", True),
    "`!|=": OperatorInfo(18, "right", True),
    "`^=": OperatorInfo(18, "right", True),
    "`!=": OperatorInfo(18, "right", True),
    
    # Level 19 - Comma
    ",": OperatorInfo(19, "left", True),
}


# ===============================================================================
# AST UTILITY FUNCTIONS
# ===============================================================================

def get_operator_precedence(operator: str) -> int:
    """Get the precedence of an operator."""
    info = OPERATOR_PRECEDENCE.get(operator)
    return info.precedence if info else 0


def get_operator_associativity(operator: str) -> str:
    """Get the associativity of an operator."""
    info = OPERATOR_PRECEDENCE.get(operator)
    return info.associativity if info else "left"


def is_binary_operator(operator: str) -> bool:
    """Check if an operator is binary."""
    info = OPERATOR_PRECEDENCE.get(operator)
    return info.is_binary if info else False


def is_assignment_operator(operator: str) -> bool:
    """Check if an operator is an assignment operator."""
    assignment_ops = {
        "=", "+=", "-=", "*=", "/=", "%=", "^=", "&=", "|=", "^^=",
        "<<=", ">>=", "`&&=", "`!&=", "`|=", "`!|=", "`^=", "`!="
    }
    return operator in assignment_ops


def is_logical_operator(operator: str) -> bool:
    """Check if an operator is a logical operator."""
    logical_ops = {"&&", "||", "!&", "!|", "and", "or", "!"}
    return operator in logical_ops


def is_bitwise_operator(operator: str) -> bool:
    """Check if an operator is a bitwise operator."""
    bitwise_ops = {
        "&", "|", "^^", "~", "<<", ">>",
        "`&&", "`!&", "`|", "`!|", "`^^", "`^!|", "`^&", "`^!&"
    }
    return operator in bitwise_ops


def is_comparison_operator(operator: str) -> bool:
    """Check if an operator is a comparison operator."""
    comparison_ops = {"==", "!=", "<", "<=", ">", ">=", "is", "not", "as", "in"}
    return operator in comparison_ops


# ===============================================================================
# AST PRINTER (for debugging)
# ===============================================================================

class ASTPrinter(ASTVisitor):
    """Visitor to print AST structure for debugging."""
    
    def __init__(self, indent_size: int = 2):
        self.indent_size = indent_size
        self.current_indent = 0
    
    def _indent(self) -> str:
        return " " * (self.current_indent * self.indent_size)
    
    def _print_line(self, text: str):
        print(f"{self._indent()}{text}")
    
    def _visit_children(self, node: ASTNode, children: List[Any]):
        self.current_indent += 1
        for child in children:
            if isinstance(child, ASTNode):
                child.accept(self)
            elif isinstance(child, list):
                for item in child:
                    if isinstance(item, ASTNode):
                        item.accept(self)
            elif child is not None:
                self._print_line(f"{type(child).__name__}: {child}")
        self.current_indent -= 1
    
    def visit_program(self, node: Program):
        self._print_line("Program")
        self._visit_children(node, [node.global_items, node.main_function])
    
    def visit_main_function(self, node: MainFunction):
        self._print_line("MainFunction")
        self._visit_children(node, [node.statements])
    
    # Add visitor methods for all node types as needed...
    # This is a basic framework - full implementation would include all visit methods


# ===============================================================================
# MODULE EXPORTS
# ===============================================================================

__all__ = [
    # Base classes
    'ASTNode', 'ASTVisitor',
    
    # Program structure
    'Program', 'MainFunction', 'GlobalItem',
    
    # Imports and using
    'ImportStatement', 'UsingStatement', 'NamespacePath',
    
    # Type specifiers
    'TypeSpecifier', 'BasicType', 'DataType', 'ArrayType', 'PointerType',
    'FunctionPointerType', 'TemplatePointerType', 'IdentifierType',
    
    # Function definitions
    'Parameter', 'FunctionDefinition', 'TemplateDefinition',
    
    # Object definitions
    'ObjectDefinition', 'ObjectMember', 'MagicMethodDefinition',
    
    # Struct definitions
    'StructDefinition',
    
    # Namespace definitions
    'NamespaceDefinition', 'NamespaceMember',
    
    # Variable declarations
    'VariableDeclaration', 'VariableDeclarator',
    'Initializer', 'ExpressionInitializer', 'ConstructorInitializer',
    'FieldInitializer', 'StructInitializer',
    
    # Statements
    'Statement', 'ExpressionStatement', 'CompoundStatement', 'IfStatement',
    'CaseClause', 'DefaultClause', 'SwitchStatement', 'WhileStatement',
    'ForStatement', 'CatchParameter', 'CatchClause', 'TryStatement',
    'ReturnStatement', 'ThrowStatement', 'BreakStatement', 'ContinueStatement',
    'AssemblyOperand', 'AssemblyInstruction', 'AssemblyBlock',
    
    # Expressions
    'Expression', 'ConditionalExpression', 'BinaryExpression', 'UnaryExpression',
    'CastExpression', 'PostfixExpression', 'ArrayAccessExpression',
    'FunctionCallExpression', 'MemberAccessExpression', 'ScopeAccessExpression',
    'PostIncrementExpression', 'PostDecrementExpression',
    'TemplateInstantiationExpression', 'AssignmentExpression',
    
    # Primary expressions
    'IdentifierExpression', 'LiteralExpression', 'IntegerLiteralExpression',
    'FloatLiteralExpression', 'CharLiteralExpression', 'StringLiteralExpression',
    'BinaryLiteralExpression', 'BooleanLiteralExpression', 'IStringExpression',
    'ArrayLiteralExpression', 'ArrayComprehensionExpression',
    'DestructuringAssignmentExpression', 'ParenthesizedExpression',
    'ThisExpression', 'SuperExpression',
    
    # Utility classes
    'OperatorType', 'OperatorInfo', 'ASTPrinter',
    
    # Utility functions
    'get_operator_precedence', 'get_operator_associativity', 'is_binary_operator',
    'is_assignment_operator', 'is_logical_operator', 'is_bitwise_operator',
    'is_comparison_operator',
    
    # Constants
    'OPERATOR_PRECEDENCE',
]