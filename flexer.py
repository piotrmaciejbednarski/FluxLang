import re
import enum
from typing import List, Optional
from dataclasses import dataclass

class TokenType(enum.Enum):
    # Literals
    INTEGER = "INTEGER"
    FLOAT = "FLOAT"
    STRING = "STRING"
    BINARY = "BINARY"
    
    # Identifiers and Keywords
    IDENTIFIER = "IDENTIFIER"
    KEYWORD = "KEYWORD"
    
    # Operators
    PLUS = "PLUS"                    # +
    MINUS = "MINUS"                  # -
    MULTIPLY = "MULTIPLY"            # *
    DIVIDE = "DIVIDE"                # /
    MODULO = "MODULO"                # %
    POWER = "POWER"                  # ^^
    
    # Assignment operators
    ASSIGN = "ASSIGN"                # =
    PLUS_ASSIGN = "PLUS_ASSIGN"      # +=
    MINUS_ASSIGN = "MINUS_ASSIGN"    # -=
    MULT_ASSIGN = "MULT_ASSIGN"      # *=
    DIV_ASSIGN = "DIV_ASSIGN"        # /=
    MOD_ASSIGN = "MOD_ASSIGN"        # %=
    POWER_ASSIGN = "POWER_ASSIGN"    # ^^=
    
    # Comparison operators
    EQUAL = "EQUAL"                  # ==
    NOT_EQUAL = "NOT_EQUAL"          # !=
    LESS = "LESS"                    # <
    GREATER = "GREATER"              # >
    LESS_EQUAL = "LESS_EQUAL"        # <=
    GREATER_EQUAL = "GREATER_EQUAL"  # >=
    
    # Logical operators
    AND = "AND"                      # and, &&
    OR = "OR"                        # or, ||
    NOT = "NOT"                      # not, !
    
    # Bitwise operators
    BIT_AND = "BIT_AND"              # &
    BIT_OR = "BIT_OR"                # |
    BIT_XOR = "BIT_XOR"              # ^, xor
    BIT_NOT = "BIT_NOT"              # ~
    LEFT_SHIFT = "LEFT_SHIFT"        # <<
    RIGHT_SHIFT = "RIGHT_SHIFT"      # >>
    BIT_AND_ASSIGN = "BIT_AND_ASSIGN"  # &=
    BIT_OR_ASSIGN = "BIT_OR_ASSIGN"    # |=
    BIT_XOR_ASSIGN = "BIT_XOR_ASSIGN"  # ^=
    LEFT_SHIFT_ASSIGN = "LEFT_SHIFT_ASSIGN"   # <<=
    RIGHT_SHIFT_ASSIGN = "RIGHT_SHIFT_ASSIGN" # >>=
    
    # Unary operators
    INCREMENT = "INCREMENT"          # ++
    DECREMENT = "DECREMENT"          # --
    ADDRESS_OF = "ADDRESS_OF"        # @
    DEREFERENCE = "DEREFERENCE"      # * (context dependent)
    
    # Special operators
    ARROW = "ARROW"                  # ->
    SCOPE = "SCOPE"                  # ::
    DOT = "DOT"                      # .
    QUESTION = "QUESTION"            # ?
    COLON = "COLON"                  # :
    IN = "IN"                        # in
    IS = "IS"                        # is
    AS = "AS"                        # as
    
    # Delimiters
    LPAREN = "LPAREN"                # (
    RPAREN = "RPAREN"                # )
    LBRACKET = "LBRACKET"            # [
    RBRACKET = "RBRACKET"            # ]
    LBRACE = "LBRACE"                # {
    RBRACE = "RBRACE"                # }
    SEMICOLON = "SEMICOLON"          # ;
    COMMA = "COMMA"                  # ,
    
    # Special strings
    I_STRING = "I_STRING"            # i"text":{expr;}
    
    # Special tokens
    NEWLINE = "NEWLINE"
    EOF = "EOF"
    COMMENT = "COMMENT"
    
    # Assembly
    ASM_BLOCK = "ASM_BLOCK"

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int
    position: int

class LexerError(Exception):
    def __init__(self, message: str, line: int, column: int):
        self.message = message
        self.line = line
        self.column = column
        super().__init__(f"Lexer error at line {line}, column {column}: {message}")

class FluxLexer:
    # Flux keywords - EXACT list from specification in alphabetical order
    KEYWORDS = {
        'and', 'array', 'as', 'asm', 'assert', 'auto', 'break', 'case', 'catch', 'const', 
        'continue', 'data', 'def', 'default', 'do', 'else', 'enum', 'for', 'if', 
        'import', 'in', 'is', 'namespace', 'not', 'object', 'operator', 
        'or', 'return', 'signed', 'sizeof', 'struct', 'super', 'switch', 'template', 
        'this', 'throw', 'try', 'typeof', 'unsigned', 'using', 'void', 'volatile', 
        'while', 'xor'
    }
    
    # Multi-character operators (order matters - longer first)
    OPERATORS = [
        ('^^=', TokenType.POWER_ASSIGN),
        ('>>=', TokenType.RIGHT_SHIFT_ASSIGN),
        ('<<=', TokenType.LEFT_SHIFT_ASSIGN),
        ('->', TokenType.ARROW),
        ('::', TokenType.SCOPE),
        ('^^', TokenType.POWER),
        ('++', TokenType.INCREMENT),
        ('--', TokenType.DECREMENT),
        ('+=', TokenType.PLUS_ASSIGN),
        ('-=', TokenType.MINUS_ASSIGN),
        ('*=', TokenType.MULT_ASSIGN),
        ('/=', TokenType.DIV_ASSIGN),
        ('%=', TokenType.MOD_ASSIGN),
        ('&=', TokenType.BIT_AND_ASSIGN),
        ('|=', TokenType.BIT_OR_ASSIGN),
        ('^=', TokenType.BIT_XOR_ASSIGN),
        ('==', TokenType.EQUAL),
        ('!=', TokenType.NOT_EQUAL),
        ('<=', TokenType.LESS_EQUAL),
        ('>=', TokenType.GREATER_EQUAL),
        ('<<', TokenType.LEFT_SHIFT),
        ('>>', TokenType.RIGHT_SHIFT),
        ('&&', TokenType.AND),
        ('||', TokenType.OR),
    ]
    
    # Single character operators
    SINGLE_OPERATORS = {
        '+': TokenType.PLUS,
        '-': TokenType.MINUS,
        '*': TokenType.MULTIPLY,
        '/': TokenType.DIVIDE,
        '%': TokenType.MODULO,
        '=': TokenType.ASSIGN,
        '<': TokenType.LESS,
        '>': TokenType.GREATER,
        '&': TokenType.BIT_AND,
        '|': TokenType.BIT_OR,
        '^': TokenType.BIT_XOR,
        '~': TokenType.BIT_NOT,
        '!': TokenType.NOT,
        '@': TokenType.ADDRESS_OF,
        '?': TokenType.QUESTION,
        ':': TokenType.COLON,
        '.': TokenType.DOT,
    }
    
    # Delimiters
    DELIMITERS = {
        '(': TokenType.LPAREN,
        ')': TokenType.RPAREN,
        '[': TokenType.LBRACKET,
        ']': TokenType.RBRACKET,
        '{': TokenType.LBRACE,
        '}': TokenType.RBRACE,
        ';': TokenType.SEMICOLON,
        ',': TokenType.COMMA,
    }
    
    def __init__(self, text: str):
        self.text = text
        self.position = 0
        self.line = 1
        self.column = 1
        self.tokens: List[Token] = []
    
    def current_char(self) -> Optional[str]:
        if self.position >= len(self.text):
            return None
        return self.text[self.position]
    
    def peek_char(self, offset: int = 1) -> Optional[str]:
        peek_pos = self.position + offset
        if peek_pos >= len(self.text):
            return None
        return self.text[peek_pos]
    
    def advance(self) -> Optional[str]:
        if self.position >= len(self.text):
            return None
        
        char = self.text[self.position]
        self.position += 1
        
        if char == '\n':
            self.line += 1
            self.column = 1
        else:
            self.column += 1
            
        return char
    
    def skip_whitespace(self):
        """Skip whitespace characters (except newlines)"""
        while self.current_char() and self.current_char() in ' \t\r':
            self.advance()
    
    def read_number(self) -> Token:
        """Read integer or float literals"""
        start_pos = self.position
        start_col = self.column
        number_str = ""
        
        # Handle hex literals (0x...)
        if self.current_char() == '0' and self.peek_char() == 'x':
            number_str += self.advance()  # '0'
            number_str += self.advance()  # 'x'
            
            while self.current_char() and self.current_char() in '0123456789ABCDEFabcdef':
                number_str += self.advance()
                
            return Token(TokenType.INTEGER, number_str, self.line, start_col, start_pos)
        
        # Handle binary literals (0b...)
        if self.current_char() == '0' and self.peek_char() == 'b':
            number_str += self.advance()  # '0'
            number_str += self.advance()  # 'b'
            
            while self.current_char() and self.current_char() in '01':
                number_str += self.advance()
                
            return Token(TokenType.INTEGER, number_str, self.line, start_col, start_pos)
        
        # Handle decimal numbers
        while self.current_char() and self.current_char().isdigit():
            number_str += self.advance()
        
        # Check for float (has decimal point)
        if self.current_char() == '.':
            # Check it's not another operator starting with dot
            if self.peek_char() and self.peek_char().isdigit():
                number_str += self.advance()  # Add the '.'
                
                while self.current_char() and self.current_char().isdigit():
                    number_str += self.advance()
                
                # Handle scientific notation
                if self.current_char() and self.current_char().lower() == 'e':
                    number_str += self.advance()
                    if self.current_char() and self.current_char() in '+-':
                        number_str += self.advance()
                    while self.current_char() and self.current_char().isdigit():
                        number_str += self.advance()
                
                return Token(TokenType.FLOAT, number_str, self.line, start_col, start_pos)
        
        # Handle scientific notation for integers
        if self.current_char() and self.current_char().lower() == 'e':
            number_str += self.advance()
            if self.current_char() and self.current_char() in '+-':
                number_str += self.advance()
            while self.current_char() and self.current_char().isdigit():
                number_str += self.advance()
            return Token(TokenType.FLOAT, number_str, self.line, start_col, start_pos)
        
        return Token(TokenType.INTEGER, number_str, self.line, start_col, start_pos)
    
    def read_string(self, quote_char: str = '"') -> Token:
        """Read string literals"""
        start_pos = self.position
        start_col = self.column
        string_content = ""
        
        self.advance()  # Skip opening quote
        
        while self.current_char() and self.current_char() != quote_char:
            if self.current_char() == '\\':
                string_content += self.advance()  # Add backslash
                if self.current_char():  # Add escaped character
                    string_content += self.advance()
            else:
                string_content += self.advance()
        
        if not self.current_char():
            raise LexerError("Unterminated string literal", self.line, self.column)
        
        self.advance()  # Skip closing quote
        
        return Token(TokenType.STRING, string_content, self.line, start_col, start_pos)
    
    def read_i_string(self) -> Token:
        """Read interpolated strings: i"text":{expr1;expr2;}"""
        start_pos = self.position
        start_col = self.column
        
        self.advance()  # Skip 'i'
        
        # Read the string part
        if self.current_char() != '"':
            raise LexerError("Expected '\"' after 'i' in i-string", self.line, self.column)
        
        string_token = self.read_string('"')
        string_part = string_token.value
        
        # Expect ':'
        if self.current_char() != ':':
            raise LexerError("Expected ':' after string in i-string", self.line, self.column)
        self.advance()
        
        # Expect '{'
        if self.current_char() != '{':
            raise LexerError("Expected '{' after ':' in i-string", self.line, self.column)
        self.advance()
        
        # Read expressions until '}'
        expr_content = ""
        brace_count = 1
        
        while self.current_char() and brace_count > 0:
            if self.current_char() == '{':
                brace_count += 1
            elif self.current_char() == '}':
                brace_count -= 1
            
            if brace_count > 0:
                expr_content += self.current_char()
            
            self.advance()
        
        if brace_count > 0:
            raise LexerError("Unterminated i-string expression block", self.line, self.column)
        
        # Return the full i-string as a single token
        full_content = f'i"{string_part}":{{{expr_content}}}'
        return Token(TokenType.I_STRING, full_content, self.line, start_col, start_pos)
    
    def read_binary_literal(self) -> Token:
        """Read binary literals: {0,1,0,1} - ONLY 0s and 1s allowed"""
        start_pos = self.position
        start_col = self.column
        
        self.advance()  # Skip '{'
        
        binary_content = ""
        found_binary_digits = False
        
        while self.current_char() and self.current_char() != '}':
            if self.current_char() in '01':
                binary_content += self.current_char()
                found_binary_digits = True
            elif self.current_char() in ' \t\n\r,':
                # Skip whitespace and commas - don't add to content
                pass
            else:
                # Invalid character for binary literal
                raise LexerError(f"Invalid character in binary literal: {self.current_char()}", 
                               self.line, self.column)
            self.advance()
        
        if not self.current_char():
            raise LexerError("Unterminated binary literal", self.line, self.column)
        
        if not found_binary_digits:
            raise LexerError("Binary literal must contain at least one binary digit (0 or 1)", 
                           self.line, self.column)
        
        self.advance()  # Skip '}'
        
        return Token(TokenType.BINARY, binary_content, self.line, start_col, start_pos)
    
    def is_binary_literal_content(self, start_pos: int) -> bool:
        """Check if content inside braces is a binary literal"""
        pos = start_pos + 1  # Skip opening '{'
        found_binary_digit = False
        
        while pos < len(self.text) and self.text[pos] != '}':
            if self.text[pos] in '01':
                found_binary_digit = True
            elif self.text[pos] not in ' \t\n\r,':
                return False  # Found non-binary character
            pos += 1
        
        return found_binary_digit and pos < len(self.text)  # Must have binary digits and closing brace
    
    def read_identifier(self) -> Token:
        """Read identifiers and keywords"""
        start_pos = self.position
        start_col = self.column
        identifier = ""
        
        # First character must be letter or underscore
        if self.current_char() and (self.current_char().isalpha() or self.current_char() == '_'):
            identifier += self.advance()
        
        # Subsequent characters can be letters, digits, or underscores
        while self.current_char() and (self.current_char().isalnum() or self.current_char() == '_'):
            identifier += self.advance()
        
        # Check if it's a keyword
        token_type = TokenType.KEYWORD if identifier in self.KEYWORDS else TokenType.IDENTIFIER
        
        return Token(token_type, identifier, self.line, start_col, start_pos)
    
    def read_comment(self) -> Token:
        """Read single-line comments"""
        start_pos = self.position
        start_col = self.column
        comment = ""
        
        # Skip '//'
        self.advance()
        self.advance()
        
        # Read until end of line
        while self.current_char() and self.current_char() != '\n':
            comment += self.advance()
        
        return Token(TokenType.COMMENT, comment, self.line, start_col, start_pos)
    
    def read_asm_block(self) -> Token:
        """Read inline assembly blocks: asm { ... }"""
        start_pos = self.position
        start_col = self.column
        
        # Skip 'asm'
        for _ in range(3):
            self.advance()
        
        self.skip_whitespace()
        
        if self.current_char() != '{':
            raise LexerError("Expected '{' after 'asm'", self.line, self.column)
        
        self.advance()  # Skip '{'
        
        asm_content = ""
        brace_count = 1
        
        while self.current_char() and brace_count > 0:
            if self.current_char() == '{':
                brace_count += 1
            elif self.current_char() == '}':
                brace_count -= 1
            
            if brace_count > 0:
                asm_content += self.current_char()
            
            self.advance()
        
        if brace_count > 0:
            raise LexerError("Unterminated asm block", self.line, self.column)
        
        return Token(TokenType.ASM_BLOCK, asm_content.strip(), self.line, start_col, start_pos)
    
    def tokenize(self) -> List[Token]:
        """Main tokenization method"""
        while self.position < len(self.text):
            char = self.current_char()
            
            if char is None:
                break
            
            # Skip whitespace
            if char in ' \t\r':
                self.skip_whitespace()
                continue
            
            # Handle newlines
            if char == '\n':
                token = Token(TokenType.NEWLINE, char, self.line, self.column, self.position)
                self.tokens.append(token)
                self.advance()
                continue
            
            # Handle comments
            if char == '/' and self.peek_char() == '/':
                self.tokens.append(self.read_comment())
                continue
            
            # Handle numbers
            if char.isdigit():
                self.tokens.append(self.read_number())
                continue
            
            # Handle strings
            if char == '"':
                self.tokens.append(self.read_string())
                continue
            
            # Handle i-strings
            if char == 'i' and self.peek_char() == '"':
                self.tokens.append(self.read_i_string())
                continue
            
            # Handle binary literals vs regular braces
            if char == '{':
                if self.is_binary_literal_content(self.position):
                    self.tokens.append(self.read_binary_literal())
                    continue
            
            # Handle multi-character operators
            found_operator = False
            for op_str, token_type in self.OPERATORS:
                if self.text[self.position:self.position + len(op_str)] == op_str:
                    token = Token(token_type, op_str, self.line, self.column, self.position)
                    self.tokens.append(token)
                    for _ in range(len(op_str)):
                        self.advance()
                    found_operator = True
                    break
            
            if found_operator:
                continue
            
            # Handle single character operators
            if char in self.SINGLE_OPERATORS:
                token_type = self.SINGLE_OPERATORS[char]
                token = Token(token_type, char, self.line, self.column, self.position)
                self.tokens.append(token)
                self.advance()
                continue
            
            # Handle delimiters
            if char in self.DELIMITERS:
                token_type = self.DELIMITERS[char]
                token = Token(token_type, char, self.line, self.column, self.position)
                self.tokens.append(token)
                self.advance()
                continue
            
            # Handle identifiers and keywords
            if char.isalpha() or char == '_':
                # Special case for 'asm' keyword - check if it's an asm block
                if self.text[self.position:self.position + 3] == 'asm':
                    # Look ahead to see if this is an asm block
                    saved_pos = self.position
                    saved_line = self.line
                    saved_col = self.column
                    
                    # Skip 'asm'
                    for _ in range(3):
                        self.advance()
                    
                    self.skip_whitespace()
                    
                    if self.current_char() == '{':
                        # It's an asm block, restore and read it
                        self.position = saved_pos
                        self.line = saved_line
                        self.column = saved_col
                        self.tokens.append(self.read_asm_block())
                        continue
                    else:
                        # It's just the 'asm' keyword, restore and read normally
                        self.position = saved_pos
                        self.line = saved_line
                        self.column = saved_col
                
                self.tokens.append(self.read_identifier())
                continue
            
            # If we get here, it's an unexpected character
            raise LexerError(f"Unexpected character: {char}", self.line, self.column)
        
        # Add EOF token
        self.tokens.append(Token(TokenType.EOF, "", self.line, self.column, self.position))
        return self.tokens
    
    def tokenize_filter_whitespace(self) -> List[Token]:
        """Tokenize and filter out whitespace and comments for parsing"""
        tokens = self.tokenize()
        return [token for token in tokens 
                if token.type not in (TokenType.COMMENT, TokenType.NEWLINE)]

# Command line interface
if __name__ == "__main__":
    import sys
    import argparse
    import os
    
    def main():
        parser = argparse.ArgumentParser(description='Flux Language Lexer')
        parser.add_argument('file', nargs='?', help='Flux source file to tokenize')
        parser.add_argument('--output', '-o', choices=['table', 'json', 'simple'], 
                          default='table', help='Output format (default: table)')
        parser.add_argument('--filter', '-f', action='store_true', 
                          help='Filter out whitespace, comments, and newlines')
        parser.add_argument('--tokens', '-t', nargs='*', 
                          help='Show only specific token types (e.g., KEYWORD IDENTIFIER)')
        
        args = parser.parse_args()
        
        # If no file provided, use test code
        if not args.file:
            print("No file provided. Using test code...\n")
            source_code = '''
import "standard.fx" as std;
using std::io, std::types;

def main() -> i32 {
    i32 x = 42;
    float y = 3.14;
    string msg = "Hello World!";
    
    print(i"Value is {}":{x;});
    
    i32[] arr = [1, 2, 3, 4];
    dict myDict = {"key": "value"};
    
    data{8} binary = {1,0,1,1,0,0,1,0};
    
    if (x > 0 && y < 10.0) {
        return x + (i32)y;
    }
    
    asm {
        mov eax, 1
        int 0x80
    };
    
    return 0;
}
            '''
        else:
            # Read file
            try:
                if not os.path.exists(args.file):
                    print(f"Error: File '{args.file}' not found.")
                    return 1
                
                with open(args.file, 'r', encoding='utf-8') as f:
                    source_code = f.read()
                    
                print(f"Tokenizing file: {args.file}")
                print(f"File size: {len(source_code)} characters\n")
                
            except IOError as e:
                print(f"Error reading file '{args.file}': {e}")
                return 1
            except UnicodeDecodeError as e:
                print(f"Error decoding file '{args.file}': {e}")
                print("Make sure the file is valid UTF-8 encoded text.")
                return 1
        
        # Tokenize
        lexer = FluxLexer(source_code)
        try:
            if args.filter:
                tokens = lexer.tokenize_filter_whitespace()
            else:
                tokens = lexer.tokenize()
            
            # Filter by token types if specified
            if args.tokens:
                token_types = [getattr(TokenType, t.upper()) for t in args.tokens if hasattr(TokenType, t.upper())]
                tokens = [token for token in tokens if token.type in token_types]
            
            # Output results
            if args.output == 'table':
                print("Tokens:")
                print("-" * 70)
                print(f"{'TOKEN TYPE':<20} | {'VALUE':<20} | {'POSITION':<15}")
                print("-" * 70)
                for token in tokens:
                    value_display = repr(token.value) if len(token.value) > 15 else token.value
                    pos_info = f"L{token.line}:C{token.column}"
                    print(f"{token.type.name:<20} | {value_display:<20} | {pos_info:<15}")
                    
            elif args.output == 'simple':
                for token in tokens:
                    print(f"{token.type.name}: {repr(token.value)}")
                    
            elif args.output == 'json':
                import json
                token_data = []
                for token in tokens:
                    token_data.append({
                        'type': token.type.name,
                        'value': token.value,
                        'line': token.line,
                        'column': token.column,
                        'position': token.position
                    })
                print(json.dumps(token_data, indent=2))
            
            print(f"\nTotal tokens: {len(tokens)}")
            
            # Show token type summary
            if args.output == 'table':
                from collections import Counter
                token_counts = Counter(token.type.name for token in tokens)
                print(f"\nToken type summary:")
                for token_type, count in token_counts.most_common():
                    print(f"  {token_type}: {count}")
        
        except LexerError as e:
            print(f"Lexer Error: {e}")
            return 1
        
        return 0
    
    sys.exit(main())