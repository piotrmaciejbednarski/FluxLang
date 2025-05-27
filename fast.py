from abc import ABC, abstractmethod
from typing import List, Optional, Union, Any
from dataclasses import dataclass
from enum import Enum
from flexer import Token, TokenType

# Base AST Node Classes
class ASTNode(ABC):
    """Base class for all AST nodes"""
    pass

class Statement(ASTNode):
    """Base class for all statement nodes"""
    pass

class Expression(ASTNode):
    """Base class for all expression nodes"""
    pass

class Declaration(ASTNode):
    """Base class for all declaration nodes"""
    pass

class TypeSpecifier(ASTNode):
    """Base class for all type specifier nodes"""
    pass

class Pattern(ASTNode):
    """Base class for all pattern nodes"""
    pass

# Program Structure
@dataclass
class Program(ASTNode):
    """Root node representing the entire program"""
    translation_unit: 'TranslationUnit'

@dataclass
class TranslationUnit(ASTNode):
    """Top-level translation unit containing external declarations"""
    external_declarations: List['ExternalDeclaration']

# External Declarations
class ExternalDeclaration(Declaration):
    """Base class for top-level declarations"""
    pass

# Import and Using Statements
@dataclass
class ImportStatement(ExternalDeclaration):
    """import "filename" as identifier;"""
    filename: str
    alias: str
    token: Token

@dataclass
class UsingStatement(ExternalDeclaration):
    """using namespace::member, other::member;"""
    qualified_ids: List['QualifiedId']
    token: Token

@dataclass
class QualifiedId(ASTNode):
    """namespace::member style identifier"""
    identifiers: List[str]
    tokens: List[Token]

# Template System
@dataclass
class TemplatePrefix(ASTNode):
    """template <T, U, V> prefix"""
    parameters: List['TemplateParameter']
    token: Token

@dataclass
class TemplateParameter(ASTNode):
    """Template parameter with optional constraint"""
    name: str
    constraint: Optional[TypeSpecifier]
    token: Token

@dataclass
class TemplateArgument(ASTNode):
    """Template argument (type or expression)"""
    value: Union[TypeSpecifier, Expression]

# Visibility and Storage
class VisibilitySpecifier(Enum):
    PUBLIC = "public"
    PRIVATE = "private"

class StorageClass(Enum):
    CONST = "const"
    VOLATILE = "volatile"
    STATIC = "static"

# Function Definitions
@dataclass
class FunctionDefinition(ExternalDeclaration):
    """def name(params) -> return_type { body };"""
    visibility: Optional[VisibilitySpecifier]
    template_prefix: Optional[TemplatePrefix]
    name: str
    parameters: List['Parameter']
    return_type: 'ReturnType'
    body: 'CompoundStatement'
    token: Token

@dataclass
class Parameter(ASTNode):
    """Function parameter with optional default value"""
    type_spec: TypeSpecifier
    name: str
    default_value: Optional[Expression]
    token: Token

@dataclass
class ReturnType(ASTNode):
    """Return type, optionally with ! for never returns"""
    type_spec: TypeSpecifier
    never_returns: bool  # true if prefixed with !
    token: Token

# Object Definitions
@dataclass
class ObjectDefinition(ExternalDeclaration):
    """object Name : ParentClass { members };"""
    visibility: Optional[VisibilitySpecifier]
    template_prefix: Optional[TemplatePrefix]
    name: str
    inheritance: Optional['InheritanceClause']
    members: List['ObjectMember']
    token: Token

@dataclass
class InheritanceClause(ASTNode):
    """Inheritance specification"""
    specifiers: List['InheritanceSpecifier']

@dataclass
class InheritanceSpecifier(ASTNode):
    """Parent class with optional exclusion"""
    qualified_id: QualifiedId
    excluded: bool  # true if prefixed with !

class ObjectMember(ASTNode):
    """Base class for object members"""
    pass

# Struct Definitions
@dataclass
class StructDefinition(ExternalDeclaration):
    """struct Name { members };"""
    visibility: Optional[VisibilitySpecifier]
    template_prefix: Optional[TemplatePrefix]
    name: str
    members: List['StructMember']
    token: Token

class StructMember(ASTNode):
    """Base class for struct members"""
    pass

# Namespace Definitions
@dataclass
class NamespaceDefinition(ExternalDeclaration):
    """namespace Name { members };"""
    name: str
    members: List['NamespaceMember']
    token: Token

class NamespaceMember(ASTNode):
    """Base class for namespace members"""
    pass

# Enum Definitions
@dataclass
class EnumDefinition(ExternalDeclaration):
    """enum Name { members };"""
    visibility: Optional[VisibilitySpecifier]
    name: str
    template_prefix: Optional[TemplatePrefix]
    members: List['EnumMember']
    token: Token

@dataclass
class EnumMember(ASTNode):
    """Enum member with optional associated types"""
    name: str
    associated_types: Optional[List[TypeSpecifier]]
    token: Token

# Module Definitions
@dataclass
class ModuleDefinition(ExternalDeclaration):
    """module name::path { members };"""
    visibility: Optional[VisibilitySpecifier]
    qualified_id: QualifiedId
    members: List['ModuleMember']
    token: Token

class ModuleMember(ASTNode):
    """Base class for module members"""
    pass

# Operator Definitions
@dataclass
class OperatorDefinition(ExternalDeclaration):
    """operator(params)[symbol] precedence(...) -> type { body };"""
    parameters: List[Parameter]
    symbol: str
    precedence: Optional['PrecedenceClause']
    return_type: TypeSpecifier
    body: 'CompoundStatement'
    token: Token

@dataclass
class PrecedenceClause(ASTNode):
    """precedence(level) clause"""
    level: Union[str, int]  # identifier or integer literal
    token: Token

# Variable Declarations
@dataclass
class VariableDeclaration(Declaration):
    """type name = value, name2 = value2;"""
    visibility: Optional[VisibilitySpecifier]
    storage_class: Optional[StorageClass]
    type_spec: TypeSpecifier
    declarators: List['Declarator']
    token: Token

@dataclass
class Declarator(ASTNode):
    """Variable declarator with optional initializer"""
    name: str
    array_size: Optional[Expression]  # for array declarators
    is_pointer: bool
    function_params: Optional[List[Parameter]]  # for function pointer declarators
    initializer: Optional['Initializer']
    token: Token

@dataclass
class Initializer(ASTNode):
    """Variable initializer"""
    value: Union[Expression, 'BracedInitList']

@dataclass
class BracedInitList(ASTNode):
    """{ value1, value2, ... } initializer"""
    values: List[Union[Expression, 'BracedInitList']]
    token: Token

# Type Specifiers
@dataclass
class SimpleTypeSpecifier(TypeSpecifier):
    """Basic type like i32, void, auto, or identifier"""
    name: Union[str, QualifiedId]
    token: Token

@dataclass
class TemplateTypeSpecifier(TypeSpecifier):
    """Template instantiation like Vector<i32>"""
    name: str
    arguments: List[TemplateArgument]
    token: Token

@dataclass
class PointerTypeSpecifier(TypeSpecifier):
    """Pointer type like i32* or function pointer"""
    base_type: TypeSpecifier
    cv_qualifiers: List[str]  # const, volatile
    function_params: Optional[List[Parameter]]  # for function pointers
    token: Token

@dataclass
class ArrayTypeSpecifier(TypeSpecifier):
    """Array type like i32[10] or i32[]"""
    element_type: TypeSpecifier
    size: Optional[Expression]
    token: Token

@dataclass
class DataTypeSpecifier(TypeSpecifier):
    """data{n} type specification"""
    signedness: Optional[str]  # "signed" or "unsigned"
    size: Expression  # bit width expression
    token: Token

# Statements
@dataclass
class ExpressionStatement(Statement):
    """Expression used as statement"""
    expression: Optional[Expression]
    token: Token

@dataclass
class CompoundStatement(Statement):
    """{ statements }"""
    statements: List[Statement]
    token: Token

# Selection Statements
@dataclass
class IfStatement(Statement):
    """if (condition) { body } else { else_body }"""
    condition: Expression
    then_body: CompoundStatement
    else_body: Optional[CompoundStatement]
    token: Token

@dataclass
class SwitchStatement(Statement):
    """switch (expression) { cases }"""
    expression: Expression
    cases: List['SwitchCase']
    token: Token

@dataclass
class SwitchCase(ASTNode):
    """case(value) or case(default) with body"""
    label: Union[Expression, str]  # expression or "default"
    body: CompoundStatement
    token: Token

@dataclass
class MatchStatement(Statement):
    """match expression { arms }"""
    expression: Expression
    arms: List['MatchArm']
    token: Token

@dataclass
class MatchArm(ASTNode):
    """case pattern if guard => expression"""
    pattern: Pattern
    guard: Optional[Expression]
    expression: Expression
    token: Token

# Iteration Statements
@dataclass
class WhileStatement(Statement):
    """while (condition) { body }"""
    condition: Expression
    body: CompoundStatement
    token: Token

@dataclass
class DoStatement(Statement):
    """do { body } while (condition)"""
    body: CompoundStatement
    condition: Expression
    token: Token

@dataclass
class ForStatement(Statement):
    """for (init; condition; increment) { body } or for (var in iterable) { body }"""
    init: Optional[Union[VariableDeclaration, Expression]]
    condition: Optional[Expression]
    increment: Optional[Expression]
    iterator_var: Optional[Union[str, 'DestructuringPattern']]  # for range-based
    iterable: Optional[Expression]  # for range-based
    body: CompoundStatement
    token: Token

# Jump Statements
@dataclass
class ReturnStatement(Statement):
    """return expression;"""
    expression: Optional[Expression]
    token: Token

@dataclass
class BreakStatement(Statement):
    """break;"""
    token: Token

@dataclass
class ContinueStatement(Statement):
    """continue;"""
    token: Token

# Exception Handling
@dataclass
class TryStatement(Statement):
    """try { body } catch (Type var) { handler } finally { cleanup }"""
    body: CompoundStatement
    catch_clauses: List['CatchClause']
    finally_clause: Optional['FinallyClause']
    token: Token

@dataclass
class CatchClause(ASTNode):
    """catch (Type variable) { body }"""
    exception_type: TypeSpecifier
    variable: Optional[str]
    body: CompoundStatement
    token: Token

@dataclass
class FinallyClause(ASTNode):
    """finally { body }"""
    body: CompoundStatement
    token: Token

@dataclass
class ThrowStatement(Statement):
    """throw expression;"""
    expression: Optional[Expression]
    token: Token

# Other Statements
@dataclass
class AssertStatement(Statement):
    """assert(condition) or assert(condition, message)"""
    condition: Expression
    message: Optional[Expression]
    token: Token

@dataclass
class DeferStatement(Statement):
    """defer expression;"""
    expression: Expression
    token: Token

@dataclass
class SpawnStatement(Statement):
    """spawn expression;"""
    expression: Expression
    token: Token

@dataclass
class AwaitStatement(Statement):
    """await expression;"""
    expression: Expression
    token: Token

@dataclass
class InlineAssembly(Statement):
    """asm { assembly code };"""
    instructions: List[str]
    token: Token

# Expressions
@dataclass
class AssignmentExpression(Expression):
    """left op= right"""
    left: Expression
    operator: str  # =, +=, -=, etc.
    right: Expression
    token: Token

@dataclass
class ConditionalExpression(Expression):
    """condition ? true_expr : false_expr"""
    condition: Expression
    true_expr: Expression
    false_expr: Expression
    token: Token

@dataclass
class BinaryExpression(Expression):
    """left op right"""
    left: Expression
    operator: str
    right: Expression
    token: Token

@dataclass
class UnaryExpression(Expression):
    """op operand"""
    operator: str
    operand: Expression
    prefix: bool  # true for prefix, false for postfix
    token: Token

@dataclass
class CastExpression(Expression):
    """(type)expression"""
    type_spec: TypeSpecifier
    expression: Expression
    token: Token

@dataclass
class CallExpression(Expression):
    """function(arguments)"""
    function: Expression
    arguments: List[Expression]
    token: Token

@dataclass
class MemberExpression(Expression):
    """object.member or object::member"""
    object: Expression
    member: str
    scope_operator: bool  # true for ::, false for .
    token: Token

@dataclass
class IndexExpression(Expression):
    """array[index]"""
    array: Expression
    index: Expression
    token: Token

@dataclass
class IteratorMethodExpression(Expression):
    """_.next() or _.setpos(pos)"""
    method: str  # "next" or "setpos"
    argument: Optional[Expression]  # for setpos
    token: Token

@dataclass
class CustomOperatorExpression(Expression):
    """Custom user-defined operator"""
    left: Expression
    operator: str
    right: Expression
    token: Token

@dataclass
class RangeExpression(Expression):
    """start..end or start..=end"""
    start: Expression
    end: Expression
    inclusive: bool  # true for ..=, false for ..
    token: Token

@dataclass
class SuperExpression(Expression):
    """super.member or super::member"""
    member: str
    scope_operator: bool  # true for ::, false for .
    token: Token

@dataclass
class ThisExpression(Expression):
    """this or this.member"""
    member: Optional[str]
    token: Token

@dataclass
class IteratorReference(Expression):
    """_ (iterator reference)"""
    token: Token

# Primary Expressions
@dataclass
class Identifier(Expression):
    """Variable or function name"""
    name: str
    token: Token

@dataclass
class ParenthesizedExpression(Expression):
    """(expression)"""
    expression: Expression
    token: Token

# Literals
@dataclass
class IntegerLiteral(Expression):
    """Integer literal like 42, 0x1A, 0b1010"""
    value: str  # Keep as string to preserve format
    token: Token

@dataclass
class FloatingLiteral(Expression):
    """Floating point literal like 3.14, 1e-5"""
    value: str
    token: Token

@dataclass
class StringLiteral(Expression):
    """String literal "text" """
    value: str
    token: Token

@dataclass
class BooleanLiteral(Expression):
    """true or false literal"""
    value: bool
    token: Token

@dataclass
class ArrayLiteral(Expression):
    """[element1, element2, ...]"""
    elements: List[Expression]
    token: Token

@dataclass
class DictionaryLiteral(Expression):
    """{key1: value1, key2: value2, ...}"""
    pairs: List['KeyValuePair']
    token: Token

@dataclass
class KeyValuePair(ASTNode):
    """key: value pair in dictionary"""
    key: Expression
    value: Expression
    token: Token

@dataclass
class BinaryLiteral(Expression):
    """{0,1,0,1} binary literal"""
    bits: str  # string of 0s and 1s
    token: Token

@dataclass
class InterpolatedString(Expression):
    """i"text {}":{expr1;expr2;}"""
    template: str
    expressions: List[Expression]
    token: Token

# Array Comprehension
@dataclass
class ArrayComprehension(Expression):
    """[expr for var in iterable if condition]"""
    expression: Expression
    variable: str
    iterable: Expression
    conditions: List['ComprehensionCondition']
    token: Token

@dataclass
class ComprehensionCondition(ASTNode):
    """if condition or for var in iterable"""
    is_filter: bool  # true for if, false for for
    condition: Optional[Expression]  # for if conditions
    variable: Optional[str]  # for nested for
    iterable: Optional[Expression]  # for nested for
    token: Token

@dataclass
class AnonymousBlock(Expression):
    """{statements; expression}"""
    statements: List[Statement]
    expression: Optional[Expression]
    token: Token

# Patterns (for match statements and destructuring)
@dataclass
class VariablePattern(Pattern):
    """Variable binding pattern"""
    name: str
    token: Token

@dataclass
class LiteralPattern(Pattern):
    """Literal matching pattern"""
    literal: Expression
    token: Token

@dataclass
class ConstructorPattern(Pattern):
    """Constructor pattern like Some(x)"""
    name: str
    patterns: List[Pattern]
    token: Token

@dataclass
class WildcardPattern(Pattern):
    """_ wildcard pattern"""
    token: Token

@dataclass
class DestructuringPattern(Pattern):
    """Destructuring pattern for objects/structs/arrays"""
    fields: List['FieldPattern']
    elements: List[Pattern]  # for array destructuring
    is_object: bool  # true for {}, false for []
    token: Token

@dataclass
class FieldPattern(ASTNode):
    """Field in destructuring pattern"""
    name: str
    pattern: Optional[Pattern]  # None for shorthand
    token: Token

# Sizeof and Typeof expressions
@dataclass
class SizeofExpression(Expression):
    """sizeof(expr) or sizeof(type)"""
    operand: Union[Expression, TypeSpecifier]
    token: Token

@dataclass
class TypeofExpression(Expression):
    """typeof(expr) or typeof(type)"""
    operand: Union[Expression, TypeSpecifier]
    token: Token

# Template Declarations (wrapping other declarations)
@dataclass
class TemplateDeclaration(ExternalDeclaration):
    """template <params> declaration"""
    template_prefix: TemplatePrefix
    declaration: Union[FunctionDefinition, ObjectDefinition, StructDefinition]
    token: Token

# CV Qualifiers
class CVQualifier(Enum):
    CONST = "const"
    VOLATILE = "volatile"

# AST Visitor Pattern (for traversal and analysis)
class ASTVisitor(ABC):
    """Base visitor class for AST traversal"""
    
    @abstractmethod
    def visit(self, node: ASTNode) -> Any:
        """Visit an AST node"""
        pass
    
    def generic_visit(self, node: ASTNode) -> Any:
        """Default visit method that visits all child nodes"""
        for field_name, field_value in node.__dict__.items():
            if isinstance(field_value, ASTNode):
                self.visit(field_value)
            elif isinstance(field_value, list):
                for item in field_value:
                    if isinstance(item, ASTNode):
                        self.visit(item)

class ASTPrinter(ASTVisitor):
    """Example visitor that prints the AST structure"""
    
    def __init__(self):
        self.indent_level = 0
    
    def visit(self, node: ASTNode) -> str:
        """Visit and print an AST node"""
        indent = "  " * self.indent_level
        result = f"{indent}{node.__class__.__name__}"
        
        # Add relevant details for some node types
        if isinstance(node, Identifier):
            result += f"({node.name})"
        elif isinstance(node, IntegerLiteral):
            result += f"({node.value})"
        elif isinstance(node, StringLiteral):
            result += f"({repr(node.value)})"
        elif isinstance(node, BinaryExpression):
            result += f"({node.operator})"
        elif isinstance(node, UnaryExpression):
            result += f"({node.operator})"
        elif isinstance(node, FunctionDefinition):
            result += f"({node.name})"
        elif isinstance(node, ObjectDefinition):
            result += f"({node.name})"
        elif isinstance(node, StructDefinition):
            result += f"({node.name})"
        
        print(result)
        
        # Visit children
        self.indent_level += 1
        self.generic_visit(node)
        self.indent_level -= 1
        
        return result

# Utility functions for AST construction
def create_qualified_id(identifiers: List[str], tokens: List[Token]) -> QualifiedId:
    """Helper to create a qualified identifier"""
    return QualifiedId(identifiers=identifiers, tokens=tokens)

def create_simple_type(name: str, token: Token) -> SimpleTypeSpecifier:
    """Helper to create a simple type specifier"""
    return SimpleTypeSpecifier(name=name, token=token)

def create_binary_expr(left: Expression, op: str, right: Expression, token: Token) -> BinaryExpression:
    """Helper to create a binary expression"""
    return BinaryExpression(left=left, operator=op, right=right, token=token)

def create_identifier(name: str, token: Token) -> Identifier:
    """Helper to create an identifier"""
    return Identifier(name=name, token=token)

# AST node type checking utilities
def is_declaration(node: ASTNode) -> bool:
    """Check if node is a declaration"""
    return isinstance(node, Declaration)

def is_statement(node: ASTNode) -> bool:
    """Check if node is a statement"""
    return isinstance(node, Statement)

def is_expression(node: ASTNode) -> bool:
    """Check if node is an expression"""
    return isinstance(node, Expression)

def is_type_specifier(node: ASTNode) -> bool:
    """Check if node is a type specifier"""
    return isinstance(node, TypeSpecifier)

# AST validation utilities
class ASTValidator(ASTVisitor):
    """Validator for basic AST structure"""
    
    def __init__(self):
        self.errors: List[str] = []
    
    def visit(self, node: ASTNode) -> None:
        """Validate an AST node"""
        # Basic validation - can be extended
        if hasattr(node, 'token') and node.token is None:
            self.errors.append(f"Node {node.__class__.__name__} missing token information")
        
        self.generic_visit(node)
    
    def validate(self, ast: ASTNode) -> List[str]:
        """Validate the AST and return list of errors"""
        self.errors.clear()
        self.visit(ast)
        return self.errors

if __name__ == "__main__":
    # Example usage - create a simple AST manually
    from flexer import Token, TokenType
    
    # Create some dummy tokens for testing
    def_token = Token(TokenType.KEYWORD, "def", 1, 1, 0)
    main_token = Token(TokenType.IDENTIFIER, "main", 1, 5, 4)
    arrow_token = Token(TokenType.ARROW, "->", 1, 12, 11)
    int_token = Token(TokenType.IDENTIFIER, "i32", 1, 15, 14)
    
    # Create a simple function definition AST
    return_type = ReturnType(
        type_spec=SimpleTypeSpecifier(name="i32", token=int_token),
        never_returns=False,
        token=arrow_token
    )
    
    func_def = FunctionDefinition(
        visibility=None,
        template_prefix=None,
        name="main",
        parameters=[],
        return_type=return_type,
        body=CompoundStatement(statements=[], token=def_token),
        token=def_token
    )
    
    program = Program(
        translation_unit=TranslationUnit(
            external_declarations=[func_def]
        )
    )
    
    # Print the AST
    printer = ASTPrinter()
    print("Example AST structure:")
    printer.visit(program)
    
    # Validate the AST
    validator = ASTValidator()
    errors = validator.validate(program)
    if errors:
        print("\nValidation errors:")
        for error in errors:
            print(f"  - {error}")
    else:
        print("\nAST validation passed!")