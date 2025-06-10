#!/usr/bin/env python3
"""
Flux Programming Language Lexer
flexer.py - Tokenizes Flux source code into a stream of tokens

This lexer handles all Flux language constructs including:
- Keywords and identifiers
- Operators with special bitwise syntax
- String literals and interpolated strings
- Numeric literals
- Comments
- Template syntax (context-unaware for '>>' vs '>')
"""

import re
from enum import Enum, auto
from dataclasses import dataclass
from typing import List, Optional, Union, Iterator


class TokenType(Enum):
    # Literals
    INTEGER = auto()
    FLOAT = auto()
    CHARACTER = auto()
    STRING = auto()
    INTERPOLATED_STRING = auto()
    BOOLEAN = auto()
    
    # Identifiers and Keywords
    IDENTIFIER = auto()
    KEYWORD = auto()
    
    # Primary expressions
    LPAREN = auto()          # (
    RPAREN = auto()          # )
    LBRACKET = auto()        # [
    RBRACKET = auto()        # ]
    DOT = auto()             # .
    SCOPE = auto()           # ::
    
    # Postfix operators
    INCREMENT = auto()       # ++
    DECREMENT = auto()       # --
    
    # Unary operators
    PLUS = auto()            # +
    MINUS = auto()           # -
    LOGICAL_NOT = auto()     # !
    BITWISE_NOT = auto()     # ~
    ADDRESS_OF = auto()      # @
    DEREFERENCE = auto()     # *
    SIZEOF = auto()          # sizeof
    TYPEOF = auto()          # typeof
    NOT = auto()             # not
    
    # Type casting
    # (handled by LPAREN/RPAREN)
    
    # Arithmetic operators
    MULTIPLY = auto()        # *
    DIVIDE = auto()          # /
    MODULO = auto()          # %
    POWER = auto()           # ^
    
    # Additive operators  
    # PLUS and MINUS already defined
    
    # Shift operators
    LEFT_SHIFT = auto()      # <<
    RIGHT_SHIFT = auto()     # >>
    
    # Relational operators
    LESS_THAN = auto()       # <
    LESS_EQUAL = auto()      # <=
    GREATER_THAN = auto()    # >
    GREATER_EQUAL = auto()   # >=
    IN = auto()              # in
    
    # Equality operators
    EQUAL = auto()           # ==
    NOT_EQUAL = auto()       # !=
    
    # Identity operators
    IS = auto()              # is
    AS = auto()              # as
    
    # Bitwise operators (standard)
    BITWISE_AND = auto()     # &
    BITWISE_XOR = auto()     # ^^
    BITWISE_OR = auto()      # |
    
    # Bitwise operators (special ` prefix)
    BIT_AND = auto()         # `&
    BIT_NAND = auto()        # `!&
    BIT_OR = auto()          # `|
    BIT_NOR = auto()         # `!|
    BIT_XOR = auto()         # `^^
    BIT_XNOR = auto()        # `^!|
    BIT_XAND = auto()        # `^&
    BIT_XNAND = auto()       # `^!&
    
    # Logical operators
    LOGICAL_AND = auto()     # && / and
    LOGICAL_NAND = auto()    # !&
    LOGICAL_OR = auto()      # || / or
    LOGICAL_NOR = auto()     # !|
    LOGICAL_XOR = auto()     # xor
    LOGICAL_XAND = auto()    # ^&
    LOGICAL_XNOR = auto()    # ^!|
    LOGICAL_XNAND = auto()   # ^!&
    
    # Conditional operator
    QUESTION = auto()        # ?
    COLON = auto()           # :
    
    # Assignment operators
    ASSIGN = auto()          # =
    PLUS_ASSIGN = auto()     # +=
    MINUS_ASSIGN = auto()    # -=
    MULTIPLY_ASSIGN = auto() # *=
    DIVIDE_ASSIGN = auto()   # /=
    MODULO_ASSIGN = auto()   # %=
    POWER_ASSIGN = auto()    # ^=
    AND_ASSIGN = auto()      # &=
    OR_ASSIGN = auto()       # |=
    XOR_ASSIGN = auto()      # ^^=
    LEFT_SHIFT_ASSIGN = auto()  # <<=
    RIGHT_SHIFT_ASSIGN = auto() # >>=
    
    # Special bitwise assignment operators
    BIT_AND_ASSIGN = auto()     # `&=
    BIT_OR_ASSIGN = auto()      # `|=
    BIT_XOR_ASSIGN = auto()     # `^|=
    BIT_NAND_ASSIGN = auto()    # `!&=
    BIT_NOR_ASSIGN = auto()     # `!|=
    BIT_XAND_ASSIGN = auto()    # `^&=
    BIT_XNOR_ASSIGN = auto()    # `^!|=
    BIT_XNAND_ASSIGN = auto()   # `^!&=
    
    # Other operators
    COMMA = auto()           # ,
    SEMICOLON = auto()       # ;
    LBRACE = auto()          # {
    RBRACE = auto()          # }
    
    # Template syntax
    TEMPLATE_OPEN = auto()   # <
    TEMPLATE_CLOSE = auto()  # >
    
    # Special tokens
    EOF = auto()
    NEWLINE = auto()
    COMMENT = auto()
    
    # Error token
    ERROR = auto()


@dataclass
class SourceLocation:
    """Represents a location in source code"""
    line: int
    column: int
    filename: str = ""


@dataclass
class Token:
    """Represents a single token in the source code"""
    type: TokenType
    value: str
    location: SourceLocation
    
    def __repr__(self):
        return f"Token({self.type.name}, '{self.value}', {self.location.line}:{self.location.column})"


class LexerError(Exception):
    """Exception raised when lexer encounters an error"""
    def __init__(self, message: str, location: SourceLocation):
        self.message = message
        self.location = location
        super().__init__(f"{message} at {location.line}:{location.column}")


class FluxLexer:
    """
    Flux Programming Language Lexer
    
    Tokenizes Flux source code while remaining context-unaware for template parsing.
    The parser will handle context-sensitive operations like breaking '>>' into '>' tokens.
    """
    
    # Flux keywords
    KEYWORDS = {
        'alignof', 'and', 'asm', 'assert', 'auto', 'break', 'bool', 'case', 'catch',
        'const', 'continue', 'data', 'def', 'default', 'do', 'else', 'enum', 'false',
        'float', 'for', 'if', 'import', 'in', 'int', 'is', 'namespace', 'not', 'object',
        'or', 'return', 'signed', 'sizeof', 'struct', 'super', 'switch', 'template',
        'this', 'throw', 'true', 'try', 'typeof', 'unsigned', 'using', 'void',
        'volatile', 'while', 'xor'
    }
    
    def __init__(self, source: str, filename: str = "<input>"):
        self.source = source
        self.filename = filename
        self.pos = 0
        self.line = 1
        self.column = 1
        self.tokens = []
        
    def current_location(self) -> SourceLocation:
        """Get current source location"""
        return SourceLocation(self.line, self.column, self.filename)
    
    def peek(self, offset: int = 0) -> Optional[str]:
        """Peek at character at current position + offset"""
        pos = self.pos + offset
        return self.source[pos] if pos < len(self.source) else None
    
    def advance(self) -> Optional[str]:
        """Advance position and return current character"""
        if self.pos >= len(self.source):
            return None
        
        char = self.source[self.pos]
        self.pos += 1
        
        if char == '\n':
            self.line += 1
            self.column = 1
        else:
            self.column += 1
            
        return char
    
    def skip_whitespace(self):
        """Skip whitespace characters"""
        while self.peek() and self.peek().isspace():
            self.advance()
    
    def read_while(self, predicate) -> str:
        """Read characters while predicate is true"""
        result = ""
        while self.peek() and predicate(self.peek()):
            result += self.advance()
        return result
    
    def match_string(self, string: str) -> bool:
        """Check if current position matches string"""
        for i, char in enumerate(string):
            if self.peek(i) != char:
                return False
        return True
    
    def consume_string(self, string: str) -> bool:
        """Consume string if it matches at current position"""
        if self.match_string(string):
            for _ in string:
                self.advance()
            return True
        return False
    
    def add_token(self, token_type: TokenType, value: str, location: Optional[SourceLocation] = None):
        """Add token to token list"""
        if location is None:
            location = self.current_location()
        self.tokens.append(Token(token_type, value, location))
    
    def read_number(self) -> Token:
        """Read numeric literal (integer or float)"""
        location = self.current_location()
        number = ""
        
        # Handle hex numbers
        if self.peek() == '0' and self.peek(1) in 'xX':
            number += self.advance()  # '0'
            number += self.advance()  # 'x' or 'X'
            hex_digits = self.read_while(lambda c: c.isdigit() or c.lower() in 'abcdef')
            if not hex_digits:
                raise LexerError("Invalid hexadecimal number", location)
            number += hex_digits
            return Token(TokenType.INTEGER, number, location)
        
        # Handle binary numbers
        if self.peek() == '0' and self.peek(1) in 'bB':
            number += self.advance()  # '0'
            number += self.advance()  # 'b' or 'B'
            bin_digits = self.read_while(lambda c: c in '01')
            if not bin_digits:
                raise LexerError("Invalid binary number", location)
            number += bin_digits
            return Token(TokenType.INTEGER, number, location)
        
        # Read integer part
        number += self.read_while(lambda c: c.isdigit())
        
        # Check for decimal point
        if self.peek() == '.' and self.peek(1) and self.peek(1).isdigit():
            number += self.advance()  # '.'
            number += self.read_while(lambda c: c.isdigit())
            
            # Check for exponent
            if self.peek() and self.peek().lower() == 'e':
                number += self.advance()  # 'e' or 'E'
                if self.peek() in '+-':
                    number += self.advance()
                exp_digits = self.read_while(lambda c: c.isdigit())
                if not exp_digits:
                    raise LexerError("Invalid float exponent", location)
                number += exp_digits
            
            return Token(TokenType.FLOAT, number, location)
        
        # Check for exponent on integer
        if self.peek() and self.peek().lower() == 'e':
            number += self.advance()  # 'e' or 'E'
            if self.peek() in '+-':
                number += self.advance()
            exp_digits = self.read_while(lambda c: c.isdigit())
            if not exp_digits:
                raise LexerError("Invalid float exponent", location)
            number += exp_digits
            return Token(TokenType.FLOAT, number, location)
        
        return Token(TokenType.INTEGER, number, location)
    
    def read_string(self, quote_char: str) -> Token:
        """Read string literal"""
        location = self.current_location()
        result = quote_char  # Include opening quote
        self.advance()  # Skip opening quote
        
        while self.peek() and self.peek() != quote_char:
            char = self.peek()
            if char == '\\':
                result += self.advance()  # backslash
                if self.peek():
                    result += self.advance()  # escaped character
            else:
                result += self.advance()
        
        if not self.peek():
            raise LexerError(f"Unterminated string literal", location)
        
        result += self.advance()  # closing quote
        
        if quote_char == '"':
            return Token(TokenType.STRING, result, location)
        else:  # single quote
            return Token(TokenType.CHARACTER, result, location)
    
    def read_interpolated_string(self) -> Token:
        """Read interpolated string literal (i"...{...}...")"""
        location = self.current_location()
        result = "i"
        self.advance()  # skip 'i'
        
        if self.peek() != '"':
            raise LexerError("Expected '\"' after 'i' for interpolated string", location)
        
        result += self.advance()  # opening quote
        brace_depth = 0
        
        while self.peek():
            char = self.peek()
            
            if char == '"' and brace_depth == 0:
                result += self.advance()  # closing quote
                break
            elif char == '{':
                brace_depth += 1
                result += self.advance()
            elif char == '}':
                brace_depth -= 1
                result += self.advance()
            elif char == '\\':
                result += self.advance()  # backslash
                if self.peek():
                    result += self.advance()  # escaped character
            else:
                result += self.advance()
        
        if brace_depth != 0 or not result.endswith('"'):
            raise LexerError("Unterminated interpolated string literal", location)
        
        return Token(TokenType.INTERPOLATED_STRING, result, location)
    
    def read_identifier(self) -> Token:
        """Read identifier or keyword"""
        location = self.current_location()
        identifier = self.read_while(lambda c: c.isalnum() or c == '_')
        
        # Check if it's a keyword
        if identifier in self.KEYWORDS:
            # Special handling for boolean keywords
            if identifier in ('true', 'false'):
                return Token(TokenType.BOOLEAN, identifier, location)
            else:
                return Token(TokenType.KEYWORD, identifier, location)
        
        return Token(TokenType.IDENTIFIER, identifier, location)
    
    def read_comment(self) -> Optional[Token]:
        """Read comment (single-line // or multi-line /* */)"""
        location = self.current_location()
        
        if self.match_string('//'):
            # Single-line comment
            comment = ""
            self.advance()  # first /
            self.advance()  # second /
            
            while self.peek() and self.peek() != '\n':
                comment += self.advance()
            
            return Token(TokenType.COMMENT, '//' + comment, location)
        
        elif self.match_string('/*'):
            # Multi-line comment
            comment = ""
            self.advance()  # /
            self.advance()  # *
            
            while self.peek():
                if self.match_string('*/'):
                    self.advance()  # *
                    self.advance()  # /
                    break
                comment += self.advance()
            else:
                raise LexerError("Unterminated multi-line comment", location)
            
            return Token(TokenType.COMMENT, '/*' + comment + '*/', location)
        
        return None
    
    def read_operator(self) -> Optional[Token]:
        """Read operator tokens"""
        location = self.current_location()
        char = self.peek()
        
        # Three-character operators
        if self.match_string('>>='):
            self.consume_string('>>=')
            return Token(TokenType.RIGHT_SHIFT_ASSIGN, '>>=', location)
        elif self.match_string('<<='):
            self.consume_string('<<=')
            return Token(TokenType.LEFT_SHIFT_ASSIGN, '<<=', location)
        elif self.match_string('^^='):
            self.consume_string('^^=')
            return Token(TokenType.XOR_ASSIGN, '^^=', location)
        elif self.match_string('!&='):
            self.consume_string('!&=')
            return Token(TokenType.LOGICAL_NAND, '!&=', location)
        elif self.match_string('!|='):
            self.consume_string('!|=')
            return Token(TokenType.LOGICAL_NOR, '!|=', location)
        elif self.match_string('^&='):
            self.consume_string('^&=')
            return Token(TokenType.LOGICAL_XAND, '^&=', location)
        elif self.match_string('^!|='):
            self.consume_string('^!|=')
            return Token(TokenType.LOGICAL_XNOR, '^!|=', location)
        elif self.match_string('^!&='):
            self.consume_string('^!&=')
            return Token(TokenType.LOGICAL_XNAND, '^!&=', location)
        
        # Special bitwise operators with ` prefix
        elif self.match_string('`^!|='):
            self.consume_string('`^!|=')
            return Token(TokenType.BIT_XNOR_ASSIGN, '`^!|=', location)
        elif self.match_string('`^!&='):
            self.consume_string('`^!&=')
            return Token(TokenType.BIT_XNAND_ASSIGN, '`^!&=', location)
        elif self.match_string('`^|='):
            self.consume_string('`^|=')
            return Token(TokenType.BIT_XOR_ASSIGN, '`^|=', location)
        elif self.match_string('`!&='):
            self.consume_string('`!&=')
            return Token(TokenType.BIT_NAND_ASSIGN, '`!&=', location)
        elif self.match_string('`!|='):
            self.consume_string('`!|=')
            return Token(TokenType.BIT_NOR_ASSIGN, '`!|=', location)
        elif self.match_string('`^&='):
            self.consume_string('`^&=')
            return Token(TokenType.BIT_XAND_ASSIGN, '`^&=', location)
        elif self.match_string('`&='):
            self.consume_string('`&=')
            return Token(TokenType.BIT_AND_ASSIGN, '`&=', location)
        elif self.match_string('`|='):
            self.consume_string('`|=')
            return Token(TokenType.BIT_OR_ASSIGN, '`|=', location)
        
        # Four-character special bitwise operators
        elif self.match_string('`^!|'):
            self.consume_string('`^!|')
            return Token(TokenType.BIT_XNOR, '`^!|', location)
        elif self.match_string('`^!&'):
            self.consume_string('`^!&')
            return Token(TokenType.BIT_XNAND, '`^!&', location)
        
        # Three-character special bitwise operators  
        elif self.match_string('`^^'):
            self.consume_string('`^^')
            return Token(TokenType.BIT_XOR, '`^^', location)
        elif self.match_string('`^&'):
            self.consume_string('`^&')
            return Token(TokenType.BIT_XAND, '`^&', location)
        elif self.match_string('`!&'):
            self.consume_string('`!&')
            return Token(TokenType.BIT_NAND, '`!&', location)
        elif self.match_string('`!|'):
            self.consume_string('`!|')
            return Token(TokenType.BIT_NOR, '`!|', location)
        
        # Two-character special bitwise operators
        elif self.match_string('`&'):
            self.consume_string('`&')
            return Token(TokenType.BIT_AND, '`&', location)
        elif self.match_string('`|'):
            self.consume_string('`|')
            return Token(TokenType.BIT_OR, '`|', location)
        
        # Two-character operators
        elif self.match_string('++'):
            self.consume_string('++')
            return Token(TokenType.INCREMENT, '++', location)
        elif self.match_string('--'):
            self.consume_string('--')
            return Token(TokenType.DECREMENT, '--', location)
        elif self.match_string('<<'):
            self.consume_string('<<')
            return Token(TokenType.LEFT_SHIFT, '<<', location)
        elif self.match_string('>>'):
            self.consume_string('>>')
            return Token(TokenType.RIGHT_SHIFT, '>>', location)
        elif self.match_string('<='):
            self.consume_string('<=')
            return Token(TokenType.LESS_EQUAL, '<=', location)
        elif self.match_string('>='):
            self.consume_string('>=')
            return Token(TokenType.GREATER_EQUAL, '>=', location)
        elif self.match_string('=='):
            self.consume_string('==')
            return Token(TokenType.EQUAL, '==', location)
        elif self.match_string('!='):
            self.consume_string('!=')
            return Token(TokenType.NOT_EQUAL, '!=', location)
        elif self.match_string('&&'):
            self.consume_string('&&')
            return Token(TokenType.LOGICAL_AND, '&&', location)
        elif self.match_string('||'):
            self.consume_string('||')
            return Token(TokenType.LOGICAL_OR, '||', location)
        elif self.match_string('^^'):
            self.consume_string('^^')
            return Token(TokenType.BITWISE_XOR, '^^', location)
        elif self.match_string('!&'):
            self.consume_string('!&')
            return Token(TokenType.LOGICAL_NAND, '!&', location)
        elif self.match_string('!|'):
            self.consume_string('!|')
            return Token(TokenType.LOGICAL_NOR, '!|', location)
        elif self.match_string('^&'):
            self.consume_string('^&')
            return Token(TokenType.LOGICAL_XAND, '^&', location)
        elif self.match_string('^!|'):
            self.consume_string('^!|')
            return Token(TokenType.LOGICAL_XNOR, '^!|', location)
        elif self.match_string('^!&'):
            self.consume_string('^!&')
            return Token(TokenType.LOGICAL_XNAND, '^!&', location)
        elif self.match_string('::'):
            self.consume_string('::')
            return Token(TokenType.SCOPE, '::', location)
        elif self.match_string('+='):
            self.consume_string('+=')
            return Token(TokenType.PLUS_ASSIGN, '+=', location)
        elif self.match_string('-='):
            self.consume_string('-=')
            return Token(TokenType.MINUS_ASSIGN, '-=', location)
        elif self.match_string('*='):
            self.consume_string('*=')
            return Token(TokenType.MULTIPLY_ASSIGN, '*=', location)
        elif self.match_string('/='):
            self.consume_string('/=')
            return Token(TokenType.DIVIDE_ASSIGN, '/=', location)
        elif self.match_string('%='):
            self.consume_string('%=')
            return Token(TokenType.MODULO_ASSIGN, '%=', location)
        elif self.match_string('^='):
            self.consume_string('^=')
            return Token(TokenType.POWER_ASSIGN, '^=', location)
        elif self.match_string('&='):
            self.consume_string('&=')
            return Token(TokenType.AND_ASSIGN, '&=', location)
        elif self.match_string('|='):
            self.consume_string('|=')
            return Token(TokenType.OR_ASSIGN, '|=', location)
        
        # Single-character operators
        elif char == '(':
            self.advance()
            return Token(TokenType.LPAREN, '(', location)
        elif char == ')':
            self.advance()
            return Token(TokenType.RPAREN, ')', location)
        elif char == '[':
            self.advance()
            return Token(TokenType.LBRACKET, '[', location)
        elif char == ']':
            self.advance()
            return Token(TokenType.RBRACKET, ']', location)
        elif char == '{':
            self.advance()
            return Token(TokenType.LBRACE, '{', location)
        elif char == '}':
            self.advance()
            return Token(TokenType.RBRACE, '}', location)
        elif char == '.':
            self.advance()
            return Token(TokenType.DOT, '.', location)
        elif char == ',':
            self.advance()
            return Token(TokenType.COMMA, ',', location)
        elif char == ';':
            self.advance()
            return Token(TokenType.SEMICOLON, ';', location)
        elif char == '?':
            self.advance()
            return Token(TokenType.QUESTION, '?', location)
        elif char == ':':
            self.advance()
            return Token(TokenType.COLON, ':', location)
        elif char == '+':
            self.advance()
            return Token(TokenType.PLUS, '+', location)
        elif char == '-':
            self.advance()
            return Token(TokenType.MINUS, '-', location)
        elif char == '*':
            self.advance()
            return Token(TokenType.MULTIPLY, '*', location)
        elif char == '/':
            self.advance()
            return Token(TokenType.DIVIDE, '/', location)
        elif char == '%':
            self.advance()
            return Token(TokenType.MODULO, '%', location)
        elif char == '^':
            self.advance()
            return Token(TokenType.POWER, '^', location)
        elif char == '&':
            self.advance()
            return Token(TokenType.BITWISE_AND, '&', location)
        elif char == '|':
            self.advance()
            return Token(TokenType.BITWISE_OR, '|', location)
        elif char == '~':
            self.advance()
            return Token(TokenType.BITWISE_NOT, '~', location)
        elif char == '!':
            self.advance()
            return Token(TokenType.LOGICAL_NOT, '!', location)
        elif char == '<':
            self.advance()
            return Token(TokenType.LESS_THAN, '<', location)
        elif char == '>':
            self.advance()
            return Token(TokenType.GREATER_THAN, '>', location)
        elif char == '=':
            self.advance()
            return Token(TokenType.ASSIGN, '=', location)
        elif char == '@':
            self.advance()
            return Token(TokenType.ADDRESS_OF, '@', location)
        
        return None
    
    def tokenize(self) -> List[Token]:
        """Tokenize the entire source code"""
        self.tokens = []
        
        while self.pos < len(self.source):
            self.skip_whitespace()
            
            if self.pos >= len(self.source):
                break
            
            location = self.current_location()
            char = self.peek()
            
            # Handle newlines separately if needed
            if char == '\n':
                self.advance()
                continue
            
            # Comments
            if char == '/':
                comment_token = self.read_comment()
                if comment_token:
                    self.add_token(comment_token.type, comment_token.value, comment_token.location)
                    continue
            
            # Numbers
            if char.isdigit():
                token = self.read_number()
                self.add_token(token.type, token.value, token.location)
                continue
            
            # Strings and characters
            if char == '"':
                token = self.read_string('"')
                self.add_token(token.type, token.value, token.location)
                continue
            elif char == "'":
                token = self.read_string("'")
                self.add_token(token.type, token.value, token.location)
                continue
            
            # Interpolated strings
            if char == 'i' and self.peek(1) == '"':
                token = self.read_interpolated_string()
                self.add_token(token.type, token.value, token.location)
                continue
            
            # Identifiers and keywords
            if char.isalpha() or char == '_':
                token = self.read_identifier()
                self.add_token(token.type, token.value, token.location)
                continue
            
            # Operators and punctuation
            operator_token = self.read_operator()
            if operator_token:
                self.add_token(operator_token.type, operator_token.value, operator_token.location)
                continue
            
            # Unknown character
            raise LexerError(f"Unexpected character '{char}'", location)
        
        # Add EOF token
        self.add_token(TokenType.EOF, "", self.current_location())
        return self.tokens
    
    def split_right_shift(self, token_index: int) -> bool:
        """
        Split a RIGHT_SHIFT token into two GREATER_THAN tokens for template parsing.
        This method is called by the parser when it needs to handle '>>' as '> >' in template context.
        Returns True if split was successful, False otherwise.
        """
        if (token_index >= len(self.tokens) or 
            self.tokens[token_index].type != TokenType.RIGHT_SHIFT):
            return False
        
        original_token = self.tokens[token_index]
        location = original_token.location
        
        # Replace the '>>' token with two '>' tokens
        first_gt = Token(TokenType.GREATER_THAN, '>', location)
        second_location = SourceLocation(location.line, location.column + 1, location.filename)
        second_gt = Token(TokenType.GREATER_THAN, '>', second_location)
        
        # Replace the original token with the two new tokens
        self.tokens[token_index:token_index+1] = [first_gt, second_gt]
        return True
    
    def get_tokens(self) -> List[Token]:
        """Get the list of tokens (for parser access)"""
        return self.tokens
    
    def print_tokens(self):
        """Print all tokens for debugging"""
        for token in self.tokens:
            print(token)


def main():
    """Run the lexer on a Flux source file"""
    import sys
    
    if len(sys.argv) != 2:
        print("Usage: python flexer.py <source_file.fx>")
        print("Example: python flexer.py example.fx")
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
    
    # Create lexer and tokenize
    lexer = FluxLexer(source_code, filename)
    try:
        tokens = lexer.tokenize()
        
        # Print results
        print(f"=== Tokenizing {filename} ===")
        lexer.print_tokens()
        print(f"\n=== Summary ===")
        print(f"Successfully tokenized {len(tokens)} tokens from {filename}")
        
        # Print token type statistics
        token_counts = {}
        for token in tokens:
            token_type = token.type.name
            token_counts[token_type] = token_counts.get(token_type, 0) + 1
        
        print(f"\nToken type counts:")
        for token_type, count in sorted(token_counts.items()):
            print(f"  {token_type}: {count}")
            
    except LexerError as e:
        print(f"Lexer error in {filename}: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()