#!/usr/bin/env python3
"""
Fixed Flux Lexer (flexer4.py)
Fixed ** tokenization issue - now properly handles pointer-to-pointer as two separate * tokens
"""

d_tokens = {0: "INT_LITERAL",
            1: "FLOAT_LITERAL",
            2: "HEX_LITERAL",
            3: "BIN_LITERAL",
            4: "OCT_LITERAL",
            5: "STRING_LITERAL",

            6: "IDENTIFIER",
            7: "KEYWORD",

            8: "ADDITION", # +
            9: "SUBTRACTION", # -
            10:"MULTIPLICATION", # *
            11:"DIVISION", # /
            12:"MODULO", # %
            13:"EXPONENTIATION", # ^

            14:"ASSIGNMENT", # =
            15:"ADDITION_ASSIGNMENT", # +=
            16:"SUBTRACTION_ASSIGNMENT", # -=
            17:"MULTIPLICATION_ASSIGNMENT", # *=
            18:"DIVISION_ASSIGNMENT", # /=
            19:"MODULO_ASSIGNMENT", # %=
            20:"EXPONENTIATION_ASSIGNMENT", # ^=

            21:"EQ", # =
            22:"NEQ", # !=
            23:"LT", # <
            24:"GT", # >
            25:"LTEQ", # <=
            26:"GTEQ", # >=

            27:"AND", # &&
            28:"NAND", # !&
            29:"OR", # ||
            30:"NOR", # !|
            31:"XOR", # ^^
            32:"NOT", # !
            33:"XNOR", # ^!|
            34:"XAND", # ^&
            35:"XNAND", # ^!&

            36:"BAND", # `&&
            37:"BNAND", # `!&
            38:"BOR", # `|
            39:"BNOR", # `!|
            40:"BXOR", # `^^
            41:"BXNOR", # `^!|
            42:"BXAND", # `^&
            43:"BXNAND", # `^!&

            44:"LSHIFT", # <<
            45:"RSHIFT", # >>

            46:"BAND_ASSIGNMENT", # `&&=
            47:"BNAND_ASSIGNMENT", # `!&=
            48:"BOR_ASSIGNMENT", # `|=
            49:"BNOR_ASSIGNMENT", # `!|=
            50:"BXOR_ASSIGNMENT", # `^=
            51:"BNOT_ASSIGNMENT", # `!=

            52:"LSHIFT_ASSIGNMENT", # <<=
            53:"RSHIFT_ASSIGNMENT", # >>=

            54:"INCREMENT", # ++
            55:"DECREMENT", # --

            56:"ADDRESS_OF", # @
            57:"DEREFERENCE", # *

            58:"RETURN_ARROW", # ->
            59:"SCOPE", # ::
            60:"DOT", # .
            61:"LPAREN", # (
            62:"RPAREN", # )
            63:"LBRACKET", # [
            64:"RBRACKET", # ]
            65:"LBRACE", # {
            66:"RBRACE", # }
            67:"SEMICOLON", # ;
            68:"COMMA",

            69:"IN",
            70:"IS",
            71:"AS",

            72:"I_STRING",

            73:"NEWLINE",
            74:"COMMENT",

            75:"ASM_BLOCK",

            76:"EOF",
            77:"QUESTION", # ?
            78:"COLON", # :
            79:"RANGE"
            }

def d_tokens_map(token: str) -> str:
	# Keywords
	if token == "and":
		return "KW_AND"
	if token == "as":
		return "KW_AS"
	if token == "asm":
		return "KW_ASM"
	if token == "assert":
		return "KW_ASSERT"
	if token == "async":
		return "KW_ASYNC"
	if token == "auto":
		return "KW_AUTO"
	if token == "await":
		return "KW_AWAIT"
	if token == "break":
		return "KW_BREAK"
	if token == "case":
		return "KW_CASE"
	if token == "catch":
		return "KW_CATCH"
	if token == "const":
		return "KW_CONST"
	if token == "continue":
		return "KW_CONTINUE"
	if token == "data":
		return "KW_DATA"
	if token == "def":
		return "KW_DEF"
	if token == "default":
		return "KW_DEFAULT"
	if token == "do":
		return "KW_DO"
	if token == "else":
		return "KW_ELSE"
	if token == "enum":
		return "KW_ENUM"
	if token == "false":
		return "KW_FALSE"
	if token == "float":
		return "KW_FLOAT"
	if token == "for":
		return "KW_FOR"
	if token == "if":
		return "KW_IF"
	if token == "import":
		return "KW_IMPORT"
	if token == "in":
		return "KW_IN"
	if token == "int":
		return "KW_INT"
	if token == "is":
		return "KW_IS"
	if token == "namespace":
		return "KW_NAMESPACE"
	if token == "nor":
		return "KW_NOR"
	if token == "not":
		return "KW_NOT"
	if token == "object":
		return "KW_OBJECT"
	if token == "operator":
		return "KW_OPERATOR"
	if token == "or":
		return "KW_OR"
	if token == "return":
		return "KW_RETURN"
	if token == "signed":
		return "KW_SIGNED"
	if token == "sizeof":
		return "KW_SIZEOF"
	if token == "struct":
		return "KW_STRUCT"
	if token == "super":
		return "KW_SUPER"
	if token == "switch":
		return "KW_SWITCH"
	if token == "template":
		return "KW_TEMPLATE"
	if token == "this":
		return "KW_THIS"
	if token == "throw":
		return "KW_THROW"
	if token == "true":
		return "KW_TRUE"
	if token == "try":
		return "KW_TRY"
	if token == "typeof":
		return "KW_TYPEOF"
	if token == "unsigned":
		return "KW_UNSIGNED"
	if token == "using":
		return "KW_USING"
	if token == "void":
		return "KW_VOID"
	if token == "volatile":
		return "KW_VOLATILE"
	if token == "while":
		return "KW_WHILE"
	if token == "xand":
		return "KW_XAND"
	if token == "xnand":
		return "KW_XNAND"
	if token == "xnor":
		return "KW_XNOR"
	if token == "xor":
		return "KW_XOR"
	
	# Operators
	if token == "+":
		return "ADDITION"
	if token == "-":
		return "SUBTRACTION"
	if token == "*":
		return "MULTIPLICATION"
	if token == "/":
		return "DIVISION"
	if token == "%":
		return "MODULO"
	if token == "^":
		return "EXPONENTIATION"
	if token == "=":
		return "ASSIGNMENT"
	if token == "+=":
		return "ADDITION_ASSIGNMENT"
	if token == "-=":
		return "SUBTRACTION_ASSIGNMENT"
	if token == "*=":
		return "MULTIPLICATION_ASSIGNMENT"
	if token == "/=":
		return "DIVISION_ASSIGNMENT"
	if token == "%=":
		return "MODULO_ASSIGNMENT"
	if token == "^=":
		return "EXPONENTIATION_ASSIGNMENT"
	if token == "==":
		return "EQ"
	if token == "!=":
		return "NEQ"
	if token == "<":
		return "LT"
	if token == ">":
		return "GT"
	if token == "<=":
		return "LTEQ"
	if token == ">=":
		return "GTEQ"
	if token == "&&":
		return "AND"
	if token == "!&":
		return "NAND"
	if token == "||":
		return "OR"
	if token == "!|":
		return "NOR"
	if token == "^^":
		return "XOR"
	if token == "!":
		return "NOT"
	if token == "^!|":
		return "XNOR"
	if token == "^&":
		return "XAND"
	if token == "^!&":
		return "XNAND"
	if token == "`&&":
		return "BAND"
	if token == "`!&":
		return "BNAND"
	if token == "`|":
		return "BOR"
	if token == "`!|":
		return "BNOR"
	if token == "`^^":
		return "BXOR"
	if token == "`^!|":
		return "BXNOR"
	if token == "`^&":
		return "BXAND"
	if token == "`^!&":
		return "BXNAND"
	if token == "<<":
		return "LSHIFT"
	if token == ">>":
		return "RSHIFT"
	if token == "`&&=":
		return "BAND_ASSIGNMENT"
	if token == "`!&=":
		return "BNAND_ASSIGNMENT"
	if token == "`|=":
		return "BOR_ASSIGNMENT"
	if token == "`!|=":
		return "BNOR_ASSIGNMENT"
	if token == "`^=":
		return "BXOR_ASSIGNMENT"
	if token == "`!=":
		return "BNOT_ASSIGNMENT"
	if token == "<<=":
		return "LSHIFT_ASSIGNMENT"
	if token == ">>=":
		return "RSHIFT_ASSIGNMENT"
	if token == "++":
		return "INCREMENT"
	if token == "--":
		return "DECREMENT"
	if token == "@":
		return "ADDRESS_OF"
	if token == "->":
		return "RETURN_ARROW"
	if token == ".":
		return "DOT"
	if token == "..":
		return "RANGE"
	if token == "?":
		return "QUESTION"
	
	# Delimiters
	if token == "(":
		return "DELIM_L_PARENTHESIS"
	if token == ")":
		return "DELIM_R_PARENTHESIS"
	if token == "[":
		return "DELIM_L_BRACKET"
	if token == "]":
		return "DELIM_R_BRACKET"
	if token == "{":
		return "DELIM_L_BRACE"
	if token == "}":
		return "DELIM_R_BRACE"
	if token == ";":
		return "DELIM_SEMICOLON"
	if token == ":":
		return "DELIM_COLON"
	if token == "::":
		return "DELIM_SCOPE"
	if token == ",":
		return "DELIM_COMMA"
	
	# Literals and special tokens
	if token == "INTEGER_LITERAL":
		return "INT_LITERAL"
	if token == "FLOAT_LITERAL":
		return "FLOAT_LITERAL"
	if token == "BINARY_LITERAL":
		return "BIN_LITERAL"
	if token == "OCTAL_LITERAL":
		return "OCT_LITERAL"
	if token == "HEXADECIMAL_LITERAL":
		return "HEX_LITERAL"
	if token == "STRING_LITERAL":
		return "STRING_LITERAL"
	if token == "I_STRING":
		return "I_STRING"
	if token == "EOF":
		return "== END OF FILE =="
	
	return f"IDENTIFIER_{token}"


keywords = ['and', 'as', 'asm', 'assert', 'auto', 'break', 'case', 'catch', 'const',
            'continue', 'data', 'def', 'default', 'do', 'false', 'else', 'enum', 'for', 'if',
            'import', 'in', 'is', 'namespace', 'nor', 'not', 'object', 'operator', 'or',
            'return', 'signed', 'sizeof', 'struct', 'super', 'switch', 'template', 'this',
            'throw', 'true', 'try', 'typeof', 'unsigned', 'using', 'void', 'volatile',
            'while', 'xand', 'xnand', 'xnor', 'xor']

# FIXED: Multi-character operators in order of longest first to avoid greedy conflicts
multi_char_operators = [
    # 4+ character operators
    '`&&=', '`!&=', '`|=', '`!|=', '`^=', '`!=',
    '<<=', '>>=',
    # 3 character operators  
    '^!|', '^!&', '`&&', '`!&', '`^^', '`^!|', '`^&', '`^!&',
    # 2 character operators
    '+=', '-=', '*=', '/=', '%=', '^=', '==', '!=', '<=', '>=', 
    '&&', '!&', '||', '!|', '^^', '<<', '>>', '++', '--', '->', 
    '::', '..', '`|', '`!|',
]

single_char_operators = ['+', '-', '*', '/', '%', '^', '=', '!', '<', '>', '&', '|', '@', '.', '?', ':']

op_chars = ['+','-','*','/','%','^','=','!','<','>','&','|','`',':','@','.']

delimiters = ['(',')','[',']','{','}','?',':',';',',','.']

def get_token_index(token_str: str) -> int:
    """Map a string to its corresponding token index"""
    token_map = {
        # Operators
        '+': 8,      # ADDITION
        '-': 9,      # SUBTRACTION
        '*': 10,     # MULTIPLICATION
        '/': 11,     # DIVISION
        '%': 12,     # MODULO
        '^': 13,     # EXPONENTIATION
        
        '=': 14,     # ASSIGNMENT
        '+=': 15,    # ADDITION_ASSIGNMENT
        '-=': 16,    # SUBTRACTION_ASSIGNMENT
        '*=': 17,    # MULTIPLICATION_ASSIGNMENT
        '/=': 18,    # DIVISION_ASSIGNMENT
        '%=': 19,    # MODULO_ASSIGNMENT
        '^=': 20,    # EXPONENTIATION_ASSIGNMENT
        
        '==': 21,    # EQ
        '!=': 22,    # NEQ
        '<': 23,     # LT
        '>': 24,     # GT
        '<=': 25,    # LTEQ
        '>=': 26,    # GTEQ
        
        '&&': 27,    # AND
        '!&': 28,    # NAND
        '||': 29,    # OR
        '!|': 30,    # NOR
        '^^': 31,    # XOR
        '!': 32,     # NOT
        '^!|': 33,   # XNOR
        '^&': 34,    # XAND
        '^!&': 35,   # XNAND
        
        '`&&': 36,   # BAND
        '`!&': 37,   # BNAND
        '`|': 38,    # BOR
        '`!|': 39,   # BNOR
        '`^^': 40,   # BXOR
        '`^!|': 41,  # BXNOR
        '`^&': 42,   # BXAND
        '`^!&': 43,  # BXNAND
        
        '<<': 44,    # LSHIFT
        '>>': 45,    # RSHIFT
        
        '`&&=': 46,  # BAND_ASSIGNMENT
        '`!&=': 47,  # BNAND_ASSIGNMENT
        '`|=': 48,   # BOR_ASSIGNMENT
        '`!|=': 49,  # BNOR_ASSIGNMENT
        '`^=': 50,   # BXOR_ASSIGNMENT
        '`!=': 51,   # BNOT_ASSIGNMENT
        
        '<<=': 52,   # LSHIFT_ASSIGNMENT
        '>>=': 53,   # RSHIFT_ASSIGNMENT
        
        '++': 54,    # INCREMENT
        '--': 55,    # DECREMENT
        
        '@': 56,     # ADDRESS_OF
        # '*': 57,   # DEREFERENCE (same as MULTIPLICATION, context-dependent)
        
        '->': 58,    # RETURN_ARROW
        '::': 59,    # SCOPE
        '.': 60,     # DOT
        '..': 79,    # RANGE
        '?': 77,     # QUESTION
        ':': 78,     # COLON
        
        # Delimiters
        '(': 61,     # LPAREN
        ')': 62,     # RPAREN
        '[': 63,     # LBRACKET
        ']': 64,     # RBRACKET
        '{': 65,     # LBRACE
        '}': 66,     # RBRACE
        ';': 67,     # SEMICOLON
        ',': 68,     # COMMA
        
        # Special keywords that aren't handled in read_identifier
        'in': 69,    # IN
        'is': 70,    # IS
        'as': 71,    # AS
    }
    
    return token_map.get(token_str, 0)  # Return 0 (INT_LITERAL) as fallback

token_list = []

class Token:

	def __init__(self, t_index: int, arg: str):
		self.t_type = d_tokens[t_index]
		self.token = d_tokens_map(arg)  # Maps `object` to "KW_OBJECT" or `switch` to "KW_SWITCH"
		token_list.append(self)

	def __str__(self) -> str:
		return self.t_type

	def name(self) -> str:
		return self.name


class Lexer:

	def __init__(self, source: str):
		self.source = source
		self.position = 0
		self.line = 1
		self.col = 1
		self.token = None
		self.tokens = []
		self.stop = False

	def skip_whitespace(self):
		while self.current_char() and self.current_char() in ' \t\r':
			self.advance()
		while self.current_char() and self.current_char() in '\n':
			self.advance()

	def skip_comment(self):
		self.advance()       # Skip the first two slashes
		self.advance()
		while self.current_char() and self.current_char() != '\n':
			self.advance()
		return

	def current_char(self) -> str:
		if self.position >= len(self.source):
			return None
		return self.source[self.position]

	def next_char(self) -> str:
		peek_pos = self.position + 1
		if peek_pos > len(self.source) - 1:
			self.stop = True
			return None
		return self.source[peek_pos]

	def advance(self):
		if self.position < len(self.source) - 1:
			char = self.current_char()
			self.position += 1
			if char == "\n":
				self.line += 1
				self.col = 1
			else:
				self.col += 1
			return char
		else:
			self.stop = True
			return None

	def x_advance(self, x: int):
		if self.position + x < len(self.source) - 1:
			char = ""
			for s in self.source[self.position:self.position + x]:
				if s == "\n":
					self.line += 1
					self.col += 1
				else:
					self.col += 1
				char += s
			return char
		else:
			self.stop = True
			return None

	def read_number(self) -> str:
		number = ""

		# Hexadecimal literals
		if self.current_char() == "0" and self.next_char() == "x":
			number += self.advance()
			number += self.advance()
			while self.current_char() and self.current_char() in '0123456789ABCDEFabcdef':
				number += self.advance()
			token = Token(2, "HEXADECIMAL_LITERAL")
			self.tokens.append(str(token))
			return number
		
		# Octal literals
		if self.current_char() == "0" and self.next_char() == "o":
			number += self.advance()
			number += self.advance()
			while self.current_char() and self.current_char() in '01234567':
				number += self.advance()
			token = Token(4, "OCTAL_LITERAL")
			self.tokens.append(str(token))
			return number

		# Binary literals
		if self.current_char() == "0" and self.next_char() == "b":
			number += self.advance()
			number += self.advance()
			while self.current_char() and self.current_char() in '01':
				number += self.advance()
			token = Token(3, "BINARY_LITERAL")
			self.tokens.append(str(token))
			return number

		# Decimal literals
		while self.current_char() and self.current_char().isdigit():
			number += self.advance()

		# Float literals
		if self.current_char() == '.':
			# Check it's not another operator starting with dot
			if self.next_char() and self.next_char().isdigit():
				number += self.advance()  # Add the '.'
				while self.current_char() and self.current_char().isdigit():
					number += self.advance()
				token = Token(1, "FLOAT_LITERAL")
				self.tokens.append(str(token))
				return number

		# If we get here, it was just an integer
		token = Token(0, "INTEGER_LITERAL")
		self.tokens.append(str(token))
		return number

	def read_string(self) -> str:
		string = ""

		self.advance()  # Skip opening quote

		while self.current_char() is not None and self.current_char() not in "\'\"":
			char = self.current_char()
			if char in '\\':
				# Handle escape sequences
				string += char
				self.advance()
				if self.current_char() is not None:
					string += self.current_char()
					self.advance()
			else:
				string += char
				self.advance()

		if self.current_char() is None:
			print(f"Unterminated string literal at {self.line}:{self.col}")

		self.advance()  # Skip closing quote
		token = Token(5, "STRING_LITERAL")
		self.tokens.append(str(token))
		return string

	def read_i_string(self) -> str:
		self.advance()  # Skip 'i'

		if self.current_char() != '"':
			print(f"Expected '\"' after 'i' in i-string at {self.line}:{self.col}")

		# Read the string part directly WITHOUT creating a separate token
		self.advance()  # Skip opening quote
		string_content = ""
		while self.current_char() is not None and self.current_char() not in "\'\"":
			char = self.current_char()
			if char in '\\':
				# Handle escape sequences
				string_content += char
				self.advance()
				if self.current_char() is not None:
					string_content += self.current_char()
					self.advance()
			else:
				string_content += char
				self.advance()

		if self.current_char() is None:
			print(f"Unterminated string literal at {self.line}:{self.col}")

		self.advance()  # Skip closing quote

		# Expect ':'
		if self.current_char() != ':':
			print(f"Expected ':' after string literal in i-string at {self.line}:{self.col}")
		self.advance()

		# Skip whitespace including newlines between ':' and '{'
		while self.current_char() and self.current_char() in ' \t\r\n':
			self.skip_whitespace()

		# Expect '{'
		if self.current_char() != '{':
			print(f"Expected '{{' after ':' in i-string at {self.line}:{self.col}")
		self.advance()

		# Read expressions until '}'
		expressions = ""
		braces = 1

		while self.current_char() and braces > 0:
			if self.current_char() == '{':
				braces += 1
			elif self.current_char() == '}':
				braces -= 1

			if braces > 0:
				expressions += self.current_char()

			self.advance()

		if braces > 0:
			print(f"Unterminated i-string expression block at {self.line}:{self.col}")

		# Create ONE token for the entire i-string
		token = Token(72, "I_STRING")
		self.tokens.append(str(token))
		return f'i"{string_content}":{{{expressions}}}'

	def read_identifier(self) -> Token:
		identifier = ""

		if self.current_char() and (self.current_char().isalnum() or self.current_char() == '_'):
			identifier += self.advance()

		while self.current_char() and (self.current_char().isalnum() or self.current_char() == '_'):
			identifier += self.advance()

		# Check for special keywords with specific token indices first
		if identifier in ['in', 'is', 'as']:
			special_token_index = get_token_index(identifier)
			token = Token(special_token_index, identifier)
		else:
			token = Token(7, identifier) if identifier in keywords else Token(6, identifier)

		self.tokens.append(str(token))
		return token

	def read_operator(self):
		"""FIXED: Read operators with proper multi-character handling to avoid ** conflicts"""
		# Try multi-character operators first (longest to shortest)
		for op in multi_char_operators:
			if self.source[self.position:self.position + len(op)] == op:
				# Found a multi-character operator
				for _ in range(len(op)):
					self.advance()
				
				token_index = get_token_index(op)
				token = Token(token_index, op)
				self.tokens.append(str(token))
				return
		
		# Try single character operators
		char = self.current_char()
		if char in single_char_operators:
			self.advance()
			token_index = get_token_index(char)
			token = Token(token_index, char)
			self.tokens.append(str(token))
			return
		
		# If we get here, it's an unknown operator character
		print(f"Warning: Unknown operator character '{char}' at {self.line}:{self.col}")
		self.advance()

	def lex(self):
		print("Beginning lex phase ...")
		while (self.stop is False):
			char = self.current_char()

			if char is None:
				break

			# Skip whitespace
			if char in " \t\r\n":
				self.skip_whitespace()
				continue

			# Skip comments
			if char == "/" and self.next_char() == "/":
				self.skip_comment()
				continue

			if char.isdigit():
				self.read_number()
				continue

			if char in "\'\"":
				self.read_string()
				continue

			if char == "i" and self.next_char() in "\'\"":
				self.read_i_string()
				continue

			if char == ">" and self.next_char() == "*":  # This is a template pointer initialization, Like Complex<i32>* myPointer;
				token_index = get_token_index(char)
				token = Token(token_index, char)
				self.tokens.append(str(token))
				self.advance()
				continue

			# Handle multi-character operators with the fixed method
			if char in op_chars:
				self.read_operator()
				continue

			if char in delimiters:
				token_index = get_token_index(char)
				token = Token(token_index, char)
				self.tokens.append(str(token))
				self.advance()
				continue

			if char.isalnum() or char == "_":
				self.read_identifier()
				continue

			# Fallback: skip unknown characters to prevent infinite loop
			print(f"Warning: Unknown character '{char}' at {self.line}:{self.col}, skipping")
			self.advance()

		print("Completed.")

		token = Token(76, "EOF")
		self.tokens.append(str(token))
		return

if __name__=="__main__":
	source = ""
	with open("./test2.fx", "r") as file:
		file.seek(0)
		source = file.read()
		file.close()

	flexer = Lexer(source)

	flexer.lex()
	print(flexer.tokens)

	for token in token_list:
		print(token.token)