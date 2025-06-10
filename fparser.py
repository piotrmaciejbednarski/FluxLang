#!/usr/bin/env python3
"""
Flux Programming Language Parser
fparser.py - Parses token streams into AST

This parser is fully context-aware and handles:
- Template angle bracket disambiguation
- Operator precedence (19 levels)
- Context tracking for scopes and templates
- All Flux language constructs
"""

import sys
from typing import List, Optional, Union, Dict, Set, Tuple, Any
from enum import Enum
from dataclasses import dataclass

# Import lexer and AST components
from flexer import FluxLexer, Token, TokenType, LexerError, SourceLocation
from fast import *


class ParseError(Exception):
    """Exception raised when parser encounters an error"""
    def __init__(self, message: str, location: SourceLocation, token: Optional[Token] = None):
        self.message = message
        self.location = location
        self.token = token
        super().__init__(f"{message} at {location.line}:{location.column}")


class ContextKind(Enum):
    """Types of parsing contexts"""
    GLOBAL = "global"
    NAMESPACE = "namespace"
    OBJECT = "object"
    STRUCT = "struct"
    FUNCTION = "function"
    BLOCK = "block"
    TEMPLATE_DECLARATION = "template_declaration"
    TEMPLATE_INSTANTIATION = "template_instantiation"
    EXPRESSION = "expression"


@dataclass
class ParseContext:
    """Tracks parsing context and scope"""
    kind: ContextKind
    depth: int
    brace_count: int = 0
    paren_count: int = 0
    bracket_count: int = 0
    angle_count: int = 0
    template_depth: int = 0
    is_template_context: bool = False


class FluxParser:
    """
    Flux Programming Language Parser
    
    Parses token streams into AST while maintaining full context awareness
    for proper template parsing and scope tracking.
    """
    
    # Operator precedence table (19 levels, highest to lowest)
    PRECEDENCE = {
        # Level 1: Primary expressions (handled separately)
        
        # Level 2: Postfix inc/dec
        '++': 18, '--': 18,
        
        # Level 3: Unary operators (handled in unary parsing)
        
        # Level 4: Type casting (handled separately)
        
        # Level 5: Exponentiation
        '^': 17,
        
        # Level 6: Multiplicative
        '*': 16, '/': 16, '%': 16,
        
        # Level 7: Additive
        '+': 15, '-': 15,
        
        # Level 8: Bit shift
        '<<': 14, '>>': 14,
        
        # Level 9: Relational
        '<': 13, '<=': 13, '>': 13, '>=': 13, 'in': 13,
        
        # Level 10: Equality
        '==': 12, '!=': 12,
        
        # Level 11: Identity
        'is': 11, 'not': 11, 'as': 11,
        
        # Level 12: Bitwise AND variants
        '&': 10, '`&': 10, '`!&': 10, '`^&': 10, '`^!&': 10,
        
        # Level 13: Bitwise XOR variants
        '^^': 9, 'xor': 9, '`^^': 9, '`^!|': 9,
        
        # Level 14: Bitwise OR variants
        '|': 8, '`|': 8, '`!|': 8,
        
        # Level 15: Logical AND/NAND
        'and': 7, '&&': 7, '!&': 7,
        
        # Level 16: Logical OR/NOR
        'or': 6, '||': 6, '!|': 6,
        
        # Level 17: Conditional (handled separately)
        
        # Level 18: Assignment operators
        '=': 2, '+=': 2, '-=': 2, '*=': 2, '/=': 2, '%=': 2, '^=': 2,
        '&=': 2, '|=': 2, '^^=': 2, '<<=': 2, '>>=': 2,
        '`&=': 2, '`|=': 2, '`^|=': 2, '`!&=': 2, '`!|=': 2,
        '`^&=': 2, '`^!|=': 2, '`^!&=': 2,
        
        # Level 19: Comma
        ',': 1,
    }
    
    # Right-associative operators
    RIGHT_ASSOCIATIVE = {'^', '=', '+=', '-=', '*=', '/=', '%=', '^=', '&=', '|=', '^^=', 
                        '<<=', '>>=', '`&=', '`|=', '`^|=', '`!&=', '`!|=', '`^&=', 
                        '`^!|=', '`^!&='}
    
    def __init__(self, tokens: List[Token], filename: str = "<input>"):
        self.tokens = tokens
        self.filename = filename
        self.pos = 0
        self.context_stack: List[ParseContext] = [ParseContext(ContextKind.GLOBAL, 0)]
        self.errors: List[ParseError] = []
        self.lexer: Optional[FluxLexer] = None  # Set by caller for '>>' splitting
        
        # Track template context for angle bracket disambiguation
        self.template_depth = 0
        self.in_template_args = False
        
        # Skip any initial comments
        self.skip_comments()
    
    def skip_comments(self) -> None:
        """Skip comment tokens"""
        while (self.pos < len(self.tokens) and 
               self.tokens[self.pos].type == TokenType.COMMENT):
            self.pos += 1
    
    def current_token(self) -> Token:
        """Get current token (skipping comments)"""
        self.skip_comments()
        if self.pos >= len(self.tokens):
            return self.tokens[-1]  # EOF token
        return self.tokens[self.pos]
    
    def peek_token(self, offset: int = 1) -> Token:
        """Peek at token at current position + offset (skipping comments)"""
        temp_pos = self.pos
        
        # Skip comments at current position
        while (temp_pos < len(self.tokens) and 
               self.tokens[temp_pos].type == TokenType.COMMENT):
            temp_pos += 1
        
        # Advance by offset, skipping comments each time
        for _ in range(offset):
            temp_pos += 1
            while (temp_pos < len(self.tokens) and 
                   self.tokens[temp_pos].type == TokenType.COMMENT):
                temp_pos += 1
        
        return self.tokens[temp_pos] if temp_pos < len(self.tokens) else self.tokens[-1]
    
    def advance(self) -> Token:
        """Advance to next token and return current (skipping comments)"""
        token = self.current_token()  # Get current non-comment token
        self.pos += 1  # Move past it
        return token
    
    def match(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of the given types"""
        return self.current_token().type in token_types
    
    def match_keyword(self, keyword: str) -> bool:
        """Check if current token is a specific keyword"""
        return (self.current_token().type == TokenType.KEYWORD and 
                self.current_token().value == keyword)
    
    def consume(self, token_type: TokenType, error_msg: str = None) -> Token:
        """Consume token of expected type or raise error"""
        if not self.match(token_type):
            if error_msg is None:
                error_msg = f"Expected {token_type.name}, got {self.current_token().type.name}"
            raise ParseError(error_msg, self.current_token().location, self.current_token())
        return self.advance()
    
    def consume_keyword(self, keyword: str) -> Token:
        """Consume specific keyword or raise error"""
        if not self.match_keyword(keyword):
            raise ParseError(f"Expected keyword '{keyword}', got '{self.current_token().value}'", 
                           self.current_token().location, self.current_token())
        return self.advance()
    
    def push_context(self, kind: ContextKind) -> None:
        """Push new parsing context"""
        current = self.context_stack[-1]
        new_context = ParseContext(
            kind=kind,
            depth=current.depth + 1,
            brace_count=current.brace_count,
            paren_count=current.paren_count,
            bracket_count=current.bracket_count,
            angle_count=current.angle_count,
            template_depth=current.template_depth,
            is_template_context=kind in (ContextKind.TEMPLATE_DECLARATION, ContextKind.TEMPLATE_INSTANTIATION)
        )
        self.context_stack.append(new_context)
    
    def pop_context(self) -> ParseContext:
        """Pop current parsing context"""
        if len(self.context_stack) <= 1:
            raise ParseError("Cannot pop global context", self.current_token().location)
        return self.context_stack.pop()
    
    def current_context(self) -> ParseContext:
        """Get current parsing context"""
        return self.context_stack[-1]
    
    def is_in_template_context(self) -> bool:
        """Check if we're in a template context"""
        return any(ctx.is_template_context for ctx in self.context_stack)
    
    def handle_angle_brackets(self) -> None:
        """Handle angle bracket tracking for template disambiguation"""
        current = self.current_context()
        
        if self.match(TokenType.LESS_THAN):
            if self.is_in_template_context():
                current.angle_count += 1
                self.template_depth += 1
        elif self.match(TokenType.GREATER_THAN):
            if self.is_in_template_context() and current.angle_count > 0:
                current.angle_count -= 1
                self.template_depth -= 1
        elif self.match(TokenType.RIGHT_SHIFT):
            # Check if we need to split '>>' into '>' '>'
            if self.is_in_template_context() and current.angle_count > 1:
                if self.lexer:
                    # Split the '>>' token into two '>' tokens
                    if self.lexer.split_right_shift(self.pos):
                        # Token was split, handle as single '>'
                        current.angle_count -= 1
                        self.template_depth -= 1
    
    def sync_to_next_statement(self) -> None:
        """Synchronize parser to next statement for error recovery"""
        while not self.match(TokenType.EOF):
            self.skip_comments()  # Skip comments during error recovery
            
            if self.match(TokenType.SEMICOLON):
                self.advance()
                return
            
            if self.match_keyword('def') or self.match_keyword('object') or \
               self.match_keyword('struct') or self.match_keyword('namespace'):
                return
            
            self.advance()
    
    def parse_module(self) -> ModuleNode:
        """Parse complete module (top-level entry point)"""
        location = self.current_token().location
        module = ModuleNode(location, self.filename)
        
        try:
            while not self.match(TokenType.EOF):
                if self.match_keyword('import'):
                    module.imports.append(self.parse_import())
                elif self.match_keyword('using'):
                    module.using_declarations.append(self.parse_using())
                else:
                    decl = self.parse_declaration()
                    if decl:
                        module.declarations.append(decl)
        except ParseError as e:
            self.errors.append(e)
            self.sync_to_next_statement()
        
        return module
    
    def parse_import(self) -> ImportNode:
        """Parse import declaration"""
        location = self.current_token().location
        self.consume_keyword('import')
        
        module_path_token = self.consume(TokenType.STRING)
        module_path = module_path_token.value.strip('"')
        
        alias = None
        if self.match_keyword('as'):
            self.advance()
            alias = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.SEMICOLON)
        return ImportNode(location, module_path, alias)
    
    def parse_using(self) -> UsingNode:
        """Parse using declaration"""
        location = self.current_token().location
        self.consume_keyword('using')
        
        # Parse namespace path
        namespace_path = []
        namespace_path.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.SCOPE):
            self.advance()
            namespace_path.append(self.consume(TokenType.IDENTIFIER).value)
        
        # Parse members (if any)
        members = []
        if self.match(TokenType.COMMA):
            self.advance()
            members.append(self.consume(TokenType.IDENTIFIER).value)
            
            while self.match(TokenType.COMMA):
                self.advance()
                members.append(self.consume(TokenType.IDENTIFIER).value)
        
        self.consume(TokenType.SEMICOLON)
        return UsingNode(location, namespace_path, members)
    
    def parse_declaration(self) -> Optional[DeclarationNode]:
        """Parse top-level declaration"""
        if self.match_keyword('namespace'):
            return self.parse_namespace()
        elif self.match_keyword('object'):
            return self.parse_object()
        elif self.match_keyword('struct'):
            return self.parse_struct()
        elif self.match_keyword('template'):
            return self.parse_template_declaration()
        elif self.match_keyword('def'):
            return self.parse_function()
        else:
            return self.parse_variable_declaration()
    
    def parse_namespace(self) -> NamespaceNode:
        """Parse namespace declaration"""
        location = self.current_token().location
        self.consume_keyword('namespace')
        
        name = self.consume(TokenType.IDENTIFIER).value
        namespace = NamespaceNode(location, name)
        
        self.consume(TokenType.LBRACE)
        self.push_context(ContextKind.NAMESPACE)
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            decl = self.parse_declaration()
            if decl:
                namespace.declarations.append(decl)
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        self.pop_context()
        
        return namespace
    
    def parse_object(self) -> ObjectDeclarationNode:
        """Parse object (class) declaration"""
        location = self.current_token().location
        self.consume_keyword('object')
        
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Parse inheritance
        base_classes = []
        if self.match(TokenType.COLON):
            self.advance()
            base_classes.append(self.parse_named_type())
            
            while self.match(TokenType.COMMA):
                self.advance()
                base_classes.append(self.parse_named_type())
        
        obj = ObjectDeclarationNode(location, name, base_classes)
        
        self.consume(TokenType.LBRACE)
        self.push_context(ContextKind.OBJECT)
        
        # Parse object members
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            if self.match_keyword('def'):
                obj.members.append(self.parse_function())
            else:
                obj.members.append(self.parse_variable_declaration())
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        self.pop_context()
        
        return obj
    
    def parse_struct(self) -> StructDeclarationNode:
        """Parse struct declaration"""
        location = self.current_token().location
        self.consume_keyword('struct')
        
        name = self.consume(TokenType.IDENTIFIER).value
        struct = StructDeclarationNode(location, name)
        
        self.consume(TokenType.LBRACE)
        self.push_context(ContextKind.STRUCT)
        
        # Parse struct members (only variable declarations)
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            member = self.parse_variable_declaration()
            if isinstance(member, VariableDeclarationNode):
                struct.members.append(member)
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        self.pop_context()
        
        return struct
    
    def parse_template_declaration(self) -> TemplateDeclarationNode:
        """Parse template declaration"""
        location = self.current_token().location
        self.consume_keyword('template')
        
        self.consume(TokenType.LESS_THAN)
        self.push_context(ContextKind.TEMPLATE_DECLARATION)
        
        # Parse template parameters
        template_params = []
        template_params.append(self.consume(TokenType.IDENTIFIER).value)
        
        while self.match(TokenType.COMMA):
            self.advance()
            template_params.append(self.consume(TokenType.IDENTIFIER).value)
        
        self.consume(TokenType.GREATER_THAN)
        
        # Parse the declaration being templated
        declaration = self.parse_declaration()
        
        self.pop_context()
        
        return TemplateDeclarationNode(location, template_params, declaration)
    
    def parse_function(self) -> FunctionDeclarationNode:
        """Parse function declaration"""
        location = self.current_token().location
        self.consume_keyword('def')
        
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Check for template parameters
        template_params = []
        is_template = False
        if self.match(TokenType.LESS_THAN):
            is_template = True
            self.advance()
            self.push_context(ContextKind.TEMPLATE_DECLARATION)
            
            template_params.append(self.consume(TokenType.IDENTIFIER).value)
            while self.match(TokenType.COMMA):
                self.advance()
                template_params.append(self.consume(TokenType.IDENTIFIER).value)
            
            self.consume(TokenType.GREATER_THAN)
            self.pop_context()
        
        # Parse parameters
        self.consume(TokenType.LPAREN)
        parameters = []
        
        if not self.match(TokenType.RPAREN):
            parameters.append(self.parse_parameter())
            
            while self.match(TokenType.COMMA):
                self.advance()
                parameters.append(self.parse_parameter())
        
        self.consume(TokenType.RPAREN)
        
        # Parse return type
        self.consume(TokenType.MINUS)
        self.consume(TokenType.GREATER_THAN)
        return_type = self.parse_type()
        
        # Parse function body (optional for declarations)
        body = None
        if self.match(TokenType.LBRACE):
            body = self.parse_block_statement()
        
        if not body:
            self.consume(TokenType.SEMICOLON)
        
        return FunctionDeclarationNode(location, name, parameters, return_type, body, 
                                     is_template, template_params)
    
    def parse_parameter(self) -> ParameterNode:
        """Parse function parameter"""
        location = self.current_token().location
        param_type = self.parse_type()
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Parse default value
        default_value = None
        if self.match(TokenType.ASSIGN):
            self.advance()
            default_value = self.parse_expression()
        
        return ParameterNode(location, param_type, name, default_value)
    
    def parse_variable_declaration(self) -> VariableDeclarationNode:
        """Parse variable declaration"""
        location = self.current_token().location
        
        # Check for const/volatile
        is_const = False
        is_volatile = False
        
        if self.match_keyword('const'):
            is_const = True
            self.advance()
        
        if self.match_keyword('volatile'):
            is_volatile = True
            self.advance()
        
        var_type = self.parse_type()
        name = self.consume(TokenType.IDENTIFIER).value
        
        # Parse initializer
        initializer = None
        if self.match(TokenType.ASSIGN):
            self.advance()
            initializer = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON)
        
        return VariableDeclarationNode(location, var_type, name, initializer, is_const, is_volatile)
    
    def parse_type(self) -> TypeNode:
        """Parse type expression"""
        if self.match_keyword('int') or self.match_keyword('float') or \
           self.match_keyword('char') or self.match_keyword('bool') or \
           self.match_keyword('void'):
            return self.parse_primitive_type()
        elif self.match_keyword('signed') or self.match_keyword('unsigned') or \
             self.match_keyword('data'):
            return self.parse_data_type()
        else:
            return self.parse_named_type()
    
    def parse_primitive_type(self) -> PrimitiveTypeNode:
        """Parse primitive type"""
        location = self.current_token().location
        type_name = self.advance().value
        return PrimitiveTypeNode(location, type_name)
    
    def parse_data_type(self) -> DataTypeNode:
        """Parse data type: [signed|unsigned] data{width:alignment}"""
        location = self.current_token().location
        
        is_signed = True
        if self.match_keyword('unsigned'):
            is_signed = False
            self.advance()
        elif self.match_keyword('signed'):
            self.advance()
        
        self.consume_keyword('data')
        self.consume(TokenType.LBRACE)
        
        width_expr = self.parse_expression()
        
        alignment_expr = None
        if self.match(TokenType.COLON):
            self.advance()
            alignment_expr = self.parse_expression()
        
        self.consume(TokenType.RBRACE)
        
        return DataTypeNode(location, is_signed, width_expr, alignment_expr)
    
    def parse_named_type(self) -> NamedTypeNode:
        """Parse named type reference"""
        location = self.current_token().location
        
        # Parse namespace path
        namespace_path = []
        name = self.consume(TokenType.IDENTIFIER).value
        
        while self.match(TokenType.SCOPE):
            self.advance()
            namespace_path.append(name)
            name = self.consume(TokenType.IDENTIFIER).value
        
        return NamedTypeNode(location, name, namespace_path)
    
    def parse_statement(self) -> StatementNode:
        """Parse statement"""
        if self.match_keyword('if'):
            return self.parse_if_statement()
        elif self.match_keyword('while'):
            return self.parse_while_statement()
        elif self.match_keyword('do'):
            return self.parse_do_while_statement()
        elif self.match_keyword('for'):
            return self.parse_for_statement()
        elif self.match_keyword('return'):
            return self.parse_return_statement()
        elif self.match_keyword('break'):
            return self.parse_break_statement()
        elif self.match_keyword('continue'):
            return self.parse_continue_statement()
        elif self.match_keyword('throw'):
            return self.parse_throw_statement()
        elif self.match_keyword('try'):
            return self.parse_try_statement()
        elif self.match_keyword('switch'):
            return self.parse_switch_statement()
        elif self.match_keyword('asm'):
            return self.parse_assembly_statement()
        elif self.match(TokenType.LBRACE):
            return self.parse_block_statement()
        else:
            # Variable declaration or expression statement
            if self.is_type_start():
                return self.parse_variable_declaration()
            else:
                return self.parse_expression_statement()
    
    def parse_block_statement(self) -> BlockStatementNode:
        """Parse block statement"""
        location = self.current_token().location
        self.consume(TokenType.LBRACE)
        
        block = BlockStatementNode(location)
        self.push_context(ContextKind.BLOCK)
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                block.add_statement(stmt)
        
        self.consume(TokenType.RBRACE)
        if not self.match(TokenType.SEMICOLON):
            # Block statements don't always need semicolons in certain contexts
            if self.current_context().kind in (ContextKind.FUNCTION, ContextKind.BLOCK):
                pass  # Optional semicolon
            else:
                self.consume(TokenType.SEMICOLON)
        else:
            self.advance()  # Consume optional semicolon
        
        self.pop_context()
        return block
    
    def parse_if_statement(self) -> IfStatementNode:
        """Parse if statement"""
        location = self.current_token().location
        self.consume_keyword('if')
        
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        then_stmt = self.parse_statement()
        
        else_stmt = None
        if self.match_keyword('else'):
            self.advance()
            if self.match_keyword('if'):
                else_stmt = self.parse_if_statement()
            else:
                else_stmt = self.parse_statement()
        
        if not self.match(TokenType.SEMICOLON):
            self.consume(TokenType.SEMICOLON)
        else:
            self.advance()
            
        return IfStatementNode(location, condition, then_stmt, else_stmt)
    
    def parse_while_statement(self) -> WhileStatementNode:
        """Parse while statement"""
        location = self.current_token().location
        self.consume_keyword('while')
        
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        body = self.parse_statement()
        self.consume(TokenType.SEMICOLON)
        
        return WhileStatementNode(location, condition, body)
    
    def parse_do_while_statement(self) -> DoWhileStatementNode:
        """Parse do-while statement"""
        location = self.current_token().location
        self.consume_keyword('do')
        
        body = self.parse_statement()
        
        self.consume_keyword('while')
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        self.consume(TokenType.SEMICOLON)
        
        return DoWhileStatementNode(location, body, condition)
    
    def parse_for_statement(self) -> Union[ForStatementNode, RangeForStatementNode]:
        """Parse for statement (C-style or range-based)"""
        location = self.current_token().location
        self.consume_keyword('for')
        
        self.consume(TokenType.LPAREN)
        
        # Check for range-based for loop
        if self.match(TokenType.IDENTIFIER) and self.peek_token().type == TokenType.KEYWORD and \
           self.peek_token().value == 'in':
            # Range-based for loop
            variable = self.advance().value
            self.consume_keyword('in')
            iterable = self.parse_expression()
            self.consume(TokenType.RPAREN)
            
            body = self.parse_statement()
            self.consume(TokenType.SEMICOLON)
            
            return RangeForStatementNode(location, variable, iterable, body)
        else:
            # C-style for loop
            init = None
            if not self.match(TokenType.SEMICOLON):
                if self.is_type_start():
                    init = self.parse_variable_declaration()
                else:
                    init = self.parse_expression_statement()
            else:
                self.advance()  # consume semicolon
            
            condition = None
            if not self.match(TokenType.SEMICOLON):
                condition = self.parse_expression()
            self.consume(TokenType.SEMICOLON)
            
            update = None
            if not self.match(TokenType.RPAREN):
                update = self.parse_expression()
            self.consume(TokenType.RPAREN)
            
            body = self.parse_statement()
            self.consume(TokenType.SEMICOLON)
            
            return ForStatementNode(location, init, condition, update, body)
    
    def parse_return_statement(self) -> ReturnStatementNode:
        """Parse return statement"""
        location = self.current_token().location
        self.consume_keyword('return')
        
        value = None
        if not self.match(TokenType.SEMICOLON):
            value = self.parse_expression()
        
        self.consume(TokenType.SEMICOLON)
        return ReturnStatementNode(location, value)
    
    def parse_break_statement(self) -> BreakStatementNode:
        """Parse break statement"""
        location = self.current_token().location
        self.consume_keyword('break')
        self.consume(TokenType.SEMICOLON)
        return BreakStatementNode(location)
    
    def parse_continue_statement(self) -> ContinueStatementNode:
        """Parse continue statement"""
        location = self.current_token().location
        self.consume_keyword('continue')
        self.consume(TokenType.SEMICOLON)
        return ContinueStatementNode(location)
    
    def parse_throw_statement(self) -> ThrowStatementNode:
        """Parse throw statement"""
        location = self.current_token().location
        self.consume_keyword('throw')
        
        expression = self.parse_expression()
        self.consume(TokenType.SEMICOLON)
        
        return ThrowStatementNode(location, expression)
    
    def parse_try_statement(self) -> TryStatementNode:
        """Parse try-catch statement"""
        location = self.current_token().location
        self.consume_keyword('try')
        
        try_block = self.parse_block_statement()
        
        catch_clauses = []
        while self.match_keyword('catch'):
            catch_clauses.append(self.parse_catch_clause())
        
        if not catch_clauses:
            raise ParseError("Try statement must have at least one catch clause", location)
        
        return TryStatementNode(location, try_block, catch_clauses)
    
    def parse_catch_clause(self) -> CatchClauseNode:
        """Parse catch clause"""
        location = self.current_token().location
        self.consume_keyword('catch')
        
        self.consume(TokenType.LPAREN)
        exception_type = self.parse_type()
        exception_name = self.consume(TokenType.IDENTIFIER).value
        self.consume(TokenType.RPAREN)
        
        body = self.parse_block_statement()
        
        return CatchClauseNode(location, exception_type, exception_name, body)
    
    def parse_switch_statement(self) -> SwitchStatementNode:
        """Parse switch statement"""
        location = self.current_token().location
        self.consume_keyword('switch')
        
        self.consume(TokenType.LPAREN)
        expression = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.LBRACE)
        
        cases = []
        default_case = None
        
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            if self.match_keyword('case'):
                cases.append(self.parse_case_clause())
            elif self.match_keyword('default'):
                if default_case:
                    raise ParseError("Multiple default clauses in switch", self.current_token().location)
                default_case = self.parse_default_clause()
            else:
                raise ParseError("Expected 'case' or 'default' in switch", self.current_token().location)
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return SwitchStatementNode(location, expression, cases, default_case)
    
    def parse_case_clause(self) -> CaseClauseNode:
        """Parse case clause"""
        location = self.current_token().location
        self.consume_keyword('case')
        
        self.consume(TokenType.LPAREN)
        value = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        body = self.parse_block_statement()
        
        return CaseClauseNode(location, value, body)
    
    def parse_default_clause(self) -> DefaultClauseNode:
        """Parse default clause"""
        location = self.current_token().location
        self.consume_keyword('default')
        
        body = self.parse_block_statement()
        
        return DefaultClauseNode(location, body)
    
    def parse_assembly_statement(self) -> AssemblyStatementNode:
        """Parse inline assembly statement"""
        location = self.current_token().location
        self.consume_keyword('asm')
        
        self.consume(TokenType.LBRACE)
        
        # Collect assembly code until closing brace
        assembly_code = ""
        brace_count = 1
        
        while brace_count > 0 and not self.match(TokenType.EOF):
            # Manually advance position to handle braces, but skip comments
            self.pos += 1
            while (self.pos < len(self.tokens) and 
                   self.tokens[self.pos].type == TokenType.COMMENT):
                self.pos += 1
                
            if self.pos >= len(self.tokens):
                break
                
            token = self.tokens[self.pos]
            
            if token.type == TokenType.LBRACE:
                brace_count += 1
            elif token.type == TokenType.RBRACE:
                brace_count -= 1
            
            if brace_count > 0:
                assembly_code += token.value + " "
        
        self.consume(TokenType.SEMICOLON)
        
        return AssemblyStatementNode(location, assembly_code.strip())
    
    def parse_expression_statement(self) -> ExpressionStatementNode:
        """Parse expression statement"""
        location = self.current_token().location
        expression = self.parse_expression()
        self.consume(TokenType.SEMICOLON)
        return ExpressionStatementNode(location, expression)
    
    def parse_expression(self, min_precedence: int = 0) -> ExpressionNode:
        """Parse expression using precedence climbing"""
        left = self.parse_unary_expression()
        
        while True:
            token = self.current_token()
            
            # Handle ternary conditional operator
            if token.type == TokenType.QUESTION:
                self.advance()
                true_expr = self.parse_expression()
                self.consume(TokenType.COLON)
                false_expr = self.parse_expression()
                left = ConditionalExpressionNode(token.location, left, true_expr, false_expr)
                continue
            
            # Get operator precedence
            if token.type not in (TokenType.KEYWORD, TokenType.IDENTIFIER) and hasattr(token, 'value'):
                op = token.value
            else:
                op = token.value if token.type == TokenType.KEYWORD else None
            
            if op not in self.PRECEDENCE:
                break
            
            precedence = self.PRECEDENCE[op]
            if precedence < min_precedence:
                break
            
            # Handle angle brackets in template context
            if op in ('<', '>'):
                self.handle_angle_brackets()
            
            self.advance()
            
            # Handle right-associative operators
            next_min_precedence = precedence + 1
            if op in self.RIGHT_ASSOCIATIVE:
                next_min_precedence = precedence
            
            right = self.parse_expression(next_min_precedence)
            
            # Create appropriate node based on operator type
            if op in ('=', '+=', '-=', '*=', '/=', '%=', '^=', '&=', '|=', '^^=', 
                     '<<=', '>>=', '`&=', '`|=', '`^|=', '`!&=', '`!|=', 
                     '`^&=', '`^!|=', '`^!&='):
                left = AssignmentNode(token.location, op, left, right)
            else:
                left = BinaryOperatorNode(token.location, op, left, right)
        
        return left
    
    def parse_unary_expression(self) -> ExpressionNode:
        """Parse unary expression"""
        token = self.current_token()
        
        # Prefix unary operators
        if token.type in (TokenType.PLUS, TokenType.MINUS, TokenType.LOGICAL_NOT, 
                         TokenType.BITWISE_NOT, TokenType.ADDRESS_OF, TokenType.MULTIPLY,
                         TokenType.INCREMENT, TokenType.DECREMENT) or \
           token.value in ('not', 'sizeof', 'typeof', 'alignof'):
            
            op = token.value
            self.advance()
            
            if op in ('sizeof', 'typeof', 'alignof'):
                self.consume(TokenType.LPAREN)
                if self.is_type_start():
                    operand = self.parse_type()
                else:
                    operand = self.parse_expression()
                self.consume(TokenType.RPAREN)
                
                if op == 'sizeof':
                    return SizeofExpressionNode(token.location, operand)
                elif op == 'typeof':
                    return TypeofExpressionNode(token.location, operand)
                else:  # alignof
                    return AlignofExpressionNode(token.location, operand)
            else:
                operand = self.parse_unary_expression()
                return UnaryOperatorNode(token.location, op, operand)
        
        return self.parse_postfix_expression()
    
    def parse_postfix_expression(self) -> ExpressionNode:
        """Parse postfix expression"""
        expr = self.parse_primary_expression()
        
        while True:
            token = self.current_token()
            
            if token.type == TokenType.LBRACKET:
                # Array access
                self.advance()
                index = self.parse_expression()
                self.consume(TokenType.RBRACKET)
                expr = ArrayAccessNode(token.location, expr, index)
            elif token.type == TokenType.DOT:
                # Member access
                self.advance()
                member = self.consume(TokenType.IDENTIFIER).value
                expr = MemberAccessNode(token.location, expr, member)
            elif token.type == TokenType.LPAREN:
                # Function call
                args, template_args = self.parse_call_arguments()
                expr = CallExpressionNode(token.location, expr, args, template_args)
            elif token.type in (TokenType.INCREMENT, TokenType.DECREMENT):
                # Postfix increment/decrement
                op = token.value
                self.advance()
                expr = UnaryOperatorNode(token.location, op, expr, is_postfix=True)
            else:
                break
        
        return expr
    
    def parse_primary_expression(self) -> ExpressionNode:
        """Parse primary expression"""
        token = self.current_token()
        
        if token.type == TokenType.INTEGER:
            self.advance()
            return IntegerLiteralNode(token.location, int(token.value))
        elif token.type == TokenType.FLOAT:
            self.advance()
            return FloatLiteralNode(token.location, float(token.value))
        elif token.type == TokenType.CHARACTER:
            self.advance()
            return CharLiteralNode(token.location, token.value)
        elif token.type == TokenType.STRING:
            self.advance()
            return StringLiteralNode(token.location, token.value)
        elif token.type == TokenType.BOOLEAN:
            self.advance()
            return BooleanLiteralNode(token.location, token.value == 'true')
        elif token.type == TokenType.INTERPOLATED_STRING:
            return self.parse_interpolated_string()
        elif token.type == TokenType.IDENTIFIER:
            return self.parse_identifier()
        elif token.type == TokenType.LPAREN:
            # Parenthesized expression or cast
            self.advance()
            if self.is_type_start():
                # Type cast
                target_type = self.parse_type()
                self.consume(TokenType.RPAREN)
                expression = self.parse_unary_expression()
                return CastExpressionNode(token.location, target_type, expression)
            else:
                # Parenthesized expression
                expr = self.parse_expression()
                self.consume(TokenType.RPAREN)
                return expr
        elif token.type == TokenType.LBRACKET:
            # Array comprehension or array literal
            return self.parse_array_comprehension()
        else:
            raise ParseError(f"Unexpected token in expression: {token.value}", token.location, token)
    
    def parse_interpolated_string(self) -> InterpolatedStringNode:
        """Parse interpolated string"""
        token = self.advance()
        # Extract template and expressions from interpolated string
        # This is a simplified implementation
        template = token.value
        expressions = []  # TODO: Parse embedded expressions
        return InterpolatedStringNode(token.location, template, expressions)
    
    def parse_identifier(self) -> IdentifierNode:
        """Parse identifier with possible namespace path"""
        location = self.current_token().location
        
        namespace_path = []
        name = self.advance().value
        
        while self.match(TokenType.SCOPE):
            self.advance()
            namespace_path.append(name)
            name = self.consume(TokenType.IDENTIFIER).value
        
        return IdentifierNode(location, name, namespace_path)
    
    def parse_call_arguments(self) -> Tuple[List[ExpressionNode], List[TypeNode]]:
        """Parse function call arguments"""
        self.consume(TokenType.LPAREN)
        
        args = []
        template_args = []
        
        # Check for template arguments first
        if self.match(TokenType.LESS_THAN):
            self.advance()
            self.push_context(ContextKind.TEMPLATE_INSTANTIATION)
            
            template_args.append(self.parse_type())
            while self.match(TokenType.COMMA):
                self.advance()
                template_args.append(self.parse_type())
            
            self.consume(TokenType.GREATER_THAN)
            self.pop_context()
        
        # Parse regular arguments
        if not self.match(TokenType.RPAREN):
            args.append(self.parse_expression())
            
            while self.match(TokenType.COMMA):
                self.advance()
                args.append(self.parse_expression())
        
        self.consume(TokenType.RPAREN)
        return args, template_args
    
    def parse_array_comprehension(self) -> Union[ArrayComprehensionNode, ExpressionNode]:
        """Parse array comprehension or array literal"""
        location = self.current_token().location
        self.consume(TokenType.LBRACKET)
        
        # For now, implement as simple array literal
        # TODO: Implement full array comprehension parsing
        expr = self.parse_expression()
        self.consume(TokenType.RBRACKET)
        
        # Return as a simple expression for now
        return expr
    
    def is_type_start(self) -> bool:
        """Check if current token can start a type"""
        if self.match_keyword('int') or self.match_keyword('float') or \
           self.match_keyword('char') or self.match_keyword('bool') or \
           self.match_keyword('void') or self.match_keyword('signed') or \
           self.match_keyword('unsigned') or self.match_keyword('data') or \
           self.match_keyword('const') or self.match_keyword('volatile'):
            return True
        
        if self.match(TokenType.IDENTIFIER):
            return True
        
        return False
    
    def parse(self) -> ModuleNode:
        """Parse the token stream and return AST"""
        try:
            return self.parse_module()
        except ParseError as e:
            self.errors.append(e)
            # Return partial AST with error nodes
            location = self.current_token().location
            module = ModuleNode(location, self.filename)
            error_node = ErrorNode(e.location, e.message)
            module.declarations.append(error_node)
            return module


def main():
    """Run the parser on a Flux source file"""
    if len(sys.argv) != 2:
        print("Usage: python fparser.py <source_file.fx>")
        print("Example: python fparser.py example.fx")
        sys.exit(1)
    
    filename = sys.argv[1]
    
    try:
        # Read the source file
        with open(filename, 'r', encoding='utf-8') as file:
            source_code = file.read()
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        sys.exit(1)
    except IOError as e:
        print(f"Error reading file '{filename}': {e}")
        sys.exit(1)
    
    # Tokenize the source
    lexer = FluxLexer(source_code, filename)
    try:
        tokens = lexer.tokenize()
    except LexerError as e:
        print(f"Lexer error in {filename}: {e}")
        sys.exit(1)
    
    # Parse the tokens
    parser = FluxParser(tokens, filename)
    parser.lexer = lexer  # For '>>' token splitting
    
    try:
        ast = parser.parse()
        
        if parser.errors:
            print(f"=== Parse Errors in {filename} ===")
            for error in parser.errors:
                print(f"  {error}")
            print()
        
        print(f"=== Parsing {filename} ===")
        print(f"Successfully parsed into AST with {len(ast.declarations)} top-level declarations")
        
        # Print AST structure summary
        print(f"\nAST Summary:")
        print(f"  Imports: {len(ast.imports)}")
        print(f"  Using declarations: {len(ast.using_declarations)}")
        print(f"  Declarations: {len(ast.declarations)}")
        
        # Count declaration types
        decl_counts = {}
        for decl in ast.declarations:
            decl_type = type(decl).__name__
            decl_counts[decl_type] = decl_counts.get(decl_type, 0) + 1
        
        if decl_counts:
            print(f"\nDeclaration type counts:")
            for decl_type, count in sorted(decl_counts.items()):
                print(f"  {decl_type}: {count}")
        
        if parser.errors:
            print(f"\nParsing completed with {len(parser.errors)} errors.")
            sys.exit(1)
        else:
            print(f"\nParsing completed successfully.")
            
    except Exception as e:
        print(f"Unexpected error during parsing: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()