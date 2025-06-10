"""
Flux AST (fast.py) - Abstract Syntax Tree for the Flux Programming Language

This module provides a comprehensive AST implementation that supports all Flux language
constructs and is designed for integration with the type checker and code generator.
"""

from abc import ABC, abstractmethod
from typing import List, Optional, Union, Any, Dict, Set
from enum import Enum
from dataclasses import dataclass, field


# ================================================================
# SOURCE LOCATION TRACKING
# ================================================================

@dataclass
class SourceLocation:
    """Tracks source code location for error reporting and debugging"""
    filename: str
    line: int
    column: int
    end_line: int = 0
    end_column: int = 0
    
    def __post_init__(self):
        if self.end_line == 0:
            self.end_line = self.line
        if self.end_column == 0:
            self.end_column = self.column


# ================================================================
# TYPE INFORMATION
# ================================================================

class TypeKind(Enum):
    """Categories of types in Flux"""
    PRIMITIVE = "primitive"
    CUSTOM_DATA = "custom_data"
    POINTER = "pointer"
    ARRAY = "array"
    FUNCTION = "function"
    OBJECT = "object"
    STRUCT = "struct"
    TEMPLATE = "template"
    TEMPLATE_PARAM = "template_param"
    VOID = "void"
    UNRESOLVED = "unresolved"


@dataclass
class TypeInfo:
    """Type information that gets populated by the type checker"""
    kind: TypeKind = TypeKind.UNRESOLVED
    name: str = ""
    bit_width: Optional[int] = None
    alignment: Optional[int] = None
    is_signed: Optional[bool] = None
    is_const: bool = False
    is_volatile: bool = False
    pointer_depth: int = 0
    array_size: Optional[int] = None
    template_args: List['TypeInfo'] = field(default_factory=list)
    symbol_ref: Optional['Symbol'] = None


# ================================================================
# SYMBOL TABLE INTEGRATION
# ================================================================

class SymbolKind(Enum):
    """Types of symbols in the symbol table"""
    VARIABLE = "variable"
    FUNCTION = "function"
    TYPE = "type"
    NAMESPACE = "namespace"
    TEMPLATE = "template"
    PARAMETER = "parameter"


@dataclass
class Symbol:
    """Symbol table entry"""
    name: str
    kind: SymbolKind
    type_info: Optional[TypeInfo] = None
    scope_level: int = 0
    is_global: bool = False
    is_template: bool = False
    ast_node: Optional['ASTNode'] = None


# ================================================================
# BASE AST NODE
# ================================================================

class ASTVisitor(ABC):
    """Abstract visitor for AST traversal"""
    
    @abstractmethod
    def visit(self, node: 'ASTNode') -> Any:
        pass


class ASTNode(ABC):
    """Base class for all AST nodes"""
    
    def __init__(self, location: SourceLocation):
        self.location = location
        self.type_info: Optional[TypeInfo] = None
        self.symbol_ref: Optional[Symbol] = None
        self.parent: Optional['ASTNode'] = None
        self.children: List['ASTNode'] = []
        self.metadata: Dict[str, Any] = {}
    
    @abstractmethod
    def accept(self, visitor: ASTVisitor) -> Any:
        """Accept a visitor for traversal"""
        pass
    
    def add_child(self, child: 'ASTNode') -> None:
        """Add a child node and set parent relationship"""
        if child:
            child.parent = self
            self.children.append(child)
    
    def remove_child(self, child: 'ASTNode') -> None:
        """Remove a child node"""
        if child in self.children:
            child.parent = None
            self.children.remove(child)
    
    def get_root(self) -> 'ASTNode':
        """Get the root node of the AST"""
        current = self
        while current.parent:
            current = current.parent
        return current


# ================================================================
# PROGRAM STRUCTURE NODES
# ================================================================

class ModuleNode(ASTNode):
    """Root node representing a complete Flux source file"""
    
    def __init__(self, location: SourceLocation, filename: str):
        super().__init__(location)
        self.filename = filename
        self.imports: List['ImportNode'] = []
        self.using_declarations: List['UsingNode'] = []
        self.declarations: List['DeclarationNode'] = []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_module(self)


class ImportNode(ASTNode):
    """Import declaration: import "file.fx" as alias"""
    
    def __init__(self, location: SourceLocation, module_path: str, alias: Optional[str] = None):
        super().__init__(location)
        self.module_path = module_path
        self.alias = alias
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_import(self)


class UsingNode(ASTNode):
    """Using declaration: using namespace::member"""
    
    def __init__(self, location: SourceLocation, namespace_path: List[str], members: List[str]):
        super().__init__(location)
        self.namespace_path = namespace_path
        self.members = members  # Empty list means using entire namespace
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_using(self)


class NamespaceNode(ASTNode):
    """Namespace definition with nested declarations"""
    
    def __init__(self, location: SourceLocation, name: str):
        super().__init__(location)
        self.name = name
        self.declarations: List['DeclarationNode'] = []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_namespace(self)


# ================================================================
# TYPE NODES
# ================================================================

class TypeNode(ASTNode):
    """Base class for type expressions"""
    pass


class PrimitiveTypeNode(TypeNode):
    """Primitive types: int, float, char, bool, void"""
    
    def __init__(self, location: SourceLocation, type_name: str):
        super().__init__(location)
        self.type_name = type_name
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_primitive_type(self)


class DataTypeNode(TypeNode):
    """Custom data type: [signed|unsigned] data{width:alignment}"""
    
    def __init__(self, location: SourceLocation, is_signed: bool, width_expr: 'ExpressionNode', 
                 alignment_expr: Optional['ExpressionNode'] = None):
        super().__init__(location)
        self.is_signed = is_signed
        self.width_expr = width_expr
        self.alignment_expr = alignment_expr
        self.add_child(width_expr)
        if alignment_expr:
            self.add_child(alignment_expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_data_type(self)


class PointerTypeNode(TypeNode):
    """Pointer type: type*"""
    
    def __init__(self, location: SourceLocation, pointee_type: TypeNode, is_const: bool = False, 
                 is_volatile: bool = False):
        super().__init__(location)
        self.pointee_type = pointee_type
        self.is_const = is_const
        self.is_volatile = is_volatile
        self.add_child(pointee_type)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_pointer_type(self)


class ArrayTypeNode(TypeNode):
    """Array type: type[] or type[size]"""
    
    def __init__(self, location: SourceLocation, element_type: TypeNode, 
                 size_expr: Optional['ExpressionNode'] = None):
        super().__init__(location)
        self.element_type = element_type
        self.size_expr = size_expr
        self.add_child(element_type)
        if size_expr:
            self.add_child(size_expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_array_type(self)


class FunctionTypeNode(TypeNode):
    """Function type: return_type (*)(param_types)"""
    
    def __init__(self, location: SourceLocation, return_type: TypeNode, 
                 parameter_types: List[TypeNode]):
        super().__init__(location)
        self.return_type = return_type
        self.parameter_types = parameter_types
        self.add_child(return_type)
        for param_type in parameter_types:
            self.add_child(param_type)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_function_type(self)


class TemplateTypeNode(TypeNode):
    """Template type instantiation: type<args>"""
    
    def __init__(self, location: SourceLocation, base_type: TypeNode, 
                 template_args: List[TypeNode]):
        super().__init__(location)
        self.base_type = base_type
        self.template_args = template_args
        self.add_child(base_type)
        for arg in template_args:
            self.add_child(arg)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_template_type(self)


class NamedTypeNode(TypeNode):
    """Named type reference: SomeType"""
    
    def __init__(self, location: SourceLocation, name: str, namespace_path: List[str] = None):
        super().__init__(location)
        self.name = name
        self.namespace_path = namespace_path or []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_named_type(self)


# ================================================================
# DECLARATION NODES
# ================================================================

class DeclarationNode(ASTNode):
    """Base class for all declarations"""
    pass


class VariableDeclarationNode(DeclarationNode):
    """Variable declaration: type name = initializer;"""
    
    def __init__(self, location: SourceLocation, var_type: TypeNode, name: str, 
                 initializer: Optional['ExpressionNode'] = None, is_const: bool = False, 
                 is_volatile: bool = False):
        super().__init__(location)
        self.var_type = var_type
        self.name = name
        self.initializer = initializer
        self.is_const = is_const
        self.is_volatile = is_volatile
        self.add_child(var_type)
        if initializer:
            self.add_child(initializer)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_variable_declaration(self)


class ParameterNode(ASTNode):
    """Function parameter"""
    
    def __init__(self, location: SourceLocation, param_type: TypeNode, name: str, 
                 default_value: Optional['ExpressionNode'] = None):
        super().__init__(location)
        self.param_type = param_type
        self.name = name
        self.default_value = default_value
        self.add_child(param_type)
        if default_value:
            self.add_child(default_value)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_parameter(self)


class FunctionDeclarationNode(DeclarationNode):
    """Function declaration: def name(params) -> return_type { body }"""
    
    def __init__(self, location: SourceLocation, name: str, parameters: List[ParameterNode], 
                 return_type: TypeNode, body: Optional['BlockStatementNode'] = None, 
                 is_template: bool = False, template_params: List[str] = None):
        super().__init__(location)
        self.name = name
        self.parameters = parameters
        self.return_type = return_type
        self.body = body
        self.is_template = is_template
        self.template_params = template_params or []
        self.is_magic_method = name.startswith('__')
        
        for param in parameters:
            self.add_child(param)
        self.add_child(return_type)
        if body:
            self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_function_declaration(self)


class ObjectDeclarationNode(DeclarationNode):
    """Object (class) declaration with inheritance"""
    
    def __init__(self, location: SourceLocation, name: str, base_classes: List[NamedTypeNode] = None,
                 is_template: bool = False, template_params: List[str] = None):
        super().__init__(location)
        self.name = name
        self.base_classes = base_classes or []
        self.is_template = is_template
        self.template_params = template_params or []
        self.members: List[DeclarationNode] = []
        
        for base in self.base_classes:
            self.add_child(base)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_object_declaration(self)


class StructDeclarationNode(DeclarationNode):
    """Struct declaration (POD type)"""
    
    def __init__(self, location: SourceLocation, name: str):
        super().__init__(location)
        self.name = name
        self.members: List[VariableDeclarationNode] = []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_struct_declaration(self)


class TemplateDeclarationNode(DeclarationNode):
    """Template declaration wrapper"""
    
    def __init__(self, location: SourceLocation, template_params: List[str], 
                 declaration: DeclarationNode):
        super().__init__(location)
        self.template_params = template_params
        self.declaration = declaration
        self.add_child(declaration)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_template_declaration(self)


# ================================================================
# STATEMENT NODES
# ================================================================

class StatementNode(ASTNode):
    """Base class for all statements"""
    pass


class ExpressionStatementNode(StatementNode):
    """Statement containing a single expression"""
    
    def __init__(self, location: SourceLocation, expression: 'ExpressionNode'):
        super().__init__(location)
        self.expression = expression
        self.add_child(expression)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_expression_statement(self)


class BlockStatementNode(StatementNode):
    """Compound statement with local scope"""
    
    def __init__(self, location: SourceLocation):
        super().__init__(location)
        self.statements: List[StatementNode] = []
    
    def add_statement(self, stmt: StatementNode):
        self.statements.append(stmt)
        self.add_child(stmt)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_block_statement(self)


class IfStatementNode(StatementNode):
    """If-else statement"""
    
    def __init__(self, location: SourceLocation, condition: 'ExpressionNode', 
                 then_stmt: StatementNode, else_stmt: Optional[StatementNode] = None):
        super().__init__(location)
        self.condition = condition
        self.then_stmt = then_stmt
        self.else_stmt = else_stmt
        self.add_child(condition)
        self.add_child(then_stmt)
        if else_stmt:
            self.add_child(else_stmt)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_if_statement(self)


class WhileStatementNode(StatementNode):
    """While loop"""
    
    def __init__(self, location: SourceLocation, condition: 'ExpressionNode', body: StatementNode):
        super().__init__(location)
        self.condition = condition
        self.body = body
        self.add_child(condition)
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_while_statement(self)


class DoWhileStatementNode(StatementNode):
    """Do-while loop"""
    
    def __init__(self, location: SourceLocation, body: StatementNode, condition: 'ExpressionNode'):
        super().__init__(location)
        self.body = body
        self.condition = condition
        self.add_child(body)
        self.add_child(condition)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_do_while_statement(self)


class ForStatementNode(StatementNode):
    """C-style for loop"""
    
    def __init__(self, location: SourceLocation, init: Optional[StatementNode], 
                 condition: Optional['ExpressionNode'], update: Optional['ExpressionNode'], 
                 body: StatementNode):
        super().__init__(location)
        self.init = init
        self.condition = condition
        self.update = update
        self.body = body
        if init:
            self.add_child(init)
        if condition:
            self.add_child(condition)
        if update:
            self.add_child(update)
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_for_statement(self)


class RangeForStatementNode(StatementNode):
    """Range-based for loop: for (x in y)"""
    
    def __init__(self, location: SourceLocation, variable: str, iterable: 'ExpressionNode', 
                 body: StatementNode):
        super().__init__(location)
        self.variable = variable
        self.iterable = iterable
        self.body = body
        self.add_child(iterable)
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_range_for_statement(self)


class ReturnStatementNode(StatementNode):
    """Return statement"""
    
    def __init__(self, location: SourceLocation, value: Optional['ExpressionNode'] = None):
        super().__init__(location)
        self.value = value
        if value:
            self.add_child(value)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_return_statement(self)


class BreakStatementNode(StatementNode):
    """Break statement"""
    
    def __init__(self, location: SourceLocation):
        super().__init__(location)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_break_statement(self)


class ContinueStatementNode(StatementNode):
    """Continue statement"""
    
    def __init__(self, location: SourceLocation):
        super().__init__(location)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_continue_statement(self)


class ThrowStatementNode(StatementNode):
    """Throw statement"""
    
    def __init__(self, location: SourceLocation, expression: 'ExpressionNode'):
        super().__init__(location)
        self.expression = expression
        self.add_child(expression)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_throw_statement(self)


class TryStatementNode(StatementNode):
    """Try-catch statement"""
    
    def __init__(self, location: SourceLocation, try_block: BlockStatementNode, 
                 catch_clauses: List['CatchClauseNode']):
        super().__init__(location)
        self.try_block = try_block
        self.catch_clauses = catch_clauses
        self.add_child(try_block)
        for clause in catch_clauses:
            self.add_child(clause)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_try_statement(self)


class CatchClauseNode(ASTNode):
    """Catch clause in try-catch"""
    
    def __init__(self, location: SourceLocation, exception_type: TypeNode, 
                 exception_name: str, body: BlockStatementNode):
        super().__init__(location)
        self.exception_type = exception_type
        self.exception_name = exception_name
        self.body = body
        self.add_child(exception_type)
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_catch_clause(self)


class SwitchStatementNode(StatementNode):
    """Switch statement"""
    
    def __init__(self, location: SourceLocation, expression: 'ExpressionNode', 
                 cases: List['CaseClauseNode'], default_case: Optional['DefaultClauseNode'] = None):
        super().__init__(location)
        self.expression = expression
        self.cases = cases
        self.default_case = default_case
        self.add_child(expression)
        for case in cases:
            self.add_child(case)
        if default_case:
            self.add_child(default_case)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_switch_statement(self)


class CaseClauseNode(ASTNode):
    """Case clause in switch statement"""
    
    def __init__(self, location: SourceLocation, value: 'ExpressionNode', body: BlockStatementNode):
        super().__init__(location)
        self.value = value
        self.body = body
        self.add_child(value)
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_case_clause(self)


class DefaultClauseNode(ASTNode):
    """Default clause in switch statement"""
    
    def __init__(self, location: SourceLocation, body: BlockStatementNode):
        super().__init__(location)
        self.body = body
        self.add_child(body)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_default_clause(self)


class AssemblyStatementNode(StatementNode):
    """Inline assembly block"""
    
    def __init__(self, location: SourceLocation, assembly_code: str):
        super().__init__(location)
        self.assembly_code = assembly_code
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_assembly_statement(self)


# ================================================================
# EXPRESSION NODES
# ================================================================

class ExpressionNode(ASTNode):
    """Base class for all expressions"""
    pass


class LiteralNode(ExpressionNode):
    """Base class for literal values"""
    pass


class IntegerLiteralNode(LiteralNode):
    """Integer literal"""
    
    def __init__(self, location: SourceLocation, value: int):
        super().__init__(location)
        self.value = value
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_integer_literal(self)


class FloatLiteralNode(LiteralNode):
    """Float literal"""
    
    def __init__(self, location: SourceLocation, value: float):
        super().__init__(location)
        self.value = value
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_float_literal(self)


class CharLiteralNode(LiteralNode):
    """Character literal"""
    
    def __init__(self, location: SourceLocation, value: str):
        super().__init__(location)
        self.value = value
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_char_literal(self)


class StringLiteralNode(LiteralNode):
    """String literal"""
    
    def __init__(self, location: SourceLocation, value: str):
        super().__init__(location)
        self.value = value
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_string_literal(self)


class BooleanLiteralNode(LiteralNode):
    """Boolean literal"""
    
    def __init__(self, location: SourceLocation, value: bool):
        super().__init__(location)
        self.value = value
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_boolean_literal(self)


class InterpolatedStringNode(ExpressionNode):
    """Interpolated string: i"text {expression}":{expression_list}"""
    
    def __init__(self, location: SourceLocation, template: str, expressions: List[ExpressionNode]):
        super().__init__(location)
        self.template = template
        self.expressions = expressions
        for expr in expressions:
            self.add_child(expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_interpolated_string(self)


class IdentifierNode(ExpressionNode):
    """Variable/function identifier"""
    
    def __init__(self, location: SourceLocation, name: str, namespace_path: List[str] = None):
        super().__init__(location)
        self.name = name
        self.namespace_path = namespace_path or []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_identifier(self)


class BinaryOperatorNode(ExpressionNode):
    """Binary operation"""
    
    def __init__(self, location: SourceLocation, operator: str, left: ExpressionNode, 
                 right: ExpressionNode):
        super().__init__(location)
        self.operator = operator
        self.left = left
        self.right = right
        self.add_child(left)
        self.add_child(right)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_binary_operator(self)


class UnaryOperatorNode(ExpressionNode):
    """Unary operation"""
    
    def __init__(self, location: SourceLocation, operator: str, operand: ExpressionNode, 
                 is_postfix: bool = False):
        super().__init__(location)
        self.operator = operator
        self.operand = operand
        self.is_postfix = is_postfix
        self.add_child(operand)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_unary_operator(self)


class AssignmentNode(ExpressionNode):
    """Assignment expression"""
    
    def __init__(self, location: SourceLocation, operator: str, left: ExpressionNode, 
                 right: ExpressionNode):
        super().__init__(location)
        self.operator = operator
        self.left = left
        self.right = right
        self.add_child(left)
        self.add_child(right)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_assignment(self)


class CallExpressionNode(ExpressionNode):
    """Function call or constructor call"""
    
    def __init__(self, location: SourceLocation, callee: ExpressionNode, 
                 arguments: List[ExpressionNode], template_args: List[TypeNode] = None):
        super().__init__(location)
        self.callee = callee
        self.arguments = arguments
        self.template_args = template_args or []
        self.add_child(callee)
        for arg in arguments:
            self.add_child(arg)
        for template_arg in self.template_args:
            self.add_child(template_arg)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_call_expression(self)


class MemberAccessNode(ExpressionNode):
    """Member access: object.member or object->member"""
    
    def __init__(self, location: SourceLocation, object_expr: ExpressionNode, member_name: str, 
                 is_pointer_access: bool = False):
        super().__init__(location)
        self.object_expr = object_expr
        self.member_name = member_name
        self.is_pointer_access = is_pointer_access
        self.add_child(object_expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_member_access(self)


class ArrayAccessNode(ExpressionNode):
    """Array subscript: array[index]"""
    
    def __init__(self, location: SourceLocation, array_expr: ExpressionNode, 
                 index_expr: ExpressionNode):
        super().__init__(location)
        self.array_expr = array_expr
        self.index_expr = index_expr
        self.add_child(array_expr)
        self.add_child(index_expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_array_access(self)


class CastExpressionNode(ExpressionNode):
    """Type cast: (type)expression"""
    
    def __init__(self, location: SourceLocation, target_type: TypeNode, expression: ExpressionNode):
        super().__init__(location)
        self.target_type = target_type
        self.expression = expression
        self.add_child(target_type)
        self.add_child(expression)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_cast_expression(self)


class ConditionalExpressionNode(ExpressionNode):
    """Ternary conditional: condition ? true_expr : false_expr"""
    
    def __init__(self, location: SourceLocation, condition: ExpressionNode, 
                 true_expr: ExpressionNode, false_expr: ExpressionNode):
        super().__init__(location)
        self.condition = condition
        self.true_expr = true_expr
        self.false_expr = false_expr
        self.add_child(condition)
        self.add_child(true_expr)
        self.add_child(false_expr)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_conditional_expression(self)


class SizeofExpressionNode(ExpressionNode):
    """Sizeof expression"""
    
    def __init__(self, location: SourceLocation, operand: Union[TypeNode, ExpressionNode]):
        super().__init__(location)
        self.operand = operand
        self.add_child(operand)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_sizeof_expression(self)


class AlignofExpressionNode(ExpressionNode):
    """Alignof expression"""
    
    def __init__(self, location: SourceLocation, operand: Union[TypeNode, ExpressionNode]):
        super().__init__(location)
        self.operand = operand
        self.add_child(operand)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_alignof_expression(self)


class TypeofExpressionNode(ExpressionNode):
    """Typeof expression"""
    
    def __init__(self, location: SourceLocation, operand: Union[TypeNode, ExpressionNode]):
        super().__init__(location)
        self.operand = operand
        self.add_child(operand)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_typeof_expression(self)


class ArrayComprehensionNode(ExpressionNode):
    """Array comprehension: [expr for (var in iterable) if (condition)]"""
    
    def __init__(self, location: SourceLocation, expression: ExpressionNode, variable: str, 
                 iterable: ExpressionNode, condition: Optional[ExpressionNode] = None):
        super().__init__(location)
        self.expression = expression
        self.variable = variable
        self.iterable = iterable
        self.condition = condition
        self.add_child(expression)
        self.add_child(iterable)
        if condition:
            self.add_child(condition)
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_array_comprehension(self)


# ================================================================
# ERROR RECOVERY NODES
# ================================================================

class ErrorNode(ASTNode):
    """Node representing a parsing error"""
    
    def __init__(self, location: SourceLocation, error_message: str, 
                 recovered_tokens: List[str] = None):
        super().__init__(location)
        self.error_message = error_message
        self.recovered_tokens = recovered_tokens or []
    
    def accept(self, visitor: ASTVisitor) -> Any:
        return visitor.visit_error(self)


# ================================================================
# VISITOR INTERFACE EXTENSIONS
# ================================================================

class ASTVisitor(ABC):
    """Extended visitor interface with all node types"""
    
    # Program structure
    def visit_module(self, node: ModuleNode) -> Any: pass
    def visit_import(self, node: ImportNode) -> Any: pass
    def visit_using(self, node: UsingNode) -> Any: pass
    def visit_namespace(self, node: NamespaceNode) -> Any: pass
    
    # Types
    def visit_primitive_type(self, node: PrimitiveTypeNode) -> Any: pass
    def visit_data_type(self, node: DataTypeNode) -> Any: pass
    def visit_pointer_type(self, node: PointerTypeNode) -> Any: pass
    def visit_array_type(self, node: ArrayTypeNode) -> Any: pass
    def visit_function_type(self, node: FunctionTypeNode) -> Any: pass
    def visit_template_type(self, node: TemplateTypeNode) -> Any: pass
    def visit_named_type(self, node: NamedTypeNode) -> Any: pass
    
    # Declarations
    def visit_variable_declaration(self, node: VariableDeclarationNode) -> Any: pass
    def visit_parameter(self, node: ParameterNode) -> Any: pass
    def visit_function_declaration(self, node: FunctionDeclarationNode) -> Any: pass
    def visit_object_declaration(self, node: ObjectDeclarationNode) -> Any: pass
    def visit_struct_declaration(self, node: StructDeclarationNode) -> Any: pass
    def visit_template_declaration(self, node: TemplateDeclarationNode) -> Any: pass
    
    # Statements
    def visit_expression_statement(self, node: ExpressionStatementNode) -> Any: pass
    def visit_block_statement(self, node: BlockStatementNode) -> Any: pass
    def visit_if_statement(self, node: IfStatementNode) -> Any: pass
    def visit_while_statement(self, node: WhileStatementNode) -> Any: pass
    def visit_do_while_statement(self, node: DoWhileStatementNode) -> Any: pass
    def visit_for_statement(self, node: ForStatementNode) -> Any: pass
    def visit_range_for_statement(self, node: RangeForStatementNode) -> Any: pass
    def visit_return_statement(self, node: ReturnStatementNode) -> Any: pass
    def visit_break_statement(self, node: BreakStatementNode) -> Any: pass
    def visit_continue_statement(self, node: ContinueStatementNode) -> Any: pass
    def visit_throw_statement(self, node: ThrowStatementNode) -> Any: pass
    def visit_try_statement(self, node: TryStatementNode) -> Any: pass
    def visit_catch_clause(self, node: CatchClauseNode) -> Any: pass
    def visit_switch_statement(self, node: SwitchStatementNode) -> Any: pass
    def visit_case_clause(self, node: CaseClauseNode) -> Any: pass
    def visit_default_clause(self, node: DefaultClauseNode) -> Any: pass
    def visit_assembly_statement(self, node: AssemblyStatementNode) -> Any: pass
    
    # Expressions
    def visit_integer_literal(self, node: IntegerLiteralNode) -> Any: pass
    def visit_float_literal(self, node: FloatLiteralNode) -> Any: pass
    def visit_char_literal(self, node: CharLiteralNode) -> Any: pass
    def visit_string_literal(self, node: StringLiteralNode) -> Any: pass
    def visit_boolean_literal(self, node: BooleanLiteralNode) -> Any: pass
    def visit_interpolated_string(self, node: InterpolatedStringNode) -> Any: pass
    def visit_identifier(self, node: IdentifierNode) -> Any: pass
    def visit_binary_operator(self, node: BinaryOperatorNode) -> Any: pass
    def visit_unary_operator(self, node: UnaryOperatorNode) -> Any: pass
    def visit_assignment(self, node: AssignmentNode) -> Any: pass
    def visit_call_expression(self, node: CallExpressionNode) -> Any: pass
    def visit_member_access(self, node: MemberAccessNode) -> Any: pass
    def visit_array_access(self, node: ArrayAccessNode) -> Any: pass
    def visit_cast_expression(self, node: CastExpressionNode) -> Any: pass
    def visit_conditional_expression(self, node: ConditionalExpressionNode) -> Any: pass
    def visit_sizeof_expression(self, node: SizeofExpressionNode) -> Any: pass
    def visit_alignof_expression(self, node: AlignofExpressionNode) -> Any: pass
    def visit_typeof_expression(self, node: TypeofExpressionNode) -> Any: pass
    def visit_array_comprehension(self, node: ArrayComprehensionNode) -> Any: pass
    
    # Error recovery
    def visit_error(self, node: ErrorNode) -> Any: pass


# ================================================================
# UTILITY FUNCTIONS
# ================================================================

def create_ast_from_tokens(tokens: List[Any]) -> ModuleNode:
    """Factory function to create AST from token stream"""
    # This would be implemented by the parser
    pass


def traverse_ast(root: ASTNode, visitor: ASTVisitor) -> Any:
    """Utility function to traverse AST with a visitor"""
    return root.accept(visitor)


def find_nodes_by_type(root: ASTNode, node_type: type) -> List[ASTNode]:
    """Find all nodes of a specific type in the AST"""
    result = []
    
    class FindVisitor(ASTVisitor):
        def visit(self, node: ASTNode) -> Any:
            if isinstance(node, node_type):
                result.append(node)
            
            for child in node.children:
                child.accept(self)
    
    root.accept(FindVisitor())
    return result


def get_ast_depth(node: ASTNode) -> int:
    """Calculate the depth of the AST from a given node"""
    if not node.children:
        return 1
    return 1 + max(get_ast_depth(child) for child in node.children)


def validate_ast(root: ASTNode) -> List[str]:
    """Validate AST structure and return list of issues"""
    issues = []
    
    class ValidationVisitor(ASTVisitor):
        def visit(self, node: ASTNode) -> Any:
            # Check parent-child relationships
            for child in node.children:
                if child.parent != node:
                    issues.append(f"Inconsistent parent-child relationship at {child.location}")
                child.accept(self)
    
    root.accept(ValidationVisitor())
    return issues