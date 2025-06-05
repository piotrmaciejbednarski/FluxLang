#!/usr/bin/env python3
"""
Flux AST (fast.py)
Abstract Syntax Tree nodes for the Flux programming language.
Builds upon the lexer (flexer.py) and provides the foundation for the parser.
"""

from __future__ import annotations
from abc import ABC, abstractmethod
from typing import List, Optional, Any, Union
from dataclasses import dataclass


# Base AST Node
class ASTNode(ABC):
    """Base class for all AST nodes"""
    
    def __init__(self, line: int = 0, col: int = 0):
        self.line = line
        self.col = col
    
    @abstractmethod
    def accept(self, visitor):
        """Accept method for visitor pattern"""
        pass


# Program and Global Items
class ProgramNode(ASTNode):
    """Root node representing the entire program"""
    
    def __init__(self, global_items: List['GlobalItemNode'], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.global_items = global_items
    
    def accept(self, visitor):
        return visitor.visit_program(self)


class GlobalItemNode(ASTNode):
    """Base class for global-level items"""
    pass


class ImportStmtNode(GlobalItemNode):
    """Import statement node"""
    
    def __init__(self, module_path: str, alias: Optional[str] = None, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.module_path = module_path
        self.alias = alias
    
    def accept(self, visitor):
        return visitor.visit_import_stmt(self)


class UsingStmtNode(GlobalItemNode):
    """Using statement node"""
    
    def __init__(self, using_list: List[NamespaceAccessNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.using_list = using_list
    
    def accept(self, visitor):
        return visitor.visit_using_stmt(self)


class NamespaceAccessNode(ASTNode):
    """Namespace access (e.g., std::io)"""
    
    def __init__(self, identifiers: List[str], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.identifiers = identifiers
    
    def accept(self, visitor):
        return visitor.visit_namespace_access(self)


class NamespaceDefNode(GlobalItemNode):
    """Namespace definition node"""
    
    def __init__(self, name: str, body: List[GlobalItemNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_namespace_def(self)

class FunctionDefNode(GlobalItemNode):
    """Function definition node"""
    
    def __init__(self, name: str, parameters: List[ParameterNode], 
                 return_type: TypeSpecNode, body: List[StatementNode], 
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.parameters = parameters
        self.return_type = return_type
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_function_def(self)


class ParameterNode(ASTNode):
    """Function parameter node"""
    
    def __init__(self, type_spec: TypeSpecNode, name: str, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.type_spec = type_spec
        self.name = name
    
    def accept(self, visitor):
        return visitor.visit_parameter(self)


class ObjectMemberNode(ASTNode):
    """Base class for object members"""
    pass


class ObjectDefNode(GlobalItemNode, ObjectMemberNode):
    """Object definition node - can be both global item and object member (for nested objects)"""
    
    def __init__(self, name: str, inheritance: Optional[InheritanceNode] = None,
                 body: Optional[List[ObjectMemberNode]] = None, 
                 is_forward_decl: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.inheritance = inheritance
        self.body = body or []
        self.is_forward_decl = is_forward_decl
    
    def accept(self, visitor):
        return visitor.visit_object_def(self)


class InheritanceNode(ASTNode):
    """Inheritance specification node"""
    
    def __init__(self, inheritance_list: List[InheritanceItemNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.inheritance_list = inheritance_list
    
    def accept(self, visitor):
        return visitor.visit_inheritance(self)


class InheritanceItemNode(ASTNode):
    """Single inheritance item (include/exclude)"""
    
    def __init__(self, name: str, is_excluded: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.is_excluded = is_excluded
    
    def accept(self, visitor):
        return visitor.visit_inheritance_item(self)


class MethodDefNode(ObjectMemberNode):
    """Method definition node"""
    
    def __init__(self, name: str, parameters: List[ParameterNode], 
                 return_type: TypeSpecNode, body: List[StatementNode],
                 is_magic_method: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.parameters = parameters
        self.return_type = return_type
        self.body = body
        self.is_magic_method = is_magic_method
    
    def accept(self, visitor):
        return visitor.visit_method_def(self)


class StructDefNode(GlobalItemNode):
    """Struct definition node"""
    
    def __init__(self, name: str, body: List[StructMemberNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_struct_def(self)


class StructMemberNode(ASTNode):
    """Struct member (variable declaration)"""
    
    def __init__(self, variable_decl: VariableDeclNode, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.variable_decl = variable_decl
    
    def accept(self, visitor):
        return visitor.visit_struct_member(self)


# Template Definitions
class TemplateDefNode(GlobalItemNode, ObjectMemberNode):
    """Base class for template definitions - can be both global items and object members"""
    
    def __init__(self, template_params: List[TemplateParamNode], 
                 is_volatile: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.template_params = template_params
        self.is_volatile = is_volatile


class TemplateParamNode(ASTNode):
    """Template parameter node"""
    
    def __init__(self, name: str, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
    
    def accept(self, visitor):
        return visitor.visit_template_param(self)


class FunctionTemplateNode(TemplateDefNode):
    """Function template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 parameters: List[ParameterNode], return_type: TypeSpecNode,
                 body: List[StatementNode], is_volatile: bool = False, 
                 line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.parameters = parameters
        self.return_type = return_type
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_function_template(self)


class ObjectTemplateNode(TemplateDefNode):
    """Object template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 inheritance: Optional[InheritanceNode], body: List[ObjectMemberNode],
                 is_volatile: bool = False, line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.inheritance = inheritance
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_object_template(self)


class StructTemplateNode(TemplateDefNode):
    """Struct template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 body: List[StructMemberNode], is_volatile: bool = False,
                 line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_struct_template(self)


class OperatorTemplateNode(TemplateDefNode):
    """Operator template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], 
                 parameters: List[ParameterNode], custom_op: str,
                 return_type: TypeSpecNode, body: List[StatementNode],
                 is_volatile: bool = False, line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.parameters = parameters
        self.custom_op = custom_op
        self.return_type = return_type
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_operator_template(self)


class ForTemplateNode(TemplateDefNode):
    """For template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 for_init: ForInitNode, condition: ExpressionNode,
                 for_update: ForUpdateNode, data_param: TemplateParamNode,
                 body: List[StatementNode], is_volatile: bool = False,
                 line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.for_init = for_init
        self.condition = condition
        self.for_update = for_update
        self.data_param = data_param
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_for_template(self)


class AsyncTemplateNode(TemplateDefNode):
    """Async template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 parameters: List[ParameterNode], return_type: TypeSpecNode,
                 body: List[StatementNode], is_volatile: bool = False,
                 line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.parameters = parameters
        self.return_type = return_type
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_async_template(self)


class SwitchTemplateNode(TemplateDefNode):
    """Switch template definition"""
    
    def __init__(self, template_params: List[TemplateParamNode], name: str,
                 parameters: List[ParameterNode], data_param: Optional[TemplateParamNode],
                 cases: List[CaseItemNode], is_volatile: bool = False,
                 line: int = 0, col: int = 0):
        super().__init__(template_params, is_volatile, line, col)
        self.name = name
        self.parameters = parameters
        self.data_param = data_param
        self.cases = cases
    
    def accept(self, visitor):
        return visitor.visit_switch_template(self)


class OperatorDefNode(GlobalItemNode):
    """Custom operator definition"""
    
    def __init__(self, parameters: List[ParameterNode], custom_op: str,
                 return_type: TypeSpecNode, body: List[StatementNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.parameters = parameters
        self.custom_op = custom_op
        self.return_type = return_type
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_operator_def(self)


# Type Specifications
class TypeSpecNode(ASTNode):
    """Base class for type specifications"""
    pass


class BaseTypeNode(TypeSpecNode):
    """Base type (int, float, char, void, bool, data{n}, identifier)"""
    
    def __init__(self, type_name: str, data_width: Optional[Union[int, 'ExpressionNode']] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.type_name = type_name
        self.data_width = data_width
    
    def accept(self, visitor):
        return visitor.visit_base_type(self)


class PointerTypeNode(TypeSpecNode):
    """Pointer type specification"""
    
    def __init__(self, base_type: TypeSpecNode, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.base_type = base_type
    
    def accept(self, visitor):
        return visitor.visit_pointer_type(self)


class ArrayTypeNode(TypeSpecNode):
    """Array type specification"""
    
    def __init__(self, element_type: TypeSpecNode, dimensions: int = 1,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.element_type = element_type
        self.dimensions = dimensions
    
    def accept(self, visitor):
        return visitor.visit_array_type(self)


class TemplateTypeNode(TypeSpecNode):
    """Template type instantiation"""
    
    def __init__(self, template_name: str, type_args: List[TypeSpecNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.template_name = template_name
        self.type_args = type_args
    
    def accept(self, visitor):
        return visitor.visit_template_type(self)


class QualifiedTypeNode(TypeSpecNode):
    """Qualified type (const, volatile, signed, unsigned)"""
    
    def __init__(self, qualifier: str, base_type: TypeSpecNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.qualifier = qualifier
        self.base_type = base_type
    
    def accept(self, visitor):
        return visitor.visit_qualified_type(self)


# Statements
class StatementNode(ASTNode):
    """Base class for statements"""
    pass


class ExpressionStmtNode(StatementNode):
    """Expression statement"""
    
    def __init__(self, expression: ExpressionNode, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_expression_stmt(self)


class VariableDeclNode(StatementNode, ObjectMemberNode):
    """Variable declaration"""
    
    def __init__(self, type_spec: Optional[TypeSpecNode], 
                 variables: List[VariableInitNode], is_auto: bool = False,
                 destructure_target: Optional[List[str]] = None,
                 destructure_source: Optional[ExpressionNode] = None,
                 destructure_fields: Optional[List[str]] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.type_spec = type_spec
        self.variables = variables
        self.is_auto = is_auto
        self.destructure_target = destructure_target
        self.destructure_source = destructure_source
        self.destructure_fields = destructure_fields
    
    def accept(self, visitor):
        return visitor.visit_variable_decl(self)


class VariableInitNode(ASTNode):
    """Variable initialization"""
    
    def __init__(self, name: str, initializer: Optional[ExpressionNode] = None,
                 constructor_args: Optional[List[ExpressionNode]] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
        self.initializer = initializer
        self.constructor_args = constructor_args
    
    def accept(self, visitor):
        return visitor.visit_variable_init(self)


class IfStmtNode(StatementNode):
    """If statement"""
    
    def __init__(self, condition: ExpressionNode, then_body: List[StatementNode],
                 else_clause: Optional[ElseClauseNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.condition = condition
        self.then_body = then_body
        self.else_clause = else_clause
    
    def accept(self, visitor):
        return visitor.visit_if_stmt(self)


class ElseClauseNode(ASTNode):
    """Else clause (else if or else)"""
    
    def __init__(self, else_if: Optional[IfStmtNode] = None,
                 else_body: Optional[List[StatementNode]] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.else_if = else_if
        self.else_body = else_body
    
    def accept(self, visitor):
        return visitor.visit_else_clause(self)


class WhileStmtNode(StatementNode):
    """While statement"""
    
    def __init__(self, condition: ExpressionNode, body: List[StatementNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.condition = condition
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_while_stmt(self)


class DoWhileStmtNode(StatementNode):
    """Do-while statement"""
    
    def __init__(self, body: List[StatementNode], condition: ExpressionNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.body = body
        self.condition = condition
    
    def accept(self, visitor):
        return visitor.visit_do_while_stmt(self)


class ForStmtNode(StatementNode):
    """For statement"""
    
    def __init__(self, for_init: Optional[ForInitNode], 
                 condition: Optional[ExpressionNode],
                 for_update: Optional[ForUpdateNode], 
                 body: List[StatementNode],
                 iterator_var: Optional[str] = None,
                 iterable: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.for_init = for_init
        self.condition = condition
        self.for_update = for_update
        self.body = body
        self.iterator_var = iterator_var  # For for-in loops
        self.iterable = iterable  # For for-in loops
    
    def accept(self, visitor):
        return visitor.visit_for_stmt(self)


class ForInitNode(ASTNode):
    """For loop initialization"""
    
    def __init__(self, variable_decl: Optional[VariableDeclNode] = None,
                 expression: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.variable_decl = variable_decl
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_for_init(self)


class ForUpdateNode(ASTNode):
    """For loop update"""
    
    def __init__(self, expression: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_for_update(self)


class SwitchStmtNode(StatementNode):
    """Switch statement"""
    
    def __init__(self, expression: ExpressionNode, cases: List[CaseItemNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
        self.cases = cases
    
    def accept(self, visitor):
        return visitor.visit_switch_stmt(self)


class CaseItemNode(ASTNode):
    """Case item in switch statement"""
    
    def __init__(self, case_value: Optional[ExpressionNode], 
                 body: List[StatementNode], is_default: bool = False,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.case_value = case_value
        self.body = body
        self.is_default = is_default
    
    def accept(self, visitor):
        return visitor.visit_case_item(self)


class BreakStmtNode(StatementNode):
    """Break statement"""
    
    def accept(self, visitor):
        return visitor.visit_break_stmt(self)


class ContinueStmtNode(StatementNode):
    """Continue statement"""
    
    def accept(self, visitor):
        return visitor.visit_continue_stmt(self)


class ReturnStmtNode(StatementNode):
    """Return statement"""
    
    def __init__(self, expression: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_return_stmt(self)


class TryCatchStmtNode(StatementNode):
    """Try-catch statement"""
    
    def __init__(self, try_body: List[StatementNode], 
                 catch_clauses: List[CatchClauseNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.try_body = try_body
        self.catch_clauses = catch_clauses
    
    def accept(self, visitor):
        return visitor.visit_try_catch_stmt(self)


class CatchClauseNode(ASTNode):
    """Catch clause"""
    
    def __init__(self, parameter: ParameterNode, body: List[StatementNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.parameter = parameter
        self.body = body
    
    def accept(self, visitor):
        return visitor.visit_catch_clause(self)


class ThrowStmtNode(StatementNode):
    """Throw statement"""
    
    def __init__(self, expression: ExpressionNode, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_throw_stmt(self)


class AssertStmtNode(StatementNode):
    """Assert statement with optional message"""
    
    def __init__(self, expression: ExpressionNode, message: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
        self.message = message  # Optional message expression
    
    def accept(self, visitor):
        return visitor.visit_assert_stmt(self)


class AsmStmtNode(StatementNode):
    """Inline assembly statement"""
    
    def __init__(self, asm_code: str, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.asm_code = asm_code
    
    def accept(self, visitor):
        return visitor.visit_asm_stmt(self)


class BlockStmtNode(StatementNode):
    """Block statement"""
    
    def __init__(self, statements: List[StatementNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.statements = statements
    
    def accept(self, visitor):
        return visitor.visit_block_stmt(self)


# Expressions
class ExpressionNode(ASTNode):
    """Base class for expressions"""
    pass


class AssignmentExprNode(ExpressionNode):
    """Assignment expression"""
    
    def __init__(self, left: ExpressionNode, operator: str, right: ExpressionNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.left = left
        self.operator = operator
        self.right = right
    
    def accept(self, visitor):
        return visitor.visit_assignment_expr(self)


class ConditionalExprNode(ExpressionNode):
    """Conditional (ternary) expression"""
    
    def __init__(self, condition: ExpressionNode, true_expr: ExpressionNode,
                 false_expr: ExpressionNode, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.condition = condition
        self.true_expr = true_expr
        self.false_expr = false_expr
    
    def accept(self, visitor):
        return visitor.visit_conditional_expr(self)


class BinaryOpExprNode(ExpressionNode):
    """Binary operation expression"""
    
    def __init__(self, left: ExpressionNode, operator: str, right: ExpressionNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.left = left
        self.operator = operator
        self.right = right
    
    def accept(self, visitor):
        return visitor.visit_binary_op_expr(self)


class UnaryOpExprNode(ExpressionNode):
    """Unary operation expression"""
    
    def __init__(self, operator: str, operand: ExpressionNode,
                 is_postfix: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.operator = operator
        self.operand = operand
        self.is_postfix = is_postfix
    
    def accept(self, visitor):
        return visitor.visit_unary_op_expr(self)


class CastExprNode(ExpressionNode):
    """Type cast expression"""
    
    def __init__(self, target_type: TypeSpecNode, expression: ExpressionNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.target_type = target_type
        self.expression = expression
    
    def accept(self, visitor):
        return visitor.visit_cast_expr(self)


class PostfixExprNode(ExpressionNode):
    """Postfix expression (array access, function call, member access, etc.)"""
    
    def __init__(self, expression: ExpressionNode, operator: str,
                 argument: Optional[Union[ExpressionNode, List[ExpressionNode], str]] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.expression = expression
        self.operator = operator  # '[', '(', '.', '::', '++', '--'
        self.argument = argument
    
    def accept(self, visitor):
        return visitor.visit_postfix_expr(self)


class ArrayAccessExprNode(ExpressionNode):
    """Array access expression"""
    
    def __init__(self, array: ExpressionNode, index: ExpressionNode,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.array = array
        self.index = index
    
    def accept(self, visitor):
        return visitor.visit_array_access_expr(self)

class RangeExprNode(ExpressionNode):
    def __init__(self, start: ExpressionNode, end: ExpressionNode, line=0, col=0):
        super().__init__(line, col)
        self.start = start
        self.end = end

    def accept(self, visitor):
        return visitor.visit_range_expr(self)

class FunctionCallExprNode(ExpressionNode):
    """Function call expression"""
    
    def __init__(self, function: ExpressionNode, arguments: List[ExpressionNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.function = function
        self.arguments = arguments
    
    def accept(self, visitor):
        return visitor.visit_function_call_expr(self)


class MemberAccessExprNode(ExpressionNode):
    """Member access expression"""
    
    def __init__(self, object_expr: ExpressionNode, member: str,
                 is_scope_access: bool = False, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.object_expr = object_expr
        self.member = member
        self.is_scope_access = is_scope_access  # True for ::, False for .
    
    def accept(self, visitor):
        return visitor.visit_member_access_expr(self)


class IdentifierExprNode(ExpressionNode):
    """Identifier expression"""
    
    def __init__(self, name: str, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.name = name
    
    def accept(self, visitor):
        return visitor.visit_identifier_expr(self)


class LiteralExprNode(ExpressionNode):
    """Literal expression"""
    
    def __init__(self, value: Any, literal_type: str, line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.value = value
        self.literal_type = literal_type  # 'int', 'float', 'char', 'string', 'bool', 'data'
    
    def accept(self, visitor):
        return visitor.visit_literal_expr(self)


class ArrayLiteralExprNode(ExpressionNode):
    """Array literal expression"""
    
    def __init__(self, elements: List[ExpressionNode], line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.elements = elements
    
    def accept(self, visitor):
        return visitor.visit_array_literal_expr(self)


class ArrayComprehensionExprNode(ExpressionNode):
    """Array comprehension expression"""
    
    def __init__(self, element_expr: ExpressionNode, iterator_var: str,
                 iterable: ExpressionNode, condition: Optional[ExpressionNode] = None,
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.element_expr = element_expr
        self.iterator_var = iterator_var
        self.iterable = iterable
        self.condition = condition
    
    def accept(self, visitor):
        return visitor.visit_array_comprehension_expr(self)


class IStringExprNode(ExpressionNode):
    """Interpolated string expression"""
    
    def __init__(self, string_literal: str, expressions: List[ExpressionNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.string_literal = string_literal
        self.expressions = expressions
    
    def accept(self, visitor):
        return visitor.visit_istring_expr(self)


class TemplateInstantiationExprNode(ExpressionNode):
    """Template instantiation expression"""
    
    def __init__(self, template_name: str, type_args: List[TypeSpecNode],
                 line: int = 0, col: int = 0):
        super().__init__(line, col)
        self.template_name = template_name
        self.type_args = type_args
    
    def accept(self, visitor):
        return visitor.visit_template_instantiation_expr(self)


class ThisExprNode(ExpressionNode):
    """'this' keyword expression"""
    
    def accept(self, visitor):
        return visitor.visit_this_expr(self)


class IteratorExprNode(ExpressionNode):
    """Iterator '_' expression"""
    
    def accept(self, visitor):
        return visitor.visit_iterator_expr(self)


# Visitor Interface
class ASTVisitor(ABC):
    """Abstract visitor interface for AST traversal"""
    
    @abstractmethod
    def visit_program(self, node: ProgramNode): pass
    
    @abstractmethod
    def visit_import_stmt(self, node: ImportStmtNode): pass
    
    @abstractmethod
    def visit_using_stmt(self, node: UsingStmtNode): pass
    
    @abstractmethod
    def visit_namespace_access(self, node: NamespaceAccessNode): pass
    
    @abstractmethod
    def visit_namespace_def(self, node: NamespaceDefNode): pass
    
    @abstractmethod
    def visit_function_def(self, node: FunctionDefNode): pass
    
    @abstractmethod
    def visit_parameter(self, node: ParameterNode): pass
    
    @abstractmethod
    def visit_object_def(self, node: ObjectDefNode): pass
    
    @abstractmethod
    def visit_inheritance(self, node: InheritanceNode): pass
    
    @abstractmethod
    def visit_inheritance_item(self, node: InheritanceItemNode): pass
    
    @abstractmethod
    def visit_method_def(self, node: MethodDefNode): pass
    
    @abstractmethod
    def visit_struct_def(self, node: StructDefNode): pass
    
    @abstractmethod
    def visit_struct_member(self, node: StructMemberNode): pass
    
    @abstractmethod
    def visit_template_param(self, node: TemplateParamNode): pass
    
    @abstractmethod
    def visit_function_template(self, node: FunctionTemplateNode): pass
    
    @abstractmethod
    def visit_object_template(self, node: ObjectTemplateNode): pass
    
    @abstractmethod
    def visit_struct_template(self, node: StructTemplateNode): pass
    
    @abstractmethod
    def visit_operator_template(self, node: OperatorTemplateNode): pass
    
    @abstractmethod
    def visit_for_template(self, node: ForTemplateNode): pass
    
    @abstractmethod
    def visit_async_template(self, node: AsyncTemplateNode): pass
    
    @abstractmethod
    def visit_switch_template(self, node: SwitchTemplateNode): pass
    
    @abstractmethod
    def visit_operator_def(self, node: OperatorDefNode): pass
    
    @abstractmethod
    def visit_base_type(self, node: BaseTypeNode): pass
    
    @abstractmethod
    def visit_pointer_type(self, node: PointerTypeNode): pass
    
    @abstractmethod
    def visit_array_type(self, node: ArrayTypeNode): pass
    
    @abstractmethod
    def visit_template_type(self, node: TemplateTypeNode): pass
    
    @abstractmethod
    def visit_qualified_type(self, node: QualifiedTypeNode): pass
    
    @abstractmethod
    def visit_expression_stmt(self, node: ExpressionStmtNode): pass
    
    @abstractmethod
    def visit_variable_decl(self, node: VariableDeclNode): pass
    
    @abstractmethod
    def visit_variable_init(self, node: VariableInitNode): pass
    
    @abstractmethod
    def visit_if_stmt(self, node: IfStmtNode): pass
    
    @abstractmethod
    def visit_else_clause(self, node: ElseClauseNode): pass
    
    @abstractmethod
    def visit_while_stmt(self, node: WhileStmtNode): pass
    
    @abstractmethod
    def visit_do_while_stmt(self, node: DoWhileStmtNode): pass
    
    @abstractmethod
    def visit_for_stmt(self, node: ForStmtNode): pass
    
    @abstractmethod
    def visit_for_init(self, node: ForInitNode): pass
    
    @abstractmethod
    def visit_for_update(self, node: ForUpdateNode): pass
    
    @abstractmethod
    def visit_switch_stmt(self, node: SwitchStmtNode): pass
    
    @abstractmethod
    def visit_case_item(self, node: CaseItemNode): pass
    
    @abstractmethod
    def visit_break_stmt(self, node: BreakStmtNode): pass
    
    @abstractmethod
    def visit_continue_stmt(self, node: ContinueStmtNode): pass
    
    @abstractmethod
    def visit_return_stmt(self, node: ReturnStmtNode): pass
    
    @abstractmethod
    def visit_try_catch_stmt(self, node: TryCatchStmtNode): pass
    
    @abstractmethod
    def visit_catch_clause(self, node: CatchClauseNode): pass
    
    @abstractmethod
    def visit_throw_stmt(self, node: ThrowStmtNode): pass
    
    @abstractmethod
    def visit_assert_stmt(self, node: AssertStmtNode): pass
    
    @abstractmethod
    def visit_asm_stmt(self, node: AsmStmtNode): pass
    
    @abstractmethod
    def visit_block_stmt(self, node: BlockStmtNode): pass
    
    @abstractmethod
    def visit_assignment_expr(self, node: AssignmentExprNode): pass
    
    @abstractmethod
    def visit_conditional_expr(self, node: ConditionalExprNode): pass
    
    @abstractmethod
    def visit_binary_op_expr(self, node: BinaryOpExprNode): pass
    
    @abstractmethod
    def visit_unary_op_expr(self, node: UnaryOpExprNode): pass
    
    @abstractmethod
    def visit_cast_expr(self, node: CastExprNode): pass
    
    @abstractmethod
    def visit_postfix_expr(self, node: PostfixExprNode): pass
    
    @abstractmethod
    def visit_array_access_expr(self, node: ArrayAccessExprNode): pass
    
    @abstractmethod
    def visit_function_call_expr(self, node: FunctionCallExprNode): pass
    
    @abstractmethod
    def visit_member_access_expr(self, node: MemberAccessExprNode): pass
    
    @abstractmethod
    def visit_identifier_expr(self, node: IdentifierExprNode): pass
    
    @abstractmethod
    def visit_literal_expr(self, node: LiteralExprNode): pass
    
    @abstractmethod
    def visit_array_literal_expr(self, node: ArrayLiteralExprNode): pass
    
    @abstractmethod
    def visit_array_comprehension_expr(self, node: ArrayComprehensionExprNode): pass
    
    @abstractmethod
    def visit_istring_expr(self, node: IStringExprNode): pass
    
    @abstractmethod
    def visit_template_instantiation_expr(self, node: TemplateInstantiationExprNode): pass
    
    @abstractmethod
    def visit_this_expr(self, node: ThisExprNode): pass
    
    @abstractmethod
    def visit_iterator_expr(self, node: IteratorExprNode): pass


# Utility function for AST node creation
def create_ast_node(node_type: str, **kwargs) -> ASTNode:
    """Factory function to create AST nodes by type name"""
    node_classes = {
        'program': ProgramNode,
        'import_stmt': ImportStmtNode,
        'using_stmt': UsingStmtNode,
        'namespace_access': NamespaceAccessNode,
        'namespace_def': NamespaceDefNode,
        'function_def': FunctionDefNode,
        'parameter': ParameterNode,
        'object_def': ObjectDefNode,
        'inheritance': InheritanceNode,
        'inheritance_item': InheritanceItemNode,
        'method_def': MethodDefNode,
        'struct_def': StructDefNode,
        'struct_member': StructMemberNode,
        'template_param': TemplateParamNode,
        'function_template': FunctionTemplateNode,
        'object_template': ObjectTemplateNode,
        'struct_template': StructTemplateNode,
        'operator_template': OperatorTemplateNode,
        'for_template': ForTemplateNode,
        'async_template': AsyncTemplateNode,
        'switch_template': SwitchTemplateNode,
        'operator_def': OperatorDefNode,
        'base_type': BaseTypeNode,
        'pointer_type': PointerTypeNode,
        'array_type': ArrayTypeNode,
        'template_type': TemplateTypeNode,
        'qualified_type': QualifiedTypeNode,
        'expression_stmt': ExpressionStmtNode,
        'variable_decl': VariableDeclNode,
        'variable_init': VariableInitNode,
        'if_stmt': IfStmtNode,
        'else_clause': ElseClauseNode,
        'while_stmt': WhileStmtNode,
        'do_while_stmt': DoWhileStmtNode,
        'for_stmt': ForStmtNode,
        'for_init': ForInitNode,
        'for_update': ForUpdateNode,
        'switch_stmt': SwitchStmtNode,
        'case_item': CaseItemNode,
        'break_stmt': BreakStmtNode,
        'continue_stmt': ContinueStmtNode,
        'return_stmt': ReturnStmtNode,
        'try_catch_stmt': TryCatchStmtNode,
        'catch_clause': CatchClauseNode,
        'throw_stmt': ThrowStmtNode,
        'assert_stmt': AssertStmtNode,
        'asm_stmt': AsmStmtNode,
        'block_stmt': BlockStmtNode,
        'assignment_expr': AssignmentExprNode,
        'conditional_expr': ConditionalExprNode,
        'binary_op_expr': BinaryOpExprNode,
        'unary_op_expr': UnaryOpExprNode,
        'cast_expr': CastExprNode,
        'postfix_expr': PostfixExprNode,
        'array_access_expr': ArrayAccessExprNode,
        'function_call_expr': FunctionCallExprNode,
        'member_access_expr': MemberAccessExprNode,
        'identifier_expr': IdentifierExprNode,
        'literal_expr': LiteralExprNode,
        'array_literal_expr': ArrayLiteralExprNode,
        'array_comprehension_expr': ArrayComprehensionExprNode,
        'istring_expr': IStringExprNode,
        'template_instantiation_expr': TemplateInstantiationExprNode,
        'this_expr': ThisExprNode,
        'iterator_expr': IteratorExprNode,
    }
    
    if node_type not in node_classes:
        raise ValueError(f"Unknown AST node type: {node_type}")
    
    return node_classes[node_type](**kwargs)


if __name__ == "__main__":
    # Example usage and testing
    print("Flux AST module loaded successfully")
    
    # Create a simple example AST
    main_func = FunctionDefNode(
        name="main",
        parameters=[],
        return_type=BaseTypeNode("int"),
        body=[
            ReturnStmtNode(LiteralExprNode(0, "int"))
        ]
    )
    
    program = ProgramNode([main_func])
    
    print("Example AST created:")
    print(f"Program with {len(program.global_items)} global items")
    print(f"Main function with {len(main_func.body)} statements")