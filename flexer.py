#!/usr/bin/env python3
"""
Flux Language Lexer (flexer.py)
Comprehensive lexical analyzer for the Flux programming language.
"""

import re
import enum
from dataclasses import dataclass
from typing import List, Optional, Iterator, NamedTuple


class TokenType(enum.Enum):
    # Literals
    INTEGER = "INTEGER"
    FLOAT = "FLOAT" 
    CHAR = "CHAR"
    STRING = "STRING"
    BINARY = "BINARY"
    BOOLEAN = "BOOLEAN"
    I_STRING = "I_STRING"
    
    # Identifiers
    IDENTIFIER = "IDENTIFIER"
    
    # Keywords
    AND = "AND"
    AS = "AS"
    ASM = "ASM"
    ASSERT = "ASSERT"
    AUTO = "AUTO"
    BREAK = "BREAK"
    BOOL = "BOOL"
    CASE = "CASE"
    CATCH = "CATCH"
    CONST = "CONST"
    CONTINUE = "CONTINUE"
    DATA = "DATA"
    DEF = "DEF"
    DEFAULT = "DEFAULT"
    DO = "DO"
    ELSE = "ELSE"
    ENUM = "ENUM"
    FALSE = "FALSE"
    FLOAT_KW = "FLOAT_KW"
    FOR = "FOR"
    IF = "IF"
    IMPORT = "IMPORT"
    IN = "IN"
    INT = "INT"
    IS = "IS"
    NAMESPACE = "NAMESPACE"
    NOT = "NOT"
    OBJECT = "OBJECT"
    OR = "OR"
    RETURN = "RETURN"
    SIGNED = "SIGNED"
    SIZEOF = "SIZEOF"
    STRUCT = "STRUCT"
    SUPER = "SUPER"
    SWITCH = "SWITCH"
    TEMPLATE = "TEMPLATE"
    THIS = "THIS"
    THROW = "THROW"
    TRUE = "TRUE"
    TRY = "TRY"
    TYPEOF = "TYPEOF"
    UNSIGNED = "UNSIGNED"
    USING = "USING"
    VOID = "VOID"
    VOLATILE = "VOLATILE"
    WHILE = "WHILE"
    XOR = "XOR"
    
    # Operators - Arithmetic
    PLUS = "PLUS"                    # +
    MINUS = "MINUS"                  # -
    MULTIPLY = "MULTIPLY"            # *
    DIVIDE = "DIVIDE"                # /
    MODULO = "MODULO"                # %
    POWER = "POWER"                  # ^
    
    # Operators - Increment/Decrement
    INCREMENT = "INCREMENT"          # ++
    DECREMENT = "DECREMENT"          # --
    
    # Operators - Assignment
    ASSIGN = "ASSIGN"                # =
    PLUS_ASSIGN = "PLUS_ASSIGN"      # +=
    MINUS_ASSIGN = "MINUS_ASSIGN"    # -=
    MUL_ASSIGN = "MUL_ASSIGN"        # *=
    DIV_ASSIGN = "DIV_ASSIGN"        # /=
    MOD_ASSIGN = "MOD_ASSIGN"        # %=
    POW_ASSIGN = "POW_ASSIGN"        # ^=
    AND_ASSIGN = "AND_ASSIGN"        # &=
    OR_ASSIGN = "OR_ASSIGN"          # |=
    XOR_ASSIGN = "XOR_ASSIGN"        # ^^=
    LSHIFT_ASSIGN = "LSHIFT_ASSIGN"  # <<=
    RSHIFT_ASSIGN = "RSHIFT_ASSIGN"  # >>=
    
    # Bitwise Assignment Operators
    BAND_ASSIGN = "BAND_ASSIGN"          # `&&=
    BNAND_ASSIGN = "BNAND_ASSIGN"        # `!&=
    BOR_ASSIGN = "BOR_ASSIGN"            # `|=
    BNOR_ASSIGN = "BNOR_ASSIGN"          # `!|=
    BXOR_ASSIGN = "BXOR_ASSIGN"          # `^=
    BNEQ_ASSIGN = "BNEQ_ASSIGN"          # `!=
    
    # Operators - Comparison
    EQUAL = "EQUAL"                  # ==
    NOT_EQUAL = "NOT_EQUAL"          # !=
    LESS_THAN = "LESS_THAN"          # <
    LESS_EQUAL = "LESS_EQUAL"        # <=
    GREATER_THAN = "GREATER_THAN"    # >
    GREATER_EQUAL = "GREATER_EQUAL"  # >=
    
    # Operators - Logical
    LOGICAL_AND = "LOGICAL_AND"      # &&
    LOGICAL_OR = "LOGICAL_OR"        # ||
    LOGICAL_NOT = "LOGICAL_NOT"      # !
    LOGICAL_NAND = "LOGICAL_NAND"    # !&
    LOGICAL_NOR = "LOGICAL_NOR"      # !|
    
    # Operators - Bitwise
    BITWISE_AND = "BITWISE_AND"      # &
    BITWISE_OR = "BITWISE_OR"        # |
    BITWISE_XOR = "BITWISE_XOR"      # ^^
    BITWISE_NOT = "BITWISE_NOT"      # ~
    
    # Special Bitwise Operators (with backtick prefix)
    BBIT_AND = "BBIT_AND"            # `&&
    BBIT_NAND = "BBIT_NAND"          # `!&
    BBIT_OR = "BBIT_OR"              # `|
    BBIT_NOR = "BBIT_NOR"            # `!|
    BBIT_XOR = "BBIT_XOR"            # `^^
    BBIT_XNOR = "BBIT_XNOR"          # `^!|
    BBIT_XAND = "BBIT_XAND"          # `^&
    BBIT_XNAND = "BBIT_XNAND"        # `^!&
    
    # Operators - Shift
    LSHIFT = "LSHIFT"                # <<
    RSHIFT = "RSHIFT"                # >>
    
    # Operators - Memory
    ADDRESS_OF = "ADDRESS_OF"        # @
    DEREFERENCE = "DEREFERENCE"      # * (context-dependent)
    
    # Operators - Other
    CONDITIONAL = "CONDITIONAL"      # ?
    COLON = "COLON"                 # :
    SCOPE = "SCOPE"                 # ::
    DOT = "DOT"                     # .
    
    # Punctuation
    SEMICOLON = "SEMICOLON"         # ;
    COMMA = "COMMA"                 # ,
    LPAREN = "LPAREN"               # (
    RPAREN = "RPAREN"               # )
    LBRACE = "LBRACE"               # {
    RBRACE = "RBRACE"               # }
    LBRACKET = "LBRACKET"           # [
    RBRACKET = "RBRACKET"           # ]
    
    # Special tokens
    ARROW = "ARROW"                 # ->
    NEWLINE = "NEWLINE"
    EOF = "EOF"
    
    # Comments
    LINE_COMMENT = "LINE_COMMENT"
    BLOCK_COMMENT = "BLOCK_COMMENT"


@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int
    

class LexerError(Exception):
    def __init__(self, message: str, line: int, column: int):
        self.message = message
        self.line = line
        self.column = column
        super().__init__(f"Lexer error at line {line}, column {column}: {message}")


class FluxLexer:
    """Comprehensive lexer for the Flux programming language."""
    
    # Keywords mapping
    KEYWORDS = {
        'and': TokenType.AND,
        'as': TokenType.AS,
        'asm': TokenType.ASM,
        'assert': TokenType.ASSERT,
        'auto': TokenType.AUTO,
        'break': TokenType.BREAK,
        'bool': TokenType.BOOL,
        'case': TokenType.CASE,
        'catch': TokenType.CATCH,
        'const': TokenType.CONST,
        'continue': TokenType.CONTINUE,
        'data': TokenType.DATA,
        'def': TokenType.DEF,
        'default': TokenType.DEFAULT,
        'do': TokenType.DO,
        'else': TokenType.ELSE,
        'enum': TokenType.ENUM,
        'false': TokenType.FALSE,
        'float': TokenType.FLOAT_KW,
        'for': TokenType.FOR,
        'if': TokenType.IF,
        'import': TokenType.IMPORT,
        'in': TokenType.IN,
        'int': TokenType.INT,
        'is': TokenType.IS,
        'namespace': TokenType.NAMESPACE,
        'not': TokenType.NOT,
        'object': TokenType.OBJECT,
        'or': TokenType.OR,
        'return': TokenType.RETURN,
        'signed': TokenType.SIGNED,
        'sizeof': TokenType.SIZEOF,
        'struct': TokenType.STRUCT,
        'super': TokenType.SUPER,
        'switch': TokenType.SWITCH,
        'template': TokenType.TEMPLATE,
        'this': TokenType.THIS,
        'throw': TokenType.THROW,
        'true': TokenType.TRUE,
        'try': TokenType.TRY,
        'typeof': TokenType.TYPEOF,
        'unsigned': TokenType.UNSIGNED,
        'using': TokenType.USING,
        'void': TokenType.VOID,
        'volatile': TokenType.VOLATILE,
        'while': TokenType.WHILE,
        'xor': TokenType.XOR,
    }
    
    def __init__(self, source: str):
        self.source = source
        self.pos = 0
        self.line = 1
        self.column = 1
        self.tokens = []
        
    def current_char(self) -> Optional[str]:
        """Get the current character."""
        if self.pos >= len(self.source):
            return None
        return self.source[self.pos]
    
    def peek_char(self, offset: int = 1) -> Optional[str]:
        """Peek at a character ahead."""
        peek_pos = self.pos + offset
        if peek_pos >= len(self.source):
            return None
        return self.source[peek_pos]
    
    def advance(self, count: int = 1) -> None:
        """Advance by count characters (default 1)."""
        for _ in range(count):
            if self.pos < len(self.source):
                if self.source[self.pos] == '\n':
                    self.line += 1
                    self.column = 1
                else:
                    self.column += 1
                self.pos += 1
    
    def skip_whitespace(self) -> None:
        """Skip whitespace characters except newlines."""
        while self.current_char() and self.current_char() in ' \t\r':
            self.advance()
    
    def read_line_comment(self) -> Token:
        """Read a line comment starting with //."""
        start_line = self.line
        start_column = self.column
        comment = ""
        
        # Skip the //
        self.advance(2)
        
        while self.current_char() and self.current_char() != '\n':
            comment += self.current_char()
            self.advance()
            
        return Token(TokenType.LINE_COMMENT, '//' + comment, start_line, start_column)
    
    def read_block_comment(self) -> Token:
        """Read a block comment /* ... */."""
        start_line = self.line
        start_column = self.column
        comment = ""
        
        # Skip the /*
        self.advance(2)
        
        while self.current_char():
            if self.current_char() == '*' and self.peek_char() == '/':
                comment += '*/'
                self.advance(2)
                break
            comment += self.current_char()
            self.advance()
        else:
            raise LexerError("Unterminated block comment", start_line, start_column)
            
        return Token(TokenType.BLOCK_COMMENT, '/*' + comment, start_line, start_column)
    
    def read_number(self) -> Token:
        """Read a numeric literal (integer, float, or binary)."""
        start_line = self.line
        start_column = self.column
        number = ""
        
        # Check for binary literal
        if self.current_char() in '01':
            # Could be binary, check for 'b' suffix
            temp_pos = self.pos
            temp_line = self.line
            temp_column = self.column
            
            # Read potential binary digits
            while self.current_char() and self.current_char() in '01':
                number += self.current_char()
                self.advance()
            
            # Check for 'b' suffix
            if self.current_char() == 'b':
                number += 'b'
                self.advance()
                return Token(TokenType.BINARY, number, start_line, start_column)
            else:
                # Not binary, reset and continue as normal number
                self.pos = temp_pos
                self.line = temp_line
                self.column = temp_column
                number = ""
        
        # Read digits
        while self.current_char() and self.current_char().isdigit():
            number += self.current_char()
            self.advance()
        
        # Check for decimal point
        if self.current_char() == '.' and self.peek_char() and self.peek_char().isdigit():
            number += self.current_char()
            self.advance()
            
            while self.current_char() and self.current_char().isdigit():
                number += self.current_char()
                self.advance()
                
            return Token(TokenType.FLOAT, number, start_line, start_column)
        
        return Token(TokenType.INTEGER, number, start_line, start_column)
    
    def read_string(self) -> Token:
        """Read a string literal."""
        start_line = self.line
        start_column = self.column
        string_value = ""
        
        # Skip opening quote
        self.advance()
        
        while self.current_char() and self.current_char() != '"':
            if self.current_char() == '\\':
                # Handle escape sequences
                self.advance()
                if self.current_char() is None:
                    raise LexerError("Unterminated string literal", start_line, start_column)
                
                escape_char = self.current_char()
                if escape_char == 'n':
                    string_value += '\n'
                elif escape_char == 't':
                    string_value += '\t'
                elif escape_char == 'r':
                    string_value += '\r'
                elif escape_char == '\\':
                    string_value += '\\'
                elif escape_char == '"':
                    string_value += '"'
                elif escape_char == '\'':
                    string_value += '\''
                elif escape_char == '0':
                    string_value += '\0'
                else:
                    string_value += escape_char
                self.advance()
            else:
                string_value += self.current_char()
                self.advance()
        
        if self.current_char() != '"':
            raise LexerError("Unterminated string literal", start_line, start_column)
        
        # Skip closing quote
        self.advance()
        
        # Check if it's a single character (char literal)
        if len(string_value) == 1:
            return Token(TokenType.CHAR, string_value, start_line, start_column)
        
        return Token(TokenType.STRING, string_value, start_line, start_column)
    
    def skip_whitespace_and_newlines(self) -> None:
        """Skip whitespace characters including newlines."""
        while self.current_char() and self.current_char() in ' \t\r\n':
            self.advance()
    
    def read_i_string(self) -> Token:
        """Read an interpolated string literal i"...":{...}."""
        start_line = self.line
        start_column = self.column
        
        # Skip 'i'
        self.advance()
        
        # Read the string part
        if self.current_char() != '"':
            raise LexerError("Expected '\"' after 'i' for i-string", start_line, start_column)
        
        string_token = self.read_string()
        
        # Skip whitespace including newlines
        self.skip_whitespace_and_newlines()
        
        # Expect ':'
        if self.current_char() != ':':
            raise LexerError("Expected ':' after i-string template", start_line, start_column)
        self.advance()
        
        # Skip whitespace including newlines
        self.skip_whitespace_and_newlines()
        
        # Expect '{'
        if self.current_char() != '{':
            raise LexerError("Expected '{' after ':' in i-string", start_line, start_column)
        
        # Read until matching '}'
        brace_count = 0
        expressions = ""
        while self.current_char():
            if self.current_char() == '{':
                brace_count += 1
            elif self.current_char() == '}':
                brace_count -= 1
                if brace_count == 0:
                    self.advance()  # Skip the closing '}'
                    break
            expressions += self.current_char()
            self.advance()
        
        if brace_count != 0:
            raise LexerError("Unmatched braces in i-string expressions", start_line, start_column)
        
        # Combine template and expressions
        i_string_value = f'i"{string_token.value}":{{{expressions}}}'
        return Token(TokenType.I_STRING, i_string_value, start_line, start_column)
    
    def read_identifier(self) -> Token:
        """Read an identifier or keyword."""
        start_line = self.line
        start_column = self.column
        identifier = ""
        
        # First character must be letter
        if not (self.current_char().isalpha() or self.current_char() == '_'):
            raise LexerError("Invalid identifier start", start_line, start_column)
        
        while (self.current_char() and 
               (self.current_char().isalnum() or self.current_char() == '_')):
            identifier += self.current_char()
            self.advance()
        
        # Check if it's a keyword
        token_type = self.KEYWORDS.get(identifier, TokenType.IDENTIFIER)
        
        # Handle special boolean literals
        if identifier == 'true':
            return Token(TokenType.BOOLEAN, 'true', start_line, start_column)
        elif identifier == 'false':
            return Token(TokenType.BOOLEAN, 'false', start_line, start_column)
        
        return Token(token_type, identifier, start_line, start_column)
    
    def read_operator(self) -> Token:
        """Read an operator token."""
        start_line = self.line
        start_column = self.column
        char = self.current_char()
        
        # Handle multi-character operators first (longest match)
        if char == '<':
            if self.peek_char() == '<':
                if self.peek_char(2) == '=':
                    self.advance(3)
                    return Token(TokenType.LSHIFT_ASSIGN, '<<=', start_line, start_column)
                else:
                    self.advance(2)
                    return Token(TokenType.LSHIFT, '<<', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.LESS_EQUAL, '<=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.LESS_THAN, '<', start_line, start_column)
        
        elif char == '>':
            if self.peek_char() == '>':
                if self.peek_char(2) == '=':
                    self.advance(3)
                    return Token(TokenType.RSHIFT_ASSIGN, '>>=', start_line, start_column)
                else:
                    self.advance(2)
                    return Token(TokenType.RSHIFT, '>>', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.GREATER_EQUAL, '>=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.GREATER_THAN, '>', start_line, start_column)
        
        elif char == '=':
            if self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.EQUAL, '==', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.ASSIGN, '=', start_line, start_column)
        
        elif char == '!':
            if self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.NOT_EQUAL, '!=', start_line, start_column)
            elif self.peek_char() == '&':
                if self.peek_char(2) == '=':
                    self.advance(3)
                    return Token(TokenType.BNAND_ASSIGN, '!&=', start_line, start_column)
                else:
                    self.advance(2)
                    return Token(TokenType.LOGICAL_NAND, '!&', start_line, start_column)
            elif self.peek_char() == '|':
                if self.peek_char(2) == '=':
                    self.advance(3)
                    return Token(TokenType.BNOR_ASSIGN, '!|=', start_line, start_column)
                else:
                    self.advance(2)
                    return Token(TokenType.LOGICAL_NOR, '!|', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.LOGICAL_NOT, '!', start_line, start_column)
        
        elif char == '&':
            if self.peek_char() == '&':
                self.advance(2)
                return Token(TokenType.LOGICAL_AND, '&&', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.AND_ASSIGN, '&=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.BITWISE_AND, '&', start_line, start_column)
        
        elif char == '|':
            if self.peek_char() == '|':
                self.advance(2)
                return Token(TokenType.LOGICAL_OR, '||', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.OR_ASSIGN, '|=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.BITWISE_OR, '|', start_line, start_column)
        
        elif char == '^':
            if self.peek_char() == '^':
                if self.peek_char(2) == '=':
                    self.advance(3)
                    return Token(TokenType.XOR_ASSIGN, '^^=', start_line, start_column)
                else:
                    self.advance(2)
                    return Token(TokenType.BITWISE_XOR, '^^', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.POW_ASSIGN, '^=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.POWER, '^', start_line, start_column)
        
        elif char == '+':
            if self.peek_char() == '+':
                self.advance(2)
                return Token(TokenType.INCREMENT, '++', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.PLUS_ASSIGN, '+=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.PLUS, '+', start_line, start_column)
        
        elif char == '-':
            if self.peek_char() == '-':
                self.advance(2)
                return Token(TokenType.DECREMENT, '--', start_line, start_column)
            elif self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.MINUS_ASSIGN, '-=', start_line, start_column)
            elif self.peek_char() == '>':
                self.advance(2)
                return Token(TokenType.ARROW, '->', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.MINUS, '-', start_line, start_column)
        
        elif char == '*':
            if self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.MUL_ASSIGN, '*=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.MULTIPLY, '*', start_line, start_column)
        
        elif char == '/':
            if self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.DIV_ASSIGN, '/=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.DIVIDE, '/', start_line, start_column)
        
        elif char == '%':
            if self.peek_char() == '=':
                self.advance(2)
                return Token(TokenType.MOD_ASSIGN, '%=', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.MODULO, '%', start_line, start_column)
        
        elif char == ':':
            if self.peek_char() == ':':
                self.advance(2)
                return Token(TokenType.SCOPE, '::', start_line, start_column)
            else:
                self.advance()
                return Token(TokenType.COLON, ':', start_line, start_column)
        
        # Handle backtick operators
        elif char == '`':
            self.advance()
            next_char = self.current_char()
            
            if next_char == '&':
                if self.peek_char() == '&':
                    if self.peek_char(2) == '=':
                        self.advance(3)
                        return Token(TokenType.BAND_ASSIGN, '`&&=', start_line, start_column)
                    else:
                        self.advance(2)
                        return Token(TokenType.BBIT_AND, '`&&', start_line, start_column)
                else:
                    raise LexerError(f"Invalid operator `{next_char}", start_line, start_column)
            
            elif next_char == '!':
                if self.peek_char() == '&':
                    if self.peek_char(2) == '=':
                        self.advance(3)
                        return Token(TokenType.BNAND_ASSIGN, '`!&=', start_line, start_column)
                    else:
                        self.advance(2)
                        return Token(TokenType.BBIT_NAND, '`!&', start_line, start_column)
                elif self.peek_char() == '|':
                    if self.peek_char(2) == '=':
                        self.advance(3)
                        return Token(TokenType.BNOR_ASSIGN, '`!|=', start_line, start_column)
                    else:
                        self.advance(2)
                        return Token(TokenType.BBIT_NOR, '`!|', start_line, start_column)
                elif self.peek_char() == '=':
                    self.advance(2)
                    return Token(TokenType.BNEQ_ASSIGN, '`!=', start_line, start_column)
                else:
                    raise LexerError(f"Invalid operator `!{self.peek_char()}", start_line, start_column)
            
            elif next_char == '|':
                if self.peek_char() == '=':
                    self.advance(2)
                    return Token(TokenType.BOR_ASSIGN, '`|=', start_line, start_column)
                else:
                    self.advance()
                    return Token(TokenType.BBIT_OR, '`|', start_line, start_column)
            
            elif next_char == '^':
                if self.peek_char() == '^':
                    if self.peek_char(2) == '=':
                        self.advance(3)
                        return Token(TokenType.BXOR_ASSIGN, '`^^=', start_line, start_column)
                    else:
                        self.advance(2)
                        return Token(TokenType.BBIT_XOR, '`^^', start_line, start_column)
                elif self.peek_char() == '!':
                    if self.peek_char(2) == '|':
                        if self.peek_char(3) == '=':
                            self.advance(4)
                            return Token(TokenType.BXNOREQ, '`^!|=', start_line, start_column)
                        else:
                            self.advance(3)
                            return Token(TokenType.BBIT_XNOR, '`^!|', start_line, start_column)
                    elif self.peek_char(2) == '&':
                        if self.peek_char(3) == '=':
                            self.advance(4)
                            return Token(TokenType.BXNANDEQ, '`^!&=', start_line, start_column)
                        else:
                            self.advance(3)
                            return Token(TokenType.BBIT_XNAND, '`^!&', start_line, start_column)
                elif self.peek_char() == '&':
                    if self.peek_char(2) == '=':
                        self.advance(3)
                        return Token(TokenType.BXANDEQ, '`^&=', start_line, start_column)
                    else:
                        self.advance(2)
                        return Token(TokenType.BBIT_XAND, '`^&', start_line, start_column)
                elif self.peek_char() == '=':
                    self.advance(2)
                    return Token(TokenType.BXOR_ASSIGN, '`^=', start_line, start_column)
                else:
                    raise LexerError(f"Invalid operator `^{self.peek_char()}", start_line, start_column)
            
            else:
                raise LexerError(f"Invalid operator `{next_char}", start_line, start_column)
        
        # Single character operators
        elif char == '~':
            self.advance()
            return Token(TokenType.BITWISE_NOT, '~', start_line, start_column)
        elif char == '@':
            self.advance()
            return Token(TokenType.ADDRESS_OF, '@', start_line, start_column)
        elif char == '?':
            self.advance()
            return Token(TokenType.CONDITIONAL, '?', start_line, start_column)
        elif char == '.':
            self.advance()
            return Token(TokenType.DOT, '.', start_line, start_column)
        else:
            raise LexerError(f"Unknown operator '{char}'", start_line, start_column)
    
    def next_token(self) -> Token:
        """Get the next token from the source."""
        while self.current_char():
            # Skip whitespace
            if self.current_char() in ' \t\r':
                self.skip_whitespace()
                continue
            
            # Handle newlines
            if self.current_char() == '\n':
                token = Token(TokenType.NEWLINE, '\n', self.line, self.column)
                self.advance()
                return token
            
            # Handle comments
            if self.current_char() == '/':
                if self.peek_char() == '/':
                    return self.read_line_comment()
                elif self.peek_char() == '*':
                    return self.read_block_comment()
                else:
                    return self.read_operator()
            
            # Handle numbers
            if self.current_char().isdigit():
                return self.read_number()
            
            # Handle strings and characters
            if self.current_char() == '"':
                return self.read_string()
            
            # Handle i-strings
            if self.current_char() == 'i' and self.peek_char() == '"':
                return self.read_i_string()
            
            # Handle identifiers and keywords
            if self.current_char().isalpha() or self.current_char() == '_':
                return self.read_identifier()
            
            # Handle punctuation
            char = self.current_char()
            start_line = self.line
            start_column = self.column
            
            if char == ';':
                self.advance()
                return Token(TokenType.SEMICOLON, ';', start_line, start_column)
            elif char == ',':
                self.advance()
                return Token(TokenType.COMMA, ',', start_line, start_column)
            elif char == '(':
                self.advance()
                return Token(TokenType.LPAREN, '(', start_line, start_column)
            elif char == ')':
                self.advance()
                return Token(TokenType.RPAREN, ')', start_line, start_column)
            elif char == '{':
                self.advance()
                return Token(TokenType.LBRACE, '{', start_line, start_column)
            elif char == '}':
                self.advance()
                return Token(TokenType.RBRACE, '}', start_line, start_column)
            elif char == '[':
                self.advance()
                return Token(TokenType.LBRACKET, '[', start_line, start_column)
            elif char == ']':
                self.advance()
                return Token(TokenType.RBRACKET, ']', start_line, start_column)
            
            # Handle operators
            elif char in '+-*/%^&|!<>=~@?.:':
                return self.read_operator()
            
            # Handle backtick operators
            elif char == '`':
                return self.read_operator()
            
            else:
                raise LexerError(f"Unexpected character '{char}'", self.line, self.column)
        
        # End of file
        return Token(TokenType.EOF, '', self.line, self.column)
    
    def tokenize(self) -> List[Token]:
        """Tokenize the entire source and return a list of tokens."""
        tokens = []
        
        while True:
            token = self.next_token()
            
            # Skip comments in the token list (but keep them for potential use)
            if token.type not in (TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT):
                tokens.append(token)
            
            if token.type == TokenType.EOF:
                break
        
        return tokens


def lex_file(filepath: str) -> List[Token]:
    """Lex a Flux source file and return the tokens."""
    try:
        with open(filepath, 'r', encoding='utf-8') as file:
            source_code = file.read()
    except FileNotFoundError:
        raise FileNotFoundError(f"Source file '{filepath}' not found")
    except IOError as e:
        raise IOError(f"Error reading file '{filepath}': {e}")
    
    lexer = FluxLexer(source_code)
    return lexer.tokenize()


def main():
    """Main function to lex a Flux source file."""
    import sys
    import os
    
    if len(sys.argv) != 2:
        print("Usage: python flexer.py <flux_source_file>")
        print("Example: python flexer.py example.fx")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' does not exist")
        sys.exit(1)
    
    if not filepath.endswith('.fx'):
        print(f"Warning: File '{filepath}' does not have .fx extension")
    
    try:
        print(f"Lexing file: {filepath}")
        print("=" * 60)
        
        tokens = lex_file(filepath)
        
        print(f"Successfully tokenized {len(tokens)} tokens")
        print("=" * 60)
        print("Tokens:")
        print("-" * 60)
        
        for i, token in enumerate(tokens):
            print(f"{i+1:4d}: {token.type.value:20} | {repr(token.value):20} | Line {token.line:3d}, Col {token.column:3d}")
        
        print("=" * 60)
        print("Lexing completed successfully")
    
    except (FileNotFoundError, IOError) as e:
        print(f"File Error: {e}")
        sys.exit(1)
    except LexerError as e:
        print(f"Lexer Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()