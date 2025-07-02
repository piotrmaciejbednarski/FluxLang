#!/usr/bin/env python3
"""
Flux Language Parser - fparser.py

Complete recursive descent parser for the Flux programming language.
Transforms token stream from flexer.py into AST using fast.py node definitions.

Usage:
    python3 fparser.py file.fx          # Basic parsing
    python3 fparser.py file.fx -v       # Verbose output with AST details
    python3 fparser.py file.fx -a       # Show AST tree structure
    python3 fparser.py file.fx -c       # Count AST nodes
"""

import sys
from typing import List, Optional, Union, Dict, Tuple, Any
from dataclasses import dataclass

# Import the lexer and AST modules
from flexer import FluxLexer, Token, TokenType
from fast import *

class ParseError(Exception):
    """Exception raised when parsing fails"""
    def __init__(self, message: str, token: Optional[Token] = None):
        self.message = message
        self.token = token
        super().__init__(self.format_message())
    
    def format_message(self) -> str:
        if self.token:
            return f"Parse error at line {self.token.line}, column {self.token.column}: {self.message}"
        return f"Parse error: {self.message}"

class FluxParser:
    """Recursive descent parser for Flux language"""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.current = 0
        self.length = len(tokens)
    
    def current_token(self) -> Optional[Token]:
        """Get current token"""
        if self.current >= self.length:
            return None
        return self.tokens[self.current]
    
    def peek_token(self, offset: int = 1) -> Optional[Token]:
        """Peek ahead at token"""
        pos = self.current + offset
        if pos >= self.length:
            return None
        return self.tokens[pos]
    
    def advance(self) -> Token:
        """Advance to next token and return current"""
        token = self.current_token()
        if self.current < self.length:
            self.current += 1
        return token
    
    def match(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of the given types"""
        current = self.current_token()
        if not current:
            return False
        return current.type in token_types
    
    def consume(self, token_type: TokenType, message: str = "") -> Token:
        """Consume token of expected type or raise error"""
        current = self.current_token()
        if not current or current.type != token_type:
            if not message:
                message = f"Expected {token_type.name}"
            raise ParseError(message, current)
        return self.advance()
    
    def synchronize(self) -> None:
        """Synchronize after parse error by finding next statement boundary"""
        while self.current_token() and not self.match(
            TokenType.SEMICOLON, TokenType.LEFT_BRACE, TokenType.RIGHT_BRACE,
            TokenType.DEF, TokenType.OBJECT, TokenType.STRUCT, TokenType.NAMESPACE,
            TokenType.IF, TokenType.WHILE, TokenType.FOR, TokenType.RETURN,
            TokenType.EOF
        ):
            self.advance()
        
        # Consume synchronization tokens to avoid getting stuck
        if self.match(TokenType.SEMICOLON, TokenType.RIGHT_BRACE):
            self.advance()
    
    # ============================================================================
    # Top-level parsing
    # ============================================================================
    
    def parse(self) -> Program:
        """Parse the entire program"""
        program = Program()
        
        while not self.match(TokenType.EOF):
            try:
                global_item = self.parse_global_item()
                if global_item:
                    program.global_items.append(global_item)
            except ParseError as e:
                print(f"Error: {e}", file=sys.stderr)
                self.synchronize()
        
        return program
    
    def parse_global_item(self) -> Optional[GlobalItem]:
        """Parse a global item"""
        current = self.current_token()
        if not current:
            return None
        
        # Skip unexpected right braces (likely from malformed blocks)
        if self.match(TokenType.RIGHT_BRACE):
            self.advance()
            return None
        
        # Import statement
        if self.match(TokenType.IMPORT):
            return self.parse_import_stmt()
        
        # Using statement
        elif self.match(TokenType.USING):
            return self.parse_using_stmt()
        
        # External FFI block
        elif self.match(TokenType.EXTERN):
            return self.parse_extern_block()
        
        # Compile-time block
        elif self.match(TokenType.COMPT):
            if self.peek_token() and self.peek_token().type == TokenType.LEFT_BRACE:
                return self.parse_compt_block()
            else:
                # Compile-time function
                return self.parse_function_def()
        
        # Function definition
        elif self.match(TokenType.DEF):
            # Check if it's a macro or function
            if self.peek_token() and self.peek_token().type == TokenType.IDENTIFIER:
                peek2 = self.peek_token(2)
                if peek2 and peek2.type in [TokenType.LEFT_PAREN, TokenType.LESS_THAN]:
                    return self.parse_function_def()
                else:
                    return self.parse_macro_def()
            else:
                return self.parse_function_def()
        
        # Object definition
        elif self.match(TokenType.OBJECT):
            return self.parse_object_def()
        
        # Struct definition
        elif self.match(TokenType.STRUCT):
            return self.parse_struct_def()
        
        # Namespace definition
        elif self.match(TokenType.NAMESPACE):
            return self.parse_namespace_def()
        
        # Function modifier or variable qualifier
        elif self.match(TokenType.VOLATILE, TokenType.CONST):
            # Could be function modifier or variable qualifier
            if self.is_function_start():
                return self.parse_function_def()
            else:
                var_decl = self.parse_variable_declaration()
                self.consume(TokenType.SEMICOLON, "Expected ';' after global variable declaration")
                return var_decl
        
        # Variable declaration
        elif self.is_type_start():
            var_decl = self.parse_variable_declaration()
            self.consume(TokenType.SEMICOLON, "Expected ';' after global variable declaration")
            return var_decl
        
        else:
            raise ParseError(f"Unexpected token in global scope: {current.type.name}", current)
    
    def is_function_start(self) -> bool:
        """Check if current position starts a function definition"""
        saved_pos = self.current
        try:
            # Skip modifiers
            while self.match(TokenType.VOLATILE, TokenType.CONST, TokenType.COMPT, TokenType.VIRTUAL):
                self.advance()
            
            if self.match(TokenType.DEF):
                self.advance()
                if self.match(TokenType.IDENTIFIER):
                    self.advance()
                    # Check for template parameters or function parameters
                    if self.match(TokenType.LESS_THAN, TokenType.LEFT_PAREN):
                        return True
            
            return False
        finally:
            self.current = saved_pos
    
    def is_type_start(self) -> bool:
        """Check if current position starts a type"""
        return self.match(
            TokenType.INT, TokenType.FLOAT_KW, TokenType.BOOL, TokenType.CHAR, TokenType.VOID,
            TokenType.SIGNED, TokenType.UNSIGNED, TokenType.DATA, TokenType.IDENTIFIER,
            TokenType.VOLATILE, TokenType.CONST
        )
    
    # ============================================================================
    # Import and Using statements
    # ============================================================================
    
    def parse_import_stmt(self) -> ImportStmt:
        """Parse import statement"""
        start_token = self.advance()  # consume 'import'
        
        path_token = self.consume(TokenType.STRING, "Expected string literal after 'import'")
        path = path_token.value
        
        alias = None
        if self.match(TokenType.AS):
            self.advance()
            alias_token = self.consume(TokenType.IDENTIFIER, "Expected identifier after 'as'")
            alias = alias_token.value
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after import statement")
        
        return ImportStmt(path=path, alias=alias, line=start_token.line, column=start_token.column)
    
    def parse_using_stmt(self) -> UsingStmt:
        """Parse using statement"""
        start_token = self.advance()  # consume 'using'
        
        names = []
        names.append(self.parse_qualified_name())
        
        while self.match(TokenType.COMMA):
            self.advance()
            names.append(self.parse_qualified_name())
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after using statement")
        
        return UsingStmt(names=names, line=start_token.line, column=start_token.column)
    
    # ============================================================================
    # External FFI
    # ============================================================================
    
    def parse_extern_block(self) -> ExternBlock:
        """Parse external function block"""
        start_token = self.advance()  # consume 'extern'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'extern'")
        lang_token = self.consume(TokenType.STRING, "Expected language string in extern block")
        language = lang_token.value
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after extern language")
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start extern block")
        
        declarations = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            decl = self.parse_extern_decl()
            declarations.append(decl)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end extern block")
        self.consume(TokenType.SEMICOLON, "Expected ';' after extern block")
        
        return ExternBlock(language=language, declarations=declarations, 
                          line=start_token.line, column=start_token.column)
    
    def parse_extern_decl(self) -> ExternDecl:
        """Parse external function declaration"""
        self.consume(TokenType.DEF, "Expected 'def' in extern declaration")
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected function name")
        name = name_token.value
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after extern function name")
        
        parameters = []
        if not self.match(TokenType.RIGHT_PAREN):
            parameters.append(self.parse_parameter())
            while self.match(TokenType.COMMA):
                self.advance()
                parameters.append(self.parse_parameter())
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after extern parameters")
        self.consume(TokenType.MINUS, "Expected '->' in extern function")
        self.consume(TokenType.GREATER_THAN, "Expected '->' in extern function")
        
        return_type = self.parse_type()
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after extern declaration")
        
        return ExternDecl(name=name, parameters=parameters, return_type=return_type,
                         line=name_token.line, column=name_token.column)
    
    # ============================================================================
    # Compile-time blocks and macros
    # ============================================================================
    
    def parse_compt_block(self) -> ComptBlock:
        """Parse compile-time block"""
        start_token = self.advance()  # consume 'compt'
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' after 'compt'")
        
        statements = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end compt block")
        self.consume(TokenType.SEMICOLON, "Expected ';' after compt block")
        
        return ComptBlock(statements=statements, line=start_token.line, column=start_token.column)
    
    def parse_macro_def(self) -> MacroDef:
        """Parse macro definition"""
        start_token = self.advance()  # consume 'def'
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected macro name")
        name = name_token.value
        
        value = None
        if not self.match(TokenType.SEMICOLON):
            value = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after macro definition")
        
        return MacroDef(name=name, value=value, line=start_token.line, column=start_token.column)
    
    # ============================================================================
    # Function definitions
    # ============================================================================
    
    def parse_function_def(self) -> FunctionDef:
        """Parse function definition"""
        start_token = self.current_token()
        
        # Parse modifiers
        modifiers = []
        while self.match(TokenType.VOLATILE, TokenType.CONST, TokenType.COMPT, TokenType.VIRTUAL):
            token = self.advance()
            if token.type == TokenType.VOLATILE:
                modifiers.append(FunctionModifier.VOLATILE)
            elif token.type == TokenType.CONST:
                modifiers.append(FunctionModifier.CONST)
            elif token.type == TokenType.COMPT:
                modifiers.append(FunctionModifier.COMPT)
            elif token.type == TokenType.VIRTUAL:
                modifiers.append(FunctionModifier.VIRTUAL)
        
        self.consume(TokenType.DEF, "Expected 'def'")
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected function name")
        name = name_token.value
        
        # Parse template parameters
        template_params = []
        if self.match(TokenType.LESS_THAN):
            template_params = self.parse_template_parameters()
        
        # Parse function parameters
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after function name")
        
        parameters = []
        if not self.match(TokenType.RIGHT_PAREN):
            parameters.append(self.parse_parameter())
            while self.match(TokenType.COMMA):
                self.advance()
                parameters.append(self.parse_parameter())
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after parameters")
        
        # Parse return type
        self.consume(TokenType.MINUS, "Expected '->' after function parameters")
        self.consume(TokenType.GREATER_THAN, "Expected '->' after function parameters")
        return_type = self.parse_type()
        
        # Parse function body
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start function body")
        
        body = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                body.append(stmt)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end function body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after function definition")
        
        return FunctionDef(
            name=name, parameters=parameters, return_type=return_type, body=body,
            modifiers=modifiers, template_params=template_params,
            line=start_token.line, column=start_token.column
        )
    
    def parse_parameter(self) -> Parameter:
        """Parse function parameter"""
        param_type = self.parse_type()
        name_token = self.consume(TokenType.IDENTIFIER, "Expected parameter name")
        name = name_token.value
        
        return Parameter(name=name, type=param_type, line=name_token.line, column=name_token.column)
    
    def parse_template_parameters(self) -> List[TemplateParameter]:
        """Parse template parameters <T, K, ...>"""
        self.consume(TokenType.LESS_THAN, "Expected '<'")
        
        params = []
        name_token = self.consume(TokenType.IDENTIFIER, "Expected template parameter name")
        params.append(TemplateParameter(name=name_token.value, line=name_token.line, column=name_token.column))
        
        while self.match(TokenType.COMMA):
            self.advance()
            name_token = self.consume(TokenType.IDENTIFIER, "Expected template parameter name")
            params.append(TemplateParameter(name=name_token.value, line=name_token.line, column=name_token.column))
        
        self.consume(TokenType.GREATER_THAN, "Expected '>'")
        
        return params
    
    # ============================================================================
    # Object definitions
    # ============================================================================
    
    def parse_object_def(self) -> ObjectDef:
        """Parse object definition"""
        start_token = self.advance()  # consume 'object'
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected object name")
        name = name_token.value
        
        # Parse template parameters
        template_params = []
        if self.match(TokenType.LESS_THAN):
            template_params = self.parse_template_parameters()
        
        # Parse inheritance
        inheritance = []
        if self.match(TokenType.COLON):
            inheritance = self.parse_inheritance_list()
        
        # Check for forward declaration
        if self.match(TokenType.SEMICOLON):
            self.advance()
            return ObjectDef(name=name, template_params=template_params, inheritance=inheritance,
                           is_forward_decl=True, line=start_token.line, column=start_token.column)
        
        # Parse object body
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start object body")
        
        members = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            # Handle access specifier blocks (public { ... } or private { ... })
            if self.match(TokenType.PUBLIC, TokenType.PRIVATE):
                access_token = self.advance()
                access = AccessSpecifier.PUBLIC if access_token.type == TokenType.PUBLIC else AccessSpecifier.PRIVATE
                
                self.consume(TokenType.LEFT_BRACE, f"Expected '{{' after '{access_token.value}'")
                
                # Parse all members in this access block
                while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
                    member = self.parse_object_member_item(access)
                    if member:
                        members.append(member)
                
                self.consume(TokenType.RIGHT_BRACE, f"Expected '}}' to end {access_token.value} block")
                
                # Consume optional semicolon after access block
                if self.match(TokenType.SEMICOLON):
                    self.advance()
            else:
                # Individual member without access specifier
                member = self.parse_object_member_item(None)
                if member:
                    members.append(member)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end object body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after object definition")
        
        return ObjectDef(
            name=name, members=members, inheritance=inheritance, template_params=template_params,
            line=start_token.line, column=start_token.column
        )
    
    def parse_inheritance_list(self) -> List[QualifiedName]:
        """Parse inheritance list"""
        self.advance()  # consume ':'
        
        inheritance = []
        inheritance.append(self.parse_qualified_name())
        
        while self.match(TokenType.COMMA):
            self.advance()
            inheritance.append(self.parse_qualified_name())
        
        return inheritance
    
    def parse_object_member(self) -> Optional[ObjectMember]:
        """Parse object member - simplified version"""
        return self.parse_object_member_item(None)
    
    def parse_object_member_item(self, access: Optional[AccessSpecifier]) -> Optional[ObjectMember]:
        """Parse individual object member item"""
        if self.match(TokenType.DEF) or self.match(TokenType.VOLATILE, TokenType.CONST, TokenType.COMPT):
            # Function definition
            func_def = self.parse_function_def()
            return ObjectFunctionDef(function=func_def, access=access, 
                                   line=func_def.line, column=func_def.column)
        
        elif self.match(TokenType.OBJECT):
            # Nested object definition
            obj_def = self.parse_object_def()
            return ObjectObjectDef(object_def=obj_def, access=access,
                                 line=obj_def.line, column=obj_def.column)
        
        elif self.match(TokenType.STRUCT):
            # Nested struct definition
            struct_def = self.parse_struct_def()
            return ObjectStructDef(struct_def=struct_def, access=access,
                                 line=struct_def.line, column=struct_def.column)
        
        elif self.is_type_start():
            # Variable declaration
            var_decl = self.parse_variable_declaration()
            self.consume(TokenType.SEMICOLON, "Expected ';' after object member variable")
            return ObjectVariableDecl(declaration=var_decl, access=access,
                                    line=var_decl.line, column=var_decl.column)
        
        else:
            current = self.current_token()
            raise ParseError(f"Unexpected token in object member: {current.type.name if current else 'EOF'}", current)
    
    # ============================================================================
    # Struct definitions
    # ============================================================================
    
    def parse_struct_def(self) -> StructDef:
        """Parse struct definition"""
        start_token = self.advance()  # consume 'struct'
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected struct name")
        name = name_token.value
        
        # Parse template parameters
        template_params = []
        if self.match(TokenType.LESS_THAN):
            template_params = self.parse_template_parameters()
        
        # Parse inheritance
        inheritance = []
        if self.match(TokenType.COLON):
            inheritance = self.parse_inheritance_list()
        
        # Check for forward declaration
        if self.match(TokenType.SEMICOLON):
            self.advance()
            return StructDef(name=name, template_params=template_params, inheritance=inheritance,
                           is_forward_decl=True, line=start_token.line, column=start_token.column)
        
        # Parse struct body
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start struct body")
        
        members = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            # Handle access specifier blocks (public { ... } or private { ... })
            if self.match(TokenType.PUBLIC, TokenType.PRIVATE):
                access_token = self.advance()
                access = AccessSpecifier.PUBLIC if access_token.type == TokenType.PUBLIC else AccessSpecifier.PRIVATE
                
                self.consume(TokenType.LEFT_BRACE, f"Expected '{{' after '{access_token.value}'")
                
                # Parse all members in this access block
                while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
                    var_decl = self.parse_variable_declaration()
                    self.consume(TokenType.SEMICOLON, "Expected ';' after struct member")
                    members.append(StructMember(declaration=var_decl, access=access,
                                              line=var_decl.line, column=var_decl.column))
                
                self.consume(TokenType.RIGHT_BRACE, f"Expected '}}' to end {access_token.value} block")
                
                # Consume optional semicolon after access block
                if self.match(TokenType.SEMICOLON):
                    self.advance()
            else:
                # Individual member without access specifier
                var_decl = self.parse_variable_declaration()
                self.consume(TokenType.SEMICOLON, "Expected ';' after struct member")
                members.append(StructMember(declaration=var_decl, access=None,
                                          line=var_decl.line, column=var_decl.column))
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end struct body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after struct definition")
        
        return StructDef(
            name=name, members=members, inheritance=inheritance, template_params=template_params,
            line=start_token.line, column=start_token.column
        )
    
    def parse_struct_member(self) -> Optional[StructMember]:
        """Parse struct member - simplified version"""
        var_decl = self.parse_variable_declaration()
        self.consume(TokenType.SEMICOLON, "Expected ';' after struct member")
        
        return StructMember(declaration=var_decl, access=None,
                          line=var_decl.line, column=var_decl.column)
    
    # ============================================================================
    # Namespace definitions
    # ============================================================================
    
    def parse_namespace_def(self) -> NamespaceDef:
        """Parse namespace definition"""
        start_token = self.advance()  # consume 'namespace'
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected namespace name")
        name = name_token.value
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start namespace body")
        
        members = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            member = self.parse_namespace_member()
            if member:
                members.append(member)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end namespace body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after namespace definition")
        
        return NamespaceDef(name=name, members=members, line=start_token.line, column=start_token.column)
    
    def parse_namespace_member(self) -> Optional[NamespaceMember]:
        """Parse namespace member"""
        if self.match(TokenType.DEF) or self.match(TokenType.VOLATILE, TokenType.CONST, TokenType.COMPT):
            # Function definition
            func_def = self.parse_function_def()
            return NamespaceFunctionDef(function=func_def, line=func_def.line, column=func_def.column)
        
        elif self.match(TokenType.OBJECT):
            # Object definition
            obj_def = self.parse_object_def()
            return NamespaceObjectDef(object_def=obj_def, line=obj_def.line, column=obj_def.column)
        
        elif self.match(TokenType.STRUCT):
            # Struct definition
            struct_def = self.parse_struct_def()
            return NamespaceStructDef(struct_def=struct_def, line=struct_def.line, column=struct_def.column)
        
        elif self.match(TokenType.NAMESPACE):
            # Nested namespace definition
            namespace_def = self.parse_namespace_def()
            return NamespaceNamespaceDef(namespace_def=namespace_def, 
                                       line=namespace_def.line, column=namespace_def.column)
        
        elif self.is_type_start():
            # Variable declaration
            var_decl = self.parse_variable_declaration()
            self.consume(TokenType.SEMICOLON, "Expected ';' after namespace member variable")
            return NamespaceVariableDecl(declaration=var_decl, line=var_decl.line, column=var_decl.column)
        
        else:
            current = self.current_token()
            raise ParseError(f"Unexpected token in namespace member: {current.type.name if current else 'EOF'}", current)
    
    # ============================================================================
    # Type parsing
    # ============================================================================
    
    def parse_type(self) -> Type:
        """Parse type"""
        qualifiers = []
        
        # Parse type qualifiers
        while self.match(TokenType.VOLATILE, TokenType.CONST):
            token = self.advance()
            if token.type == TokenType.VOLATILE:
                qualifiers.append(TypeQualifier.VOLATILE)
            elif token.type == TokenType.CONST:
                qualifiers.append(TypeQualifier.CONST)
        
        base_type = self.parse_base_type()
        
        # Apply qualifiers to base type
        if qualifiers and hasattr(base_type, 'qualifiers'):
            base_type.qualifiers.extend(qualifiers)
        
        # Parse pointer and array suffixes
        while self.match(TokenType.MULTIPLY, TokenType.LEFT_BRACKET):
            if self.match(TokenType.MULTIPLY):
                self.advance()
                base_type = PointerType(pointee_type=base_type, qualifiers=qualifiers)
                qualifiers = []  # Reset qualifiers for next level
            elif self.match(TokenType.LEFT_BRACKET):
                self.advance()
                self.consume(TokenType.RIGHT_BRACKET, "Expected ']' after '['")
                base_type = ArrayType(element_type=base_type, qualifiers=qualifiers)
                qualifiers = []  # Reset qualifiers for next level
        
        return base_type
    
    def parse_base_type(self) -> Type:
        """Parse base type"""
        current = self.current_token()
        if not current:
            raise ParseError("Expected type")
        
        # Primitive types
        if self.match(TokenType.INT):
            self.advance()
            return PrimitiveType(PrimitiveTypeKind.INT)
        elif self.match(TokenType.FLOAT_KW):
            self.advance()
            return PrimitiveType(PrimitiveTypeKind.FLOAT)
        elif self.match(TokenType.BOOL):
            self.advance()
            return PrimitiveType(PrimitiveTypeKind.BOOL)
        elif self.match(TokenType.CHAR):
            self.advance()
            return PrimitiveType(PrimitiveTypeKind.CHAR)
        elif self.match(TokenType.VOID):
            self.advance()
            return PrimitiveType(PrimitiveTypeKind.VOID)
        
        # Data types
        elif self.match(TokenType.SIGNED, TokenType.UNSIGNED, TokenType.DATA):
            return self.parse_data_type()
        
        # Named types (user-defined, templates)
        elif self.match(TokenType.IDENTIFIER):
            return self.parse_named_type()
        
        # Function pointer type
        elif self.match(TokenType.LEFT_PAREN):
            # Could be function pointer or parenthesized type
            saved_pos = self.current
            try:
                return self.parse_function_pointer_type()
            except ParseError:
                self.current = saved_pos
                raise ParseError(f"Unexpected token in type: {current.type.name}", current)
        
        else:
            raise ParseError(f"Unexpected token in type: {current.type.name}", current)
    
    def parse_data_type(self) -> DataType:
        """Parse data type"""
        signedness = None
        
        # Parse signedness
        if self.match(TokenType.SIGNED):
            self.advance()
            signedness = DataSignedness.SIGNED
        elif self.match(TokenType.UNSIGNED):
            self.advance()
            signedness = DataSignedness.UNSIGNED
        
        self.consume(TokenType.DATA, "Expected 'data'")
        self.consume(TokenType.LEFT_BRACE, "Expected '{' after 'data'")
        
        # Parse bit width
        bit_width_token = self.consume(TokenType.INTEGER, "Expected bit width")
        bit_width = int(bit_width_token.value)
        
        # Parse optional alignment
        alignment = None
        if self.match(TokenType.COLON):
            self.advance()
            align_token = self.consume(TokenType.INTEGER, "Expected alignment value")
            alignment = int(align_token.value)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' after data type")
        
        return DataType(bit_width=bit_width, alignment=alignment, signedness=signedness)
    
    def parse_named_type(self) -> NamedType:
        """Parse named type"""
        name = self.parse_qualified_name()
        
        # Parse template arguments
        template_args = []
        if self.match(TokenType.LESS_THAN):
            template_args = self.parse_template_arguments()
        
        return NamedType(name=name, template_args=template_args)
    
    def parse_template_arguments(self) -> List[Union[Type, Expression]]:
        """Parse template arguments <type, expr, ...>"""
        self.consume(TokenType.LESS_THAN, "Expected '<'")
        
        args = []
        
        # Try to parse first argument
        args.append(self.parse_template_argument())
        
        while self.match(TokenType.COMMA):
            self.advance()
            args.append(self.parse_template_argument())
        
        self.consume(TokenType.GREATER_THAN, "Expected '>'")
        
        return args
    
    def parse_template_argument(self) -> Union[Type, Expression]:
        """Parse template argument (could be type or expression)"""
        saved_pos = self.current
        
        # Try parsing as type first
        try:
            return self.parse_type()
        except ParseError:
            # If that fails, try parsing as expression
            self.current = saved_pos
            return self.parse_expression()
    
    def parse_function_pointer_type(self) -> FunctionPointerType:
        """Parse function pointer type"""
        # Simplified - would need more complex logic for full support
        raise ParseError("Function pointer types not fully implemented")
    
    # ============================================================================
    # Variable declarations
    # ============================================================================
    
    def parse_variable_declaration(self) -> VariableDeclaration:
        """Parse variable declaration"""
        start_token = self.current_token()
        
        # Check for function pointer
        if self.is_function_pointer():
            func_ptr = self.parse_function_pointer_declaration()
            return func_ptr
        
        var_type = self.parse_type()
        
        declarators = []
        declarators.append(self.parse_variable_declarator())
        
        while self.match(TokenType.COMMA):
            self.advance()
            declarators.append(self.parse_variable_declarator())
        
        return VariableDeclaration(type=var_type, declarators=declarators,
                                 line=start_token.line, column=start_token.column)
    
    def is_function_pointer(self) -> bool:
        """Check if current position is a function pointer declaration"""
        saved_pos = self.current
        try:
            # Skip type qualifiers and base type
            while self.match(TokenType.VOLATILE, TokenType.CONST):
                self.advance()
            
            if self.is_type_start():
                self.parse_base_type()
                
                # Look for (* pattern
                if self.match(TokenType.LEFT_PAREN) and self.peek_token() and self.peek_token().type == TokenType.MULTIPLY:
                    return True
            
            return False
        except:
            return False
        finally:
            self.current = saved_pos
    
    def parse_function_pointer_declaration(self) -> FunctionPointerDeclaration:
        """Parse function pointer declaration"""
        return_type = self.parse_type()
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' in function pointer")
        self.consume(TokenType.MULTIPLY, "Expected '*' in function pointer")
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected function pointer name")
        name = name_token.value
        
        # Parse template parameters if present
        template_params = []
        if self.match(TokenType.LESS_THAN):
            template_params = self.parse_template_parameters()
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function pointer name")
        self.consume(TokenType.LEFT_PAREN, "Expected '(' for function pointer parameters")
        
        parameters = []
        if not self.match(TokenType.RIGHT_PAREN):
            parameters.append(self.parse_parameter())
            while self.match(TokenType.COMMA):
                self.advance()
                parameters.append(self.parse_parameter())
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function pointer parameters")
        self.consume(TokenType.MINUS, "Expected '->' in function pointer")
        self.consume(TokenType.GREATER_THAN, "Expected '->' in function pointer")
        
        func_return_type = self.parse_type()
        
        # Parse optional initializer
        initializer = None
        if self.match(TokenType.ASSIGN):
            self.advance()
            initializer = self.parse_expression()
        
        return FunctionPointerDeclaration(
            return_type=return_type, name=name, parameters=parameters,
            template_params=template_params, initializer=initializer,
            line=name_token.line, column=name_token.column
        )
    
    def parse_variable_declarator(self) -> VariableDeclarator:
        """Parse variable declarator"""
        name_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
        name = name_token.value
        
        # Array declarator
        if self.match(TokenType.LEFT_BRACKET):
            self.advance()
            
            size = None
            if not self.match(TokenType.RIGHT_BRACKET):
                size = self.parse_expression()
            
            self.consume(TokenType.RIGHT_BRACKET, "Expected ']' after array size")
            
            initializer = None
            if self.match(TokenType.ASSIGN):
                self.advance()
                initializer = self.parse_array_initializer()
            
            return ArrayVariableDeclarator(name=name, size=size, initializer=initializer,
                                         line=name_token.line, column=name_token.column)
        
        # Object instantiation
        elif self.match(TokenType.LEFT_PAREN):
            self.advance()
            
            arguments = []
            if not self.match(TokenType.RIGHT_PAREN):
                arguments.append(self.parse_expression())
                while self.match(TokenType.COMMA):
                    self.advance()
                    arguments.append(self.parse_expression())
            
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after object arguments")
            
            return ObjectInstantiationDeclarator(name=name, arguments=arguments,
                                               line=name_token.line, column=name_token.column)
        
        # Simple variable declarator
        else:
            initializer = None
            if self.match(TokenType.ASSIGN):
                self.advance()
                initializer = self.parse_expression()
            
            return SimpleVariableDeclarator(name=name, initializer=initializer,
                                          line=name_token.line, column=name_token.column)
    
    # ============================================================================
    # Statement parsing
    # ============================================================================
    
    def parse_statement(self) -> Optional[Statement]:
        """Parse statement"""
        current = self.current_token()
        if not current:
            return None
        
        # Expression statement (including assignment)
        if self.match(TokenType.LEFT_BRACE):
            return self.parse_compound_stmt()
        elif self.match(TokenType.IF):
            return self.parse_if_stmt()
        elif self.match(TokenType.WHILE):
            return self.parse_while_stmt()
        elif self.match(TokenType.DO):
            return self.parse_do_while_stmt()
        elif self.match(TokenType.FOR):
            return self.parse_for_stmt()
        elif self.match(TokenType.SWITCH):
            return self.parse_switch_stmt()
        elif self.match(TokenType.MATCH):
            return self.parse_match_stmt()
        elif self.match(TokenType.TRY):
            return self.parse_try_catch_stmt()
        elif self.match(TokenType.RETURN):
            return self.parse_return_stmt()
        elif self.match(TokenType.BREAK):
            return self.parse_break_stmt()
        elif self.match(TokenType.CONTINUE):
            return self.parse_continue_stmt()
        elif self.match(TokenType.THROW):
            return self.parse_throw_stmt()
        elif self.match(TokenType.ASSERT):
            return self.parse_assert_stmt()
        elif self.match(TokenType.ASM):
            return self.parse_asm_stmt()
        elif self.match(TokenType.AUTO):
            return self.parse_destructuring_stmt()
        elif self.match(TokenType.COMPT):
            return self.parse_compt_block()
        elif self.is_type_start():
            # Variable declaration
            var_decl = self.parse_variable_declaration()
            self.consume(TokenType.SEMICOLON, "Expected ';' after variable declaration")
            return VariableDeclStmt(declaration=var_decl, line=var_decl.line, column=var_decl.column)
        else:
            # Expression statement
            expr = self.parse_expression()
            self.consume(TokenType.SEMICOLON, "Expected ';' after expression")
            return ExpressionStmt(expression=expr, line=expr.line, column=expr.column)
    
    def parse_compound_stmt(self) -> CompoundStmt:
        """Parse compound statement"""
        start_token = self.advance()  # consume '{'
        
        statements = []
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end compound statement")
        self.consume(TokenType.SEMICOLON, "Expected ';' after compound statement")
        
        return CompoundStmt(statements=statements, line=start_token.line, column=start_token.column)
    
    def parse_if_stmt(self) -> IfStmt:
        """Parse if statement"""
        start_token = self.advance()  # consume 'if'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'if'")
        condition = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after if condition")
        
        then_stmt = self.parse_statement()
        
        elif_parts = []
        while self.match(TokenType.ELSE) and self.peek_token() and self.peek_token().type == TokenType.IF:
            self.advance()  # consume 'else'
            self.advance()  # consume 'if'
            
            self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'else if'")
            elif_condition = self.parse_expression()
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after else if condition")
            
            elif_stmt = self.parse_statement()
            elif_parts.append((elif_condition, elif_stmt))
        
        else_stmt = None
        if self.match(TokenType.ELSE):
            self.advance()
            else_stmt = self.parse_statement()
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after if statement")
        
        return IfStmt(condition=condition, then_stmt=then_stmt, elif_parts=elif_parts, else_stmt=else_stmt,
                     line=start_token.line, column=start_token.column)
    
    def parse_while_stmt(self) -> WhileStmt:
        """Parse while statement"""
        start_token = self.advance()  # consume 'while'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'while'")
        condition = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after while condition")
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON, "Expected ';' after while statement")
        
        return WhileStmt(condition=condition, body=body, line=start_token.line, column=start_token.column)
    
    def parse_do_while_stmt(self) -> DoWhileStmt:
        """Parse do-while statement"""
        start_token = self.advance()  # consume 'do'
        
        body = self.parse_statement()
        
        self.consume(TokenType.WHILE, "Expected 'while' after do body")
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'while'")
        condition = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after while condition")
        self.consume(TokenType.SEMICOLON, "Expected ';' after do-while statement")
        
        return DoWhileStmt(body=body, condition=condition, line=start_token.line, column=start_token.column)
    
    def parse_for_stmt(self) -> ForStmt:
        """Parse for statement"""
        start_token = self.advance()  # consume 'for'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'for'")
        
        # Check if it's Python-style or C-style
        saved_pos = self.current
        is_python_style = False
        
        try:
            # Look for variable 'in' pattern
            if self.match(TokenType.IDENTIFIER):
                self.advance()
                if self.match(TokenType.COMMA):
                    # Multiple variables
                    while self.match(TokenType.COMMA):
                        self.advance()
                        self.consume(TokenType.IDENTIFIER, "Expected variable name")
                
                if self.match(TokenType.IN):
                    is_python_style = True
        except:
            pass
        
        self.current = saved_pos
        
        if is_python_style:
            return self.parse_python_style_for_stmt(start_token)
        else:
            return self.parse_c_style_for_stmt(start_token)
    
    def parse_c_style_for_stmt(self, start_token: Token) -> CStyleForStmt:
        """Parse C-style for statement"""
        # Parse init
        init = None
        if not self.match(TokenType.SEMICOLON):
            init = self.parse_expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after for init")
        
        # Parse condition
        condition = None
        if not self.match(TokenType.SEMICOLON):
            condition = self.parse_expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after for condition")
        
        # Parse update
        update = None
        if not self.match(TokenType.RIGHT_PAREN):
            update = self.parse_expression()
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after for clauses")
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON, "Expected ';' after for statement")
        
        return CStyleForStmt(init=init, condition=condition, update=update, body=body,
                           line=start_token.line, column=start_token.column)
    
    def parse_python_style_for_stmt(self, start_token: Token) -> PythonStyleForStmt:
        """Parse Python-style for statement"""
        variables = []
        
        # Parse variables
        var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
        variables.append(var_token.value)
        
        while self.match(TokenType.COMMA):
            self.advance()
            var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
            variables.append(var_token.value)
        
        self.consume(TokenType.IN, "Expected 'in' in for loop")
        
        iterable = self.parse_expression()
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after for iterable")
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON, "Expected ';' after for statement")
        
        return PythonStyleForStmt(variables=variables, iterable=iterable, body=body,
                                line=start_token.line, column=start_token.column)
    
    def parse_switch_stmt(self) -> SwitchStmt:
        """Parse switch statement"""
        start_token = self.advance()  # consume 'switch'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'switch'")
        expression = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after switch expression")
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start switch body")
        
        cases = []
        default_case = None
        
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            if self.match(TokenType.CASE):
                self.advance()
                self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'case'")
                value = self.parse_expression()
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after case value")
                body = self.parse_statement()
                cases.append(SwitchCase(value=value, body=body, line=value.line, column=value.column))
            
            elif self.match(TokenType.DEFAULT):
                self.advance()
                body = self.parse_statement()
                self.consume(TokenType.SEMICOLON, "Expected ';' after default case")
                default_case = DefaultCase(body=body, line=body.line, column=body.column)
                break
            
            else:
                current = self.current_token()
                raise ParseError(f"Expected 'case' or 'default' in switch body", current)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end switch body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after switch statement")
        
        return SwitchStmt(expression=expression, cases=cases, default_case=default_case,
                         line=start_token.line, column=start_token.column)
    
    def parse_match_stmt(self) -> MatchStmt:
        """Parse match statement"""
        start_token = self.advance()  # consume 'match'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'match'")
        expression = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after match expression")
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' to start match body")
        
        cases = []
        default_case = None
        
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            if self.match(TokenType.CASE):
                self.advance()
                self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'case'")
                pattern = self.parse_pattern()
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after case pattern")
                body = self.parse_statement()
                cases.append(MatchCase(pattern=pattern, body=body, line=pattern.line, column=pattern.column))
            
            elif self.match(TokenType.DEFAULT):
                self.advance()
                body = self.parse_statement()
                self.consume(TokenType.SEMICOLON, "Expected ';' after default case")
                default_case = DefaultCase(body=body, line=body.line, column=body.column)
                break
            
            else:
                current = self.current_token()
                raise ParseError(f"Expected 'case' or 'default' in match body", current)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end match body")
        self.consume(TokenType.SEMICOLON, "Expected ';' after match statement")
        
        return MatchStmt(expression=expression, cases=cases, default_case=default_case,
                        line=start_token.line, column=start_token.column)
    
    def parse_pattern(self) -> Pattern:
        """Parse pattern for match statement"""
        expr = self.parse_expression()
        
        if self.match(TokenType.IN):
            self.advance()
            container = self.parse_expression()
            
            # Check if container is a range expression
            if isinstance(container, RangeExpression):
                return RangePattern(expr=expr, range_expr=container, line=expr.line, column=expr.column)
            else:
                return InPattern(expr=expr, container=container, line=expr.line, column=expr.column)
        
        return SimplePattern(expression=expr, line=expr.line, column=expr.column)
    
    def parse_try_catch_stmt(self) -> TryCatchStmt:
        """Parse try-catch statement"""
        start_token = self.advance()  # consume 'try'
        
        try_body = self.parse_statement()
        
        catch_clauses = []
        while self.match(TokenType.CATCH):
            self.advance()
            self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'catch'")
            catch_type = self.parse_type()
            var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name in catch")
            variable = var_token.value
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after catch variable")
            
            catch_body = self.parse_statement()
            catch_clauses.append(CatchClause(type=catch_type, variable=variable, body=catch_body,
                                           line=var_token.line, column=var_token.column))
        
        if not catch_clauses:
            raise ParseError("Try statement must have at least one catch clause")
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after try-catch statement")
        
        return TryCatchStmt(try_body=try_body, catch_clauses=catch_clauses,
                          line=start_token.line, column=start_token.column)
    
    def parse_return_stmt(self) -> ReturnStmt:
        """Parse return statement"""
        start_token = self.advance()  # consume 'return'
        
        value = None
        if not self.match(TokenType.SEMICOLON):
            value = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON, "Expected ';' after return statement")
        
        return ReturnStmt(value=value, line=start_token.line, column=start_token.column)
    
    def parse_break_stmt(self) -> BreakStmt:
        """Parse break statement"""
        start_token = self.advance()  # consume 'break'
        self.consume(TokenType.SEMICOLON, "Expected ';' after 'break'")
        
        return BreakStmt(line=start_token.line, column=start_token.column)
    
    def parse_continue_stmt(self) -> ContinueStmt:
        """Parse continue statement"""
        start_token = self.advance()  # consume 'continue'
        self.consume(TokenType.SEMICOLON, "Expected ';' after 'continue'")
        
        return ContinueStmt(line=start_token.line, column=start_token.column)
    
    def parse_throw_stmt(self) -> ThrowStmt:
        """Parse throw statement"""
        start_token = self.advance()  # consume 'throw'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'throw'")
        expression = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after throw expression")
        self.consume(TokenType.SEMICOLON, "Expected ';' after throw statement")
        
        return ThrowStmt(expression=expression, line=start_token.line, column=start_token.column)
    
    def parse_assert_stmt(self) -> AssertStmt:
        """Parse assert statement"""
        start_token = self.advance()  # consume 'assert'
        
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'assert'")
        condition = self.parse_expression()
        
        message = None
        if self.match(TokenType.COMMA):
            self.advance()
            message = self.parse_expression()
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after assert")
        self.consume(TokenType.SEMICOLON, "Expected ';' after assert statement")
        
        return AssertStmt(condition=condition, message=message, line=start_token.line, column=start_token.column)
    
    def parse_asm_stmt(self) -> AsmStmt:
        """Parse inline assembly statement"""
        start_token = self.advance()  # consume 'asm'
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' after 'asm'")
        
        # Read assembly content until closing brace
        content = ""
        while not self.match(TokenType.RIGHT_BRACE, TokenType.EOF):
            token = self.advance()
            content += token.value + " "
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end asm block")
        self.consume(TokenType.SEMICOLON, "Expected ';' after asm statement")
        
        return AsmStmt(content=content.strip(), line=start_token.line, column=start_token.column)
    
    def parse_destructuring_stmt(self) -> DestructuringStmt:
        """Parse destructuring assignment statement"""
        start_token = self.advance()  # consume 'auto'
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' after 'auto'")
        
        target_vars = []
        var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
        target_vars.append(var_token.value)
        
        while self.match(TokenType.COMMA):
            self.advance()
            var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
            target_vars.append(var_token.value)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' after target variables")
        self.consume(TokenType.ASSIGN, "Expected '=' in destructuring assignment")
        
        source_expr = self.parse_expression()
        
        self.consume(TokenType.LEFT_BRACE, "Expected '{' after source expression")
        
        source_fields = []
        field_token = self.consume(TokenType.IDENTIFIER, "Expected field name")
        source_fields.append(field_token.value)
        
        while self.match(TokenType.COMMA):
            self.advance()
            field_token = self.consume(TokenType.IDENTIFIER, "Expected field name")
            source_fields.append(field_token.value)
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' after source fields")
        self.consume(TokenType.SEMICOLON, "Expected ';' after destructuring assignment")
        
        return DestructuringStmt(target_vars=target_vars, source_expr=source_expr, source_fields=source_fields,
                               line=start_token.line, column=start_token.column)
    
    # ============================================================================
    # Expression parsing with precedence
    # ============================================================================
    
    def parse_expression(self) -> Expression:
        """Parse expression (lowest precedence)"""
        return self.parse_assignment_expr()
    
    def parse_assignment_expr(self) -> Expression:
        """Parse assignment expression"""
        expr = self.parse_conditional_expr()
        
        if self.match_assignment_op():
            op_token = self.advance()
            op = self.token_to_assignment_op(op_token.type)
            right = self.parse_assignment_expr()
            
            return AssignmentExpression(left=expr, operator=op, right=right,
                                      line=expr.line, column=expr.column)
        
        return expr
    
    def match_assignment_op(self) -> bool:
        """Check if current token is an assignment operator"""
        return self.match(
            TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN,
            TokenType.MULTIPLY_ASSIGN, TokenType.DIVIDE_ASSIGN, TokenType.MODULO_ASSIGN,
            TokenType.POWER_ASSIGN, TokenType.AND_ASSIGN, TokenType.OR_ASSIGN,
            TokenType.XOR_ASSIGN, TokenType.LEFT_SHIFT_ASSIGN, TokenType.RIGHT_SHIFT_ASSIGN,
            TokenType.B_AND_ASSIGN, TokenType.B_NAND_ASSIGN, TokenType.B_OR_ASSIGN,
            TokenType.B_NOR_ASSIGN, TokenType.B_XOR_ASSIGN, TokenType.B_NOT_ASSIGN
        )
    
    def token_to_assignment_op(self, token_type: TokenType) -> AssignmentOperator:
        """Convert token type to assignment operator"""
        mapping = {
            TokenType.ASSIGN: AssignmentOperator.ASSIGN,
            TokenType.PLUS_ASSIGN: AssignmentOperator.PLUS_ASSIGN,
            TokenType.MINUS_ASSIGN: AssignmentOperator.MINUS_ASSIGN,
            TokenType.MULTIPLY_ASSIGN: AssignmentOperator.MULTIPLY_ASSIGN,
            TokenType.DIVIDE_ASSIGN: AssignmentOperator.DIVIDE_ASSIGN,
            TokenType.MODULO_ASSIGN: AssignmentOperator.MODULO_ASSIGN,
            TokenType.POWER_ASSIGN: AssignmentOperator.POWER_ASSIGN,
            TokenType.AND_ASSIGN: AssignmentOperator.AND_ASSIGN,
            TokenType.OR_ASSIGN: AssignmentOperator.OR_ASSIGN,
            TokenType.XOR_ASSIGN: AssignmentOperator.XOR_ASSIGN,
            TokenType.LEFT_SHIFT_ASSIGN: AssignmentOperator.LEFT_SHIFT_ASSIGN,
            TokenType.RIGHT_SHIFT_ASSIGN: AssignmentOperator.RIGHT_SHIFT_ASSIGN,
            TokenType.B_AND_ASSIGN: AssignmentOperator.B_AND_ASSIGN,
            TokenType.B_NAND_ASSIGN: AssignmentOperator.B_NAND_ASSIGN,
            TokenType.B_OR_ASSIGN: AssignmentOperator.B_OR_ASSIGN,
            TokenType.B_NOR_ASSIGN: AssignmentOperator.B_NOR_ASSIGN,
            TokenType.B_XOR_ASSIGN: AssignmentOperator.B_XOR_ASSIGN,
            TokenType.B_NOT_ASSIGN: AssignmentOperator.B_NOT_ASSIGN
        }
        return mapping.get(token_type, AssignmentOperator.ASSIGN)
    
    def parse_conditional_expr(self) -> Expression:
        """Parse conditional expression (ternary)"""
        expr = self.parse_logical_or_expr()
        
        if self.match(TokenType.QUESTION):
            self.advance()
            true_expr = self.parse_expression()
            self.consume(TokenType.COLON, "Expected ':' in conditional expression")
            false_expr = self.parse_conditional_expr()
            
            return ConditionalExpression(condition=expr, true_expr=true_expr, false_expr=false_expr,
                                       line=expr.line, column=expr.column)
        
        return expr
    
    def parse_logical_or_expr(self) -> Expression:
        """Parse logical OR expression"""
        expr = self.parse_logical_and_expr()
        
        while self.match(TokenType.OR, TokenType.LOGICAL_OR, TokenType.LOGICAL_NOR):
            op_token = self.advance()
            right = self.parse_logical_and_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_logical_and_expr(self) -> Expression:
        """Parse logical AND expression"""
        expr = self.parse_bitwise_or_expr()
        
        while self.match(TokenType.AND, TokenType.LOGICAL_AND, TokenType.LOGICAL_NAND):
            op_token = self.advance()
            right = self.parse_bitwise_or_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_bitwise_or_expr(self) -> Expression:
        """Parse bitwise OR expression"""
        expr = self.parse_bitwise_xor_expr()
        
        while self.match(TokenType.BITWISE_OR, TokenType.BITWISE_B_OR, TokenType.BITWISE_B_NOR):
            op_token = self.advance()
            right = self.parse_bitwise_xor_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_bitwise_xor_expr(self) -> Expression:
        """Parse bitwise XOR expression"""
        expr = self.parse_bitwise_and_expr()
        
        while self.match(TokenType.BITWISE_XOR, TokenType.XOR, TokenType.BITWISE_B_XOR, 
                        TokenType.BITWISE_B_XNOR):
            op_token = self.advance()
            right = self.parse_bitwise_and_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_bitwise_and_expr(self) -> Expression:
        """Parse bitwise AND expression"""
        expr = self.parse_identity_expr()
        
        while self.match(TokenType.BITWISE_AND, TokenType.BITWISE_B_AND, TokenType.BITWISE_B_NAND,
                        TokenType.BITWISE_B_XAND, TokenType.BITWISE_B_XNAND):
            op_token = self.advance()
            right = self.parse_identity_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_identity_expr(self) -> Expression:
        """Parse identity expression (is, not)"""
        expr = self.parse_equality_expr()
        
        while self.match(TokenType.IS, TokenType.NOT):
            op_token = self.advance()
            right = self.parse_equality_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_equality_expr(self) -> Expression:
        """Parse equality expression"""
        expr = self.parse_relational_expr()
        
        while self.match(TokenType.EQUAL, TokenType.NOT_EQUAL):
            op_token = self.advance()
            right = self.parse_relational_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_relational_expr(self) -> Expression:
        """Parse relational expression"""
        expr = self.parse_shift_expr()
        
        while self.match(TokenType.LESS_THAN, TokenType.LESS_EQUAL, TokenType.GREATER_THAN,
                        TokenType.GREATER_EQUAL, TokenType.IN):
            op_token = self.advance()
            right = self.parse_shift_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_shift_expr(self) -> Expression:
        """Parse shift expression"""
        expr = self.parse_additive_expr()
        
        while self.match(TokenType.LEFT_SHIFT, TokenType.RIGHT_SHIFT):
            op_token = self.advance()
            right = self.parse_additive_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_additive_expr(self) -> Expression:
        """Parse additive expression"""
        expr = self.parse_multiplicative_expr()
        
        while self.match(TokenType.PLUS, TokenType.MINUS):
            op_token = self.advance()
            right = self.parse_multiplicative_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_multiplicative_expr(self) -> Expression:
        """Parse multiplicative expression"""
        expr = self.parse_exponential_expr()
        
        while self.match(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            op_token = self.advance()
            right = self.parse_exponential_expr()
            op = self.token_to_binary_op(op_token.type)
            expr = BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_exponential_expr(self) -> Expression:
        """Parse exponential expression (right associative)"""
        expr = self.parse_cast_expr()
        
        if self.match(TokenType.POWER):
            op_token = self.advance()
            right = self.parse_exponential_expr()  # Right associative
            op = self.token_to_binary_op(op_token.type)
            return BinaryExpression(left=expr, operator=op, right=right,
                                  line=expr.line, column=expr.column)
        
        return expr
    
    def parse_cast_expr(self) -> Expression:
        """Parse cast expression"""
        if self.match(TokenType.LEFT_PAREN):
            # Could be cast or parenthesized expression
            saved_pos = self.current
            try:
                self.advance()  # consume '('
                cast_type = self.parse_type()
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after cast type")
                expr = self.parse_cast_expr()
                
                return CastExpression(type=cast_type, expression=expr,
                                    line=cast_type.line, column=cast_type.column)
            except ParseError:
                # Not a cast, backtrack
                self.current = saved_pos
        
        return self.parse_unary_expr()
    
    def parse_unary_expr(self) -> Expression:
        """Parse unary expression"""
        if self.match(TokenType.INCREMENT, TokenType.DECREMENT, TokenType.PLUS, TokenType.MINUS,
                     TokenType.LOGICAL_NOT, TokenType.NOT, TokenType.BITWISE_NOT,
                     TokenType.ADDRESS_OF, TokenType.MULTIPLY, TokenType.SIZEOF,
                     TokenType.TYPEOF, TokenType.ALIGNOF):
            op_token = self.advance()
            operand = self.parse_unary_expr()
            op = self.token_to_unary_op(op_token.type)
            
            return UnaryExpression(operator=op, operand=operand, is_postfix=False,
                                 line=op_token.line, column=op_token.column)
        
        return self.parse_postfix_expr()
    
    def parse_postfix_expr(self) -> Expression:
        """Parse postfix expression"""
        expr = self.parse_primary_expr()
        
        while True:
            if self.match(TokenType.LEFT_BRACKET):
                # Array access
                self.advance()
                index = self.parse_expression()
                self.consume(TokenType.RIGHT_BRACKET, "Expected ']' after array index")
                expr = ArrayAccess(array=expr, index=index, line=expr.line, column=expr.column)
            
            elif self.match(TokenType.LEFT_PAREN):
                # Function call
                self.advance()
                arguments = []
                if not self.match(TokenType.RIGHT_PAREN):
                    arguments.append(self.parse_expression())
                    while self.match(TokenType.COMMA):
                        self.advance()
                        arguments.append(self.parse_expression())
                
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function arguments")
                expr = FunctionCall(function=expr, arguments=arguments, line=expr.line, column=expr.column)
            
            elif self.match(TokenType.DOT):
                # Member access
                self.advance()
                member_token = self.consume(TokenType.IDENTIFIER, "Expected member name after '.'")
                expr = MemberAccess(object=expr, member=member_token.value, line=expr.line, column=expr.column)
            
            elif self.match(TokenType.SCOPE):
                # Scope access
                self.advance()
                member_token = self.consume(TokenType.IDENTIFIER, "Expected member name after '::'")
                expr = ScopeAccess(scope=expr, member=member_token.value, line=expr.line, column=expr.column)
            
            elif self.match(TokenType.INCREMENT, TokenType.DECREMENT):
                # Postfix increment/decrement
                op_token = self.advance()
                op = self.token_to_unary_op(op_token.type)
                expr = UnaryExpression(operator=op, operand=expr, is_postfix=True,
                                     line=expr.line, column=expr.column)
            
            else:
                break
        
        return expr
    
    def parse_primary_expr(self) -> Expression:
        """Parse primary expression"""
        current = self.current_token()
        if not current:
            raise ParseError("Unexpected end of input")
        
        # Literals
        if self.match(TokenType.INTEGER):
            token = self.advance()
            radix = 10
            if token.value.startswith('0x'):
                radix = 16
            elif token.value.startswith('0b'):
                radix = 2
            return IntegerLiteral(value=token.value, radix=radix, line=token.line, column=token.column)
        
        elif self.match(TokenType.FLOAT):
            token = self.advance()
            return FloatLiteral(value=token.value, line=token.line, column=token.column)
        
        elif self.match(TokenType.CHAR):
            token = self.advance()
            return CharacterLiteral(value=token.value, line=token.line, column=token.column)
        
        elif self.match(TokenType.STRING):
            token = self.advance()
            return StringLiteral(value=token.value, line=token.line, column=token.column)
        
        elif self.match(TokenType.BOOLEAN):
            token = self.advance()
            return BooleanLiteral(value=token.value == 'true', line=token.line, column=token.column)
        
        # String interpolation
        elif self.match(TokenType.I_STRING):
            return self.parse_i_string()
        
        elif self.match(TokenType.F_STRING):
            token = self.advance()
            return FStringLiteral(content=token.value, line=token.line, column=token.column)
        
        # Array literal
        elif self.match(TokenType.LEFT_BRACKET):
            return self.parse_array_literal_or_comprehension()
        
        # Struct initializer
        elif self.match(TokenType.LEFT_BRACE):
            return self.parse_struct_initializer()
        
        # Parenthesized expression
        elif self.match(TokenType.LEFT_PAREN):
            # Check for function pointer call
            if self.is_function_pointer_call():
                return self.parse_function_pointer_call()
            else:
                self.advance()
                expr = self.parse_expression()
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after expression")
                return expr
        
        # Keywords
        elif self.match(TokenType.THIS):
            self.advance()
            return ThisExpression(line=current.line, column=current.column)
        
        elif self.match(TokenType.SUPER):
            self.advance()
            return SuperExpression(line=current.line, column=current.column)
        
        elif self.match(TokenType.VIRTUAL):
            return self.parse_virtual_qualified_name()
        
        # Range expression
        elif self.is_range_expression():
            return self.parse_range_expression()
        
        # Identifier or qualified name
        elif self.match(TokenType.IDENTIFIER):
            return self.parse_identifier_or_qualified_name()
        
        else:
            raise ParseError(f"Unexpected token: {current.type.name}", current)
    
    def parse_i_string(self) -> IStringLiteral:
        """Parse i-string interpolation"""
        token = self.advance()
        
        # Parse the i-string format: i"template":{expr1;expr2;...}
        # This is simplified - would need more complex parsing for full support
        content = token.value
        
        # Extract template and expressions (simplified)
        template = ""
        expressions = []
        
        # This would need proper parsing of the i-string format
        # For now, return basic structure
        return IStringLiteral(template=template, expressions=expressions,
                            line=token.line, column=token.column)
    
    def parse_array_literal_or_comprehension(self) -> Expression:
        """Parse array literal or comprehension"""
        start_token = self.advance()  # consume '['
        
        if self.match(TokenType.RIGHT_BRACKET):
            # Empty array
            self.advance()
            return ArrayLiteral(elements=[], line=start_token.line, column=start_token.column)
        
        # Parse first element/expression
        first_expr = self.parse_expression()
        
        # Check for comprehension
        if self.match(TokenType.FOR):
            # Array comprehension
            self.advance()
            self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'for' in comprehension")
            
            var_token = self.consume(TokenType.IDENTIFIER, "Expected variable name")
            variable = var_token.value
            
            self.consume(TokenType.IN, "Expected 'in' in comprehension")
            iterable = self.parse_expression()
            
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after comprehension iterator")
            
            condition = None
            if self.match(TokenType.IF):
                self.advance()
                self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'if' in comprehension")
                condition = self.parse_expression()
                self.consume(TokenType.RIGHT_PAREN, "Expected ')' after comprehension condition")
            
            self.consume(TokenType.RIGHT_BRACKET, "Expected ']' to end comprehension")
            
            return ArrayComprehension(expression=first_expr, variable=variable, iterable=iterable,
                                    condition=condition, line=start_token.line, column=start_token.column)
        
        else:
            # Array literal
            elements = [first_expr]
            
            while self.match(TokenType.COMMA):
                self.advance()
                if self.match(TokenType.RIGHT_BRACKET):
                    break
                elements.append(self.parse_expression())
            
            self.consume(TokenType.RIGHT_BRACKET, "Expected ']' to end array literal")
            
            return ArrayLiteral(elements=elements, line=start_token.line, column=start_token.column)
    
    def parse_struct_initializer(self) -> StructInitializer:
        """Parse struct initializer"""
        start_token = self.advance()  # consume '{'
        
        items = []
        
        if not self.match(TokenType.RIGHT_BRACE):
            items.append(self.parse_struct_init_item())
            
            while self.match(TokenType.COMMA):
                self.advance()
                if self.match(TokenType.RIGHT_BRACE):
                    break
                items.append(self.parse_struct_init_item())
        
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end struct initializer")
        
        return StructInitializer(items=items, line=start_token.line, column=start_token.column)
    
    def parse_struct_init_item(self) -> StructInitItem:
        """Parse struct initialization item"""
        name_token = self.consume(TokenType.IDENTIFIER, "Expected field name")
        name = name_token.value
        
        value = None
        if self.match(TokenType.ASSIGN):
            self.advance()
            value = self.parse_expression()
        
        return StructInitItem(name=name, value=value, line=name_token.line, column=name_token.column)
    
    def parse_array_initializer(self) -> ArrayInitializer:
        """Parse array initializer"""
        if self.match(TokenType.LEFT_BRACKET):
            # Array-style initializer
            start_token = self.advance()
            
            elements = []
            if not self.match(TokenType.RIGHT_BRACKET):
                elements.append(self.parse_expression())
                while self.match(TokenType.COMMA):
                    self.advance()
                    if self.match(TokenType.RIGHT_BRACKET):
                        break
                    elements.append(self.parse_expression())
            
            self.consume(TokenType.RIGHT_BRACKET, "Expected ']' to end array initializer")
            
            return ArrayInitializer(elements=elements, is_struct_style=False,
                                  line=start_token.line, column=start_token.column)
        
        elif self.match(TokenType.LEFT_BRACE):
            # Struct-style initializer
            start_token = self.advance()
            
            struct_items = []
            if not self.match(TokenType.RIGHT_BRACE):
                struct_items.append(self.parse_struct_init_item())
                while self.match(TokenType.COMMA):
                    self.advance()
                    if self.match(TokenType.RIGHT_BRACE):
                        break
                    struct_items.append(self.parse_struct_init_item())
            
            self.consume(TokenType.RIGHT_BRACE, "Expected '}' to end struct initializer")
            
            return ArrayInitializer(struct_items=struct_items, is_struct_style=True,
                                  line=start_token.line, column=start_token.column)
        
        else:
            raise ParseError("Expected '[' or '{' for array initializer")
    
    def is_function_pointer_call(self) -> bool:
        """Check if current position is a function pointer call"""
        return (self.match(TokenType.LEFT_PAREN) and 
                self.peek_token() and self.peek_token().type == TokenType.MULTIPLY)
    
    def parse_function_pointer_call(self) -> FunctionPointerCall:
        """Parse function pointer call (*ptr)(args)"""
        start_token = self.advance()  # consume '('
        self.consume(TokenType.MULTIPLY, "Expected '*' in function pointer call")
        
        pointer = self.parse_expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function pointer")
        self.consume(TokenType.LEFT_PAREN, "Expected '(' for function arguments")
        
        arguments = []
        if not self.match(TokenType.RIGHT_PAREN):
            arguments.append(self.parse_expression())
            while self.match(TokenType.COMMA):
                self.advance()
                arguments.append(self.parse_expression())
        
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function arguments")
        
        return FunctionPointerCall(pointer=pointer, arguments=arguments,
                                 line=start_token.line, column=start_token.column)
    
    def parse_virtual_qualified_name(self) -> VirtualQualifiedName:
        """Parse virtual::qualified_name"""
        start_token = self.advance()  # consume 'virtual'
        self.consume(TokenType.SCOPE, "Expected '::' after 'virtual'")
        
        name = self.parse_qualified_name()
        
        return VirtualQualifiedName(name=name, line=start_token.line, column=start_token.column)
    
    def is_range_expression(self) -> bool:
        """Check if current position starts a range expression"""
        saved_pos = self.current
        try:
            self.parse_shift_expr()  # Parse left side
            if self.match(TokenType.RANGE):
                return True
            return False
        except:
            return False
        finally:
            self.current = saved_pos
    
    def parse_range_expression(self) -> RangeExpression:
        """Parse range expression (start..end)"""
        start = self.parse_shift_expr()
        self.consume(TokenType.RANGE, "Expected '..' in range expression")
        end = self.parse_shift_expr()
        
        return RangeExpression(start=start, end=end, line=start.line, column=start.column)
    
    def parse_identifier_or_qualified_name(self) -> Expression:
        """Parse identifier or qualified name"""
        parts = []
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected identifier")
        parts.append(name_token.value)
        
        # Check for qualified name
        while self.match(TokenType.SCOPE):
            self.advance()
            next_token = self.consume(TokenType.IDENTIFIER, "Expected identifier after '::'")
            parts.append(next_token.value)
        
        if len(parts) == 1:
            return Identifier(name=parts[0], line=name_token.line, column=name_token.column)
        else:
            return QualifiedName(parts=parts, line=name_token.line, column=name_token.column)
    
    def parse_qualified_name(self) -> QualifiedName:
        """Parse qualified name"""
        parts = []
        
        name_token = self.consume(TokenType.IDENTIFIER, "Expected identifier")
        parts.append(name_token.value)
        
        while self.match(TokenType.SCOPE):
            self.advance()
            next_token = self.consume(TokenType.IDENTIFIER, "Expected identifier after '::'")
            parts.append(next_token.value)
        
        return QualifiedName(parts=parts, line=name_token.line, column=name_token.column)
    
    # ============================================================================
    # Operator mapping utilities
    # ============================================================================
    
    def token_to_binary_op(self, token_type: TokenType) -> BinaryOperator:
        """Convert token type to binary operator"""
        mapping = {
            TokenType.PLUS: BinaryOperator.ADD,
            TokenType.MINUS: BinaryOperator.SUBTRACT,
            TokenType.MULTIPLY: BinaryOperator.MULTIPLY,
            TokenType.DIVIDE: BinaryOperator.DIVIDE,
            TokenType.MODULO: BinaryOperator.MODULO,
            TokenType.POWER: BinaryOperator.POWER,
            TokenType.EQUAL: BinaryOperator.EQUAL,
            TokenType.NOT_EQUAL: BinaryOperator.NOT_EQUAL,
            TokenType.LESS_THAN: BinaryOperator.LESS_THAN,
            TokenType.LESS_EQUAL: BinaryOperator.LESS_EQUAL,
            TokenType.GREATER_THAN: BinaryOperator.GREATER_THAN,
            TokenType.GREATER_EQUAL: BinaryOperator.GREATER_EQUAL,
            TokenType.LOGICAL_AND: BinaryOperator.LOGICAL_AND,
            TokenType.LOGICAL_OR: BinaryOperator.LOGICAL_OR,
            TokenType.LOGICAL_NAND: BinaryOperator.LOGICAL_NAND,
            TokenType.LOGICAL_NOR: BinaryOperator.LOGICAL_NOR,
            TokenType.AND: BinaryOperator.AND_KEYWORD,
            TokenType.OR: BinaryOperator.OR_KEYWORD,
            TokenType.BITWISE_AND: BinaryOperator.BITWISE_AND,
            TokenType.BITWISE_OR: BinaryOperator.BITWISE_OR,
            TokenType.BITWISE_XOR: BinaryOperator.BITWISE_XOR,
            TokenType.XOR: BinaryOperator.XOR_KEYWORD,
            TokenType.BITWISE_B_AND: BinaryOperator.BITWISE_B_AND,
            TokenType.BITWISE_B_NAND: BinaryOperator.BITWISE_B_NAND,
            TokenType.BITWISE_B_OR: BinaryOperator.BITWISE_B_OR,
            TokenType.BITWISE_B_NOR: BinaryOperator.BITWISE_B_NOR,
            TokenType.BITWISE_B_XOR: BinaryOperator.BITWISE_B_XOR,
            TokenType.BITWISE_B_XNOR: BinaryOperator.BITWISE_B_XNOR,
            TokenType.BITWISE_B_XAND: BinaryOperator.BITWISE_B_XAND,
            TokenType.BITWISE_B_XNAND: BinaryOperator.BITWISE_B_XNAND,
            TokenType.LEFT_SHIFT: BinaryOperator.LEFT_SHIFT,
            TokenType.RIGHT_SHIFT: BinaryOperator.RIGHT_SHIFT,
            TokenType.IS: BinaryOperator.IS,
            TokenType.NOT: BinaryOperator.NOT,
            TokenType.IN: BinaryOperator.IN,
            TokenType.RANGE: BinaryOperator.RANGE
        }
        return mapping.get(token_type, BinaryOperator.ADD)
    
    def token_to_unary_op(self, token_type: TokenType) -> UnaryOperator:
        """Convert token type to unary operator"""
        mapping = {
            TokenType.PLUS: UnaryOperator.PLUS,
            TokenType.MINUS: UnaryOperator.MINUS,
            TokenType.LOGICAL_NOT: UnaryOperator.LOGICAL_NOT,
            TokenType.NOT: UnaryOperator.NOT_KEYWORD,
            TokenType.BITWISE_NOT: UnaryOperator.BITWISE_NOT,
            TokenType.ADDRESS_OF: UnaryOperator.ADDRESS_OF,
            TokenType.MULTIPLY: UnaryOperator.DEREFERENCE,
            TokenType.INCREMENT: UnaryOperator.PRE_INCREMENT,
            TokenType.DECREMENT: UnaryOperator.PRE_DECREMENT,
            TokenType.SIZEOF: UnaryOperator.SIZEOF,
            TokenType.TYPEOF: UnaryOperator.TYPEOF,
            TokenType.ALIGNOF: UnaryOperator.ALIGNOF
        }
        return mapping.get(token_type, UnaryOperator.PLUS)


def create_parser(source_code: str) -> FluxParser:
    """Create a parser from source code"""
    lexer = FluxLexer(source_code)
    tokens = lexer.tokenize()
    return FluxParser(tokens)


def parse_file(filename: str) -> Program:
    """Parse a Flux file and return the AST"""
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            source_code = f.read()
    except FileNotFoundError:
        raise ParseError(f"File '{filename}' not found")
    except IOError as e:
        raise ParseError(f"Error reading file '{filename}': {e}")
    
    parser = create_parser(source_code)
    return parser.parse()


def print_ast_tree(node: ASTNode, indent: int = 0, show_details: bool = False) -> None:
    """Print AST in tree format"""
    prefix = "  " * indent
    node_name = type(node).__name__
    
    if show_details:
        details = []
        if hasattr(node, 'name') and node.name:
            details.append(f"name='{node.name}'")
        if hasattr(node, 'value') and node.value is not None:
            details.append(f"value='{node.value}'")
        if hasattr(node, 'operator') and node.operator:
            details.append(f"op={node.operator.value}")
        if hasattr(node, 'type') and node.type:
            details.append(f"type={type(node.type).__name__}")
        
        detail_str = f" ({', '.join(details)})" if details else ""
        print(f"{prefix}{node_name}{detail_str}")
    else:
        print(f"{prefix}{node_name}")
    
    # Print child nodes
    for field_name, field_value in node.__dict__.items():
        if field_name in ['line', 'column']:
            continue
            
        if isinstance(field_value, ASTNode):
            print(f"{prefix}  {field_name}:")
            print_ast_tree(field_value, indent + 2, show_details)
        elif isinstance(field_value, list):
            non_ast_items = [item for item in field_value if not isinstance(item, ASTNode)]
            ast_items = [item for item in field_value if isinstance(item, ASTNode)]
            
            if ast_items:
                print(f"{prefix}  {field_name}: [{len(ast_items)} items]")
                for i, item in enumerate(ast_items):
                    print(f"{prefix}    [{i}]:")
                    print_ast_tree(item, indent + 3, show_details)
            elif non_ast_items:
                print(f"{prefix}  {field_name}: {non_ast_items}")


def count_ast_nodes(node: ASTNode) -> Dict[str, int]:
    """Count different types of AST nodes"""
    counts = {}
    
    def count_node(n: ASTNode):
        node_type = type(n).__name__
        counts[node_type] = counts.get(node_type, 0) + 1
        
        for field_name, field_value in n.__dict__.items():
            if isinstance(field_value, ASTNode):
                count_node(field_value)
            elif isinstance(field_value, list):
                for item in field_value:
                    if isinstance(item, ASTNode):
                        count_node(item)
                    elif isinstance(item, tuple):
                        for sub_item in item:
                            if isinstance(sub_item, ASTNode):
                                count_node(sub_item)
    
    count_node(node)
    return counts


def validate_ast_structure(program: Program) -> List[str]:
    """Validate AST structure and return list of issues"""
    from fast import ASTValidator
    validator = ASTValidator()
    return validator.validate(program)


if __name__ == "__main__":
    import argparse
    
    def main():
        parser = argparse.ArgumentParser(description='Flux Language Parser (fparser.py)')
        parser.add_argument('file', help='Flux source file to parse (.fx)')
        parser.add_argument('-v', '--verbose', action='store_true',
                          help='Show verbose parsing information')
        parser.add_argument('-a', '--ast', action='store_true',
                          help='Show AST tree structure')
        parser.add_argument('-c', '--count', action='store_true',
                          help='Count AST node types')
        parser.add_argument('-d', '--details', action='store_true',
                          help='Show detailed AST information (with -a)')
        parser.add_argument('--validate', action='store_true',
                          help='Validate AST structure')
        
        args = parser.parse_args()
        
        try:
            if args.verbose:
                print(f"Parsing file: {args.file}")
                print("=" * 50)
            
            # Parse the file
            program = parse_file(args.file)
            
            if args.verbose:
                print("✓ Parsing completed successfully")
                print(f"✓ Generated AST with {len(program.global_items)} global items")
            
            # Validate AST structure
            if args.validate or args.verbose:
                issues = validate_ast_structure(program)
                if issues:
                    print("\n⚠ AST Validation Issues:")
                    for issue in issues:
                        print(f"  - {issue}")
                else:
                    print("✓ AST validation passed")
            
            # Count nodes
            if args.count or args.verbose:
                node_counts = count_ast_nodes(program)
                total_nodes = sum(node_counts.values())
                
                print(f"\n=== AST Node Summary ===")
                print(f"Total nodes: {total_nodes}")
                print(f"Node types: {len(node_counts)}")
                print("\nNode type counts:")
                for node_type, count in sorted(node_counts.items()):
                    print(f"  {node_type:25} : {count:4}")
            
            # Show AST tree
            if args.ast:
                print(f"\n=== AST Tree Structure for {args.file} ===")
                print_ast_tree(program, show_details=args.details)
            
            # Show global items summary
            if args.verbose:
                print(f"\n=== Global Items Summary ===")
                item_types = {}
                for item in program.global_items:
                    item_type = type(item).__name__
                    item_types[item_type] = item_types.get(item_type, 0) + 1
                
                for item_type, count in sorted(item_types.items()):
                    print(f"  {item_type:25} : {count:4}")
                
                # Show function names
                functions = [item for item in program.global_items if isinstance(item, FunctionDef)]
                if functions:
                    print(f"\nFunction definitions:")
                    for func in functions:
                        modifiers = [mod.value for mod in func.modifiers]
                        mod_str = f" [{', '.join(modifiers)}]" if modifiers else ""
                        template_str = f"<{', '.join(tp.name for tp in func.template_params)}>" if func.template_params else ""
                        print(f"  {func.name}{template_str}{mod_str}")
                
                # Show object names
                objects = [item for item in program.global_items if isinstance(item, ObjectDef)]
                if objects:
                    print(f"\nObject definitions:")
                    for obj in objects:
                        template_str = f"<{', '.join(tp.name for tp in obj.template_params)}>" if obj.template_params else ""
                        inheritance_str = f" : {', '.join(inh.parts[-1] if hasattr(inh, 'parts') else str(inh) for inh in obj.inheritance)}" if obj.inheritance else ""
                        fwd_str = " (forward)" if obj.is_forward_decl else ""
                        print(f"  {obj.name}{template_str}{inheritance_str}{fwd_str}")
                
                # Show struct names
                structs = [item for item in program.global_items if isinstance(item, StructDef)]
                if structs:
                    print(f"\nStruct definitions:")
                    for struct in structs:
                        template_str = f"<{', '.join(tp.name for tp in struct.template_params)}>" if struct.template_params else ""
                        inheritance_str = f" : {', '.join(inh.parts[-1] if hasattr(inh, 'parts') else str(inh) for inh in struct.inheritance)}" if struct.inheritance else ""
                        fwd_str = " (forward)" if struct.is_forward_decl else ""
                        print(f"  {struct.name}{template_str}{inheritance_str}{fwd_str}")
                
                # Show namespace names
                namespaces = [item for item in program.global_items if isinstance(item, NamespaceDef)]
                if namespaces:
                    print(f"\nNamespace definitions:")
                    for ns in namespaces:
                        print(f"  {ns.name} ({len(ns.members)} members)")
        
        except ParseError as e:
            print(f"Parse Error: {e}", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Unexpected Error: {e}", file=sys.stderr)
            sys.exit(1)
    
    main()