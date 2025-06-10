#!/usr/bin/env python3
"""
Flux Language Parser (fparser.py)
Comprehensive recursive descent parser for the Flux programming language.
"""

import sys
from typing import List, Optional, Union, Any
from flexer import FluxLexer, Token, TokenType, LexerError
from fast import *


class ParserError(Exception):
    def __init__(self, message: str, token: Token):
        self.message = message
        self.token = token
        super().__init__(f"Parser error at line {token.line}, column {token.column}: {message}")


class FluxParser:
    """Comprehensive recursive descent parser for the Flux programming language."""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.current = 0
        
    def current_token(self) -> Token:
        """Get the current token."""
        if self.current >= len(self.tokens):
            return self.tokens[-1]  # Return EOF token
        return self.tokens[self.current]
    
    def peek_token(self, offset: int = 1) -> Token:
        """Peek at a token ahead."""
        peek_pos = self.current + offset
        if peek_pos >= len(self.tokens):
            return self.tokens[-1]  # Return EOF token
        return self.tokens[peek_pos]
    
    def advance(self) -> Token:
        """Advance to the next token and return the current one."""
        token = self.current_token()
        if self.current < len(self.tokens) - 1:
            self.current += 1
        return token
    
    def match(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of the given types."""
        return self.current_token().type in token_types
    
    def consume(self, token_type: TokenType, message: str = None) -> Token:
        """Consume a token of the expected type or raise an error."""
        if self.current_token().type == token_type:
            return self.advance()
        
        if message is None:
            message = f"Expected {token_type.value}, got {self.current_token().type.value}"
        raise ParserError(message, self.current_token())
    
    def skip_newlines(self):
        """Skip newline tokens."""
        while self.match(TokenType.NEWLINE):
            self.advance()
    
    # ===============================================================================
    # MAIN PARSING ENTRY POINT
    # ===============================================================================
    
    def parse(self) -> Program:
        """Parse the entire program: <program> ::= <global_item>* <main_function>"""
        self.skip_newlines()
        
        global_items = []
        main_function = None
        
        # Parse global items until we hit main function or EOF
        while not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.EOF):
                break
                
            # Check for main function
            if (self.match(TokenType.DEF) and 
                self.peek_token().type == TokenType.IDENTIFIER and 
                self.peek_token().value == "main"):
                main_function = self.parse_main_function()
                break
            else:
                global_item = self.parse_global_item()
                if global_item:
                    global_items.append(global_item)
        
        if main_function is None:
            raise ParserError("Program must have a main() function", self.current_token())
        
        # Parse any remaining global items after main (shouldn't be any, but handle gracefully)
        while not self.match(TokenType.EOF):
            self.skip_newlines()
            if self.match(TokenType.EOF):
                break
            global_item = self.parse_global_item()
            if global_item:
                global_items.append(global_item)
        
        return Program(global_items, main_function)
    
    def parse_main_function(self) -> MainFunction:
        """Parse main function: def main() -> int { statements }"""
        self.consume(TokenType.DEF)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        if name_token.value != "main":
            raise ParserError("Expected 'main' function name", name_token)
        
        self.consume(TokenType.LPAREN)
        self.consume(TokenType.RPAREN)
        self.consume(TokenType.ARROW)
        
        return_type = self.parse_type_specifier()
        if not isinstance(return_type, BasicType) or return_type.type_name != "int":
            raise ParserError("Main function must return int", self.current_token())
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        return MainFunction(statements)
    
    # ===============================================================================
    # GLOBAL ITEMS
    # ===============================================================================
    
    def parse_global_item(self) -> Optional[GlobalItem]:
        """Parse a global item."""
        self.skip_newlines()
        
        if self.match(TokenType.EOF):
            return None
        
        # Import statement
        if self.match(TokenType.IMPORT):
            return self.parse_import_statement()
        
        # Using statement
        elif self.match(TokenType.USING):
            return self.parse_using_statement()
        
        # Volatile function or template
        elif self.match(TokenType.VOLATILE):
            return self.parse_volatile_definition()
        
        # Function definition
        elif self.match(TokenType.DEF):
            return self.parse_function_definition()
        
        # Template definition
        elif self.match(TokenType.TEMPLATE):
            return self.parse_template_definition()
        
        # Object definition
        elif self.match(TokenType.OBJECT):
            return self.parse_object_definition()
        
        # Struct definition
        elif self.match(TokenType.STRUCT):
            return self.parse_struct_definition()
        
        # Namespace definition
        elif self.match(TokenType.NAMESPACE):
            return self.parse_namespace_definition()
        
        # Variable declaration
        elif self.is_type_specifier():
            return self.parse_variable_declaration()
        
        else:
            raise ParserError(f"Unexpected token in global scope: {self.current_token().type.value}", self.current_token())
    
    def parse_import_statement(self) -> ImportStatement:
        """Parse import statement: import "file.fx" [as alias];"""
        self.consume(TokenType.IMPORT)
        
        module_path_token = self.consume(TokenType.STRING)
        module_path = module_path_token.value
        
        alias = None
        if self.match(TokenType.AS):
            self.advance()  # consume 'as'
            alias_token = self.consume(TokenType.IDENTIFIER)
            alias = alias_token.value
        
        self.consume(TokenType.SEMICOLON)
        return ImportStatement(module_path, alias)
    
    def parse_using_statement(self) -> UsingStatement:
        """Parse using statement: using namespace::path, other::path;"""
        self.consume(TokenType.USING)
        
        namespace_paths = []
        namespace_paths.append(self.parse_namespace_path())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            namespace_paths.append(self.parse_namespace_path())
        
        self.consume(TokenType.SEMICOLON)
        return UsingStatement(namespace_paths)
    
    def parse_namespace_path(self) -> NamespacePath:
        """Parse namespace path: identifier::identifier::..."""
        components = []
        
        components.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.SCOPE):
            self.advance()  # consume ::
            components.append(self.consume(TokenType.IDENTIFIER).value)
        
        return NamespacePath(components)
    
    def parse_volatile_definition(self) -> Union[FunctionDefinition, TemplateDefinition]:
        """Parse volatile function or template definition."""
        self.consume(TokenType.VOLATILE)
        
        if self.match(TokenType.DEF):
            func_def = self.parse_function_definition()
            func_def.is_volatile = True
            return func_def
        elif self.match(TokenType.TEMPLATE):
            template_def = self.parse_template_definition()
            template_def.is_volatile = True
            return template_def
        else:
            raise ParserError("Expected 'def' or 'template' after 'volatile'", self.current_token())
    
    # ===============================================================================
    # FUNCTION DEFINITIONS
    # ===============================================================================
    
    def parse_function_definition(self) -> FunctionDefinition:
        """Parse function definition: def name(params) -> return_type { statements }"""
        self.consume(TokenType.DEF)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        self.consume(TokenType.LPAREN)
        parameters = self.parse_parameter_list() if not self.match(TokenType.RPAREN) else []
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.ARROW)
        return_type = self.parse_type_specifier()
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        return FunctionDefinition(name, parameters, return_type, statements)
    
    def parse_parameter_list(self) -> List[Parameter]:
        """Parse parameter list: param, param, ..."""
        parameters = []
        
        parameters.append(self.parse_parameter())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            parameters.append(self.parse_parameter())
        
        return parameters
    
    def parse_parameter(self) -> Parameter:
        """Parse parameter: type name"""
        type_spec = self.parse_type_specifier()
        name_token = self.consume(TokenType.IDENTIFIER)
        return Parameter(type_spec, name_token.value)
    
    # ===============================================================================
    # TEMPLATE DEFINITIONS
    # ===============================================================================
    
    def parse_template_definition(self) -> TemplateDefinition:
        """Parse template definition: template<T,U> name(params) -> return_type { statements }"""
        self.consume(TokenType.TEMPLATE)
        
        self.consume(TokenType.LESS_THAN)
        template_parameters = self.parse_template_parameter_list()
        self.consume(TokenType.GREATER_THAN)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        self.consume(TokenType.LPAREN)
        parameters = self.parse_parameter_list() if not self.match(TokenType.RPAREN) else []
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.ARROW)
        return_type = self.parse_type_specifier()
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        return TemplateDefinition(name, template_parameters, parameters, return_type, statements)
    
    def parse_template_parameter_list(self) -> List[str]:
        """Parse template parameter list: T, U, V, ..."""
        parameters = []
        
        parameters.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            parameters.append(self.consume(TokenType.IDENTIFIER).value)
        
        return parameters
    
    # ===============================================================================
    # OBJECT DEFINITIONS
    # ===============================================================================
    
    def parse_object_definition(self) -> ObjectDefinition:
        """Parse object definition: object name [: inheritance] { members }"""
        self.consume(TokenType.OBJECT)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        inheritance = []
        if self.match(TokenType.COLON):
            self.advance()  # consume :
            inheritance = self.parse_inheritance_list()
        
        self.consume(TokenType.LBRACE)
        members = self.parse_object_member_list()
        self.consume(TokenType.RBRACE)
        
        return ObjectDefinition(name, inheritance, members)
    
    def parse_inheritance_list(self) -> List[str]:
        """Parse inheritance list: parent1, parent2, ..."""
        inheritance = []
        
        inheritance.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            inheritance.append(self.consume(TokenType.IDENTIFIER).value)
        
        return inheritance
    
    def parse_object_member_list(self) -> List[ObjectMember]:
        """Parse object member list."""
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.RBRACE):
                break
            
            member = self.parse_object_member()
            if member:
                members.append(member)
        
        return members
    
    def parse_object_member(self) -> Optional[ObjectMember]:
        """Parse an object member."""
        self.skip_newlines()
        
        if self.match(TokenType.RBRACE) or self.match(TokenType.EOF):
            return None
        
        # Magic method definition
        if self.match(TokenType.DEF) and self.peek_token().value.startswith('__'):
            return self.parse_magic_method_definition()
        
        # Regular function definition
        elif self.match(TokenType.DEF):
            return self.parse_function_definition()
        
        # Nested object definition
        elif self.match(TokenType.OBJECT):
            return self.parse_object_definition()
        
        # Variable declaration
        elif self.is_type_specifier():
            return self.parse_variable_declaration()
        
        else:
            raise ParserError(f"Unexpected token in object member: {self.current_token().type.value}", self.current_token())
    
    def parse_magic_method_definition(self) -> MagicMethodDefinition:
        """Parse magic method definition: def __method(params) -> return_type { statements }"""
        self.consume(TokenType.DEF)
        
        method_name_token = self.consume(TokenType.IDENTIFIER)
        method_name = method_name_token.value
        
        if not method_name.startswith('__'):
            raise ParserError(f"Expected magic method name starting with '__', got {method_name}", method_name_token)
        
        self.consume(TokenType.LPAREN)
        parameters = self.parse_parameter_list() if not self.match(TokenType.RPAREN) else []
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.ARROW)
        return_type = self.parse_type_specifier()
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        return MagicMethodDefinition(method_name, parameters, return_type, statements)
    
    # ===============================================================================
    # STRUCT DEFINITIONS
    # ===============================================================================
    
    def parse_struct_definition(self) -> StructDefinition:
        """Parse struct definition: struct name { members }"""
        self.consume(TokenType.STRUCT)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        self.consume(TokenType.LBRACE)
        members = self.parse_struct_member_list()
        self.consume(TokenType.RBRACE)
        
        return StructDefinition(name, members)
    
    def parse_struct_member_list(self) -> List[VariableDeclaration]:
        """Parse struct member list (only variable declarations)."""
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.RBRACE):
                break
            
            if self.is_type_specifier():
                member = self.parse_variable_declaration()
                members.append(member)
            else:
                raise ParserError("Structs can only contain variable declarations", self.current_token())
        
        return members
    
    # ===============================================================================
    # NAMESPACE DEFINITIONS
    # ===============================================================================
    
    def parse_namespace_definition(self) -> NamespaceDefinition:
        """Parse namespace definition: namespace name { members }"""
        self.consume(TokenType.NAMESPACE)
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        self.consume(TokenType.LBRACE)
        members = self.parse_namespace_member_list()
        self.consume(TokenType.RBRACE)
        
        return NamespaceDefinition(name, members)
    
    def parse_namespace_member_list(self) -> List[NamespaceMember]:
        """Parse namespace member list."""
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.RBRACE):
                break
            
            member = self.parse_namespace_member()
            if member:
                members.append(member)
        
        return members
    
    def parse_namespace_member(self) -> Optional[NamespaceMember]:
        """Parse a namespace member."""
        self.skip_newlines()
        
        if self.match(TokenType.RBRACE) or self.match(TokenType.EOF):
            return None
        
        # Function definition
        if self.match(TokenType.DEF):
            return self.parse_function_definition()
        
        # Template definition
        elif self.match(TokenType.TEMPLATE):
            return self.parse_template_definition()
        
        # Object definition
        elif self.match(TokenType.OBJECT):
            return self.parse_object_definition()
        
        # Struct definition
        elif self.match(TokenType.STRUCT):
            return self.parse_struct_definition()
        
        # Variable declaration
        elif self.is_type_specifier():
            return self.parse_variable_declaration()
        
        else:
            raise ParserError(f"Unexpected token in namespace member: {self.current_token().type.value}", self.current_token())
    
    # ===============================================================================
    # TYPE SPECIFIERS
    # ===============================================================================
    
    def is_type_specifier(self) -> bool:
        """Check if the current token starts a type specifier."""
        return self.match(
            TokenType.INT, TokenType.FLOAT_KW, TokenType.BOOL, TokenType.VOID,
            TokenType.AUTO, TokenType.THIS, TokenType.SIGNED, TokenType.UNSIGNED,
            TokenType.DATA, TokenType.IDENTIFIER, TokenType.TEMPLATE
        )
    
    def parse_type_specifier(self) -> TypeSpecifier:
        """Parse a type specifier."""
        # Basic types
        if self.match(TokenType.INT):
            self.advance()
            return BasicType("int")
        elif self.match(TokenType.FLOAT_KW):
            self.advance()
            return BasicType("float")
        elif self.match(TokenType.BOOL):
            self.advance()
            return BasicType("bool")
        elif self.match(TokenType.VOID):
            self.advance()
            return BasicType("void")
        elif self.match(TokenType.AUTO):
            self.advance()
            return BasicType("auto")
        elif self.match(TokenType.THIS):
            self.advance()
            return BasicType("this")
        
        # Data type: [signed|unsigned] data{width}
        elif self.match(TokenType.SIGNED, TokenType.UNSIGNED, TokenType.DATA):
            return self.parse_data_type()
        
        # Template pointer: template* name
        elif self.match(TokenType.TEMPLATE):
            self.advance()
            self.consume(TokenType.MULTIPLY)
            name_token = self.consume(TokenType.IDENTIFIER)
            return TemplatePointerType(name_token.value)
        
        # Identifier type (user-defined) or function pointer
        elif self.match(TokenType.IDENTIFIER):
            return self.parse_identifier_or_function_pointer_type()
        
        else:
            raise ParserError("Expected type specifier", self.current_token())
    
    def parse_data_type(self) -> DataType:
        """Parse data type: [signed|unsigned] data{width}"""
        signed = None
        
        if self.match(TokenType.SIGNED):
            self.advance()
            signed = True
        elif self.match(TokenType.UNSIGNED):
            self.advance()
            signed = False
        
        self.consume(TokenType.DATA)
        self.consume(TokenType.LBRACE)
        width_token = self.consume(TokenType.INTEGER)
        width = int(width_token.value)
        self.consume(TokenType.RBRACE)
        
        return DataType(width, signed)
    
    def parse_identifier_or_function_pointer_type(self) -> TypeSpecifier:
        """Parse identifier type or function pointer type."""
        # Look ahead to see if this is a function pointer
        # Function pointer: return_type (*name)(param_types)
        
        base_type = None
        
        # First, parse the base type (which could be the return type of a function pointer)
        name_token = self.consume(TokenType.IDENTIFIER)
        base_type = IdentifierType(name_token.value)
        
        # Check for array type modifier
        while self.match(TokenType.LBRACKET):
            self.advance()  # consume [
            self.consume(TokenType.RBRACKET)
            base_type = ArrayType(base_type)
        
        # Check for pointer type modifier or function pointer
        if self.match(TokenType.MULTIPLY):
            self.advance()  # consume *
            
            # Check if this is a function pointer: (*name)
            if self.match(TokenType.IDENTIFIER):
                # This is a function pointer: return_type (*name)(param_types)
                name_token = self.consume(TokenType.IDENTIFIER)
                name = name_token.value
                
                self.consume(TokenType.RPAREN)  # consume ) for (*name)
                self.consume(TokenType.LPAREN)  # consume ( for (param_types)
                
                parameter_types = []
                if not self.match(TokenType.RPAREN):
                    parameter_types = self.parse_type_list()
                
                self.consume(TokenType.RPAREN)
                
                return FunctionPointerType(base_type, parameter_types, name)
            else:
                # Regular pointer type
                return PointerType(base_type)
        
        return base_type
    
    def parse_type_list(self) -> List[TypeSpecifier]:
        """Parse type list: type, type, ..."""
        types = []
        
        types.append(self.parse_type_specifier())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            types.append(self.parse_type_specifier())
        
        return types
    
    # ===============================================================================
    # VARIABLE DECLARATIONS
    # ===============================================================================
    
    def parse_variable_declaration(self) -> VariableDeclaration:
        """Parse variable declaration: type declarator1, declarator2, ...;"""
        type_spec = self.parse_type_specifier()
        
        declarators = []
        declarators.append(self.parse_variable_declarator())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            declarators.append(self.parse_variable_declarator())
        
        self.consume(TokenType.SEMICOLON)
        return VariableDeclaration(type_spec, declarators)
    
    def parse_variable_declarator(self) -> VariableDeclarator:
        """Parse variable declarator: name [initializer]"""
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        initializer = None
        if self.match(TokenType.ASSIGN, TokenType.LPAREN, TokenType.LBRACE):
            initializer = self.parse_initializer()
        
        return VariableDeclarator(name, initializer)
    
    def parse_initializer(self) -> Initializer:
        """Parse initializer: = expr | (args) | {fields}"""
        if self.match(TokenType.ASSIGN):
            self.advance()  # consume =
            expression = self.parse_expression()
            return ExpressionInitializer(expression)
        
        elif self.match(TokenType.LPAREN):
            self.advance()  # consume (
            arguments = []
            if not self.match(TokenType.RPAREN):
                arguments = self.parse_expression_list()
            self.consume(TokenType.RPAREN)
            return ConstructorInitializer(arguments)
        
        elif self.match(TokenType.LBRACE):
            self.advance()  # consume {
            field_initializers = []
            if not self.match(TokenType.RBRACE):
                field_initializers = self.parse_field_initializer_list()
            self.consume(TokenType.RBRACE)
            return StructInitializer(field_initializers)
        
        else:
            raise ParserError("Expected initializer", self.current_token())
    
    def parse_field_initializer_list(self) -> List[FieldInitializer]:
        """Parse field initializer list: field = expr, field = expr, ..."""
        initializers = []
        
        initializers.append(self.parse_field_initializer())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            initializers.append(self.parse_field_initializer())
        
        return initializers
    
    def parse_field_initializer(self) -> FieldInitializer:
        """Parse field initializer: field = expression"""
        field_name_token = self.consume(TokenType.IDENTIFIER)
        field_name = field_name_token.value
        
        self.consume(TokenType.ASSIGN)
        expression = self.parse_expression()
        
        return FieldInitializer(field_name, expression)
    
    # ===============================================================================
    # STATEMENTS
    # ===============================================================================
    
    def parse_statement_list(self) -> List[Statement]:
        """Parse statement list."""
        statements = []
        
        while not self.match(TokenType.RBRACE, TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.RBRACE, TokenType.EOF):
                break
            
            statement = self.parse_statement()
            if statement:
                statements.append(statement)
        
        return statements
    
    def parse_statement(self) -> Optional[Statement]:
        """Parse a statement."""
        self.skip_newlines()
        
        if self.match(TokenType.RBRACE, TokenType.EOF):
            return None
        
        # Compound statement
        if self.match(TokenType.LBRACE):
            return self.parse_compound_statement()
        
        # If statement
        elif self.match(TokenType.IF):
            return self.parse_if_statement()
        
        # Switch statement
        elif self.match(TokenType.SWITCH):
            return self.parse_switch_statement()
        
        # While statement
        elif self.match(TokenType.WHILE):
            return self.parse_while_statement()
        
        # For statement
        elif self.match(TokenType.FOR):
            return self.parse_for_statement()
        
        # Try statement
        elif self.match(TokenType.TRY):
            return self.parse_try_statement()
        
        # Return statement
        elif self.match(TokenType.RETURN):
            return self.parse_return_statement()
        
        # Throw statement
        elif self.match(TokenType.THROW):
            return self.parse_throw_statement()
        
        # Break statement
        elif self.match(TokenType.BREAK):
            return self.parse_break_statement()
        
        # Continue statement
        elif self.match(TokenType.CONTINUE):
            return self.parse_continue_statement()
        
        # Assembly block
        elif self.match(TokenType.ASM):
            return self.parse_assembly_block()
        
        # Variable declaration
        elif self.is_type_specifier():
            return self.parse_variable_declaration()
        
        # Expression statement
        else:
            return self.parse_expression_statement()
    
    def parse_compound_statement(self) -> CompoundStatement:
        """Parse compound statement: { statements }"""
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        return CompoundStatement(statements)
    
    def parse_expression_statement(self) -> ExpressionStatement:
        """Parse expression statement: [expression];"""
        expression = None
        
        if not self.match(TokenType.SEMICOLON):
            expression = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON)
        return ExpressionStatement(expression)
    
    def parse_if_statement(self) -> IfStatement:
        """Parse if statement: if (condition) then_stmt [else else_stmt];"""
        self.consume(TokenType.IF)
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        then_statement = self.parse_statement()
        
        else_statement = None
        if self.match(TokenType.ELSE):
            self.advance()  # consume else
            else_statement = self.parse_statement()
        
        self.consume(TokenType.SEMICOLON)
        return IfStatement(condition, then_statement, else_statement)
    
    def parse_switch_statement(self) -> SwitchStatement:
        """Parse switch statement: switch (expression) { cases default };"""
        self.consume(TokenType.SWITCH)
        self.consume(TokenType.LPAREN)
        expression = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.LBRACE)
        
        cases = []
        default_clause = None
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.CASE):
                cases.append(self.parse_case_clause())
            elif self.match(TokenType.DEFAULT):
                if default_clause is not None:
                    raise ParserError("Multiple default clauses in switch", self.current_token())
                default_clause = self.parse_default_clause()
            else:
                break
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        return SwitchStatement(expression, cases, default_clause)
    
    def parse_case_clause(self) -> CaseClause:
        """Parse case clause: case (expression) { statements }"""
        self.consume(TokenType.CASE)
        self.consume(TokenType.LPAREN)
        expression = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return CaseClause(expression, statements)
    
    def parse_default_clause(self) -> DefaultClause:
        """Parse default clause: default { statements }"""
        self.consume(TokenType.DEFAULT)
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return DefaultClause(statements)
    
    def parse_while_statement(self) -> WhileStatement:
        """Parse while statement: while (condition) body;"""
        self.consume(TokenType.WHILE)
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON)
        
        return WhileStatement(condition, body)
    
    def parse_for_statement(self) -> ForStatement:
        """Parse for statement: for (variable in iterable) body;"""
        self.consume(TokenType.FOR)
        self.consume(TokenType.LPAREN)
        
        variable_token = self.consume(TokenType.IDENTIFIER)
        variable = variable_token.value
        
        self.consume(TokenType.IN)
        iterable = self.parse_expression()
        
        self.consume(TokenType.RPAREN)
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON)
        
        return ForStatement(variable, iterable, body)
    
    def parse_try_statement(self) -> TryStatement:
        """Parse try statement: try { statements } catch_clauses"""
        self.consume(TokenType.TRY)
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        catch_clauses = []
        while self.match(TokenType.CATCH):
            catch_clauses.append(self.parse_catch_clause())
        
        if not catch_clauses:
            raise ParserError("Try statement must have at least one catch clause", self.current_token())
        
        return TryStatement(statements, catch_clauses)
    
    def parse_catch_clause(self) -> CatchClause:
        """Parse catch clause: catch (parameter) { statements }"""
        self.consume(TokenType.CATCH)
        self.consume(TokenType.LPAREN)
        
        parameter = self.parse_catch_parameter()
        
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.LBRACE)
        statements = self.parse_statement_list()
        self.consume(TokenType.RBRACE)
        
        return CatchClause(parameter, statements)
    
    def parse_catch_parameter(self) -> CatchParameter:
        """Parse catch parameter: type name or auto name"""
        if self.match(TokenType.AUTO):
            self.advance()
            type_spec = BasicType("auto")
        else:
            type_spec = self.parse_type_specifier()
        
        name_token = self.consume(TokenType.IDENTIFIER)
        name = name_token.value
        
        return CatchParameter(type_spec, name)
    
    def parse_return_statement(self) -> ReturnStatement:
        """Parse return statement: return [expression];"""
        self.consume(TokenType.RETURN)
        
        expression = None
        if not self.match(TokenType.SEMICOLON):
            expression = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON)
        return ReturnStatement(expression)
    
    def parse_throw_statement(self) -> ThrowStatement:
        """Parse throw statement: throw(expression);"""
        self.consume(TokenType.THROW)
        self.consume(TokenType.LPAREN)
        expression = self.parse_expression()
        self.consume(TokenType.RPAREN)
        self.consume(TokenType.SEMICOLON)
        
        return ThrowStatement(expression)
    
    def parse_break_statement(self) -> BreakStatement:
        """Parse break statement: break;"""
        self.consume(TokenType.BREAK)
        self.consume(TokenType.SEMICOLON)
        return BreakStatement()
    
    def parse_continue_statement(self) -> ContinueStatement:
        """Parse continue statement: continue;"""
        self.consume(TokenType.CONTINUE)
        self.consume(TokenType.SEMICOLON)
        return ContinueStatement()
    
    def parse_assembly_block(self) -> AssemblyBlock:
        """Parse assembly block: asm { instructions }"""
        self.consume(TokenType.ASM)
        self.consume(TokenType.LBRACE)
        
        instructions = []
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            self.skip_newlines()
            
            if self.match(TokenType.RBRACE):
                break
            
            instruction = self.parse_assembly_instruction()
            instructions.append(instruction)
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return AssemblyBlock(instructions)
    
    def parse_assembly_instruction(self) -> AssemblyInstruction:
        """Parse assembly instruction: mnemonic [operands]"""
        mnemonic_token = self.consume(TokenType.IDENTIFIER)
        mnemonic = mnemonic_token.value
        
        operands = []
        if not self.match(TokenType.NEWLINE, TokenType.RBRACE):
            operands.append(self.parse_assembly_operand())
            
            while self.match(TokenType.COMMA):
                self.advance()  # consume comma
                operands.append(self.parse_assembly_operand())
        
        # Skip newline after instruction
        if self.match(TokenType.NEWLINE):
            self.advance()
        
        return AssemblyInstruction(mnemonic, operands)
    
    def parse_assembly_operand(self) -> AssemblyOperand:
        """Parse assembly operand: identifier, integer, or register"""
        if self.match(TokenType.IDENTIFIER):
            value = self.advance().value
        elif self.match(TokenType.INTEGER):
            value = self.advance().value
        else:
            raise ParserError("Expected assembly operand", self.current_token())
        
        return AssemblyOperand(value)
    
    # ===============================================================================
    # EXPRESSIONS
    # ===============================================================================
    
    def parse_expression(self) -> Expression:
        """Parse expression (top-level entry point)."""
        return self.parse_assignment_expression()
    
    def parse_assignment_expression(self) -> Expression:
        """Parse assignment expression: logical_or_expr [assignment_op expr]"""
        expr = self.parse_conditional_expression()
        
        if self.is_assignment_operator():
            operator = self.advance().value
            right = self.parse_assignment_expression()
            return AssignmentExpression(expr, operator, right)
        
        return expr
    
    def is_assignment_operator(self) -> bool:
        """Check if current token is an assignment operator."""
        return self.match(
            TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN,
            TokenType.MUL_ASSIGN, TokenType.DIV_ASSIGN, TokenType.MOD_ASSIGN,
            TokenType.POW_ASSIGN, TokenType.AND_ASSIGN, TokenType.OR_ASSIGN,
            TokenType.XOR_ASSIGN, TokenType.LSHIFT_ASSIGN, TokenType.RSHIFT_ASSIGN,
            TokenType.BAND_ASSIGN, TokenType.BNAND_ASSIGN, TokenType.BOR_ASSIGN,
            TokenType.BNOR_ASSIGN, TokenType.BXOR_ASSIGN, TokenType.BNEQ_ASSIGN
        )
    
    def parse_conditional_expression(self) -> Expression:
        """Parse conditional expression: logical_or_expr [? expr : expr]"""
        expr = self.parse_logical_or_expression()
        
        if self.match(TokenType.CONDITIONAL):
            self.advance()  # consume ?
            true_expr = self.parse_expression()
            self.consume(TokenType.COLON)
            false_expr = self.parse_conditional_expression()
            return ConditionalExpression(expr, true_expr, false_expr)
        
        return expr
    
    def parse_logical_or_expression(self) -> Expression:
        """Parse logical OR expression: logical_and_expr (|| logical_and_expr)*"""
        expr = self.parse_logical_and_expression()
        
        while self.match(TokenType.LOGICAL_OR, TokenType.OR, TokenType.LOGICAL_NOR):
            operator = self.advance().value
            right = self.parse_logical_and_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_logical_and_expression(self) -> Expression:
        """Parse logical AND expression: bitwise_or_expr (&& bitwise_or_expr)*"""
        expr = self.parse_bitwise_or_expression()
        
        while self.match(TokenType.LOGICAL_AND, TokenType.AND, TokenType.LOGICAL_NAND):
            operator = self.advance().value
            right = self.parse_bitwise_or_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_bitwise_or_expression(self) -> Expression:
        """Parse bitwise OR expression: bitwise_xor_expr (| bitwise_xor_expr)*"""
        expr = self.parse_bitwise_xor_expression()
        
        while self.match(TokenType.BITWISE_OR, TokenType.BBIT_OR, TokenType.BBIT_NOR):
            operator = self.advance().value
            right = self.parse_bitwise_xor_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_bitwise_xor_expression(self) -> Expression:
        """Parse bitwise XOR expression: bitwise_and_expr (^^ bitwise_and_expr)*"""
        expr = self.parse_bitwise_and_expression()
        
        while self.match(TokenType.BITWISE_XOR, TokenType.XOR, TokenType.BBIT_XOR, 
                          TokenType.BBIT_XNOR, TokenType.BBIT_XNAND):
            operator = self.advance().value
            right = self.parse_bitwise_and_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_bitwise_and_expression(self) -> Expression:
        """Parse bitwise AND expression: identity_expr (& identity_expr)*"""
        expr = self.parse_identity_expression()
        
        while self.match(TokenType.BITWISE_AND, TokenType.BBIT_AND, TokenType.BBIT_NAND,
                          TokenType.BBIT_XAND, TokenType.BBIT_XNAND):
            operator = self.advance().value
            right = self.parse_identity_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_identity_expression(self) -> Expression:
        """Parse identity expression: equality_expr (is|not|as equality_expr)*"""
        expr = self.parse_equality_expression()
        
        while self.match(TokenType.IS, TokenType.NOT, TokenType.AS):
            operator = self.advance().value
            right = self.parse_equality_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_equality_expression(self) -> Expression:
        """Parse equality expression: relational_expr (==|!= relational_expr)*"""
        expr = self.parse_relational_expression()
        
        while self.match(TokenType.EQUAL, TokenType.NOT_EQUAL):
            operator = self.advance().value
            right = self.parse_relational_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_relational_expression(self) -> Expression:
        """Parse relational expression: shift_expr (<|<=|>|>=|in shift_expr)*"""
        expr = self.parse_shift_expression()
        
        while self.match(TokenType.LESS_THAN, TokenType.LESS_EQUAL, 
                          TokenType.GREATER_THAN, TokenType.GREATER_EQUAL, TokenType.IN):
            operator = self.advance().value
            right = self.parse_shift_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_shift_expression(self) -> Expression:
        """Parse shift expression: additive_expr (<<|>> additive_expr)*"""
        expr = self.parse_additive_expression()
        
        while self.match(TokenType.LSHIFT, TokenType.RSHIFT):
            operator = self.advance().value
            right = self.parse_additive_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_additive_expression(self) -> Expression:
        """Parse additive expression: multiplicative_expr (+|- multiplicative_expr)*"""
        expr = self.parse_multiplicative_expression()
        
        while self.match(TokenType.PLUS, TokenType.MINUS):
            operator = self.advance().value
            right = self.parse_multiplicative_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_multiplicative_expression(self) -> Expression:
        """Parse multiplicative expression: exponential_expr (*|/|% exponential_expr)*"""
        expr = self.parse_exponential_expression()
        
        while self.match(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            operator = self.advance().value
            right = self.parse_exponential_expression()
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_exponential_expression(self) -> Expression:
        """Parse exponential expression: cast_expr [^ exponential_expr]"""
        expr = self.parse_cast_expression()
        
        if self.match(TokenType.POWER):
            operator = self.advance().value
            right = self.parse_exponential_expression()  # Right associative
            expr = BinaryExpression(expr, operator, right)
        
        return expr
    
    def parse_cast_expression(self) -> Expression:
        """Parse cast expression: (type) unary_expr | unary_expr"""
        if (self.match(TokenType.LPAREN) and 
            self.is_type_specifier_ahead()):
            self.advance()  # consume (
            target_type = self.parse_type_specifier()
            self.consume(TokenType.RPAREN)
            expression = self.parse_unary_expression()
            return CastExpression(target_type, expression)
        
        return self.parse_unary_expression()
    
    def is_type_specifier_ahead(self) -> bool:
        """Check if there's a type specifier after the current position."""
        # Save current position
        saved_pos = self.current
        
        try:
            # Try to parse a type specifier
            self.parse_type_specifier()
            # If successful, check if there's a closing paren
            result = self.match(TokenType.RPAREN)
        except:
            result = False
        finally:
            # Restore position
            self.current = saved_pos
        
        return result
    
    def parse_unary_expression(self) -> Expression:
        """Parse unary expression: unary_op unary_expr | postfix_expr"""
        if self.match(TokenType.INCREMENT, TokenType.DECREMENT, TokenType.PLUS, TokenType.MINUS,
                      TokenType.LOGICAL_NOT, TokenType.BITWISE_NOT, TokenType.ADDRESS_OF,
                      TokenType.MULTIPLY, TokenType.NOT, TokenType.SIZEOF, TokenType.TYPEOF):
            operator = self.advance().value
            operand = self.parse_unary_expression()
            return UnaryExpression(operator, operand)
        
        return self.parse_postfix_expression()
    
    def parse_postfix_expression(self) -> Expression:
        """Parse postfix expression: primary_expr postfix_op*"""
        expr = self.parse_primary_expression()
        
        while True:
            if self.match(TokenType.LBRACKET):
                # Array access: expr[index]
                self.advance()  # consume [
                index = self.parse_expression()
                self.consume(TokenType.RBRACKET)
                expr = ArrayAccessExpression(expr, index)
            
            elif self.match(TokenType.LPAREN):
                # Function call: expr(args)
                self.advance()  # consume (
                arguments = []
                if not self.match(TokenType.RPAREN):
                    arguments = self.parse_expression_list()
                self.consume(TokenType.RPAREN)
                expr = FunctionCallExpression(expr, arguments)
            
            elif self.match(TokenType.DOT):
                # Member access: expr.member
                self.advance()  # consume .
                member_token = self.consume(TokenType.IDENTIFIER)
                expr = MemberAccessExpression(expr, member_token.value)
            
            elif self.match(TokenType.SCOPE):
                # Scope access: expr::member
                self.advance()  # consume ::
                member_token = self.consume(TokenType.IDENTIFIER)
                expr = ScopeAccessExpression(expr, member_token.value)
            
            elif self.match(TokenType.INCREMENT):
                # Post-increment: expr++
                self.advance()  # consume ++
                expr = PostIncrementExpression(expr)
            
            elif self.match(TokenType.DECREMENT):
                # Post-decrement: expr--
                self.advance()  # consume --
                expr = PostDecrementExpression(expr)
            
            elif self.match(TokenType.LESS_THAN):
                # Template instantiation: expr<types>
                # Need to check if this is really a template instantiation
                if self.is_template_instantiation():
                    self.advance()  # consume <
                    template_arguments = self.parse_template_argument_list()
                    self.consume(TokenType.GREATER_THAN)
                    expr = TemplateInstantiationExpression(expr, template_arguments)
                else:
                    break
            
            else:
                break
        
        return expr
    
    def is_template_instantiation(self) -> bool:
        """Check if < starts a template instantiation."""
        # This is a simplified check - could be improved with more lookahead
        saved_pos = self.current
        
        try:
            self.advance()  # consume <
            # Try to parse template arguments
            while not self.match(TokenType.GREATER_THAN, TokenType.EOF):
                if self.is_type_specifier():
                    self.parse_type_specifier()
                    if self.match(TokenType.COMMA):
                        self.advance()
                    elif self.match(TokenType.GREATER_THAN):
                        break
                    else:
                        return False
                else:
                    return False
            result = self.match(TokenType.GREATER_THAN)
        except:
            result = False
        finally:
            self.current = saved_pos
        
        return result
    
    def parse_template_argument_list(self) -> List[TypeSpecifier]:
        """Parse template argument list: type, type, ..."""
        arguments = []
        
        arguments.append(self.parse_type_specifier())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            arguments.append(self.parse_type_specifier())
        
        return arguments
    
    def parse_expression_list(self) -> List[Expression]:
        """Parse expression list: expr, expr, ..."""
        expressions = []
        
        expressions.append(self.parse_expression())
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            expressions.append(self.parse_expression())
        
        return expressions
    
    def parse_primary_expression(self) -> Expression:
        """Parse primary expression."""
        # Identifier
        if self.match(TokenType.IDENTIFIER):
            name = self.advance().value
            return IdentifierExpression(name)
        
        # Integer literal
        elif self.match(TokenType.INTEGER):
            value = int(self.advance().value)
            return IntegerLiteralExpression(value)
        
        # Float literal
        elif self.match(TokenType.FLOAT):
            value = float(self.advance().value)
            return FloatLiteralExpression(value)
        
        # Character literal
        elif self.match(TokenType.CHAR):
            value = self.advance().value
            return CharLiteralExpression(value)
        
        # String literal
        elif self.match(TokenType.STRING):
            value = self.advance().value
            return StringLiteralExpression(value)
        
        # Binary literal
        elif self.match(TokenType.BINARY):
            value = self.advance().value
            return BinaryLiteralExpression(value)
        
        # Boolean literal
        elif self.match(TokenType.BOOLEAN):
            value = self.advance().value == "true"
            return BooleanLiteralExpression(value)
        
        # I-string
        elif self.match(TokenType.I_STRING):
            return self.parse_i_string_expression()
        
        # Array literal
        elif self.match(TokenType.LBRACKET):
            return self.parse_array_literal_or_comprehension()
        
        # Destructuring assignment
        elif self.match(TokenType.AUTO) and self.peek_token().type == TokenType.LBRACE:
            return self.parse_destructuring_assignment()
        
        # Parenthesized expression
        elif self.match(TokenType.LPAREN):
            self.advance()  # consume (
            expr = self.parse_expression()
            self.consume(TokenType.RPAREN)
            return ParenthesizedExpression(expr)
        
        # This expression
        elif self.match(TokenType.THIS):
            self.advance()
            return ThisExpression()
        
        # Super expression
        elif self.match(TokenType.SUPER):
            self.advance()
            member = None
            if self.match(TokenType.DOT):
                self.advance()  # consume .
                member_token = self.consume(TokenType.IDENTIFIER)
                member = member_token.value
            return SuperExpression(member)
        
        else:
            raise ParserError(f"Unexpected token in expression: {self.current_token().type.value}", self.current_token())
    
    def parse_i_string_expression(self) -> IStringExpression:
        """Parse i-string expression: i"template":{expressions}"""
        i_string_token = self.advance()
        i_string_value = i_string_token.value
        
        # Parse the i-string format: i"template":{expr;expr;...}
        # Extract template and expressions from the token value
        if not i_string_value.startswith('i"'):
            raise ParserError("Invalid i-string format", i_string_token)
        
        # Find the template part
        template_start = 2  # after i"
        template_end = i_string_value.find('":', template_start)
        if template_end == -1:
            raise ParserError("Invalid i-string format - missing ':' after template", i_string_token)
        
        template = i_string_value[template_start:template_end]
        
        # Find the expressions part
        expr_start = i_string_value.find('{', template_end)
        expr_end = i_string_value.rfind('}')
        if expr_start == -1 or expr_end == -1:
            raise ParserError("Invalid i-string format - missing expression block", i_string_token)
        
        expressions_text = i_string_value[expr_start+1:expr_end]
        
        # Parse expressions from the text (this is simplified - in a real implementation,
        # you'd need to properly tokenize and parse the expression text)
        expressions = []
        if expressions_text.strip():
            # For now, treat each semicolon-separated part as a simple identifier
            # In a full implementation, you'd tokenize and parse this properly
            expr_parts = expressions_text.split(';')
            for part in expr_parts:
                part = part.strip()
                if part:
                    # This is simplified - should properly parse each expression
                    expressions.append(IdentifierExpression(part))
        
        return IStringExpression(template, expressions)
    
    def parse_array_literal_or_comprehension(self) -> Expression:
        """Parse array literal or array comprehension."""
        self.consume(TokenType.LBRACKET)
        
        if self.match(TokenType.RBRACKET):
            # Empty array literal
            self.advance()  # consume ]
            return ArrayLiteralExpression([])
        
        # Parse first expression
        expr = self.parse_expression()
        
        # Check if this is an array comprehension
        if self.match(TokenType.FOR):
            # Array comprehension: [expr for (var in iterable) [if (condition)]]
            self.advance()  # consume for
            self.consume(TokenType.LPAREN)
            
            variable_token = self.consume(TokenType.IDENTIFIER)
            variable = variable_token.value
            
            self.consume(TokenType.IN)
            iterable = self.parse_expression()
            
            self.consume(TokenType.RPAREN)
            
            condition = None
            if self.match(TokenType.IF):
                self.advance()  # consume if
                self.consume(TokenType.LPAREN)
                condition = self.parse_expression()
                self.consume(TokenType.RPAREN)
            
            self.consume(TokenType.RBRACKET)
            return ArrayComprehensionExpression(expr, variable, iterable, condition)
        
        else:
            # Array literal: [expr, expr, ...]
            elements = [expr]
            
            while self.match(TokenType.COMMA):
                self.advance()  # consume comma
                if self.match(TokenType.RBRACKET):
                    break  # Trailing comma
                elements.append(self.parse_expression())
            
            self.consume(TokenType.RBRACKET)
            return ArrayLiteralExpression(elements)
    
    def parse_destructuring_assignment(self) -> DestructuringAssignmentExpression:
        """Parse destructuring assignment: auto {vars} = object{fields}"""
        self.consume(TokenType.AUTO)
        
        self.consume(TokenType.LBRACE)
        variables = []
        variables.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            variables.append(self.consume(TokenType.IDENTIFIER).value)
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.ASSIGN)
        
        object_name_token = self.consume(TokenType.IDENTIFIER)
        object_name = object_name_token.value
        
        self.consume(TokenType.LBRACE)
        fields = []
        fields.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.COMMA):
            self.advance()  # consume comma
            fields.append(self.consume(TokenType.IDENTIFIER).value)
        
        self.consume(TokenType.RBRACE)
        
        return DestructuringAssignmentExpression(variables, object_name, fields)


# ===============================================================================
# UTILITY FUNCTIONS
# ===============================================================================

def parse_file(filepath: str) -> Program:
    """Parse a Flux source file and return the AST."""
    try:
        with open(filepath, 'r', encoding='utf-8') as file:
            source_code = file.read()
    except FileNotFoundError:
        raise FileNotFoundError(f"Source file '{filepath}' not found")
    except IOError as e:
        raise IOError(f"Error reading file '{filepath}': {e}")
    
    # Tokenize
    lexer = FluxLexer(source_code)
    tokens = lexer.tokenize()
    
    # Parse
    parser = FluxParser(tokens)
    return parser.parse()


def parse_string(source_code: str) -> Program:
    """Parse Flux source code from a string and return the AST."""
    # Tokenize
    lexer = FluxLexer(source_code)
    tokens = lexer.tokenize()
    
    # Parse
    parser = FluxParser(tokens)
    return parser.parse()


def main():
    """Main function to parse a Flux source file."""
    import sys
    import os
    
    if len(sys.argv) != 2:
        print("Usage: python fparser.py <flux_source_file>")
        print("Example: python fparser.py example.fx")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' does not exist")
        sys.exit(1)
    
    if not filepath.endswith('.fx'):
        print(f"Warning: File '{filepath}' does not have .fx extension")
    
    try:
        print(f"Parsing file: {filepath}")
        print("=" * 60)
        
        ast = parse_file(filepath)
        
        print("Successfully parsed AST")
        print("=" * 60)
        print("AST Structure:")
        print("-" * 60)
        
        # Print basic AST info
        print(f"Program with {len(ast.global_items)} global items")
        print(f"Main function with {len(ast.main_function.statements)} statements")
        
        for i, item in enumerate(ast.global_items):
            print(f"  Global item {i+1}: {type(item).__name__}")
        
        print("=" * 60)
        print("Parsing completed successfully")
    
    except (FileNotFoundError, IOError) as e:
        print(f"File Error: {e}")
        sys.exit(1)
    except (LexerError, ParserError) as e:
        print(f"Parse Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()