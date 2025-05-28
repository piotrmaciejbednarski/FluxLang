from typing import Dict, List, Optional, Union, Set, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum
from flexer import Token, TokenType
from fast import *

# Type system representation
class TypeKind(Enum):
    VOID = "void"
    AUTO = "auto" 
    DATA = "data"
    INTEGER = "integer"
    FLOAT = "float"
    BOOLEAN = "boolean"
    STRING = "string"
    ARRAY = "array"
    POINTER = "pointer"
    FUNCTION = "function"
    OBJECT = "object"
    STRUCT = "struct"
    TEMPLATE = "template"
    ENUM = "enum"
    THIS = "this"
    MODULE = "module"

@dataclass
class FluxType:
    """Base class for all types in Flux"""
    kind: TypeKind
    name: str
    size: Optional[int] = None
    signed: Optional[bool] = None
    qualifiers: Set[str] = field(default_factory=set)
    
    def __str__(self):
        return self.name
    
    def __repr__(self):
        return f"{self.__class__.__name__}({self.name})"

@dataclass  
class DataType(FluxType):
    """data{n} type"""
    bit_width: int = 0
    
    def __post_init__(self):
        self.kind = TypeKind.DATA
        self.size = self.bit_width
        if not self.name.startswith("data"):
            self.name = f"data{{{self.bit_width}}}"

@dataclass
class ArrayType(FluxType):
    """Array type"""
    element_type: FluxType = None
    array_size: Optional[int] = None
    
    def __post_init__(self):
        self.kind = TypeKind.ARRAY
        if self.element_type and not self.name.endswith("[]"):
            self.name = f"{self.element_type.name}[]"

@dataclass
class PointerType(FluxType):
    """Pointer type"""
    pointee_type: FluxType = None
    function_params: Optional[List[FluxType]] = None
    
    def __post_init__(self):
        self.kind = TypeKind.POINTER
        if self.pointee_type and not self.name.endswith("*"):
            if self.function_params is not None:
                param_str = ",".join(p.name for p in self.function_params)
                self.name = f"{self.pointee_type.name}*({param_str})"
            else:
                self.name = f"{self.pointee_type.name}*"

@dataclass
class FunctionType(FluxType):
    """Function type"""
    return_type: FluxType = None
    parameter_types: List[FluxType] = field(default_factory=list)
    never_returns: bool = False
    
    def __post_init__(self):
        self.kind = TypeKind.FUNCTION
        param_str = ",".join(p.name for p in self.parameter_types)
        never_str = "!" if self.never_returns else ""
        return_name = self.return_type.name if self.return_type else "void"
        self.name = f"def({param_str}) -> {never_str}{return_name}"

@dataclass
class ObjectType(FluxType):
    """Object type"""
    members: Dict[str, FluxType] = field(default_factory=dict)
    methods: Dict[str, FunctionType] = field(default_factory=dict)
    parent_types: List['ObjectType'] = field(default_factory=list)
    template_params: Optional[List[str]] = None
    
    def __post_init__(self):
        self.kind = TypeKind.OBJECT

@dataclass
class StructType(FluxType):
    """Struct type"""
    members: Dict[str, FluxType] = field(default_factory=dict)
    template_params: Optional[List[str]] = None
    
    def __post_init__(self):
        self.kind = TypeKind.STRUCT

@dataclass
class TemplateType(FluxType):
    """Template type"""
    base_type: FluxType = None
    arguments: List[FluxType] = field(default_factory=list)
    
    def __post_init__(self):
        self.kind = TypeKind.TEMPLATE
        arg_str = ",".join(a.name for a in self.arguments)
        base_name = self.base_type.name if self.base_type else "unknown"
        self.name = f"{base_name}<{arg_str}>"

@dataclass
class EnumType(FluxType):
    """Enum type"""
    members: Dict[str, Optional[List[FluxType]]] = field(default_factory=dict)
    
    def __post_init__(self):
        self.kind = TypeKind.ENUM

# Symbol table
@dataclass
class Symbol:
    """Symbol in symbol table"""
    name: str
    type: FluxType
    kind: str  # variable, function, object, type, parameter, etc.
    token: Optional[Token]
    scope_level: int
    is_mutable: bool = True
    is_initialized: bool = False

class Scope:
    """Represents a scope (namespace, function, object, etc.)"""
    def __init__(self, name: str, scope_type: str = "block", parent: Optional['Scope'] = None):
        self.name = name
        self.scope_type = scope_type  # global, namespace, function, object, struct, block
        self.parent = parent
        self.symbols: Dict[str, Symbol] = {}
        self.children: List['Scope'] = []
        
    def lookup_local(self, name: str) -> Optional[Symbol]:
        """Look up symbol only in this scope"""
        return self.symbols.get(name)
        
    def lookup(self, name: str) -> Optional[Symbol]:
        """Look up symbol in this scope and parent scopes"""
        if name in self.symbols:
            return self.symbols[name]
        if self.parent:
            return self.parent.lookup(name)
        return None
    
    def define(self, symbol: Symbol):
        """Define symbol in this scope"""
        self.symbols[symbol.name] = symbol
    
    def get_qualified_name(self) -> str:
        """Get fully qualified name of this scope"""
        if self.parent and self.parent.name != "global":
            return f"{self.parent.get_qualified_name()}::{self.name}"
        return self.name

class SymbolTable:
    """Symbol table with scope management"""
    def __init__(self):
        self.global_scope = Scope("global", "global")
        self.current_scope = self.global_scope
        self.scope_stack = [self.global_scope]
        
    def enter_scope(self, name: str, scope_type: str = "block") -> Scope:
        """Enter a new scope"""
        new_scope = Scope(name, scope_type, self.current_scope)
        self.current_scope.children.append(new_scope)
        self.current_scope = new_scope
        self.scope_stack.append(new_scope)
        return new_scope
    
    def exit_scope(self):
        """Exit current scope"""
        if len(self.scope_stack) > 1:
            self.scope_stack.pop()
            self.current_scope = self.scope_stack[-1]
    
    def lookup(self, name: str) -> Optional[Symbol]:
        """Look up symbol"""
        return self.current_scope.lookup(name)
    
    def lookup_local(self, name: str) -> Optional[Symbol]:
        """Look up symbol only in current scope"""
        return self.current_scope.lookup_local(name)
    
    def define(self, symbol: Symbol):
        """Define symbol in current scope"""
        symbol.scope_level = len(self.scope_stack) - 1
        self.current_scope.define(symbol)
    
    def lookup_qualified(self, qualified_name: str) -> Optional[Symbol]:
        """Look up qualified name like namespace::member"""
        parts = qualified_name.split("::")
        if len(parts) == 1:
            return self.lookup(parts[0])
        
        # Navigate to the right scope
        scope = self.global_scope
        for part in parts[:-1]:
            found = False
            for child in scope.children:
                if child.name == part:
                    scope = child
                    found = True
                    break
            if not found:
                return None
        
        # Look up the final symbol
        return scope.symbols.get(parts[-1])

# Type checking errors
class TypeCheckError(Exception):
    def __init__(self, message: str, token: Optional[Token] = None):
        self.message = message
        self.token = token
        if token:
            super().__init__(f"Type error at line {token.line}, column {token.column}: {message}")
        else:
            super().__init__(f"Type error: {message}")

# Main type checker
class FluxTypeChecker(ASTVisitor):
    """Type checker for Flux language"""
    
    def __init__(self):
        self.symbol_table = SymbolTable()
        self.errors: List[TypeCheckError] = []
        self.current_function_return_type: Optional[FluxType] = None
        self.current_object_type: Optional[ObjectType] = None
        self.template_instantiations: Dict[str, FluxType] = {}
        
        # Initialize built-in types
        self._init_builtin_types()
    
    def _init_builtin_types(self):
        """Initialize built-in types from Flux specification"""
        # Only data and fundamental types - everything else should be user-defined
        builtin_types = [
            # Basic types
            FluxType(TypeKind.VOID, "void"),
            FluxType(TypeKind.AUTO, "auto"),
            FluxType(TypeKind.THIS, "this"),
            # Only data is truly built-in - everything else built from data
            FluxType(TypeKind.DATA, "data"),
        ]
        
        # Define fundamental types in global scope
        for flux_type in builtin_types:
            symbol = Symbol(flux_type.name, flux_type, "type", None, 0, False, True)
            self.symbol_table.define(symbol)
    
    def check(self, ast: Program) -> List[TypeCheckError]:
        """Check types in the AST"""
        self.errors.clear()
        try:
            self.visit(ast)
        except Exception as e:
            self.errors.append(TypeCheckError(f"Internal type checker error: {e}"))
        
        return self.errors
    
    def visit(self, node: ASTNode) -> Optional[FluxType]:
        """Visit an AST node and return its type"""
        if node is None:
            return None
            
        method_name = f"visit_{node.__class__.__name__}"
        visitor = getattr(self, method_name, self.generic_visit)
        return visitor(node)
    
    def generic_visit(self, node: ASTNode) -> Optional[FluxType]:
        """Default visitor - visits children"""
        for field_name, field_value in node.__dict__.items():
            if isinstance(field_value, ASTNode):
                self.visit(field_value)
            elif isinstance(field_value, list):
                for item in field_value:
                    if isinstance(item, ASTNode):
                        self.visit(item)
        return None
    
    def error(self, message: str, token: Optional[Token] = None):
        """Record a type error"""
        self.errors.append(TypeCheckError(message, token))
    
    # Visitor methods for specific node types
    
    def visit_Program(self, node: Program) -> Optional[FluxType]:
        """Visit program node"""
        return self.visit(node.translation_unit)
    
    def visit_TranslationUnit(self, node: TranslationUnit) -> Optional[FluxType]:
        """Visit translation unit"""
        for decl in node.external_declarations:
            self.visit(decl)
        return None
    
    def visit_ImportStatement(self, node: ImportStatement) -> Optional[FluxType]:
        """Visit import statement"""
        # For now, just create a placeholder module type
        # Real implementation would load actual files
        module_type = FluxType(TypeKind.MODULE, f"module_{node.alias}")
        symbol = Symbol(node.alias, module_type, "module", node.token, 0, False, True)
        self.symbol_table.define(symbol)
        return None
    
    def visit_UsingStatement(self, node: UsingStatement) -> Optional[FluxType]:
        """Visit using statement"""
        for qualified_id in node.qualified_ids:
            # Look up the qualified identifier and bring it into current scope
            name = "::".join(qualified_id.identifiers)
            
            # First try to find the qualified symbol
            symbol = self.symbol_table.lookup_qualified(name)
            if symbol:
                # Add to current scope with unqualified name
                local_name = qualified_id.identifiers[-1]
                local_symbol = Symbol(local_name, symbol.type, symbol.kind, 
                                    qualified_id.tokens[-1] if qualified_id.tokens else None, 
                                    symbol.scope_level, symbol.is_mutable, symbol.is_initialized)
                self.symbol_table.define(local_symbol)
            else:
                # Try to find the namespace and import all its symbols
                if len(qualified_id.identifiers) >= 2:
                    namespace_path = "::".join(qualified_id.identifiers)
                    self._import_namespace_symbols(namespace_path, qualified_id.tokens[0] if qualified_id.tokens else None)
                # If not found, silently ignore (modules may not be loaded yet)
        return None
    
    def _import_namespace_symbols(self, namespace_path: str, token: Optional[Token]):
        """Import all symbols from a namespace into current scope"""
        parts = namespace_path.split("::")
        
        # Navigate to the namespace scope
        scope = self.symbol_table.global_scope
        for part in parts:
            found = False
            for child in scope.children:
                if child.name == part:
                    scope = child
                    found = True
                    break
            if not found:
                # Silently ignore missing namespaces (modules may not be loaded)
                return
        
        # Import all symbols from the namespace
        for symbol_name, symbol in scope.symbols.items():
            # Don't overwrite existing symbols
            if not self.symbol_table.lookup_local(symbol_name):
                local_symbol = Symbol(symbol_name, symbol.type, symbol.kind, 
                                    token, symbol.scope_level, symbol.is_mutable, symbol.is_initialized)
                self.symbol_table.define(local_symbol)
    
    def visit_FunctionDefinition(self, node: FunctionDefinition) -> Optional[FluxType]:
        """Visit function definition"""
        # Build parameter types
        param_types = []
        for param in node.parameters:
            param_type = self.visit(param.type_spec)
            if param_type:
                param_types.append(param_type)
        
        # Get return type
        return_type = self.visit(node.return_type.type_spec)
        if not return_type:
            return_type = FluxType(TypeKind.VOID, "void")
        
        # Create function type
        func_type = FunctionType(
            kind=TypeKind.FUNCTION,
            name=node.name,
            return_type=return_type,
            parameter_types=param_types,
            never_returns=node.return_type.never_returns
        )
        
        # Check for function overloading
        existing_symbol = self.symbol_table.lookup_local(node.name)
        if existing_symbol and existing_symbol.kind == "function":
            # Allow overloading with different parameter types
            if not self._can_overload_function(existing_symbol.type, func_type):
                self.error(f"Function '{node.name}' already defined with incompatible signature", node.token)
        
        # Define function in current scope
        symbol = Symbol(node.name, func_type, "function", node.token, 0, False, True)
        self.symbol_table.define(symbol)
        
        # Enter function scope for body checking
        self.symbol_table.enter_scope(f"function_{node.name}", "function")
        self.current_function_return_type = return_type
        
        # Add parameters to function scope
        for param in node.parameters:
            param_type = self.visit(param.type_spec)
            if param_type:
                param_symbol = Symbol(param.name, param_type, "parameter", param.token, 0, True, True)
                self.symbol_table.define(param_symbol)
        
        # Check function body
        self.visit(node.body)
        
        # Exit function scope
        self.current_function_return_type = None
        self.symbol_table.exit_scope()
        
        return func_type
    
    def visit_ObjectDefinition(self, node: ObjectDefinition) -> Optional[FluxType]:
        """Visit object definition"""
        # Create object type
        obj_type = ObjectType(kind=TypeKind.OBJECT, name=node.name)
        
        # Handle inheritance
        if node.inheritance:
            for spec in node.inheritance.specifiers:
                parent_name = "::".join(spec.qualified_id.identifiers)
                parent_symbol = self.symbol_table.lookup_qualified(parent_name)
                if parent_symbol and isinstance(parent_symbol.type, ObjectType):
                    if not spec.excluded:
                        obj_type.parent_types.append(parent_symbol.type)
                        # Inherit members and methods
                        obj_type.members.update(parent_symbol.type.members)
                        obj_type.methods.update(parent_symbol.type.methods)
                else:
                    self.error(f"Unknown parent type: {parent_name}", 
                              spec.qualified_id.tokens[0] if spec.qualified_id.tokens else None)
        
        # Define object in current scope first
        symbol = Symbol(node.name, obj_type, "object", node.token, 0, False, True)
        self.symbol_table.define(symbol)
        
        # Enter object scope
        self.symbol_table.enter_scope(f"object_{node.name}", "object")
        self.current_object_type = obj_type
        
        # Process members
        for member in node.members:
            if isinstance(member, FunctionDefinition):
                member_type = self.visit(member)
                if member_type and isinstance(member_type, FunctionType):
                    obj_type.methods[member.name] = member_type
            elif isinstance(member, VariableDeclaration):
                # Process variable declaration and add to object members
                member_type = self.visit(member.type_spec)
                if member_type:
                    for declarator in member.declarators:
                        # Handle array and pointer declarators
                        final_type = member_type
                        if declarator.array_size:
                            final_type = ArrayType(kind=TypeKind.ARRAY, name=f"{member_type.name}[]", 
                                                 element_type=member_type)
                        if declarator.is_pointer:
                            final_type = PointerType(kind=TypeKind.POINTER, name=f"{final_type.name}*",
                                                   pointee_type=final_type)
                        
                        obj_type.members[declarator.name] = final_type
                        
                        # Also define in object scope for access checking
                        member_symbol = Symbol(declarator.name, final_type, "member", 
                                             declarator.token, 0, True, False)
                        self.symbol_table.define(member_symbol)
            elif isinstance(member, ObjectDefinition):
                # Nested object
                member_type = self.visit(member)
                if member_type:
                    obj_type.members[member.name] = member_type
            elif isinstance(member, StructDefinition):
                # Nested struct
                member_type = self.visit(member)
                if member_type:
                    obj_type.members[member.name] = member_type
        
        # Exit object scope
        self.current_object_type = None
        self.symbol_table.exit_scope()
        
        return obj_type
    
    def visit_StructDefinition(self, node: StructDefinition) -> Optional[FluxType]:
        """Visit struct definition"""
        struct_type = StructType(kind=TypeKind.STRUCT, name=node.name)
        
        # Define struct in current scope
        symbol = Symbol(node.name, struct_type, "struct", node.token, 0, False, True)
        self.symbol_table.define(symbol)
        
        # Enter struct scope
        self.symbol_table.enter_scope(f"struct_{node.name}", "struct")
        
        # Process members
        for member in node.members:
            if isinstance(member, VariableDeclaration):
                member_type = self.visit(member.type_spec)
                if member_type:
                    for declarator in member.declarators:
                        struct_type.members[declarator.name] = member_type
                        # Also define in scope for access
                        member_symbol = Symbol(declarator.name, member_type, "member", 
                                             declarator.token, 0, True, False)
                        self.symbol_table.define(member_symbol)
            elif isinstance(member, StructDefinition):
                # Nested struct
                nested_type = self.visit(member)
                if nested_type:
                    struct_type.members[member.name] = nested_type
        
        # Exit struct scope
        self.symbol_table.exit_scope()
        
        return struct_type
    
    def visit_NamespaceDefinition(self, node: NamespaceDefinition) -> Optional[FluxType]:
        """Visit namespace definition"""
        # Enter namespace scope
        self.symbol_table.enter_scope(node.name, "namespace")
        
        # Process members
        for member in node.members:
            self.visit(member)
        
        # Exit namespace scope
        self.symbol_table.exit_scope()
        
        return None
    
    def visit_EnumDefinition(self, node: EnumDefinition) -> Optional[FluxType]:
        """Visit enum definition"""
        enum_type = EnumType(kind=TypeKind.ENUM, name=node.name)
        
        # Process enum members
        for member in node.members:
            enum_type.members[member.name] = member.associated_types
        
        # Define enum in current scope
        symbol = Symbol(node.name, enum_type, "enum", node.token, 0, False, True)
        self.symbol_table.define(symbol)
        
        return enum_type
    
    def visit_VariableDeclaration(self, node: VariableDeclaration) -> Optional[FluxType]:
        """Visit variable declaration"""
        var_type = self.visit(node.type_spec)
        if not var_type:
            return None
        
        for declarator in node.declarators:
            # Handle array declarators
            final_type = var_type
            if declarator.array_size:
                array_size_type = self.visit(declarator.array_size)
                if array_size_type and not self.is_integer_type(array_size_type):
                    self.error("Array size must be integer", declarator.token)
                final_type = ArrayType(kind=TypeKind.ARRAY, name=f"{var_type.name}[]", 
                                     element_type=var_type)
            
            # Handle pointer declarators
            if declarator.is_pointer:
                final_type = PointerType(kind=TypeKind.POINTER, name=f"{final_type.name}*",
                                       pointee_type=final_type)
            
            # Handle function pointer declarators
            if declarator.function_params is not None:
                param_types = []
                for param in declarator.function_params:
                    param_type = self.visit(param.type_spec)
                    if param_type:
                        param_types.append(param_type)
                
                final_type = PointerType(kind=TypeKind.POINTER, name=f"{final_type.name}*",
                                       pointee_type=final_type, function_params=param_types)
            
            is_initialized = False
            # Check initializer
            if declarator.initializer:
                init_type = self.visit(declarator.initializer.value)
                if init_type:
                    if not self.types_compatible(final_type, init_type):
                        self.error(f"Cannot initialize variable '{declarator.name}' of type '{final_type.name}' with value of type '{init_type.name}'",
                                  declarator.token)
                    is_initialized = True
            
            # Check if already defined in current scope
            existing = self.symbol_table.lookup_local(declarator.name)
            if existing:
                self.error(f"Variable '{declarator.name}' already defined in this scope", declarator.token)
            
            # Special case: Type alias detection
            # If this is a global scope declaration with no initializer,
            # treat it as a type alias (like typedef in C)
            if (self.symbol_table.current_scope == self.symbol_table.global_scope and 
                not declarator.initializer):
                
                # Debug output
                if hasattr(self, '_debug') and self._debug:
                    print(f"Creating type alias: {declarator.name} -> {final_type}")
                
                # Create a type alias - copy the type with the new name
                if isinstance(final_type, DataType):
                    type_alias = DataType(kind=final_type.kind, name=declarator.name, 
                                        bit_width=final_type.bit_width, 
                                        signed=final_type.signed)
                elif isinstance(final_type, ArrayType):
                    type_alias = ArrayType(kind=final_type.kind, name=declarator.name,
                                         element_type=final_type.element_type,
                                         array_size=final_type.array_size)
                else:
                    type_alias = FluxType(final_type.kind, declarator.name, 
                                        size=getattr(final_type, 'size', None),
                                        signed=getattr(final_type, 'signed', None),
                                        qualifiers=getattr(final_type, 'qualifiers', set()))
                
                type_symbol = Symbol(declarator.name, type_alias, "type", declarator.token, 0, False, True)
                self.symbol_table.define(type_symbol)
                
                # Debug output
                if hasattr(self, '_debug') and self._debug:
                    print(f"Defined type alias symbol: {type_symbol.name} of kind {type_symbol.kind}")
                    
            else:
                # Regular variable
                is_const = node.storage_class == StorageClass.CONST if node.storage_class else False
                symbol = Symbol(declarator.name, final_type, "variable", declarator.token, 0, 
                              not is_const, is_initialized)
                self.symbol_table.define(symbol)
        
        return var_type
    
    def visit_SimpleTypeSpecifier(self, node: SimpleTypeSpecifier) -> Optional[FluxType]:
        """Visit simple type specifier"""
        if isinstance(node.name, str):
            # Handle special 'this' type
            if node.name == "this":
                if self.current_object_type:
                    return self.current_object_type
                else:
                    self.error("'this' type used outside object context", node.token)
                    return None
            
            # Handle member types like Object.NestedType
            if "." in node.name:
                return self._resolve_member_type(node.name, node.token)
            
            symbol = self.symbol_table.lookup(node.name)
            if symbol and symbol.kind in ["type", "object", "struct", "enum"]:
                return symbol.type
            else:
                self.error(f"Unknown type: {node.name}", node.token)
                return None
                
        elif isinstance(node.name, QualifiedId):
            qualified_name = "::".join(node.name.identifiers)
            symbol = self.symbol_table.lookup_qualified(qualified_name)
            if symbol and symbol.kind in ["type", "object", "struct", "enum"]:
                return symbol.type
            else:
                self.error(f"Unknown type: {qualified_name}", node.token)
                return None
        else:
            self.error(f"Invalid type specifier: {node.name}", node.token)
            return None
    
    def visit_ArrayTypeSpecifier(self, node: ArrayTypeSpecifier) -> Optional[FluxType]:
        """Visit array type specifier"""
        element_type = self.visit(node.element_type)
        if not element_type:
            return None
        
        array_size = None
        if node.size:
            size_type = self.visit(node.size)
            if size_type and not self.is_integer_type(size_type):
                self.error("Array size must be integer constant", node.token)
        
        return ArrayType(kind=TypeKind.ARRAY, name=f"{element_type.name}[]", 
                        element_type=element_type, array_size=array_size)
    
    def visit_PointerTypeSpecifier(self, node: PointerTypeSpecifier) -> Optional[FluxType]:
        """Visit pointer type specifier"""
        pointee_type = self.visit(node.base_type)
        if not pointee_type:
            return None
        
        function_params = None
        if node.function_params:
            function_params = []
            for param in node.function_params:
                param_type = self.visit(param.type_spec)
                if param_type:
                    function_params.append(param_type)
        
        return PointerType(kind=TypeKind.POINTER, name=f"{pointee_type.name}*",
                          pointee_type=pointee_type, function_params=function_params)
    
    def visit_DataTypeSpecifier(self, node: DataTypeSpecifier) -> Optional[FluxType]:
        """Visit data type specifier"""
        # Try to evaluate the size expression if it's a literal
        bit_width = 32  # Default
        if isinstance(node.size, IntegerLiteral):
            try:
                bit_width = int(node.size.value)
            except ValueError:
                bit_width = 32
        
        signed = True
        if node.signedness == "unsigned":
            signed = False
        elif node.signedness == "signed":
            signed = True
        
        return DataType(kind=TypeKind.DATA, name=f"data{{{bit_width}}}", 
                       bit_width=bit_width, signed=signed)
    
    # Statement visitors
    
    def visit_CompoundStatement(self, node: CompoundStatement) -> Optional[FluxType]:
        """Visit compound statement"""
        self.symbol_table.enter_scope("block", "block")
        
        for stmt in node.statements:
            self.visit(stmt)
        
        self.symbol_table.exit_scope()
        return None
    
    def visit_IfStatement(self, node: IfStatement) -> Optional[FluxType]:
        """Visit if statement"""
        condition_type = self.visit(node.condition)
        if condition_type and not self.is_boolean_compatible(condition_type):
            self.error("If condition must be boolean-compatible", node.token)
        
        self.visit(node.then_body)
        if node.else_body:
            self.visit(node.else_body)
        
        return None
    
    def visit_WhileStatement(self, node: WhileStatement) -> Optional[FluxType]:
        """Visit while statement"""
        condition_type = self.visit(node.condition)
        if condition_type and not self.is_boolean_compatible(condition_type):
            self.error("While condition must be boolean-compatible", node.token)
        
        self.visit(node.body)
        return None
    
    def visit_ForStatement(self, node: ForStatement) -> Optional[FluxType]:
        """Visit for statement"""
        self.symbol_table.enter_scope("for", "block")
        
        if node.iterator_var and node.iterable:
            # Range-based for loop
            iterable_type = self.visit(node.iterable)
            if iterable_type:
                if isinstance(iterable_type, ArrayType):
                    # Define iterator variable with element type
                    if isinstance(node.iterator_var, str):
                        iter_symbol = Symbol(node.iterator_var, iterable_type.element_type, 
                                           "variable", node.token, 0, False, True)
                        self.symbol_table.define(iter_symbol)
                else:
                    self.error("Can only iterate over arrays", node.token)
        else:
            # C-style for loop
            if node.init:
                if isinstance(node.init, Expression):
                    self.visit(node.init)
                else:
                    self.visit(node.init)
            
            if node.condition:
                condition_type = self.visit(node.condition)
                if condition_type and not self.is_boolean_compatible(condition_type):
                    self.error("For condition must be boolean-compatible", node.token)
            
            if node.increment:
                self.visit(node.increment)
        
        self.visit(node.body)
        self.symbol_table.exit_scope()
        return None
    
    def visit_ReturnStatement(self, node: ReturnStatement) -> Optional[FluxType]:
        """Visit return statement"""
        if not self.current_function_return_type:
            self.error("Return statement outside function", node.token)
            return None
        
        if node.expression:
            expr_type = self.visit(node.expression)
            if expr_type:
                # Special case: 'this' return type in constructors
                if (self.current_function_return_type.kind == TypeKind.THIS or
                    (hasattr(self.current_function_return_type, 'name') and 
                     self.current_function_return_type.name == "this")):
                    # In constructors, returning 'this' or the object itself is valid
                    if (isinstance(expr_type, ObjectType) or 
                        expr_type.kind == TypeKind.THIS or
                        (hasattr(expr_type, 'name') and expr_type.name == "this")):
                        return None  # Valid constructor return
                
                if not self.types_compatible(self.current_function_return_type, expr_type):
                    # More detailed error for constructors
                    if (hasattr(self.current_function_return_type, 'name') and 
                        self.current_function_return_type.name == "this"):
                        # This is okay for constructors
                        return None
                    else:
                        self.error(f"Cannot return value of type '{expr_type.name}' from function returning '{self.current_function_return_type.name}'",
                                  node.token)
        else:
            # Empty return
            if (self.current_function_return_type.kind != TypeKind.VOID and
                not (hasattr(self.current_function_return_type, 'name') and 
                     self.current_function_return_type.name in ["this", "void"])):
                self.error("Cannot return empty value from non-void function", node.token)
        
        return None
    
    def visit_ExpressionStatement(self, node: ExpressionStatement) -> Optional[FluxType]:
        """Visit expression statement"""
        if node.expression:
            return self.visit(node.expression)
        return None
    
    # Expression visitors
    
    def visit_BinaryExpression(self, node: BinaryExpression) -> Optional[FluxType]:
        """Visit binary expression"""
        left_type = self.visit(node.left)
        right_type = self.visit(node.right)
        
        if not left_type or not right_type:
            return None
        
        # Type checking for binary operators
        if node.operator in ['+', '-', '*', '/', '%']:
            # Arithmetic operators
            if self.is_numeric_type(left_type) and self.is_numeric_type(right_type):
                return self.promote_numeric_types(left_type, right_type)
            elif node.operator == '+' and (left_type.kind == TypeKind.STRING or right_type.kind == TypeKind.STRING):
                # String concatenation - return first string type found
                return left_type if left_type.kind == TypeKind.STRING else right_type
            else:
                self.error(f"Arithmetic operator '{node.operator}' requires numeric operands",
                          node.token)
                return None
        
        elif node.operator in ['==', '!=', '<', '>', '<=', '>=']:
            # Comparison operators - create a boolean data type result
            if self.types_compatible(left_type, right_type) or (self.is_numeric_type(left_type) and self.is_numeric_type(right_type)):
                # Return a boolean data type
                return DataType(kind=TypeKind.BOOLEAN, name="bool", bit_width=1, signed=False)
            else:
                self.error(f"Cannot compare values of types '{left_type.name}' and '{right_type.name}'",
                          node.token)
                return None
        
        elif node.operator in ['and', '&&', 'or', '||']:
            # Logical operators
            if self.is_boolean_compatible(left_type) and self.is_boolean_compatible(right_type):
                return DataType(kind=TypeKind.BOOLEAN, name="bool", bit_width=1, signed=False)
            else:
                self.error(f"Logical operator '{node.operator}' requires boolean operands",
                          node.token)
                return None
        
        elif node.operator in ['&', '|', '^', '<<', '>>', 'xor']:
            # Bitwise operators
            if self.is_integer_type(left_type) and self.is_integer_type(right_type):
                return self.promote_numeric_types(left_type, right_type)
            else:
                self.error(f"Bitwise operator '{node.operator}' requires integer operands",
                          node.token)
                return None
        
        elif node.operator == 'in':
            # Membership operator
            if isinstance(right_type, ArrayType):
                if self.types_compatible(left_type, right_type.element_type):
                    return DataType(kind=TypeKind.BOOLEAN, name="bool", bit_width=1, signed=False)
            self.error(f"Cannot check membership of '{left_type.name}' in '{right_type.name}'",
                      node.token)
            return None
        
        elif node.operator in ['is', 'as']:
            # Type checking operators
            if node.operator == 'is':
                return DataType(kind=TypeKind.BOOLEAN, name="bool", bit_width=1, signed=False)
            else:
                return right_type
        
        return left_type  # Default fallback
    
    def visit_UnaryExpression(self, node: UnaryExpression) -> Optional[FluxType]:
        """Visit unary expression"""
        operand_type = self.visit(node.operand)
        if not operand_type:
            return None
        
        if node.operator in ['+', '-']:
            if self.is_numeric_type(operand_type):
                return operand_type
            else:
                self.error(f"Unary '{node.operator}' requires numeric operand", node.token)
                return None
        
        elif node.operator in ['!', 'not']:
            if self.is_boolean_compatible(operand_type):
                return DataType(kind=TypeKind.BOOLEAN, name="bool", bit_width=1, signed=False)
            else:
                self.error(f"Unary '{node.operator}' requires boolean operand", node.token)
                return None
        
        elif node.operator in ['~']:
            if self.is_integer_type(operand_type):
                return operand_type
            else:
                self.error(f"Unary '{node.operator}' requires integer operand", node.token)
                return None
        
        elif node.operator == '@':
            # Address-of operator
            return PointerType(kind=TypeKind.POINTER, name=f"{operand_type.name}*",
                              pointee_type=operand_type)
        
        elif node.operator == '*':
            # Dereference operator
            if isinstance(operand_type, PointerType):
                return operand_type.pointee_type
            else:
                self.error("Cannot dereference non-pointer type", node.token)
                return None
        
        elif node.operator in ['++', '--']:
            if self.is_numeric_type(operand_type) or isinstance(operand_type, PointerType):
                return operand_type
            else:
                self.error(f"Unary '{node.operator}' requires numeric or pointer operand", node.token)
                return None
        
        return operand_type
    
    def visit_AssignmentExpression(self, node: AssignmentExpression) -> Optional[FluxType]:
        """Visit assignment expression"""
        left_type = self.visit(node.left)
        right_type = self.visit(node.right)
        
        # Debug output
        if hasattr(self, '_debug') and self._debug:
            print(f"Assignment: {node.left.__class__.__name__} {node.operator} {node.right.__class__.__name__}")
            if isinstance(node.left, MemberExpression):
                print(f"  Left member: {node.left.object.__class__.__name__}.{node.left.member}")
        
        if not left_type or not right_type:
            return None
        
        # Check if left side is assignable
        if not self._is_lvalue(node.left):
            self.error("Left side of assignment must be assignable", node.token)
            return None
        
        if node.operator == '=':
            if not self.types_compatible(left_type, right_type):
                self.error(f"Cannot assign value of type '{right_type.name}' to variable of type '{left_type.name}'",
                          node.token)
        else:
            # Compound assignment operators
            op = node.operator[:-1]  # Remove '=' from '+=', '-=', etc.
            if not self._check_binary_operator_types(left_type, right_type, op):
                self.error(f"Compound assignment '{node.operator}' incompatible with types '{left_type.name}' and '{right_type.name}'",
                          node.token)
        
        return left_type
    
    def visit_CallExpression(self, node: CallExpression) -> Optional[FluxType]:
        """Visit call expression"""
        func_type = self.visit(node.function)
        if not func_type:
            return None
        
        if isinstance(func_type, FunctionType):
            # Regular function call
            return self._check_function_call(func_type, node.arguments, node.token)
        elif isinstance(func_type, ObjectType):
            # Constructor call
            # Look for __init method
            if "__init" in func_type.methods:
                init_method = func_type.methods["__init"]
                self._check_function_call(init_method, node.arguments, node.token)
            return func_type
        elif isinstance(func_type, PointerType) and func_type.function_params is not None:
            # Function pointer call
            param_types = func_type.function_params
            if len(node.arguments) != len(param_types):
                self.error(f"Function pointer expects {len(param_types)} arguments, got {len(node.arguments)}",
                          node.token)
            else:
                for i, (arg, expected_type) in enumerate(zip(node.arguments, param_types)):
                    arg_type = self.visit(arg)
                    if arg_type and not self.types_compatible(expected_type, arg_type):
                        self.error(f"Argument {i+1}: expected '{expected_type.name}', got '{arg_type.name}'",
                                  node.token)
            return func_type.pointee_type
        else:
            self.error("Attempt to call non-function", node.token)
            return None
    
    def visit_MemberExpression(self, node: MemberExpression) -> Optional[FluxType]:
        """Visit member expression"""
        object_type = self.visit(node.object)
        if not object_type:
            return None
        
        if isinstance(object_type, ObjectType):
            # Check methods first
            if node.member in object_type.methods:
                return object_type.methods[node.member]
            # Check members
            elif node.member in object_type.members:
                return object_type.members[node.member]
            else:
                self.error(f"Object of type '{object_type.name}' has no member '{node.member}'",
                          node.token)
                return None
        
        elif isinstance(object_type, StructType):
            if node.member in object_type.members:
                return object_type.members[node.member]
            else:
                self.error(f"Struct of type '{object_type.name}' has no member '{node.member}'",
                          node.token)
                return None
        
        elif isinstance(object_type, PointerType):
            # Pointer member access (should be -> in full implementation)
            pointee = object_type.pointee_type
            if isinstance(pointee, (ObjectType, StructType)):
                if node.member in pointee.members:
                    return pointee.members[node.member]
                elif isinstance(pointee, ObjectType) and node.member in pointee.methods:
                    return pointee.methods[node.member]
            
            self.error(f"Cannot access member '{node.member}' of type '{object_type.name}'",
                      node.token)
            return None
        
        else:
            self.error(f"Cannot access member '{node.member}' of non-object/struct type",
                      node.token)
            return None
    
    def visit_IndexExpression(self, node: IndexExpression) -> Optional[FluxType]:
        """Visit index expression"""
        array_type = self.visit(node.array)
        index_type = self.visit(node.index)
        
        if not array_type or not index_type:
            return None
        
        if not self.is_integer_type(index_type):
            self.error("Array index must be integer", node.token)
            return None
        
        if isinstance(array_type, ArrayType):
            return array_type.element_type
        elif isinstance(array_type, PointerType):
            return array_type.pointee_type
        else:
            self.error("Cannot index non-array/pointer type", node.token)
            return None
    
    def visit_CastExpression(self, node: CastExpression) -> Optional[FluxType]:
        """Visit cast expression"""
        target_type = self.visit(node.type_spec)
        expr_type = self.visit(node.expression)
        
        if not target_type or not expr_type:
            return None
        
        # Check if cast is valid
        if not self._is_valid_cast(expr_type, target_type):
            self.error(f"Cannot cast from '{expr_type.name}' to '{target_type.name}'", node.token)
        
        return target_type
    
    def visit_ConditionalExpression(self, node: ConditionalExpression) -> Optional[FluxType]:
        """Visit conditional expression"""
        condition_type = self.visit(node.condition)
        true_type = self.visit(node.true_expr)
        false_type = self.visit(node.false_expr)
        
        if not condition_type or not true_type or not false_type:
            return None
        
        if not self.is_boolean_compatible(condition_type):
            self.error("Conditional expression condition must be boolean-compatible", node.token)
        
        if self.types_compatible(true_type, false_type):
            return true_type
        else:
            # Try to find common type
            common_type = self._find_common_type(true_type, false_type)
            if common_type:
                return common_type
            else:
                self.error(f"Conditional expression branches have incompatible types: '{true_type.name}' and '{false_type.name}'",
                          node.token)
                return true_type
    
    def visit_Identifier(self, node: Identifier) -> Optional[FluxType]:
        """Visit identifier"""
        symbol = self.symbol_table.lookup(node.name)
        if symbol:
            return symbol.type
        else:
            # Try to handle qualified names like "example::NamespaceObj"
            if "::" in node.name:
                symbol = self.symbol_table.lookup_qualified(node.name)
                if symbol:
                    return symbol.type
            
            # Try to handle member types like "ContainerObj.NestedObj"
            if "." in node.name:
                member_type = self._resolve_member_type(node.name, node.token)
                if member_type:
                    return member_type
            
            self.error(f"Undefined identifier: {node.name}", node.token)
            return None
    
    def visit_IntegerLiteral(self, node: IntegerLiteral) -> Optional[FluxType]:
        """Visit integer literal"""
        # Determine type based on value and suffix
        i32_symbol = self.symbol_table.lookup("i32")
        return i32_symbol.type if i32_symbol else None
    
    def visit_FloatingLiteral(self, node: FloatingLiteral) -> Optional[FluxType]:
        """Visit floating literal"""
        float_symbol = self.symbol_table.lookup("float")
        return float_symbol.type if float_symbol else None
    
    def visit_StringLiteral(self, node: StringLiteral) -> Optional[FluxType]:
        """Visit string literal"""
        string_symbol = self.symbol_table.lookup("string")
        return string_symbol.type if string_symbol else None
    
    def visit_BooleanLiteral(self, node: BooleanLiteral) -> Optional[FluxType]:
        """Visit boolean literal"""
        bool_symbol = self.symbol_table.lookup("bool")
        return bool_symbol.type if bool_symbol else None
    
    def visit_ArrayLiteral(self, node: ArrayLiteral) -> Optional[FluxType]:
        """Visit array literal"""
        if not node.elements:
            # Empty array - needs type inference from context
            auto_type = FluxType(TypeKind.AUTO, "auto")
            return ArrayType(kind=TypeKind.ARRAY, name="auto[]", element_type=auto_type)
        
        # Infer element type from first element
        first_element_type = self.visit(node.elements[0])
        if not first_element_type:
            return None
        
        # Check all elements have compatible types
        for element in node.elements[1:]:
            element_type = self.visit(element)
            if element_type and not self.types_compatible(first_element_type, element_type):
                self.error("Array elements must have compatible types", node.token)
                return None
        
        return ArrayType(kind=TypeKind.ARRAY, name=f"{first_element_type.name}[]",
                        element_type=first_element_type, array_size=len(node.elements))
    
    def visit_DictionaryLiteral(self, node: DictionaryLiteral) -> Optional[FluxType]:
        """Visit dictionary literal"""
        # For now, return a generic dict type
        # Full implementation would infer key/value types
        dict_type = FluxType(TypeKind.OBJECT, "dict")
        return dict_type
    
    def visit_ThisExpression(self, node: ThisExpression) -> Optional[FluxType]:
        """Visit this expression"""
        if not self.current_object_type:
            self.error("'this' used outside object context", node.token)
            return None
        
        # Debug output
        if hasattr(self, '_debug') and self._debug:
            print(f"ThisExpression: current_object_type = {self.current_object_type.name if self.current_object_type else None}")
            if node.member:
                print(f"  accessing member: {node.member}")
        
        if node.member:
            if node.member in self.current_object_type.members:
                return self.current_object_type.members[node.member]
            elif node.member in self.current_object_type.methods:
                return self.current_object_type.methods[node.member]
            else:
                self.error(f"Object has no member '{node.member}'", node.token)
                return None
        
        return self.current_object_type
    
    def visit_SuperExpression(self, node: SuperExpression) -> Optional[FluxType]:
        """Visit super expression"""
        if not self.current_object_type or not self.current_object_type.parent_types:
            self.error("'super' used outside derived object context", node.token)
            return None
        
        # For now, use first parent type
        parent_type = self.current_object_type.parent_types[0]
        
        if node.member:
            if node.member in parent_type.members:
                return parent_type.members[node.member]
            elif node.member in parent_type.methods:
                return parent_type.methods[node.member]
            else:
                self.error(f"Parent object has no member '{node.member}'", node.token)
                return None
        
        return parent_type
    
    # Utility methods
    
    def types_compatible(self, expected: FluxType, actual: FluxType) -> bool:
        """Check if two types are compatible"""
        if expected.name == actual.name:
            return True
        
        # Handle auto type
        if expected.kind == TypeKind.AUTO or actual.kind == TypeKind.AUTO:
            return True
        
        # Handle numeric promotions
        if self.is_numeric_type(expected) and self.is_numeric_type(actual):
            return True
        
        # Handle inheritance
        if isinstance(actual, ObjectType) and isinstance(expected, ObjectType):
            return self._is_subtype(actual, expected)
        
        # Handle array decay to pointer (Flux decay rules)
        if isinstance(actual, ArrayType) and isinstance(expected, PointerType):
            return self.types_compatible(expected.pointee_type, actual.element_type)
        
        # Handle function decay to function pointer
        if isinstance(actual, FunctionType) and isinstance(expected, PointerType):
            return self.types_compatible(expected.pointee_type, actual)
        
        return False
    
    def is_numeric_type(self, flux_type: FluxType) -> bool:
        """Check if type is numeric"""
        return flux_type.kind in [TypeKind.INTEGER, TypeKind.FLOAT, TypeKind.DATA]
    
    def is_integer_type(self, flux_type: FluxType) -> bool:
        """Check if type is integer"""
        return flux_type.kind in [TypeKind.INTEGER, TypeKind.DATA] or \
               (flux_type.kind == TypeKind.DATA and isinstance(flux_type, DataType) and not flux_type.signed)
    
    def is_boolean_compatible(self, flux_type: FluxType) -> bool:
        """Check if type is boolean-compatible"""
        return flux_type.kind in [TypeKind.BOOLEAN, TypeKind.INTEGER, TypeKind.DATA] or \
               flux_type.name in ["bool", "bit"]
    
    def promote_numeric_types(self, left: FluxType, right: FluxType) -> FluxType:
        """Promote numeric types for arithmetic"""
        # Simplified promotion rules based on Flux specification
        if left.kind == TypeKind.FLOAT or right.kind == TypeKind.FLOAT:
            float_symbol = self.symbol_table.lookup("float")
            return float_symbol.type if float_symbol else left
        elif left.kind == TypeKind.INTEGER or right.kind == TypeKind.INTEGER:
            int_symbol = self.symbol_table.lookup("i32")
            return int_symbol.type if int_symbol else left
        else:
            return left  # Default
    
    def _is_subtype(self, derived: ObjectType, base: ObjectType) -> bool:
        """Check if derived is subtype of base"""
        if derived == base:
            return True
        
        for parent in derived.parent_types:
            if self._is_subtype(parent, base):
                return True
        
        return False
    
    def _can_overload_function(self, existing: FunctionType, new: FunctionType) -> bool:
        """Check if functions can be overloaded"""
        # Functions can be overloaded if they have different parameter types
        if len(existing.parameter_types) != len(new.parameter_types):
            return True
        
        for old_param, new_param in zip(existing.parameter_types, new.parameter_types):
            if not self.types_compatible(old_param, new_param):
                return True
        
        return False
    
    def _is_lvalue(self, expr: Expression) -> bool:
        """Check if expression is an lvalue (can be assigned to)"""
        # Regular lvalues
        if isinstance(expr, (Identifier, MemberExpression, IndexExpression)):
            return True
        
        # ThisExpression with a member (like this.value)
        if isinstance(expr, ThisExpression) and expr.member:
            return True
        
        # Dereference expression (*ptr)
        if isinstance(expr, UnaryExpression) and expr.operator == '*':
            return True
        
        # Debug output
        if hasattr(self, '_debug') and self._debug:
            print(f"_is_lvalue check: {expr.__class__.__name__} -> False")
        
        return False
    
    def _check_binary_operator_types(self, left: FluxType, right: FluxType, op: str) -> bool:
        """Check if binary operator is valid for given types"""
        if op in ['+', '-', '*', '/', '%']:
            return self.is_numeric_type(left) and self.is_numeric_type(right)
        elif op in ['&', '|', '^', '<<', '>>']:
            return self.is_integer_type(left) and self.is_integer_type(right)
        elif op in ['and', 'or']:
            return self.is_boolean_compatible(left) and self.is_boolean_compatible(right)
        return True
    
    def _check_function_call(self, func_type: FunctionType, arguments: List[Expression], token: Token) -> FluxType:
        """Check function call arguments"""
        if len(arguments) != len(func_type.parameter_types):
            self.error(f"Function expects {len(func_type.parameter_types)} arguments, got {len(arguments)}",
                      token)
        else:
            for i, (arg, expected_type) in enumerate(zip(arguments, func_type.parameter_types)):
                arg_type = self.visit(arg)
                if arg_type and not self.types_compatible(expected_type, arg_type):
                    self.error(f"Argument {i+1}: expected '{expected_type.name}', got '{arg_type.name}'",
                              token)
        
        return func_type.return_type
    
    def _is_valid_cast(self, from_type: FluxType, to_type: FluxType) -> bool:
        """Check if cast is valid"""
        # Allow all casts for now - full implementation would have detailed cast rules
        return True
    
    def _find_common_type(self, type1: FluxType, type2: FluxType) -> Optional[FluxType]:
        """Find common type for two types"""
        if self.types_compatible(type1, type2):
            return type1
        
        # Numeric promotion
        if self.is_numeric_type(type1) and self.is_numeric_type(type2):
            return self.promote_numeric_types(type1, type2)
        
        return None
    
    def _resolve_member_type(self, member_type_name: str, token: Token) -> Optional[FluxType]:
        """Resolve member type like Object.NestedType"""
        parts = member_type_name.split(".")
        if len(parts) < 2:
            return None
        
        # Look up the container type
        container_symbol = self.symbol_table.lookup(parts[0])
        if not container_symbol:
            self.error(f"Unknown container type: {parts[0]}", token)
            return None
        
        current_type = container_symbol.type
        
        # Navigate through the member hierarchy
        for member_name in parts[1:]:
            if isinstance(current_type, ObjectType):
                if member_name in current_type.members:
                    current_type = current_type.members[member_name]
                else:
                    self.error(f"Object '{current_type.name}' has no member type '{member_name}'", token)
                    return None
            elif isinstance(current_type, StructType):
                if member_name in current_type.members:
                    current_type = current_type.members[member_name]
                else:
                    self.error(f"Struct '{current_type.name}' has no member type '{member_name}'", token)
                    return None
            else:
                self.error(f"Type '{current_type.name}' does not contain member types", token)
                return None
        
        return current_type


# Command line interface for type checker
if __name__ == "__main__":
    import sys
    import argparse
    import os
    from flexer import FluxLexer, LexerError
    from fparser import FluxParser, ParseError
    
    def main():
        parser = argparse.ArgumentParser(description='Flux Language Type Checker')
        parser.add_argument('file', nargs='?', help='Flux source file to type check')
        parser.add_argument('--debug', '-d', action='store_true', help='Enable debug output')
        parser.add_argument('--verbose', '-v', action='store_true', help='Verbose error reporting')
        
        args = parser.parse_args()
        
        # Get source code
        if not args.file:
            print("No file provided. Using test code...\n")
            source_code = '''
// Define basic types from data as per Flux specification
signed data{32} i32;
unsigned data{8} byte;

object TestObj
{
    i32 value;
    
    def __init(i32 val) -> this
    {
        this.value = val;
        return;
    };
    
    def getValue() -> i32
    {
        return this.value;
    };
};

def main() -> i32
{
    TestObj instance(42);
    i32 result = instance.getValue();
    return result;
};
            '''
        else:
            try:
                if not os.path.exists(args.file):
                    print(f"Error: File '{args.file}' not found.")
                    return 1
                
                with open(args.file, 'r', encoding='utf-8') as f:
                    source_code = f.read()
                    
                print(f"Type checking file: {args.file}")
                print(f"File size: {len(source_code)} characters\n")
                
            except IOError as e:
                print(f"Error reading file '{args.file}': {e}")
                return 1
        
        try:
            # Tokenize
            if args.debug:
                print("Tokenizing...")
            lexer = FluxLexer(source_code)
            tokens = lexer.tokenize_filter_whitespace()
            
            # Parse
            if args.debug:
                print("Parsing...")
            flux_parser = FluxParser(tokens)
            ast = flux_parser.parse()
            
            # Type check
            if args.debug:
                print("Type checking...")
            type_checker = FluxTypeChecker()
            if args.debug:
                type_checker._debug = True
            errors = type_checker.check(ast)
            
            # Report results
            if errors:
                print(f"Type checking failed with {len(errors)} error(s):")
                for i, error in enumerate(errors, 1):
                    if args.verbose or error.token:
                        print(f"  {i}. {error}")
                    else:
                        print(f"  {i}. {error.message}")
                return 1
            else:
                print("Type checking completed successfully!")
                
                if args.verbose:
                    # Print symbol table summary
                    print("\nSymbol table summary:")
                    print(f"  Global scope symbols: {len(type_checker.symbol_table.global_scope.symbols)}")
                    
                    # Count different symbol types
                    symbol_counts = {}
                    def count_symbols(scope):
                        for symbol in scope.symbols.values():
                            symbol_counts[symbol.kind] = symbol_counts.get(symbol.kind, 0) + 1
                        for child in scope.children:
                            count_symbols(child)
                    
                    count_symbols(type_checker.symbol_table.global_scope)
                    
                    for kind, count in sorted(symbol_counts.items()):
                        print(f"    {kind}: {count}")
                
                return 0
            
        except (LexerError, ParseError, Exception) as e:
            print(f"Error: {e}")
            if args.debug:
                import traceback
                traceback.print_exc()
            return 1
    
    sys.exit(main())