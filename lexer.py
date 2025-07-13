# Flux Lexer



class Token:
	lexeme = None
	name = None
	row = 0
	col = 0

	def __init__(row: int, col: int, name: str, lexeme: str):
		self.row = row
		self.col = col
		self.name = name
		self.lexeme = lexeme
		return


class Lexer:
	position = 0
	row = 0
	col = 0
	tokens = []

	def __init__():
		return

    def current_char(self) -> Optional[str]:
        if self.position >= self.length:
            return None
        return self.source[self.position]
    
    def peek_char(self, offset: int = 1) -> Optional[str]:
        pos = self.position + offset
        if pos >= self.length:
            return None
        return self.source[pos]
    
    def advance(self, count: int = 1) -> None:
        if self.position < self.length and self.source[self.position] == '\n':
            self.line += 1
            self.column = count
        else:
            self.column += count
        self.position += count
    
    def skip_whitespace(self) -> None:
        while self.current_char() and self.current_char() in ' \t\r\n':
            self.advance()
    
    def skip_comment(self) -> None:
        if self.current_char() == '/' and self.peek_char() == '/':
            # Skip until end of line
            while self.current_char() and self.current_char() != '\n':
                self.advance()
    
    def read_string(self, quote_char: str) -> str:
        result = ""
        self.advance()  # Skip opening quote
        
        while self.current_char() and self.current_char() != quote_char:
            if self.current_char() == '\\':
                self.advance()
                escape_char = self.current_char()
                if escape_char in 'ntr\\''"':
                    escape_map = {'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', "'": "'", '"': '"'}
                    result += escape_map.get(escape_char, escape_char)
                elif escape_char == 'x':
                    # Hex escape
                    self.advance()
                    hex_digits = ""
                    for _ in range(2):
                        if self.current_char() and self.current_char() in '0123456789abcdefABCDEF':
                            hex_digits += self.current_char()
                            self.advance()
                        else:
                            break
                    if hex_digits:
                        result += chr(int(hex_digits, 16))
                        continue
                elif escape_char and escape_char.isdigit():
                    # Octal escape
                    octal_digits = escape_char
                    self.advance()
                    for _ in range(2):
                        if self.current_char() and self.current_char() in '01234567':
                            octal_digits += self.current_char()
                            self.advance()
                        else:
                            break
                    result += chr(int(octal_digits, 8))
                    continue
                else:
                    result += escape_char if escape_char else '\\'
            else:
                result += self.current_char()
            self.advance()
        
        if self.current_char() == quote_char:
            self.advance()  # Skip closing quote
        
        return result
    
    def read_f_string(self) -> str:
        """Read f-string with embedded expressions"""
        result = ""
        self.advance()  # Skip opening quote
        
        while self.current_char() and self.current_char() != '"':
            if self.current_char() == '{':
                # Start of embedded expression
                result += self.current_char()
                self.advance()
                brace_count = 1
                
                while self.current_char() and brace_count > 0:
                    if self.current_char() == '{':
                        brace_count += 1
                    elif self.current_char() == '}':
                        brace_count -= 1
                    result += self.current_char()
                    self.advance()
            elif self.current_char() == '\\':
                self.advance()
                escape_char = self.current_char()
                if escape_char in 'ntr\\''"':
                    escape_map = {'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', "'": "'", '"': '"'}
                    result += escape_map.get(escape_char, escape_char)
                else:
                    result += escape_char if escape_char else '\\'
                self.advance()
            else:
                result += self.current_char()
                self.advance()
        
        if self.current_char() == '"':
            self.advance()  # Skip closing quote
        
        return result
    
    def read_number(self) -> Token:
        start_pos = (self.line, self.column)
        result = ""
        is_float = False
        
        # Handle hex (0x) and binary (0b) prefixes
        if self.current_char() == '0':
            result += self.current_char()
            self.advance()
            
            if self.current_char() and self.current_char().lower() == 'x':
                # Hexadecimal
                result += self.current_char()
                self.advance()
                while self.current_char() and self.current_char() in '0123456789abcdefABCDEF':
                    result += self.current_char()
                    self.advance()
                return Token(TokenType.INTEGER, result, start_pos[0], start_pos[1])
            
            elif self.current_char() and self.current_char().lower() == 'b':
                # Binary
                result += self.current_char()
                self.advance()
                while self.current_char() and self.current_char() in '01':
                    result += self.current_char()
                    self.advance()
                return Token(TokenType.INTEGER, result, start_pos[0], start_pos[1])
        
        # Read decimal digits
        while self.current_char() and self.current_char().isdigit():
            result += self.current_char()
            self.advance()
        
        # Check for decimal point
        if self.current_char() == '.' and self.peek_char() and self.peek_char().isdigit():
            is_float = True
            result += self.current_char()
            self.advance()
            while self.current_char() and self.current_char().isdigit():
                result += self.current_char()
                self.advance()
        

        
        token_type = TokenType.FLOAT if is_float else TokenType.INTEGER
        return Token(token_type, result, start_pos[0], start_pos[1])
    
    def read_identifier(self) -> Token:
        start_pos = (self.line, self.column)
        result = ""
        
        while (self.current_char() and 
               (self.current_char().isalnum() or self.current_char() == '_')):
            result += self.current_char()
            self.advance()
        
        # Check if it's a keyword
        token_type = self.keywords.get(result, TokenType.IDENTIFIER)
        
        # Special handling for boolean literals
        if result == 'true' or result == 'false':
            token_type = TokenType.BOOLEAN
        
        return Token(token_type, result, start_pos[0], start_pos[1])
    
    def read_interpolation_string(self) -> Token:
        """Read i-string format: i"string": {expr1; expr2; ...}"""
        start_pos = (self.line, self.column)
        
        # Read the string part
        string_part = self.read_string('"')
        
        # Skip whitespace and colon
        self.skip_whitespace()
        if self.current_char() == ':':
            self.advance()
        
        # Read the interpolation block
        self.skip_whitespace()
        if self.current_char() == '{':
            interpolation_part = ""
            brace_count = 1
            interpolation_part += self.current_char()
            self.advance()
            
            while self.current_char() and brace_count > 0:
                if self.current_char() == '{':
                    brace_count += 1
                elif self.current_char() == '}':
                    brace_count -= 1
                interpolation_part += self.current_char()
                self.advance()
            
            result = f'i"{string_part}":{interpolation_part}'
        else:
            result = f'i"{string_part}"'
        
        return Token(TokenType.I_STRING, result, start_pos[0], start_pos[1])
    
    def tokenize(self) -> List[Token]:
        tokens = []
        
        while self.position < self.length:
            # Skip whitespace
            if self.current_char() and self.current_char() in ' \t\r\n':
                self.skip_whitespace()
                continue
            
            # Skip comments
            if self.current_char() == '/' and self.peek_char() == '/':
                self.skip_comment()
                continue
            
            start_pos = (self.line, self.column)
            char = self.current_char()
            
            if not char:
                break
            
            # String interpolation
            if char == 'i' and self.peek_char() == '"':
                self.advance()  # Skip 'i'
                tokens.append(self.read_interpolation_string())
                continue
            
            if char == 'f' and self.peek_char() == '"':
                self.advance()  # Skip 'f'
                f_string_content = self.read_f_string()
                tokens.append(Token(TokenType.F_STRING, f'f"{f_string_content}"', start_pos[0], start_pos[1]))
                continue
            
            # String literals
            if char in '"\'':
                if char == '"':
                    content = self.read_string('"')
                    tokens.append(Token(TokenType.STRING, content, start_pos[0], start_pos[1]))
                else:
                    content = self.read_string("'")
                    tokens.append(Token(TokenType.CHAR, content, start_pos[0], start_pos[1]))
                continue
            
            # Numbers
            if char.isdigit():
                tokens.append(self.read_number())
                continue
            
            # Identifiers and keywords
            if char.isalpha() or char == '_':
                tokens.append(self.read_identifier())
                continue
            
            # Multi-character operators (order matters - longest first)
            if char == '<' and self.peek_char() == '<' and self.peek_char(2) == '=':
                tokens.append(Token(TokenType.LEFT_SHIFT_ASSIGN, '<<=', start_pos[0], start_pos[1]))
                self.advance(count=3)
                continue
            
            if char == '>' and self.peek_char() == '>' and self.peek_char(2) == '=':
                tokens.append(Token(TokenType.RIGHT_SHIFT_ASSIGN, '>>=', start_pos[0], start_pos[1]))
                self.advance(count=3)
                continue
            
            if char == '^' and self.peek_char() == '^' and self.peek_char(2) == '=':
                tokens.append(Token(TokenType.XOR_ASSIGN, '^^=', start_pos[0], start_pos[1]))
                self.advance(count=3)
                continue
            
            # Bitwise operators with backtick prefix
            if char == '`':
                next_char = self.peek_char()
                third_char = self.peek_char(2)
                fourth_char = self.peek_char(3)
                
                # 4-character bitwise operators
                if next_char == '^' and third_char == '!' and fourth_char in '&|':
                    if fourth_char == '&':
                        if self.peek_char(4) == '=':
                            tokens.append(Token(TokenType.B_NOT_ASSIGN, '`^!&=', start_pos[0], start_pos[1]))
                            for _ in range(5): self.advance()
                        else:
                            tokens.append(Token(TokenType.BITWISE_B_XNAND, '`^!&', start_pos[0], start_pos[1]))
                            for _ in range(4): self.advance()
                    else:  # fourth_char == '|'
                        if self.peek_char(4) == '=':
                            tokens.append(Token(TokenType.B_NOT_ASSIGN, '`^!|=', start_pos[0], start_pos[1]))
                            for _ in range(5): self.advance()
                        else:
                            tokens.append(Token(TokenType.BITWISE_B_XNOR, '`^!|', start_pos[0], start_pos[1]))
                            for _ in range(4): self.advance()
                    continue
                
                # 3-character bitwise operators
                if next_char == '!' and third_char in '&|':
                    if third_char == '&':
                        if self.peek_char(3) == '=':
                            tokens.append(Token(TokenType.B_NAND_ASSIGN, '`!&=', start_pos[0], start_pos[1]))
                            for _ in range(4): self.advance()
                        else:
                            tokens.append(Token(TokenType.BITWISE_B_NAND, '`!&', start_pos[0], start_pos[1]))
                            for _ in range(3): self.advance()
                    else:  # third_char == '|'
                        if self.peek_char(3) == '=':
                            tokens.append(Token(TokenType.B_NOR_ASSIGN, '`!|=', start_pos[0], start_pos[1]))
                            for _ in range(4): self.advance()
                        else:
                            tokens.append(Token(TokenType.BITWISE_B_NOR, '`!|', start_pos[0], start_pos[1]))
                            for _ in range(3): self.advance()
                    continue
                
                if next_char == '^' and third_char == '&':
                    if self.peek_char(3) == '=':
                        tokens.append(Token(TokenType.B_AND_ASSIGN, '`^&=', start_pos[0], start_pos[1]))
                        for _ in range(4): self.advance()
                    else:
                        tokens.append(Token(TokenType.BITWISE_B_XAND, '`^&', start_pos[0], start_pos[1]))
                        for _ in range(3): self.advance()
                    continue
                
                if next_char == '^' and third_char == '^':
                    if self.peek_char(3) == '=':
                        tokens.append(Token(TokenType.B_XOR_ASSIGN, '`^=', start_pos[0], start_pos[1]))
                        for _ in range(4): self.advance()
                    else:
                        tokens.append(Token(TokenType.BITWISE_B_XOR, '`^^', start_pos[0], start_pos[1]))
                        for _ in range(3): self.advance()
                    continue
                
                # 2-character bitwise operators
                if next_char == '&':
                    if third_char == '=':
                        tokens.append(Token(TokenType.B_AND_ASSIGN, '`&=', start_pos[0], start_pos[1]))
                        for _ in range(3): self.advance()
                    else:
                        tokens.append(Token(TokenType.BITWISE_B_AND, '`&', start_pos[0], start_pos[1]))
                        self.advance(count=2)
                    continue
                
                if next_char == '|':
                    if third_char == '=':
                        tokens.append(Token(TokenType.B_OR_ASSIGN, '`|=', start_pos[0], start_pos[1]))
                        for _ in range(3): self.advance()
                    else:
                        tokens.append(Token(TokenType.BITWISE_B_OR, '`|', start_pos[0], start_pos[1]))
                        self.advance(count=2)
                    continue
                
                if next_char == '!':
                    if third_char == '=':
                        tokens.append(Token(TokenType.B_NOT_ASSIGN, '`!=', start_pos[0], start_pos[1]))
                        for _ in range(3): self.advance()
                    continue
            
            # Two-character operators
            if char == '=' and self.peek_char() == '=':
                tokens.append(Token(TokenType.EQUAL, '==', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '!' and self.peek_char() == '=':
                tokens.append(Token(TokenType.NOT_EQUAL, '!=', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '<' and self.peek_char() == '=':
                tokens.append(Token(TokenType.LESS_EQUAL, '<=', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '>' and self.peek_char() == '=':
                tokens.append(Token(TokenType.GREATER_EQUAL, '>=', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '<' and self.peek_char() == '<':
                tokens.append(Token(TokenType.LEFT_SHIFT, '<<', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '>' and self.peek_char() == '>':
                tokens.append(Token(TokenType.RIGHT_SHIFT, '>>', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '&' and self.peek_char() == '&':
                tokens.append(Token(TokenType.LOGICAL_AND, '&&', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '|' and self.peek_char() == '|':
                tokens.append(Token(TokenType.LOGICAL_OR, '||', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '!' and self.peek_char() == '&':
                tokens.append(Token(TokenType.LOGICAL_NAND, '!&', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '!' and self.peek_char() == '|':
                tokens.append(Token(TokenType.LOGICAL_NOR, '!|', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '^' and self.peek_char() == '^':
                tokens.append(Token(TokenType.BITWISE_XOR, '^^', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '+' and self.peek_char() == '+':
                tokens.append(Token(TokenType.INCREMENT, '++', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '-' and self.peek_char() == '-':
                tokens.append(Token(TokenType.DECREMENT, '--', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '+' and self.peek_char() == '=':
                tokens.append(Token(TokenType.PLUS_ASSIGN, '+=', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '-' and self.peek_char() == '=':
                tokens.append(Token(TokenType.MINUS_ASSIGN, '-=', start_pos[0], start_pos[1]))
                self.advance(count=2)
                continue
            
            if char == '*' and self.peek_char() == '=':
                tokens.append(Token(TokenType.MULTIPLY_ASSIGN, '*=', start_pos[0], start_pos[1]))
                self.advance()
                continue
            
            if char == '/' and self.peek_char() == '=':
                tokens.append(Token(TokenType.DIVIDE_ASSIGN, '/=', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == '%' and self.peek_char() == '=':
                tokens.append(Token(TokenType.MODULO_ASSIGN, '%=', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == '^' and self.peek_char() == '=':
                tokens.append(Token(TokenType.POWER_ASSIGN, '^=', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == '&' and self.peek_char() == '=':
                tokens.append(Token(TokenType.AND_ASSIGN, '&=', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == '|' and self.peek_char() == '=':
                tokens.append(Token(TokenType.OR_ASSIGN, '|=', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == '.' and self.peek_char() == '.':
                tokens.append(Token(TokenType.RANGE, '..', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            if char == ':' and self.peek_char() == ':':
                tokens.append(Token(TokenType.SCOPE, '::', start_pos[0], start_pos[1]))
                self.advance()
                self.advance()
                continue
            
            # Single-character operators and delimiters
            single_char_tokens = {
                '+': TokenType.PLUS,
                '-': TokenType.MINUS,
                '*': TokenType.MULTIPLY,
                '/': TokenType.DIVIDE,
                '%': TokenType.MODULO,
                '^': TokenType.POWER,
                '<': TokenType.LESS_THAN,
                '>': TokenType.GREATER_THAN,
                '&': TokenType.BITWISE_AND,
                '|': TokenType.BITWISE_OR,
                '!': TokenType.LOGICAL_NOT,
                '~': TokenType.BITWISE_NOT,
                '@': TokenType.ADDRESS_OF,
                '=': TokenType.ASSIGN,
                '?': TokenType.QUESTION,
                ':': TokenType.COLON,
                '(': TokenType.LEFT_PAREN,
                ')': TokenType.RIGHT_PAREN,
                '[': TokenType.LEFT_BRACKET,
                ']': TokenType.RIGHT_BRACKET,
                '{': TokenType.LEFT_BRACE,
                '}': TokenType.RIGHT_BRACE,
                ';': TokenType.SEMICOLON,
                ',': TokenType.COMMA,
                '.': TokenType.DOT,
            }
            
            if char in single_char_tokens:
                tokens.append(Token(single_char_tokens[char], char, start_pos[0], start_pos[1]))
                self.advance()
                continue
            
            # Unknown character - skip it or raise error (depending on preference)
            self.advance()
        
        # Add EOF token
        tokens.append(Token(TokenType.EOF, '', self.line, self.column))
        return tokens