#!/usr/bin/env python3
"""
Flux Language Parser

A recursive descent parser for the Flux programming language.
Converts tokens from the lexer into an Abstract Syntax Tree (AST).

Usage:
    python3 parser.py file.fx          # Parse and show AST
    python3 parser.py file.fx -v       # Verbose parsing with debug info
    python3 parser.py file.fx -a       # Show AST structure
"""

import sys
from typing import List, Optional, Union, Any
from flexer import FluxLexer, TokenType, Token
from fast import *

class ParseError(Exception):
    """Exception raised when parsing fails"""
    def __init__(self, message: str, token: Optional[Token] = None):
        self.message = message
        self.token = token
        super().__init__(f"Parse error: {message}" + (f" at {token.line}:{token.column}" if token else ""))

class FluxParser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.position = 0
        self.current_token = self.tokens[0] if tokens else None
    
    def error(self, message: str) -> None:
        """Raise a parse error with current token context"""
        raise ParseError(message, self.current_token)
    
    def advance(self) -> Token:
        """Move to the next token"""
        if self.position < len(self.tokens) - 1:
            self.position += 1
            self.current_token = self.tokens[self.position]
        return self.current_token
    
    def peek(self, offset: int = 1) -> Optional[Token]:
        """Look ahead at the next token without consuming it"""
        pos = self.position + offset
        if pos < len(self.tokens):
            return self.tokens[pos]
        return None
    
    def expect(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of the given types"""
        if self.current_token is None:
            return False
        return self.current_token.type in token_types
    
    def consume(self, expected_type: TokenType, message: str = None) -> Token:
        """Consume a token of the expected type or raise error"""
        if not self.expect(expected_type):
            msg = message or f"Expected {expected_type.name}, got {self.current_token.type.name if self.current_token else 'EOF'}"
            self.error(msg)
        token = self.current_token
        self.advance()
        return token
    
    def synchronize(self) -> None:
        """Synchronize parser state after an error"""
        self.advance()
        while not self.expect(TokenType.EOF):
            if self.tokens[self.position - 1].type == TokenType.SEMICOLON:
                return
            if self.expect(TokenType.DEF, TokenType.STRUCT, TokenType.OBJECT, 
                         TokenType.NAMESPACE, TokenType.IF, TokenType.WHILE,
                         TokenType.FOR, TokenType.RETURN, TokenType.IMPORT):
                return
            self.advance()

    # ============ GRAMMAR RULES ============
    
    def parse(self) -> Program:
        """
        program -> statement* EOF
        """
        statements = []
        while not self.expect(TokenType.EOF):
            try:
                stmt = self.statement()
                if isinstance(stmt, list):
                    statements.extend(stmt)
                elif stmt:
                    statements.append(stmt)
            except ParseError as e:
                print(f"Parse error: {e}", file=sys.stderr)
                self.synchronize()
        return Program(statements)
    
    def statement(self) -> Optional[Statement]:
        """
        statement -> import_statement
                  | function_def_statement
                  | struct_def
                  | object_def_statement
                  | namespace_def
                  | custom_type_statement
                  | variable_declaration ';'
                  | expression_statement
                  | assignment_statement
                  | control_statement
        """
        if self.expect(TokenType.IMPORT):
            return self.import_statement()
        elif self.expect(TokenType.USING):
            return self.using_statement()
        elif self.expect(TokenType.DEF):
            return self.function_def()
        elif self.expect(TokenType.UNION):
            return self.union_def()
        elif self.expect(TokenType.STRUCT):
            return self.struct_def()
        elif self.expect(TokenType.OBJECT):
            return self.object_def()
        elif self.expect(TokenType.NAMESPACE):
            return self.namespace_def()
        elif self.expect(TokenType.IF):
            return self.if_statement()
        elif self.expect(TokenType.WHILE):
            return self.while_statement()
        elif self.expect(TokenType.FOR):
            return self.for_statement()
        elif self.expect(TokenType.DO):
            return self.do_while_statement()
        elif self.expect(TokenType.SWITCH):
            return self.switch_statement()
        elif self.expect(TokenType.TRY):
            return self.try_statement()
        elif self.expect(TokenType.RETURN):
            return self.return_statement()
        elif self.expect(TokenType.BREAK):
            return self.break_statement()
        elif self.expect(TokenType.CONTINUE):
            return self.continue_statement()
        elif self.expect(TokenType.THROW):
            return self.throw_statement()
        elif self.expect(TokenType.ASSERT):
            return self.assert_statement()
        elif self.expect(TokenType.LEFT_BRACE):
            return self.block_statement()
        elif self.is_variable_declaration():
            return self.variable_declaration_statement()
        elif self.expect(TokenType.UNSIGNED):
            return self.variable_declaration_statement()
        elif self.expect(TokenType.SIGNED):
            return self.variable_declaration_statement()
        elif self.expect(TokenType.CONST):
            self.advance()
            if self.expect(TokenType.VOLATILE):
                self.advance()
                if self.expect(TokenType.DEF):
                    return self.function_def()
                else:
                    return self.variable_declaration_statement()
        elif self.expect(TokenType.SEMICOLON):
            self.advance()
            return None
        elif self.expect(TokenType.AUTO) and self.peek() and self.peek().type == TokenType.LEFT_BRACE:
            # Handle destructuring assignment
            destructure = self.destructuring_assignment()
            self.consume(TokenType.SEMICOLON)
            return destructure
        elif self.expect(TokenType.ASM):
            return self.asm_statement()
        else:
            return self.expression_statement()
    
    def import_statement(self) -> ImportStatement:
        """
        import_statement -> 'import' STRING_LITERAL ';'
        """
        self.consume(TokenType.IMPORT)
        module_name = self.consume(TokenType.STRING_LITERAL).value
        self.consume(TokenType.SEMICOLON)
        return ImportStatement(module_name)
    
    def using_statement(self) -> UsingStatement:
        """
        using_statement -> 'using' namespace_path (',' namespace_path)* ';'
        namespace_path -> IDENTIFIER ('::' IDENTIFIER)*
        """
        self.consume(TokenType.USING)
        
        # Parse namespace path (e.g., "standard::io")
        namespace_path = self.consume(TokenType.IDENTIFIER).value
        while self.expect(TokenType.SCOPE):  # ::
            self.advance()
            namespace_path += "::" + self.consume(TokenType.IDENTIFIER).value
        
        # For now, handle only single namespace per statement
        # TODO: Add support for comma-separated namespaces
        self.consume(TokenType.SEMICOLON)
        return UsingStatement(namespace_path)
    
    def function_def(self) -> FunctionDef:
        """
        function_def -> ('const')? ('volatile')? 'def' IDENTIFIER '(' parameter_list? ')' '->' type_spec ';'
        function_def -> ('const')? ('volatile')? 'def' IDENTIFIER '(' parameter_list? ')' '->' type_spec block ';'
        """
        is_const = False
        is_volatile = False
        
        if self.expect(TokenType.CONST):
            is_const = True
            self.advance()
        
        if self.expect(TokenType.VOLATILE):
            is_volatile = True
            self.advance()
        
        self.consume(TokenType.DEF)
        name = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.LEFT_PAREN)
        parameters = self.parameter_list() if not self.expect(TokenType.RIGHT_PAREN) else []
        self.consume(TokenType.RIGHT_PAREN)
        
        self.consume(TokenType.RETURN_ARROW)
        return_type = self.type_spec()
        
        # Check if this is a prototype (ends with semicolon) or definition (has block)
        is_prototype = False
        body = None
        if self.expect(TokenType.SEMICOLON):
            is_prototype = True
            self.advance()
            body = Block([])  # Empty block for prototype
        else:
            body = self.block()
            self.consume(TokenType.SEMICOLON)
        
        return FunctionDef(name, parameters, return_type, body, is_const, is_volatile, is_prototype)
    
    def parameter_list(self) -> List[Parameter]:
        """
        parameter_list -> parameter (',' parameter)*
        """
        params = [self.parameter()]
        
        while self.expect(TokenType.COMMA):
            self.advance()
            params.append(self.parameter())
        
        return params
    
    def parameter(self) -> Parameter:
        """
        parameter -> type_spec IDENTIFIER
        """
        type_spec = self.type_spec()
        name = self.consume(TokenType.IDENTIFIER).value
        return Parameter(name, type_spec)

    def union_def(self) -> UnionDef:
        """
        union_def -> 'union' IDENTIFIER (';' | '{' union_member* '}' ';')
        """
        self.consume(TokenType.UNION)
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Handle forward declaration
        if self.expect(TokenType.SEMICOLON):
            self.advance()
            return UnionDef(name, [])
        
        self.consume(TokenType.LEFT_BRACE)
        members = []
        
        while not self.expect(TokenType.RIGHT_BRACE):
            members.append(self.union_member())
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.SEMICOLON)
        return UnionDef(name, members)

    def union_member(self) -> UnionMember:
        """
        union_member -> type_spec IDENTIFIER ('=' expression)? ';'
        """
        type_spec = self.type_spec()
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Optional initial value
        initial_value = None
        if self.expect(TokenType.ASSIGN):
            self.advance()
            initial_value = self.expression()
        
        self.consume(TokenType.SEMICOLON)
        return UnionMember(name, type_spec, initial_value)
    
    def struct_def(self) -> StructDef:
        """
        struct_def -> 'struct' IDENTIFIER'{' struct_member* '}'
        """
        self.consume(TokenType.STRUCT)
        name = self.consume(TokenType.IDENTIFIER).value
        
        base_structs = []
        members = []
        nested_structs = []

        # Handle forward declarations
        if self.expect(TokenType.SEMICOLON):
            self.advance()
            return StructDef(name, members, base_structs, nested_structs)

        self.consume(TokenType.LEFT_BRACE)
        
        while not self.expect(TokenType.RIGHT_BRACE):
            if self.expect(TokenType.PUBLIC):
                self.advance()
                self.consume(TokenType.LEFT_BRACE)
                while not self.expect(TokenType.RIGHT_BRACE):
                    if self.expect(TokenType.STRUCT):
                        nested_struct = self.struct_def()
                        nested_structs.append(nested_struct)
                        self.consume(TokenType.SEMICOLON)
                    else:
                        member = self.struct_member()
                        member.is_private = False
                        members.append(member)
                self.consume(TokenType.RIGHT_BRACE)
                self.consume(TokenType.SEMICOLON)
            elif self.expect(TokenType.PRIVATE):
                self.advance()
                self.consume(TokenType.LEFT_BRACE)
                while not self.expect(TokenType.RIGHT_BRACE):
                    if self.expect(TokenType.STRUCT):
                        nested_struct = self.struct_def()
                        nested_structs.append(nested_struct)
                        self.consume(TokenType.SEMICOLON)
                    else:
                        member = self.struct_member()
                        member.is_private = True
                        members.append(member)
                self.consume(TokenType.RIGHT_BRACE)
                self.consume(TokenType.SEMICOLON)
            elif self.expect(TokenType.STRUCT):
                # Handle nested struct
                nested_struct = self.struct_def()
                nested_structs.append(nested_struct)
                # Allow both with and without semicolon for nested structs
                if self.expect(TokenType.SEMICOLON):
                    self.advance()
            else:
                members.append(self.struct_member())
        
        self.consume(TokenType.RIGHT_BRACE)
        # Make semicolon optional after struct definition
        if self.expect(TokenType.SEMICOLON):
            self.advance()
        return StructDef(name, members, base_structs, nested_structs)
    
    def struct_member(self) -> StructMember:
        """
        struct_member -> type_spec IDENTIFIER ';' // OR STRUCT, for nested
        """
        if self.expect(TokenType.STRUCT):
            self.advance()
            name = self.consume(TokenType.IDENTIFIER).value
            self.consume(TokenType.SEMICOLON)
            # Create a dummy type spec for the forward declaration
            type_spec = TypeSpec(name, is_signed=True)  # Using name as base_type
            return StructMember(name, type_spec)

        type_spec = self.type_spec()
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Handle optional initial value
        initial_value = None
        if self.expect(TokenType.ASSIGN):
            self.advance()
            initial_value = self.expression()
        
        self.consume(TokenType.SEMICOLON)
        return StructMember(name, type_spec, initial_value)
    
    def object_def(self) -> ObjectDef:
        """
        object_def -> 'object' IDENTIFIER '{' object_body '}'
        object_body -> (object_member | access_specifier)*
        """
        self.consume(TokenType.OBJECT)
        name = self.consume(TokenType.IDENTIFIER).value

        
        # Parse inheritance -- TODO, impelment after we have v1 Flux base
        #base_objects = []
        #if self.expect(TokenType.COLON):
        #    self.advance()
        #    base_objects.append(self.consume(TokenType.IDENTIFIER).value)
        #    while self.expect(TokenType.COMMA):
        #        self.advance()
        #        base_objects.append(self.consume(TokenType.IDENTIFIER).value)
        
        methods = []
        members = []
        nested_objects = []
        nested_structs = []

        if self.expect(TokenType.SEMICOLON):
            is_prototype = True
            self.advance()
            return ObjectDef(name, methods, members, nested_objects, nested_structs)

        self.consume(TokenType.LEFT_BRACE)
        
        while not self.expect(TokenType.RIGHT_BRACE):
            if self.expect(TokenType.PUBLIC, TokenType.PRIVATE):
                is_private = self.current_token.type == TokenType.PRIVATE
                self.advance()
                self.consume(TokenType.LEFT_BRACE)
                
                while not self.expect(TokenType.RIGHT_BRACE):
                    if self.expect(TokenType.DEF):
                        method = self.function_def()
                        method.is_private = is_private
                        methods.append(method)
                    elif self.expect(TokenType.OBJECT):
                        nested_obj = self.object_def()
                        nested_obj.is_private = is_private
                        nested_objects.append(nested_obj)
                        self.consume(TokenType.SEMICOLON)
                    elif self.expect(TokenType.STRUCT):
                        nested_struct = self.struct_def()
                        nested_struct.is_private = is_private
                        nested_structs.append(nested_struct)
                        self.consume(TokenType.SEMICOLON)
                    else:
                        # Field declaration
                        var = self.variable_declaration()
                        self.consume(TokenType.SEMICOLON)
                        member = StructMember(var.name, var.type_spec, var.initial_value, is_private)
                        members.append(member)
                
                self.consume(TokenType.RIGHT_BRACE)
                self.consume(TokenType.SEMICOLON)
            else:
                # Regular member (defaults to public)
                if self.expect(TokenType.DEF):
                    method = self.function_def()
                    methods.append(method)
                elif self.expect(TokenType.OBJECT):
                    nested_obj = self.object_def()
                    nested_objects.append(nested_obj)
                    self.consume(TokenType.SEMICOLON)
                elif self.expect(TokenType.STRUCT):
                    nested_struct = self.struct_def()
                    nested_structs.append(nested_struct)
                    self.consume(TokenType.SEMICOLON)
                else:
                    # Field declaration
                    var = self.variable_declaration()
                    self.consume(TokenType.SEMICOLON)
                    member = StructMember(var.name, var.type_spec, var.initial_value, False)
                    members.append(member)
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.SEMICOLON)
        return ObjectDef(name, methods, members, nested_objects, nested_structs)
    
    def namespace_def(self) -> NamespaceDef:
        """
        namespace_def -> 'namespace' IDENTIFIER  '{' namespace_body* '}'
        """
        self.consume(TokenType.NAMESPACE)
        name = self.consume(TokenType.IDENTIFIER).value
        
        # INHERITANCE - TODO after v1 Flux
        base_namespaces = []
        #if self.expect(TokenType.COLON):
        #    self.advance()
        #    base_namespaces.append(self.consume(TokenType.IDENTIFIER).value)
        #    while self.expect(TokenType.COMMA):
        #        self.advance()
        #        base_namespaces.append(self.consume(TokenType.IDENTIFIER).value)
        
        functions = []
        structs = []
        objects = []
        variables = []
        nested_namespaces = []

        if self.expect(TokenType.SEMICOLON):
            self.advance()
            return NamespaceDef(name, functions, structs, objects, variables, nested_namespaces, base_namespaces)

        self.consume(TokenType.LEFT_BRACE)
        
        while not self.expect(TokenType.RIGHT_BRACE):
            if self.expect(TokenType.DEF):
                functions.append(self.function_def())
            elif self.expect(TokenType.STRUCT):
                structs.append(self.struct_def())
            elif self.expect(TokenType.OBJECT):
                objects.append(self.object_def())
            elif self.expect(TokenType.NAMESPACE):
                nested_namespaces.append(self.namespace_def())
            elif self.is_variable_declaration():
                var_decl = self.variable_declaration()
                variables.append(var_decl)
                self.consume(TokenType.SEMICOLON)
            else:
                self.error("Expected function, struct, object, namespace, or variable declaration")
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.SEMICOLON)
        return NamespaceDef(name, functions, structs, objects, variables, nested_namespaces, base_namespaces)
    
    def type_spec(self) -> TypeSpec:
        """
        type_spec -> ('const')? ('volatile')? ('signed'|'unsigned')? base_type alignment? array_spec? pointer_spec?
        """
        is_const = False
        is_volatile = False
        is_signed = True


        if self.expect(TokenType.CONST):
            is_const = True
            self.advance()
        
        if self.expect(TokenType.VOLATILE):
            is_volatile = True
            self.advance()
        
        if self.expect(TokenType.SIGNED):
            is_signed = True
            self.advance()

        elif self.expect(TokenType.UNSIGNED):
            is_signed = False
            self.advance()
        
        # Base type
        base_type = self.base_type()
        
        # Bit width and alignment for data types
        bit_width = None
        alignment = None
        
        if base_type == DataType.DATA and self.expect(TokenType.LEFT_BRACE):
            self.advance()
            bit_width = int(self.consume(TokenType.INTEGER).value, 0)
            
            if self.expect(TokenType.COLON):
                self.advance()
                alignment = int(self.consume(TokenType.INTEGER).value, 0)
            
            self.consume(TokenType.RIGHT_BRACE)
        
        # Array specification
        is_array = False
        array_size = None
        
        if self.expect(TokenType.LEFT_BRACKET):
            is_array = True
            self.advance()
            if not self.expect(TokenType.RIGHT_BRACKET):
                array_size = int(self.consume(TokenType.INTEGER).value, 0)
            self.consume(TokenType.RIGHT_BRACKET)
        
        # Pointer specification
        is_pointer = False
        if self.expect(TokenType.MULTIPLY):
            is_pointer = True
            self.advance()
        
        return TypeSpec(base_type, is_signed, is_const, is_volatile, 
                       bit_width, alignment, is_array, array_size, is_pointer)
    
    def base_type(self) -> DataType:
        """
        base_type -> 'int' | 'float' | 'char' | 'bool' | 'data' | 'void' | IDENTIFIER
        """
        if self.expect(TokenType.INT):
            self.advance()
            return DataType.INT
        elif self.expect(TokenType.FLOAT_KW):
            self.advance()
            return DataType.FLOAT
        elif self.expect(TokenType.CHAR):
            self.advance()
            return DataType.CHAR
        elif self.expect(TokenType.BOOL_KW):
            self.advance()
            return DataType.BOOL
        elif self.expect(TokenType.DATA):
            self.advance()
            return DataType.DATA
        elif self.expect(TokenType.VOID):
            self.advance()
            return DataType.VOID
        elif self.expect(TokenType.THIS):
            self.advance()
            return DataType.THIS
        # Fixed-width integer types
        elif self.expect(TokenType.UINT8):
            self.advance()
            return DataType.UINT8
        elif self.expect(TokenType.UINT16):
            self.advance()
            return DataType.UINT16
        elif self.expect(TokenType.UINT32):
            self.advance()
            return DataType.UINT32
        elif self.expect(TokenType.UINT64):
            self.advance()
            return DataType.UINT64
        elif self.expect(TokenType.INT8):
            self.advance()
            return DataType.INT8
        elif self.expect(TokenType.INT16):
            self.advance()
            return DataType.INT16
        elif self.expect(TokenType.INT32):
            self.advance()
            return DataType.INT32
        elif self.expect(TokenType.INT64):
            self.advance()
            return DataType.INT64
        elif self.expect(TokenType.IDENTIFIER):
            # Custom type - for now treat as DATA
            self.advance()
            return DataType.DATA
        else:
            self.error("Expected type specifier")
    
    def is_variable_declaration(self) -> bool:
        """Check if current position starts a variable declaration"""
        saved_pos = self.position
        try:
            # Skip type specifiers
            if self.expect(TokenType.CONST):
                self.advance()
            if self.expect(TokenType.VOLATILE):
                self.advance()
            if self.expect(TokenType.SIGNED, TokenType.UNSIGNED):
                self.advance()
            
            # Must have a base type
            if not self.expect(TokenType.INT, TokenType.FLOAT_KW, TokenType.CHAR, 
                             TokenType.BOOL_KW, TokenType.DATA, TokenType.VOID, 
                             TokenType.IDENTIFIER, TokenType.UINT8, TokenType.UINT16,
                             TokenType.UINT32, TokenType.UINT64, TokenType.INT8,
                             TokenType.INT16, TokenType.INT32, TokenType.INT64):
                return False
            
            self.advance()
            
            # Skip data type specification
            if self.expect(TokenType.LEFT_BRACE):
                self.advance()
                if self.expect(TokenType.INTEGER):
                    self.advance()
                if self.expect(TokenType.COLON):
                    self.advance()
                    if self.expect(TokenType.INTEGER):
                        self.advance()
                if self.expect(TokenType.RIGHT_BRACE):
                    self.advance()
            
            # Skip array specification
            if self.expect(TokenType.LEFT_BRACKET):
                self.advance()
                if self.expect(TokenType.INTEGER):
                    self.advance()
                if self.expect(TokenType.RIGHT_BRACKET):
                    self.advance()
            
            # Skip pointer
            if self.expect(TokenType.MULTIPLY):
                self.advance()
            
            # Must have identifier or 'as' keyword
            return self.expect(TokenType.IDENTIFIER, TokenType.AS)
        finally:
            self.position = saved_pos
            self.current_token = self.tokens[self.position] if self.position < len(self.tokens) else None
    
    def variable_declaration_statement(self) -> Statement:
        """
        variable_declaration_statement -> variable_declaration ';'
        """
        decl = self.variable_declaration()
        self.consume(TokenType.SEMICOLON)
        return ExpressionStatement(decl) if isinstance(decl, VariableDeclaration) else decl
    
    def variable_declaration(self) -> Union[VariableDeclaration, TypeDeclaration]:
        """
        variable_declaration -> type_spec IDENTIFIER ('=' expression)?
                             | type_spec 'as' IDENTIFIER ('=' expression)?
        """
        type_spec = self.type_spec()

        # Check if this is a type declaration (using 'as')
        if self.expect(TokenType.AS):
            self.advance()
            type_name = self.consume(TokenType.IDENTIFIER).value
            
            # Optional initial value
            initial_value = None
            if self.expect(TokenType.ASSIGN):
                self.advance()
                initial_value = self.expression()
            
            return TypeDeclaration(type_name, type_spec, initial_value)
        else:
            # Regular variable declaration
            name = self.consume(TokenType.IDENTIFIER).value
            
            # Handle comma-separated variables
            names = [name]
            while self.expect(TokenType.COMMA):
                self.advance()
                names.append(self.consume(TokenType.IDENTIFIER).value)
            
            # Optional initial value (only for last variable)
            initial_value = None
            if self.expect(TokenType.ASSIGN):
                self.advance()
                initial_value = self.expression()
            
            # For now, just return the first declaration
            return VariableDeclaration(names[0], type_spec, initial_value)
    
    def block_statement(self) -> Block:
        """
        block_statement -> block
        """
        return self.block()
    
    def block(self) -> Block:
        """
        block -> '{' statement* '}'
        """
        self.consume(TokenType.LEFT_BRACE)
        statements = []
        
        while not self.expect(TokenType.RIGHT_BRACE):
            stmt = self.statement()
            if stmt:
                statements.append(stmt)
        
        self.consume(TokenType.RIGHT_BRACE)
        return Block(statements)

    def asm_statement(self) -> ExpressionStatement:
        """
        asm_statement -> ('volatile')? 'asm' '{' assembly_tokens '}' ';'
        """
        # Check for volatile keyword
        is_volatile = False
        if self.expect(TokenType.VOLATILE):
            is_volatile = True
            self.advance()
        
        self.consume(TokenType.ASM)
        self.consume(TokenType.LEFT_BRACE)
        
        # Collect assembly tokens
        asm_tokens = []
        while not self.expect(TokenType.RIGHT_BRACE):
            asm_tokens.append(self.current_token.value)
            self.advance()
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.SEMICOLON)
        
        return ExpressionStatement(InlineAsm(
            body=' '.join(asm_tokens),
            is_volatile=is_volatile
        ))
    
    def if_statement(self) -> IfStatement:
        """
        if_statement -> 'if' '(' expression ')' block ('elif' '(' expression ')' block)* ('else' block)? ';'
        """
        self.consume(TokenType.IF)
        self.consume(TokenType.LEFT_PAREN)
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN)
        then_block = self.block()
        
        elif_blocks = []
        while self.expect(TokenType.ELIF):
            self.advance()
            self.consume(TokenType.LEFT_PAREN)
            elif_condition = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
            elif_block = self.block()
            elif_blocks.append((elif_condition, elif_block))
        
        else_block = None
        if self.expect(TokenType.ELSE):
            self.advance()
            else_block = self.block()
        
        self.consume(TokenType.SEMICOLON)
        return IfStatement(condition, then_block, elif_blocks, else_block)
    
    def while_statement(self) -> WhileLoop:
        """
        while_statement -> 'while' '(' expression ')' block ';'
        """
        self.consume(TokenType.WHILE)
        self.consume(TokenType.LEFT_PAREN)
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN)
        body = self.block()
        self.consume(TokenType.SEMICOLON)
        return WhileLoop(condition, body)
    
    def do_while_statement(self) -> DoWhileLoop:
        """
        do_while_statement -> 'do' block 'while' '(' expression ')' ';'
        """
        self.consume(TokenType.DO)
        body = self.block()
        self.consume(TokenType.WHILE)
        self.consume(TokenType.LEFT_PAREN)
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN)
        self.consume(TokenType.SEMICOLON)
        return DoWhileLoop(body, condition)
    
    def for_statement(self) -> Union[ForLoop, ForInLoop]:
        """
        for_statement -> 'for' '(' (for_in_loop | for_c_loop) ')' block ';'
        """
        self.consume(TokenType.FOR)
        self.consume(TokenType.LEFT_PAREN)
        
        # Check if it's a for-in loop by looking ahead
        saved_pos = self.position
        is_for_in = False
        
        # Look for pattern: identifier (',' identifier)* 'in' expression
        if self.expect(TokenType.IDENTIFIER):
            self.advance()
            while self.expect(TokenType.COMMA):
                self.advance()
                if self.expect(TokenType.IDENTIFIER):
                    self.advance()
                else:
                    break
            if self.expect(TokenType.IN):
                is_for_in = True
        
        # Restore position
        self.position = saved_pos
        self.current_token = self.tokens[self.position]
        
        if is_for_in:
            # for-in loop
            variables = []
            variables.append(self.consume(TokenType.IDENTIFIER).value)
            
            while self.expect(TokenType.COMMA):
                self.advance()
                variables.append(self.consume(TokenType.IDENTIFIER).value)
            
            self.consume(TokenType.IN)
            iterable = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
            body = self.block()
            self.consume(TokenType.SEMICOLON)
            
            return ForInLoop(variables, iterable, body)
        else:
            # C-style for loop
            init = None
            if not self.expect(TokenType.SEMICOLON):
                if self.is_variable_declaration():
                    init = ExpressionStatement(self.variable_declaration())
                else:
                    init = self.expression_statement()
            else:
                self.consume(TokenType.SEMICOLON)
            
            condition = None
            if not self.expect(TokenType.SEMICOLON):
                condition = self.expression()
            self.consume(TokenType.SEMICOLON)
            
            update = None
            if not self.expect(TokenType.RIGHT_PAREN):
                update = self.expression_statement()
            
            self.consume(TokenType.RIGHT_PAREN)
            body = self.block()
            self.consume(TokenType.SEMICOLON)
            
            return ForLoop(init, condition, update, body)
    
    def switch_statement(self) -> SwitchStatement:
        """
        switch_statement -> 'switch' '(' expression ')' '{' switch_case* '}' ';'
        """
        self.consume(TokenType.SWITCH)
        self.consume(TokenType.LEFT_PAREN)
        expression = self.expression()
        self.consume(TokenType.RIGHT_PAREN)
        self.consume(TokenType.LEFT_BRACE)
        
        cases = []
        while not self.expect(TokenType.RIGHT_BRACE):
            case = self.switch_case()
            cases.append(case)
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.SEMICOLON)
        return SwitchStatement(expression, cases)
    
    def switch_case(self) -> Case:
        """
        switch_case -> ('case' '(' expression ')' | 'default' '{' statement* '}' ';') block
        """
        value = None
        if self.expect(TokenType.CASE):
            self.advance()
            self.consume(TokenType.LEFT_PAREN)
            value = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
        elif self.expect(TokenType.DEFAULT):
            self.advance()
            value = None
        else:
            self.error("Expected 'case' or 'default'")
        
        body = self.block()
        self.consume(TokenType.SEMICOLON)
        return Case(value, body)
    
    def try_statement(self) -> TryBlock:
        """
        try_statement -> 'try' block catch_block+ ';'
        """
        self.consume(TokenType.TRY)
        try_body = self.block()
        
        catch_blocks = []
        while self.expect(TokenType.CATCH):
            self.advance()
            self.consume(TokenType.LEFT_PAREN)
            
            # Exception type and name
            if self.expect(TokenType.AUTO):
                self.advance()
                exception_type = None
                exception_name = self.consume(TokenType.IDENTIFIER).value
            else:
                exception_type = self.type_spec()
                exception_name = self.consume(TokenType.IDENTIFIER).value
            
            self.consume(TokenType.RIGHT_PAREN)
            catch_body = self.block()
            catch_blocks.append((exception_type, exception_name, catch_body))
        
        self.consume(TokenType.SEMICOLON)
        return TryBlock(try_body, catch_blocks)
    
    def return_statement(self) -> ReturnStatement:
        """
        return_statement -> 'return' expression? ';'
        """
        self.consume(TokenType.RETURN)
        value = None
        if not self.expect(TokenType.SEMICOLON):
            value = self.expression()
        self.consume(TokenType.SEMICOLON)
        return ReturnStatement(value)
    
    def break_statement(self) -> BreakStatement:
        """
        break_statement -> 'break' ';'
        """
        self.consume(TokenType.BREAK)
        self.consume(TokenType.SEMICOLON)
        return BreakStatement()
    
    def continue_statement(self) -> ContinueStatement:
        """
        continue_statement -> 'continue' ';'
        """
        self.consume(TokenType.CONTINUE)
        self.consume(TokenType.SEMICOLON)
        return ContinueStatement()

    def throw_statement(self) -> ThrowStatement:
        """
        throw_statement -> 'throw' '(' expression ')' ';'
        """
        self.consume(TokenType.THROW)
        self.consume(TokenType.LEFT_PAREN)
        expression = self.expression()
        self.consume(TokenType.RIGHT_PAREN)
        self.consume(TokenType.SEMICOLON)
        return ThrowStatement(expression)
    
    def assert_statement(self) -> AssertStatement:
        """
        assert_statement -> 'assert' '(' expression (',' CHAR)? ')' ';'
        """
        self.consume(TokenType.ASSERT)
        self.consume(TokenType.LEFT_PAREN)
        condition = self.expression()
        
        message = None
        if self.expect(TokenType.COMMA):
            self.advance()
            message = self.consume(TokenType.CHAR).value
        
        self.consume(TokenType.RIGHT_PAREN)
        self.consume(TokenType.SEMICOLON)
        return AssertStatement(condition, message)
    
    def expression_statement(self) -> ExpressionStatement:
        """
        expression_statement -> expression ';'
        """
        expr = self.expression()
        self.consume(TokenType.SEMICOLON)
        return ExpressionStatement(expr)
    
    def expression(self) -> Expression:
        """
        expression -> assignment_expression
        """
        return self.assignment_expression()
    
    def assignment_expression(self) -> Expression:
        """
        assignment_expression -> logical_or_expression ('=' assignment_expression)?
        """
        expr = self.logical_or_expression()
        
        if self.expect(TokenType.ASSIGN):
            self.advance()
            value = self.assignment_expression()
            return Assignment(expr, value)
        
        return expr
    
    def logical_or_expression(self) -> Expression:
        """
        logical_or_expression -> logical_and_expression ('or' logical_and_expression)*
        """
        expr = self.logical_and_expression()
        
        while self.expect(TokenType.OR):
            operator = Operator.OR
            self.advance()
            right = self.logical_and_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def logical_and_expression(self) -> Expression:
        """
        logical_and_expression -> logical_xor_expression ('and' equality_expression)*
        """
        expr = self.logical_xor_expression()
        
        while self.expect(TokenType.AND):
            operator = Operator.AND
            self.advance()
            right = self.logical_xor_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr

    def logical_xor_expression(self) -> Expression:
        """
        logical_xor_expression -> equality_expression ('xor' equality_expression)*
        """
        expr = self.equality_expression()

        while (self.expect(TokenType.XOR)):
            operator = Operator.XOR
            self.advance()
            right = self.equality_expression()
            expr = BianryOp(expr, operator, right)

        return expr
    
    def equality_expression(self) -> Expression:
        """
        equality_expression -> relational_expression (('==' | '!=') relational_expression)*
        """
        expr = self.relational_expression()
        
        while self.expect(TokenType.EQUAL, TokenType.NOT_EQUAL):
            if self.current_token.type == TokenType.EQUAL:
                operator = Operator.EQUAL
            elif self.current_token.type == TokenType.NOT_EQUAL:
                operator = Operator.NOT_EQUAL
            
            self.advance()
            right = self.relational_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def relational_expression(self) -> Expression:
        """
        relational_expression -> shift_expression (('<' | '<=' | '>' | '>=') shift_expression)*
        """
        expr = self.shift_expression()
        
        while self.expect(TokenType.LESS_EQUAL, TokenType.LESS_THAN, TokenType.LESS_EQUAL, TokenType.GREATER_THAN, TokenType.GREATER_EQUAL):
            if self.current_token.type == TokenType.LESS_EQUAL:
                operator = Operator.LESS_EQUAL, TokenType.LESS_THAN
            elif self.current_token.type == TokenType.LESS_EQUAL:
                operator = Operator.LESS_EQUAL
            elif self.current_token.type == TokenType.GREATER_THAN:
                operator = Operator.GREATER_THAN
            else:  # GE
                operator = Operator.GREATER_EQUAL
            
            self.advance()
            right = self.shift_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def shift_expression(self) -> Expression:
        """
        shift_expression -> additive_expression (('<<' | '>>') additive_expression)*
        """
        expr = self.additive_expression()
        
        while self.expect(TokenType.LESS_EQUAL, TokenType.LEFT_SHIFT, TokenType.RIGHT_SHIFT):
            if self.current_token.type == TokenType.LESS_EQUAL:
                operator = Operator.BITSHIFT_LEFT
            else:  # RIGHT_SHIFT
                operator = Operator.BITSHIFT_RIGHT
            
            self.advance()
            right = self.additive_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def additive_expression(self) -> Expression:
        """
        additive_expression -> multiplicative_expression (('+' | '-') multiplicative_expression)*
        """
        expr = self.multiplicative_expression()
        
        while self.expect(TokenType.PLUS, TokenType.MINUS):
            if self.current_token.type == TokenType.PLUS:
                operator = Operator.ADD
            else:  # MINUS
                operator = Operator.SUB
            
            self.advance()
            right = self.multiplicative_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def multiplicative_expression(self) -> Expression:
        """
        multiplicative_expression -> cast_expression (('*' | '/' | '%') cast_expression)*
        """
        expr = self.cast_expression()
        
        while self.expect(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            if self.current_token.type == TokenType.MULTIPLY:
                operator = Operator.MUL
            elif self.current_token.type == TokenType.DIVIDE:
                operator = Operator.DIV
            else:  # MODULO
                operator = Operator.MOD
            
            self.advance()
            right = self.cast_expression()
            expr = BinaryOp(expr, operator, right)
        
        return expr
    
    def cast_expression(self) -> Expression:
        """
        cast_expression -> ('(' type_spec ')')? unary_expression
        """
        if self.expect(TokenType.LEFT_PAREN):
            # Look ahead to see if this is a cast
            saved_pos = self.position
            try:
                self.advance()  # consume '('
                target_type = self.type_spec()
                if self.expect(TokenType.RIGHT_PAREN):
                    self.advance()  # consume ')'
                    expr = self.unary_expression()
                    return CastExpression(target_type, expr)
                else:
                    # Not a cast, restore position
                    self.position = saved_pos
                    self.current_token = self.tokens[self.position]
            except:
                # Not a cast, restore position
                self.position = saved_pos
                self.current_token = self.tokens[self.position]
        
        return self.unary_expression()
    
    def unary_expression(self) -> Expression:
        """
        unary_expression -> ('not' | '-' | '+' | '*' | '@' | '++' | '--') unary_expression
                         | postfix_expression
        """
        if self.expect(TokenType.NOT):
            operator = Operator.NOT
            self.advance()
            operand = self.unary_expression()
            return UnaryOp(operator, operand)
        elif self.expect(TokenType.MINUS):
            operator = Operator.SUB
            self.advance()
            operand = self.unary_expression()
            return UnaryOp(operator, operand)
        elif self.expect(TokenType.PLUS):
            operator = Operator.ADD
            self.advance()
            operand = self.unary_expression()
            return UnaryOp(operator, operand)
        elif self.expect(TokenType.MULTIPLY):
            # Pointer dereference
            self.advance()
            operand = self.unary_expression()
            return PointerDeref(operand)
        elif self.expect(TokenType.ADDRESS_OF):
            # Address-of operator
            self.advance()
            operand = self.unary_expression()
            return AddressOf(operand)
        elif self.expect(TokenType.INCREMENT):
            # Prefix increment
            self.advance()
            operand = self.unary_expression()
            return UnaryOp(Operator.INCREMENT, operand)
        elif self.expect(TokenType.DECREMENT):
            # Prefix decrement
            self.advance()
            operand = self.unary_expression()
            return UnaryOp(Operator.DECREMENT, operand)
        else:
            return self.postfix_expression()
    
    def postfix_expression(self) -> Expression:
        """
        postfix_expression -> primary_expression (postfix_operator)*
        postfix_operator -> '[' expression ']'
                         | '(' argument_list? ')'
                         | '.' IDENTIFIER
                         | '->' IDENTIFIER
                         | '++'
                         | '--'
        """
        expr = self.primary_expression()
        
        while True:
            if self.expect(TokenType.LEFT_BRACKET):
                # Array access
                self.advance()
                index = self.expression()
                self.consume(TokenType.RIGHT_BRACKET)
                expr = ArrayAccess(expr, index)
            elif self.expect(TokenType.LEFT_PAREN):
                # Function call
                self.advance()
                args = []
                if not self.expect(TokenType.RIGHT_PAREN):
                    args = self.argument_list()
                self.consume(TokenType.RIGHT_PAREN)
                if isinstance(expr, Identifier):
                    expr = FunctionCall(expr.name, args)
                else:
                    # Method call or complex expression
                    expr = FunctionCall("", args)  # This might need refinement
            elif self.expect(TokenType.DOT):
                # Member access
                self.advance()
                member = self.consume(TokenType.IDENTIFIER).value
                expr = MemberAccess(expr, member)
            elif self.expect(TokenType.INCREMENT):
                # Postfix increment
                self.advance()
                expr = UnaryOp(Operator.INCREMENT, expr, is_postfix=True)
            elif self.expect(TokenType.DECREMENT):
                # Postfix decrement
                self.advance()
                expr = UnaryOp(Operator.DECREMENT, expr, is_postfix=True)
            else:
                break
        
        return expr
    
    def argument_list(self) -> List[Expression]:
        """
        argument_list -> expression (',' expression)*
        """
        args = [self.expression()]
        
        while self.expect(TokenType.COMMA):
            self.advance()
            args.append(self.expression())
        
        return args
    
    def primary_expression(self) -> Expression:
        """
        primary_expression -> IDENTIFIER
                           | INTEGER
                           | FLOAT
                           | CHAR
                           | STRING_LITERAL
                           | 'true'
                           | 'false'
                           | 'void'
                           | 'this'
                           | 'super'
                           | '(' expression ')'
                           | array_literal
                           | struct_literal
        """
        if self.expect(TokenType.IDENTIFIER):
            name = self.current_token.value
            self.advance()
            return Identifier(name)
        elif self.expect(TokenType.XOR):
            # Handle XOR as a function call
            self.advance()
            self.consume(TokenType.LEFT_PAREN)
            args = []
            if not self.expect(TokenType.RIGHT_PAREN):
                args = self.argument_list()
            self.consume(TokenType.RIGHT_PAREN)
            return FunctionCall("xor", args)
        elif self.expect(TokenType.INTEGER):
            value = int(self.current_token.value, 0)
            self.advance()
            return Literal(value, DataType.INT)
        elif self.expect(TokenType.FLOAT):
            value = float(self.current_token.value)
            self.advance()
            return Literal(value, DataType.FLOAT)
        elif self.expect(TokenType.CHAR):
            value = self.current_token.value
            self.advance()
            return Literal(value, DataType.CHAR)  # String as char array
        elif self.expect(TokenType.STRING_LITERAL):
            value = self.current_token.value
            self.advance()
            return Literal(value, DataType.CHAR)
        elif self.expect(TokenType.TRUE):
            self.advance()
            return Literal(True, DataType.BOOL)
        elif self.expect(TokenType.FALSE):
            self.advance()
            return Literal(False, DataType.BOOL)
        elif self.expect(TokenType.VOID):
            self.advance()
            return Literal(None, DataType.VOID)
        elif self.expect(TokenType.THIS):
            self.advance()
            return Identifier("this")
        elif self.expect(TokenType.SUPER):
            self.advance()
            return Identifier("super")
        elif self.expect(TokenType.LEFT_PAREN):
            self.advance()
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
            return expr
        elif self.expect(TokenType.LEFT_BRACKET):
            return self.array_literal()
        elif self.expect(TokenType.LEFT_BRACE):
            return self.struct_literal()
        if self.expect(TokenType.SIZEOF):
            return self.sizeof_expression()
        elif self.expect(TokenType.ALIGNOF):
            return self.alignof_expression()
        else:
            self.error(f"Unexpected token: {self.current_token.type.name if self.current_token else 'EOF'}")
    
    def alignof_expression(self) -> AlignOf:
        """
        alignof_expression -> 'alignof' '(' (type_spec | expression) ')'
        """
        self.consume(TokenType.ALIGNOF)
        self.consume(TokenType.LEFT_PAREN)
        
        # Look ahead to determine if it's a type or expression
        saved_pos = self.position
        try:
            # Try to parse as type spec first
            target = self.type_spec()
            self.consume(TokenType.RIGHT_PAREN)
            return AlignOf(target)
        except ParseError:
            # If type parsing fails, try as expression
            self.position = saved_pos
            self.current_token = self.tokens[self.position]
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
            return AlignOf(expr)

    def sizeof_expression(self) -> SizeOf:
        """
        sizeof_expression -> 'sizeof' '(' (type_spec | expression) ')'
        """
        self.consume(TokenType.SIZEOF)
        self.consume(TokenType.LEFT_PAREN)
        
        # Look ahead to determine if it's a type or expression
        saved_pos = self.position
        try:
            # Try to parse as type spec first
            target = self.type_spec()
            self.consume(TokenType.RIGHT_PAREN)
            return SizeOf(target)
        except ParseError:
            # If type parsing fails, try as expression
            self.position = saved_pos
            self.current_token = self.tokens[self.position]
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN)
            return SizeOf(expr)

    def array_literal(self) -> Expression:
        """
        array_literal -> '[' (expression (',' expression)*)? ']'
        """
        self.consume(TokenType.LEFT_BRACKET)
        elements = []
        
        if not self.expect(TokenType.RIGHT_BRACKET):
            elements.append(self.expression())
            while self.expect(TokenType.COMMA):
                self.advance()
                elements.append(self.expression())
        
        self.consume(TokenType.RIGHT_BRACKET)
        return Literal(elements, DataType.DATA)  # Array literal
    
    def struct_literal(self) -> Expression:
        """
        struct_literal -> '{' (IDENTIFIER '=' expression (',' IDENTIFIER '=' expression)*)? '}'
        """
        self.consume(TokenType.LEFT_BRACE)
        members = {}
        
        if not self.expect(TokenType.RIGHT_BRACE):
            name = self.consume(TokenType.IDENTIFIER).value
            self.consume(TokenType.ASSIGN)
            value = self.statement()
            members[name] = value
            
            while self.expect(TokenType.COMMA):
                self.advance()
                name = self.consume(TokenType.IDENTIFIER).value
                self.consume(TokenType.ASSIGN)
                value = self.expression()
                members[name] = value
        
        self.consume(TokenType.RIGHT_BRACE)
        return Literal(members, DataType.DATA)  # Struct literal

    def struct_body_item(self):
            if self.expect(TokenType.PUBLIC):
                self.advance()
                self.consume(TokenType.LEFT_BRACE)
                while not self.expect(TokenType.RIGHT_BRACE):
                    self.parse_object_body_item(methods, members, nested_objects, nested_structs, is_private=False)
                self.consume(TokenType.RIGHT_BRACE)
                self.consume(TokenType.SEMICOLON)
            elif self.expect(TokenType.PRIVATE):
                self.advance()
                self.consume(TokenType.LEFT_BRACE)
                while not self.expect(TokenType.RIGHT_BRACE):
                    self.parse_object_body_item(methods, members, nested_objects, nested_structs, is_private=True)
                self.consume(TokenType.RIGHT_BRACE)
                self.consume(TokenType.SEMICOLON)

    def destructuring_assignment(self) -> DestructuringAssignment:
        """
        destructuring_assignment -> 'auto' '{' destructure_vars '}' '=' expression ('from' IDENTIFIER)?
        """
        self.consume(TokenType.AUTO)
        self.consume(TokenType.LEFT_BRACE)
        
        # Parse variables in destructuring pattern
        variables = []
        while not self.expect(TokenType.RIGHT_BRACE):
            if self.expect(TokenType.IDENTIFIER):
                name = self.consume(TokenType.IDENTIFIER).value
                if self.expect(TokenType.AS):
                    self.advance()
                    type_spec = self.type_spec()
                    variables.append((name, type_spec))
                else:
                    variables.append(name)
            
            if not self.expect(TokenType.RIGHT_BRACE):
                self.consume(TokenType.COMMA)
        
        self.consume(TokenType.RIGHT_BRACE)
        self.consume(TokenType.ASSIGN)
        source = self.expression()
        
        # Optional 'from' clause
        source_type = None
        if self.expect(TokenType.FROM):
            self.advance()
            source_type = Identifier(self.consume(TokenType.IDENTIFIER).value)
        
        is_explicit = any(isinstance(var, tuple) for var in variables)
        return DestructuringAssignment(variables, source, source_type, is_explicit)

# Add main function for testing
def main():
    """Main function for testing the parser"""
    if len(sys.argv) < 2:
        print("Usage: python3 parser3.py <file.fx> [-v] [-a]")
        sys.exit(1)
    
    filename = sys.argv[1]
    verbose = "-v" in sys.argv
    show_ast = "-a" in sys.argv
    
    try:
        with open(filename, 'r') as f:
            source = f.read()
        
        # Tokenize
        lexer = FluxLexer(source)
        tokens = lexer.tokenize()
        
        if verbose:
            print("Tokens:")
            for token in tokens:
                print(f"  {token}")
            print()
        
        # Parse
        parser = FluxParser(tokens)
        ast = parser.parse()
        
        if show_ast:
            print("AST:")
            print(ast)
        else:
            print("Parse successful!")
            print(f"Generated AST with {len(ast.statements)} top-level statements")
    
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        sys.exit(1)
    except ParseError as e:
        print(f"Parse error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()