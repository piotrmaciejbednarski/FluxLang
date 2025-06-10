#!/usr/bin/env python3
"""
Flux Type Checker (ftyper.py)
Performs type checking and semantic analysis on Flux AST nodes.
Builds upon the parser and AST to ensure type safety and semantic correctness.

Only supports the built-in primitive types: int, float, char, data{n}, void
All other types are user-defined and handled dynamically.
"""

from fast3 import *
from fparser3 import parse
from typing import Dict, List, Optional, Union, Set, Tuple
from abc import ABC, abstractmethod
import re


class FluxType(ABC):
    """Base class for all Flux types"""
    
    def __init__(self, name: str):
        self.name = name
    
    @abstractmethod
    def is_compatible_with(self, other: 'FluxType') -> bool:
        """Check if this type is compatible with another type"""
        pass
    
    @abstractmethod
    def can_cast_to(self, other: 'FluxType') -> bool:
        """Check if this type can be cast to another type"""
        pass
    
    def __str__(self) -> str:
        return self.name
    
    def __eq__(self, other) -> bool:
        return isinstance(other, FluxType) and self.name == other.name


class PrimitiveType(FluxType):
    """Primitive types: int, float, char, void only"""
    
    def __init__(self, name: str):
        if name not in ["int", "float", "char", "void"]:
            raise ValueError(f"Invalid primitive type: {name}")
        super().__init__(name)
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, PrimitiveType):
            if self.name == other.name:
                return True
            if self.name in ["int", "float"] and other.name in ["int", "float"]:
                return True
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, PrimitiveType):
            if self.name == "void" or other.name == "void":
                return self.name == other.name
            return True
        if isinstance(other, DataType):
            return self.name != "void"
        return False


class DataType(FluxType):
    """Data type: data{n}"""
    
    def __init__(self, width: int):
        super().__init__(f"data{{{width}}}")
        self.width = width
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, DataType):
            return self.width == other.width
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, PrimitiveType) and other.name == "void":
            return False
        return True


class QualifiedType(FluxType):
    """Qualified type: const T, volatile T, signed T, unsigned T"""
    
    def __init__(self, qualifier: str, base_type: FluxType):
        super().__init__(f"{qualifier} {base_type.name}")
        self.qualifier = qualifier
        self.base_type = base_type
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, QualifiedType):
            return (self.qualifier == other.qualifier and 
                   self.base_type.is_compatible_with(other.base_type))
        else:
            return self.base_type.is_compatible_with(other)
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, QualifiedType):
            return self.base_type.can_cast_to(other.base_type)
        else:
            return self.base_type.can_cast_to(other)


class PointerType(FluxType):
    """Pointer type: T*"""
    
    def __init__(self, base_type: FluxType):
        super().__init__(f"{base_type.name}*")
        self.base_type = base_type
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, PointerType):
            return self.base_type.is_compatible_with(other.base_type)
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, PointerType):
            return True
        if isinstance(other, DataType):
            return True
        return False


class ArrayType(FluxType):
    """Array type: T[], T[][]"""
    
    def __init__(self, element_type: FluxType, dimensions: int = 1):
        brackets = "[]" * dimensions
        super().__init__(f"{element_type.name}{brackets}")
        self.element_type = element_type
        self.dimensions = dimensions
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, ArrayType):
            return (self.element_type.is_compatible_with(other.element_type) and 
                   self.dimensions == other.dimensions)
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, ArrayType):
            return self.element_type.can_cast_to(other.element_type)
        return False


class FunctionType(FluxType):
    """Function type: (param_types) -> return_type"""
    
    def __init__(self, param_types: List[FluxType], return_type: FluxType):
        param_str = ", ".join(str(t) for t in param_types)
        super().__init__(f"({param_str}) -> {return_type}")
        self.param_types = param_types
        self.return_type = return_type
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, FunctionType):
            if len(self.param_types) != len(other.param_types):
                return False
            for p1, p2 in zip(self.param_types, other.param_types):
                if not p1.is_compatible_with(p2):
                    return False
            return self.return_type.is_compatible_with(other.return_type)
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        return self.is_compatible_with(other)


class UserDefinedType(FluxType):
    """User-defined type (objects, structs, type aliases)"""
    
    def __init__(self, name: str, namespace: str = ""):
        full_name = f"{namespace}::{name}" if namespace else name
        super().__init__(full_name)
        self.simple_name = name
        self.namespace = namespace
        self.definition = None
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, UserDefinedType):
            return self.name == other.name
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        if isinstance(other, UserDefinedType):
            return self.name == other.name
        return False


class TemplateType(FluxType):
    """Template type: Template<T1, T2, ...>"""
    
    def __init__(self, name: str, type_args: List[FluxType]):
        type_arg_str = ", ".join(str(t) for t in type_args)
        super().__init__(f"{name}<{type_arg_str}>")
        self.template_name = name
        self.type_args = type_args
    
    def is_compatible_with(self, other: FluxType) -> bool:
        if isinstance(other, TemplateType):
            if self.template_name != other.template_name:
                return False
            if len(self.type_args) != len(other.type_args):
                return False
            for t1, t2 in zip(self.type_args, other.type_args):
                if not t1.is_compatible_with(t2):
                    return False
            return True
        return False
    
    def can_cast_to(self, other: FluxType) -> bool:
        return self.is_compatible_with(other)


class TypeCheckError(Exception):
    """Type checking error"""
    
    def __init__(self, message: str, line: int = 0, col: int = 0, node_info: str = ""):
        self.message = message
        self.line = line
        self.col = col
        self.node_info = node_info
        super().__init__(f"Type error at {line}:{col}: {message} (in {node_info})")


class Symbol:
    """Symbol table entry"""
    
    def __init__(self, name: str, symbol_type: FluxType, definition: ASTNode):
        self.name = name
        self.type = symbol_type
        self.definition = definition
        self.is_function = isinstance(symbol_type, FunctionType)
        self.is_type = isinstance(definition, (ObjectDefNode, StructDefNode))


class Scope:
    """Symbol table scope"""
    
    def __init__(self, name: str = "", parent: Optional['Scope'] = None):
        self.name = name
        self.parent = parent
        self.symbols: Dict[str, Symbol] = {}
        self.children: List['Scope'] = []
        
        if parent:
            parent.children.append(self)
    
    def define(self, symbol: Symbol):
        """Define a symbol in this scope"""
        if symbol.name in self.symbols:
            raise TypeCheckError(f"Symbol '{symbol.name}' already defined in scope '{self.name}'")
        self.symbols[symbol.name] = symbol
    
    def lookup(self, name: str) -> Optional[Symbol]:
        """Look up a symbol in this scope and parent scopes"""
        if name in self.symbols:
            return self.symbols[name]
        if self.parent:
            return self.parent.lookup(name)
        return None
    
    def lookup_local(self, name: str) -> Optional[Symbol]:
        """Look up a symbol only in this scope"""
        return self.symbols.get(name)


class TypeChecker(ASTVisitor):
    """Type checker that implements the visitor pattern"""
    
    def __init__(self):
        self.global_scope = Scope("global")
        self.current_scope = self.global_scope
        self.current_function_return_type = None
        self.current_object = None
        self.errors = []
    
    def enter_scope(self, name: str = "") -> Scope:
        """Enter a new scope"""
        new_scope = Scope(name, self.current_scope)
        self.current_scope = new_scope
        return new_scope
    
    def exit_scope(self):
        """Exit the current scope"""
        if self.current_scope.parent:
            self.current_scope = self.current_scope.parent
    
    def error(self, message: str, node: ASTNode = None, context: str = ""):
        """Report a type checking error with full debugging info"""
        line = node.line if node else 0
        col = node.col if node else 0
        node_type = type(node).__name__ if node else "Unknown"
        node_info = f"{node_type} {context}".strip()
        
        error = TypeCheckError(message, line, col, node_info)
        self.errors.append(error)
        print(f"DEBUG: {error}")  # Immediate debug output
    
    def type_check(self, ast: ProgramNode) -> List[TypeCheckError]:
        """Perform type checking on the AST"""
        print("Beginning type checking phase...")
        self.errors = []
        try:
            ast.accept(self)
        except Exception as e:
            print(f"FATAL ERROR during type checking: {e}")
            print(f"Current scope: {self.current_scope.name}")
            print(f"Error count so far: {len(self.errors)}")
            raise
        
        print(f"Type checking completed with {len(self.errors)} errors.")
        return self.errors
    
    def extract_literal_value(self, literal_token: str) -> Union[int, float, str]:
        """Extract actual value from literal token"""
        try:
            # Handle different literal types
            if literal_token.isdigit():
                return int(literal_token)
            elif '.' in literal_token and all(c.isdigit() or c == '.' for c in literal_token):
                return float(literal_token)
            elif literal_token.startswith('"') and literal_token.endswith('"'):
                return literal_token[1:-1]  # Remove quotes
            elif literal_token.startswith("'") and literal_token.endswith("'"):
                return literal_token[1:-1]  # Remove quotes
            else:
                # Try to extract from hex, binary, etc.
                if literal_token.startswith('0x'):
                    return int(literal_token, 16)
                elif literal_token.endswith('b'):
                    return int(literal_token[:-1], 2)
                else:
                    return 1  # Default fallback
        except ValueError:
            return 1  # Safe fallback
    
    def resolve_type_spec(self, type_spec: TypeSpecNode) -> FluxType:
        """Resolve a type specification to a FluxType"""
        try:
            if isinstance(type_spec, BaseTypeNode):
                if type_spec.type_name in ["int", "float", "char", "void"]:
                    return PrimitiveType(type_spec.type_name)
                elif type_spec.type_name == "data":
                    # Handle data{n} width parsing
                    if type_spec.data_width is None:
                        return DataType(32)  # Default width
                    elif isinstance(type_spec.data_width, LiteralExprNode):
                        try:
                            if hasattr(type_spec.data_width, 'value'):
                                width_val = self.extract_literal_value(str(type_spec.data_width.value))
                                return DataType(int(width_val))
                            else:
                                return DataType(32)
                        except (ValueError, AttributeError) as e:
                            self.error(f"Invalid data width: {e}", type_spec, f"data width parsing")
                            return DataType(32)
                    else:
                        # For now, assume width is computable at compile time
                        return DataType(32)
                elif type_spec.type_name == "this":
                    if self.current_object:
                        return self.current_object
                    else:
                        self.error("'this' type used outside of object context", type_spec)
                        return PrimitiveType("void")
                else:
                    # User-defined type
                    symbol = self.current_scope.lookup(type_spec.type_name)
                    if symbol and symbol.is_type:
                        return UserDefinedType(type_spec.type_name)
                    else:
                        return UserDefinedType(type_spec.type_name)
            
            elif isinstance(type_spec, QualifiedTypeNode):
                base_type = self.resolve_type_spec(type_spec.base_type)
                return QualifiedType(type_spec.qualifier, base_type)
            
            elif isinstance(type_spec, PointerTypeNode):
                base_type = self.resolve_type_spec(type_spec.base_type)
                return PointerType(base_type)
            
            elif isinstance(type_spec, ArrayTypeNode):
                element_type = self.resolve_type_spec(type_spec.element_type)
                return ArrayType(element_type, type_spec.dimensions)
            
            elif isinstance(type_spec, TemplateTypeNode):
                type_args = [self.resolve_type_spec(arg) for arg in type_spec.type_args]
                return TemplateType(type_spec.template_name, type_args)
            
            else:
                self.error(f"Unknown type specification: {type(type_spec).__name__}", type_spec)
                return PrimitiveType("void")
                
        except Exception as e:
            self.error(f"Error resolving type: {e}", type_spec, f"resolve_type_spec")
            return PrimitiveType("void")
    
    def visit_program(self, node: ProgramNode):
        try:
            for i, item in enumerate(node.global_items):
                print(f"DEBUG: Processing global item {i}: {type(item).__name__}")
                item.accept(self)
        except Exception as e:
            self.error(f"Error processing program: {e}", node, "visit_program")
        return None
    
    def visit_import_stmt(self, node: ImportStmtNode):
        print(f"DEBUG: Processing import: {node.module_path}")
        return None
    
    def visit_using_stmt(self, node: UsingStmtNode):
        print(f"DEBUG: Processing using statement with {len(node.using_list)} items")
        return None
    
    def visit_namespace_access(self, node: NamespaceAccessNode):
        return None
    
    def visit_namespace_def(self, node: NamespaceDefNode):
        print(f"DEBUG: Processing namespace: {node.name}")
        namespace_scope = self.enter_scope(f"namespace:{node.name}")
        try:
            for item in node.body:
                item.accept(self)
        except Exception as e:
            self.error(f"Error in namespace {node.name}: {e}", node, "namespace body")
        finally:
            self.exit_scope()
        return None
    
    def visit_function_def(self, node: FunctionDefNode):
        print(f"DEBUG: Processing function: {node.name}")
        try:
            param_types = []
            for param in node.parameters:
                param_type = self.resolve_type_spec(param.type_spec)
                param_types.append(param_type)
            
            return_type = self.resolve_type_spec(node.return_type)
            func_type = FunctionType(param_types, return_type)
            
            func_symbol = Symbol(node.name, func_type, node)
            try:
                self.current_scope.define(func_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, f"function definition")
            
            func_scope = self.enter_scope(f"function:{node.name}")
            prev_return_type = self.current_function_return_type
            self.current_function_return_type = return_type
            
            for param in node.parameters:
                param_type = self.resolve_type_spec(param.type_spec)
                param_symbol = Symbol(param.name, param_type, param)
                try:
                    func_scope.define(param_symbol)
                except TypeCheckError as e:
                    self.error(e.message, param, f"parameter definition")
            
            for stmt in node.body:
                stmt.accept(self)
            
            self.current_function_return_type = prev_return_type
            self.exit_scope()
            
        except Exception as e:
            self.error(f"Error processing function {node.name}: {e}", node, "function processing")
        
        return None
    
    def visit_parameter(self, node: ParameterNode):
        return self.resolve_type_spec(node.type_spec)
    
    def visit_object_def(self, node: ObjectDefNode):
        print(f"DEBUG: Processing object: {node.name}")
        try:
            if node.is_forward_decl:
                obj_type = UserDefinedType(node.name)
                obj_symbol = Symbol(node.name, obj_type, node)
                try:
                    self.current_scope.define(obj_symbol)
                except TypeCheckError as e:
                    self.error(e.message, node, "forward declaration")
                return None
            
            obj_type = UserDefinedType(node.name)
            obj_symbol = Symbol(node.name, obj_type, node)
            
            try:
                self.current_scope.define(obj_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, "object definition")
            
            obj_scope = self.enter_scope(f"object:{node.name}")
            prev_object = self.current_object
            self.current_object = obj_type
            
            if node.inheritance:
                node.inheritance.accept(self)
            
            for member in node.body:
                member.accept(self)
            
            self.current_object = prev_object
            self.exit_scope()
            
        except Exception as e:
            self.error(f"Error processing object {node.name}: {e}", node, "object processing")
        
        return None
    
    def visit_inheritance(self, node: InheritanceNode):
        for item in node.inheritance_list:
            item.accept(self)
        return None
    
    def visit_inheritance_item(self, node: InheritanceItemNode):
        symbol = self.current_scope.lookup(node.name)
        if not symbol or not symbol.is_type:
            self.error(f"Cannot inherit from undefined type '{node.name}'", node, "inheritance")
        return None
    
    def visit_method_def(self, node: MethodDefNode):
        print(f"DEBUG: Processing method: {node.name}")
        return self.visit_function_def(node)
    
    def visit_struct_def(self, node: StructDefNode):
        print(f"DEBUG: Processing struct: {node.name}")
        try:
            struct_type = UserDefinedType(node.name)
            struct_symbol = Symbol(node.name, struct_type, node)
            
            try:
                self.current_scope.define(struct_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, "struct definition")
            
            struct_scope = self.enter_scope(f"struct:{node.name}")
            
            for member in node.body:
                member.accept(self)
            
            self.exit_scope()
            
        except Exception as e:
            self.error(f"Error processing struct {node.name}: {e}", node, "struct processing")
        
        return None
    
    def visit_struct_member(self, node: StructMemberNode):
        return node.variable_decl.accept(self)
    
    def visit_template_param(self, node: TemplateParamNode):
        return None
    
    def visit_function_template(self, node: FunctionTemplateNode):
        print(f"DEBUG: Processing function template: {node.name}")
        return None
    
    def visit_object_template(self, node: ObjectTemplateNode):
        print(f"DEBUG: Processing object template: {node.name}")
        return None
    
    def visit_struct_template(self, node: StructTemplateNode):
        print(f"DEBUG: Processing struct template: {node.name}")
        return None
    
    def visit_operator_template(self, node: OperatorTemplateNode):
        return None
    
    def visit_for_template(self, node: ForTemplateNode):
        return None
    
    def visit_async_template(self, node: AsyncTemplateNode):
        return None
    
    def visit_switch_template(self, node: SwitchTemplateNode):
        return None
    
    def visit_operator_def(self, node: OperatorDefNode):
        print(f"DEBUG: Processing operator: {node.custom_op}")
        try:
            param_types = []
            for param in node.parameters:
                param_type = self.resolve_type_spec(param.type_spec)
                param_types.append(param_type)
            
            return_type = self.resolve_type_spec(node.return_type)
            func_type = FunctionType(param_types, return_type)
            
            op_symbol = Symbol(f"operator_{node.custom_op}", func_type, node)
            try:
                self.current_scope.define(op_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, "operator definition")
                
        except Exception as e:
            self.error(f"Error processing operator {node.custom_op}: {e}", node, "operator processing")
        
        return None
    
    def visit_base_type(self, node: BaseTypeNode):
        return self.resolve_type_spec(node)
    
    def visit_pointer_type(self, node: PointerTypeNode):
        return self.resolve_type_spec(node)
    
    def visit_array_type(self, node: ArrayTypeNode):
        return self.resolve_type_spec(node)
    
    def visit_template_type(self, node: TemplateTypeNode):
        return self.resolve_type_spec(node)
    
    def visit_qualified_type(self, node: QualifiedTypeNode):
        return self.resolve_type_spec(node)
    
    def visit_expression_stmt(self, node: ExpressionStmtNode):
        try:
            node.expression.accept(self)
        except Exception as e:
            self.error(f"Error in expression statement: {e}", node, "expression statement")
        return None
    
    def visit_variable_decl(self, node: VariableDeclNode):
        print(f"DEBUG: Processing variable declaration")
        try:
            if node.is_auto:
                if node.destructure_source and node.destructure_target and node.destructure_fields:
                    source_type = node.destructure_source.accept(self)
                    
                    if len(node.destructure_target) != len(node.destructure_fields):
                        self.error(f"Number of target variables ({len(node.destructure_target)}) does not match number of fields ({len(node.destructure_fields)})", node, "auto destructuring")
                    
                    for target_var in node.destructure_target:
                        auto_type = DataType(32)
                        var_symbol = Symbol(target_var, auto_type, node)
                        try:
                            self.current_scope.define(var_symbol)
                        except TypeCheckError as e:
                            self.error(e.message, node, f"auto variable {target_var}")
                else:
                    self.error("Invalid auto destructuring declaration", node, "auto syntax")
            else:
                var_type = self.resolve_type_spec(node.type_spec)
                
                for var_init in node.variables:
                    var_symbol = Symbol(var_init.name, var_type, var_init)
                    try:
                        self.current_scope.define(var_symbol)
                    except TypeCheckError as e:
                        self.error(e.message, var_init, f"variable {var_init.name}")
                    
                    if var_init.initializer:
                        init_type = var_init.initializer.accept(self)
                        if init_type and not var_type.is_compatible_with(init_type):
                            if not init_type.can_cast_to(var_type):
                                self.error(f"Cannot initialize variable of type '{var_type}' with value of type '{init_type}'", var_init, f"initialization")
                    
                    if var_init.constructor_args:
                        for arg in var_init.constructor_args:
                            arg.accept(self)
                            
        except Exception as e:
            self.error(f"Error processing variable declaration: {e}", node, "variable declaration")
        
        return None
    
    def visit_variable_init(self, node: VariableInitNode):
        return None
    
    def visit_if_stmt(self, node: IfStmtNode):
        try:
            node.condition.accept(self)
            
            if_scope = self.enter_scope("if")
            for stmt in node.then_body:
                stmt.accept(self)
            self.exit_scope()
            
            if node.else_clause:
                node.else_clause.accept(self)
                
        except Exception as e:
            self.error(f"Error in if statement: {e}", node, "if statement")
        
        return None
    
    def visit_else_clause(self, node: ElseClauseNode):
        try:
            if node.else_if:
                node.else_if.accept(self)
            elif node.else_body:
                else_scope = self.enter_scope("else")
                for stmt in node.else_body:
                    stmt.accept(self)
                self.exit_scope()
        except Exception as e:
            self.error(f"Error in else clause: {e}", node, "else clause")
        return None
    
    def visit_while_stmt(self, node: WhileStmtNode):
        try:
            node.condition.accept(self)
            
            while_scope = self.enter_scope("while")
            for stmt in node.body:
                stmt.accept(self)
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in while statement: {e}", node, "while statement")
        
        return None
    
    def visit_do_while_stmt(self, node: DoWhileStmtNode):
        try:
            do_scope = self.enter_scope("do")
            for stmt in node.body:
                stmt.accept(self)
            self.exit_scope()
            
            node.condition.accept(self)
        except Exception as e:
            self.error(f"Error in do-while statement: {e}", node, "do-while statement")
        return None
    
    def visit_for_stmt(self, node: ForStmtNode):
        try:
            for_scope = self.enter_scope("for")
            
            if node.iterator_var and node.iterable:
                iterable_type = node.iterable.accept(self)
                
                if isinstance(iterable_type, ArrayType):
                    iter_type = iterable_type.element_type
                else:
                    iter_type = PrimitiveType("int")
                
                iter_symbol = Symbol(node.iterator_var, iter_type, node)
                try:
                    for_scope.define(iter_symbol)
                except TypeCheckError as e:
                    self.error(e.message, node, f"iterator variable")
            else:
                if node.for_init:
                    node.for_init.accept(self)
                if node.condition:
                    node.condition.accept(self)
                if node.for_update:
                    node.for_update.accept(self)
            
            for stmt in node.body:
                stmt.accept(self)
            
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in for statement: {e}", node, "for statement")
        
        return None
    
    def visit_for_init(self, node: ForInitNode):
        try:
            if node.variable_decl:
                node.variable_decl.accept(self)
            elif node.expression:
                node.expression.accept(self)
        except Exception as e:
            self.error(f"Error in for init: {e}", node, "for init")
        return None
    
    def visit_for_update(self, node: ForUpdateNode):
        try:
            if node.expression:
                node.expression.accept(self)
        except Exception as e:
            self.error(f"Error in for update: {e}", node, "for update")
        return None
    
    def visit_switch_stmt(self, node: SwitchStmtNode):
        try:
            switch_expr_type = node.expression.accept(self)
            
            switch_scope = self.enter_scope("switch")
            
            for case in node.cases:
                case.accept(self)
            
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in switch statement: {e}", node, "switch statement")
        return None
    
    def visit_case_item(self, node: CaseItemNode):
        try:
            if node.case_value:
                case_type = node.case_value.accept(self)
            
            case_scope = self.enter_scope("case")
            for stmt in node.body:
                stmt.accept(self)
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in case item: {e}", node, "case item")
        
        return None
    
    def visit_break_stmt(self, node: BreakStmtNode):
        return None
    
    def visit_continue_stmt(self, node: ContinueStmtNode):
        return None
    
    def visit_return_stmt(self, node: ReturnStmtNode):
        try:
            if node.expression:
                return_type = node.expression.accept(self)
                if self.current_function_return_type:
                    if not self.current_function_return_type.is_compatible_with(return_type):
                        if not return_type.can_cast_to(self.current_function_return_type):
                            self.error(f"Cannot return type '{return_type}' from function expecting '{self.current_function_return_type}'", node, "return type mismatch")
            elif self.current_function_return_type and self.current_function_return_type.name != "void":
                self.error(f"Function must return a value of type '{self.current_function_return_type}'", node, "missing return value")
        except Exception as e:
            self.error(f"Error in return statement: {e}", node, "return statement")
        
        return None
    
    def visit_try_catch_stmt(self, node: TryCatchStmtNode):
        try:
            try_scope = self.enter_scope("try")
            for stmt in node.try_body:
                stmt.accept(self)
            self.exit_scope()
            
            for catch_clause in node.catch_clauses:
                catch_clause.accept(self)
        except Exception as e:
            self.error(f"Error in try-catch statement: {e}", node, "try-catch")
        
        return None
    
    def visit_catch_clause(self, node: CatchClauseNode):
        try:
            catch_scope = self.enter_scope("catch")
            
            param_type = self.resolve_type_spec(node.parameter.type_spec)
            param_symbol = Symbol(node.parameter.name, param_type, node.parameter)
            try:
                catch_scope.define(param_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, "catch parameter")
            
            for stmt in node.body:
                stmt.accept(self)
            
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in catch clause: {e}", node, "catch clause")
        return None
    
    def visit_throw_stmt(self, node: ThrowStmtNode):
        try:
            node.expression.accept(self)
        except Exception as e:
            self.error(f"Error in throw statement: {e}", node, "throw statement")
        return None
    
    def visit_assert_stmt(self, node: AssertStmtNode):
        try:
            node.expression.accept(self)
            if node.message:
                node.message.accept(self)
        except Exception as e:
            self.error(f"Error in assert statement: {e}", node, "assert statement")
        return None
    
    def visit_asm_stmt(self, node: AsmStmtNode):
        return None
    
    def visit_block_stmt(self, node: BlockStmtNode):
        try:
            block_scope = self.enter_scope("block")
            for stmt in node.statements:
                stmt.accept(self)
            self.exit_scope()
        except Exception as e:
            self.error(f"Error in block statement: {e}", node, "block statement")
        return None
    
    def visit_assignment_expr(self, node: AssignmentExprNode):
        try:
            left_type = node.left.accept(self)
            right_type = node.right.accept(self)
            
            if left_type and right_type:
                if not left_type.is_compatible_with(right_type):
                    if not right_type.can_cast_to(left_type):
                        self.error(f"Cannot assign type '{right_type}' to type '{left_type}'", node, "assignment")
            
            return left_type
        except Exception as e:
            self.error(f"Error in assignment: {e}", node, "assignment expression")
            return DataType(32)
    
    def visit_conditional_expr(self, node: ConditionalExprNode):
        try:
            condition_type = node.condition.accept(self)
            true_type = node.true_expr.accept(self)
            false_type = node.false_expr.accept(self)
            
            if true_type and false_type:
                if true_type.is_compatible_with(false_type):
                    return true_type
                else:
                    return DataType(32)
            
            return true_type or false_type or DataType(32)
        except Exception as e:
            self.error(f"Error in conditional expression: {e}", node, "conditional expression")
            return DataType(32)
    
    def visit_binary_op_expr(self, node: BinaryOpExprNode):
        try:
            left_type = node.left.accept(self)
            right_type = node.right.accept(self)
            
            if node.operator in ["EQ", "NEQ", "LT", "GT", "LTEQ", "GTEQ"]:
                return DataType(1)
            elif node.operator in ["AND", "OR", "XOR", "NAND", "NOR"]:
                return DataType(1)
            elif node.operator in ["ADDITION", "SUBTRACTION", "MULTIPLICATION", "DIVISION", "MODULO"]:
                if left_type and right_type:
                    if isinstance(left_type, PrimitiveType) and isinstance(right_type, PrimitiveType):
                        if left_type.name == "float" or right_type.name == "float":
                            return PrimitiveType("float")
                        else:
                            return PrimitiveType("int")
                return left_type or right_type or PrimitiveType("int")
            else:
                return left_type or right_type or DataType(32)
        except Exception as e:
            self.error(f"Error in binary operation: {e}", node, "binary operation")
            return DataType(32)
    
    def visit_unary_op_expr(self, node: UnaryOpExprNode):
        try:
            operand_type = node.operand.accept(self)
            
            if node.operator in ["NOT", "KW_NOT"]:
                return DataType(1)
            elif node.operator == "ADDRESS_OF":
                if operand_type:
                    return PointerType(operand_type)
                return PointerType(DataType(32))
            elif node.operator == "MULTIPLICATION":
                if isinstance(operand_type, PointerType):
                    return operand_type.base_type
                return DataType(32)
            elif node.operator in ["KW_SIZEOF", "KW_TYPEOF"]:
                return PrimitiveType("int")
            else:
                return operand_type
        except Exception as e:
            self.error(f"Error in unary operation: {e}", node, "unary operation")
            return DataType(32)
    
    def visit_cast_expr(self, node: CastExprNode):
        try:
            expr_type = node.expression.accept(self)
            target_type = self.resolve_type_spec(node.target_type)
            
            if expr_type and not expr_type.can_cast_to(target_type):
                self.error(f"Cannot cast type '{expr_type}' to '{target_type}'", node, "cast expression")
            
            return target_type
        except Exception as e:
            self.error(f"Error in cast expression: {e}", node, "cast expression")
            return DataType(32)
    
    def visit_postfix_expr(self, node: PostfixExprNode):
        try:
            base_type = node.expression.accept(self)
            
            if node.operator == "[":
                if isinstance(base_type, ArrayType):
                    return base_type.element_type
                elif isinstance(base_type, PointerType):
                    return base_type.base_type
                return DataType(32)
            elif node.operator == "(":
                if isinstance(base_type, FunctionType):
                    return base_type.return_type
                return DataType(32)
            elif node.operator in [".", "::"]:
                return DataType(32)
            else:
                return base_type
        except Exception as e:
            self.error(f"Error in postfix expression: {e}", node, "postfix expression")
            return DataType(32)
    
    def visit_array_access_expr(self, node: ArrayAccessExprNode):
        try:
            array_type = node.array.accept(self)
            index_type = node.index.accept(self)
            
            if isinstance(array_type, ArrayType):
                return array_type.element_type
            elif isinstance(array_type, PointerType):
                return array_type.base_type
            
            return DataType(32)
        except Exception as e:
            self.error(f"Error in array access: {e}", node, "array access")
            return DataType(32)
    
    def visit_range_expr(self, node: RangeExprNode):
        try:
            start_type = node.start.accept(self)
            end_type = node.end.accept(self)
            
            if start_type and end_type:
                if isinstance(start_type, PrimitiveType) and isinstance(end_type, PrimitiveType):
                    return ArrayType(start_type)
            
            return ArrayType(PrimitiveType("int"))
        except Exception as e:
            self.error(f"Error in range expression: {e}", node, "range expression")
            return ArrayType(PrimitiveType("int"))
    
    def visit_function_call_expr(self, node: FunctionCallExprNode):
        try:
            func_type = node.function.accept(self)
            
            for arg in node.arguments:
                arg.accept(self)
            
            if isinstance(func_type, FunctionType):
                return func_type.return_type
            
            return DataType(32)
        except Exception as e:
            self.error(f"Error in function call: {e}", node, "function call")
            return DataType(32)
    
    def visit_member_access_expr(self, node: MemberAccessExprNode):
        try:
            object_type = node.object_expr.accept(self)
            return DataType(32)
        except Exception as e:
            self.error(f"Error in member access: {e}", node, "member access")
            return DataType(32)
    
    def visit_identifier_expr(self, node: IdentifierExprNode):
        try:
            symbol = self.current_scope.lookup(node.name)
            if symbol:
                return symbol.type
            else:
                self.error(f"Undefined identifier '{node.name}'", node, f"identifier lookup")
                return DataType(32)
        except Exception as e:
            self.error(f"Error looking up identifier '{node.name}': {e}", node, "identifier lookup")
            return DataType(32)
    
    def visit_literal_expr(self, node: LiteralExprNode):
        try:
            if node.literal_type == "int":
                return PrimitiveType("int")
            elif node.literal_type == "float":
                return PrimitiveType("float")
            elif node.literal_type == "char":
                return PrimitiveType("char")
            elif node.literal_type == "string":
                return ArrayType(PrimitiveType("char"))
            elif node.literal_type == "data":
                return DataType(32)
            elif node.literal_type == "type":
                return DataType(32)
            else:
                return DataType(32)
        except Exception as e:
            self.error(f"Error in literal: {e}", node, "literal expression")
            return DataType(32)
    
    def visit_array_literal_expr(self, node: ArrayLiteralExprNode):
        try:
            if node.elements:
                element_type = node.elements[0].accept(self)
                for elem in node.elements[1:]:
                    elem.accept(self)
                return ArrayType(element_type or DataType(32))
            return ArrayType(DataType(32))
        except Exception as e:
            self.error(f"Error in array literal: {e}", node, "array literal")
            return ArrayType(DataType(32))
    
    def visit_array_comprehension_expr(self, node: ArrayComprehensionExprNode):
        try:
            iterable_type = node.iterable.accept(self)
            
            comp_scope = self.enter_scope("comprehension")
            
            if isinstance(iterable_type, ArrayType):
                iter_type = iterable_type.element_type
            else:
                iter_type = PrimitiveType("int")
            
            iter_symbol = Symbol(node.iterator_var, iter_type, node)
            try:
                comp_scope.define(iter_symbol)
            except TypeCheckError as e:
                self.error(e.message, node, "comprehension iterator")
            
            element_type = node.element_expr.accept(self)
            
            if node.condition:
                node.condition.accept(self)
            
            self.exit_scope()
            
            return ArrayType(element_type or DataType(32))
        except Exception as e:
            self.error(f"Error in array comprehension: {e}", node, "array comprehension")
            return ArrayType(DataType(32))
    
    def visit_istring_expr(self, node: IStringExprNode):
        try:
            for expr in node.expressions:
                expr.accept(self)
            return ArrayType(PrimitiveType("char"))
        except Exception as e:
            self.error(f"Error in i-string: {e}", node, "i-string")
            return ArrayType(PrimitiveType("char"))
    
    def visit_template_instantiation_expr(self, node: TemplateInstantiationExprNode):
        try:
            type_args = [self.resolve_type_spec(arg) for arg in node.type_args]
            return TemplateType(node.template_name, type_args)
        except Exception as e:
            self.error(f"Error in template instantiation: {e}", node, "template instantiation")
            return DataType(32)
    
    def visit_this_expr(self, node: ThisExprNode):
        try:
            if self.current_object:
                return self.current_object
            else:
                self.error("'this' used outside of object context", node, "this expression")
                return DataType(32)
        except Exception as e:
            self.error(f"Error in this expression: {e}", node, "this expression")
            return DataType(32)
    
    def visit_iterator_expr(self, node: IteratorExprNode):
        return DataType(32)


def type_check_flux_program(source_code: str) -> Tuple[ProgramNode, List[TypeCheckError]]:
    """Parse and type check a Flux program"""
    print("DEBUG: Starting parse...")
    ast = parse(source_code)
    print("DEBUG: Parse completed, starting type check...")
    type_checker = TypeChecker()
    errors = type_checker.type_check(ast)
    return ast, errors


if __name__ == "__main__":
    try:
        with open("./master_example2.fx", "r") as file:
            source_code = file.read()
        
        print("DEBUG: Reading source file completed")
        ast, errors = type_check_flux_program(source_code)
        
        if errors:
            print(f"Type checking found {len(errors)} errors:")
            for error in errors:
                print(f"  {error}")
        else:
            print("Type checking completed successfully with no errors!")
            
    except FileNotFoundError:
        print("Error: master_example2.fx file not found")
    except Exception as e:
        import traceback
        print(f"Unexpected error: {e}")
        print("Traceback:")
        traceback.print_exc()