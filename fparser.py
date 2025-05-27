from typing import List, Optional, Union, Dict, Set
from flexer import FluxLexer, Token, TokenType, LexerError
from fast import *

class ParseError(Exception):
    """Exception raised when parsing fails"""
    def __init__(self, message: str, token: Optional[Token] = None):
        self.message = message
        self.token = token
        if token:
            super().__init__(f"Parse error at line {token.line}, column {token.column}: {message}")
        else:
            super().__init__(f"Parse error: {message}")

class FluxParser:
    """Fixed recursive descent parser for Flux language"""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.position = 0
        self.current_token = self.tokens[0] if tokens else None
    
    # Token management
    def peek(self, offset: int = 0) -> Optional[Token]:
        """Peek at token at current position + offset"""
        pos = self.position + offset
        return self.tokens[pos] if pos < len(self.tokens) else None
    
    def advance(self) -> Token:
        """Move to next token and return current"""
        current = self.current_token
        if self.position < len(self.tokens) - 1:
            self.position += 1
            self.current_token = self.tokens[self.position]
        return current
    
    def match(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of the given types"""
        return self.current_token and self.current_token.type in token_types
    
    def check(self, token_type: TokenType) -> bool:
        """Check if current token is of given type"""
        return self.current_token and self.current_token.type == token_type
    
    def consume(self, token_type: TokenType, message: str = None) -> Token:
        """Consume token of expected type or raise error"""
        if not self.check(token_type):
            if message is None:
                message = f"Expected {token_type.name}, got {self.current_token.type.name if self.current_token else 'EOF'}"
            raise ParseError(message, self.current_token)
        return self.advance()
    
    def is_at_end(self) -> bool:
        """Check if we're at end of tokens"""
        return not self.current_token or self.current_token.type == TokenType.EOF
    
    def synchronize(self):
        """Skip tokens until we find a statement boundary for error recovery"""
        self.advance()
        while not self.is_at_end():
            if self.peek(-1) and self.peek(-1).type == TokenType.SEMICOLON:
                return
            if self.match(TokenType.KEYWORD):
                keyword = self.current_token.value
                if keyword in ['def', 'object', 'struct', 'namespace', 'enum', 'if', 'for', 'while', 'return']:
                    return
            self.advance()
    
    # Main parsing entry point
    def parse(self) -> Program:
        """Parse the entire program"""
        declarations = []
        while not self.is_at_end():
            try:
                decl = self.parse_external_declaration()
                if decl:
                    declarations.append(decl)
            except ParseError as e:
                print(f"Parse Error: {e}")
                self.synchronize()
        
        return Program(
            translation_unit=TranslationUnit(external_declarations=declarations)
        )
    
    def parse_external_declaration(self) -> Optional[ExternalDeclaration]:
        """Parse top-level declaration"""
        if self.is_at_end():
            return None
        
        if not self.match(TokenType.KEYWORD, TokenType.IDENTIFIER):
            raise ParseError("Expected declaration")
        
        if self.match(TokenType.KEYWORD):
            keyword = self.current_token.value
            
            if keyword == 'import':
                return self.parse_import()
            elif keyword == 'using':
                return self.parse_using()
            elif keyword == 'def':
                return self.parse_function()
            elif keyword == 'object':
                return self.parse_object()
            elif keyword == 'struct':
                return self.parse_struct()
            elif keyword == 'namespace':
                return self.parse_namespace()
            elif keyword == 'enum':
                return self.parse_enum()
            elif keyword == 'operator':
                return self.parse_operator()
            else:
                # Could be a variable declaration with keyword type
                return self.parse_variable_declaration()
        else:
            # Identifier - could be variable declaration with custom type
            return self.parse_variable_declaration()
    
    # Import and using
    def parse_import(self) -> ImportStatement:
        """Parse import statement"""
        token = self.consume(TokenType.KEYWORD)  # 'import'
        filename = self.consume(TokenType.STRING).value
        self.consume(TokenType.KEYWORD)  # 'as'
        alias = self.consume(TokenType.IDENTIFIER).value
        self.consume(TokenType.SEMICOLON)
        return ImportStatement(filename=filename, alias=alias, token=token)
    
    def parse_using(self) -> UsingStatement:
        """Parse using statement"""
        token = self.consume(TokenType.KEYWORD)  # 'using'
        qualified_ids = [self.parse_qualified_id()]
        
        while self.match(TokenType.COMMA):
            self.advance()
            qualified_ids.append(self.parse_qualified_id())
        
        self.consume(TokenType.SEMICOLON)
        return UsingStatement(qualified_ids=qualified_ids, token=token)
    
    def parse_qualified_id(self) -> QualifiedId:
        """Parse qualified identifier"""
        identifiers = [self.consume(TokenType.IDENTIFIER).value]
        tokens = [self.peek(-1)]
        
        while self.match(TokenType.SCOPE):
            self.advance()
            identifiers.append(self.consume(TokenType.IDENTIFIER).value)
            tokens.append(self.peek(-1))
        
        return QualifiedId(identifiers=identifiers, tokens=tokens)
    
    # Function definition
    def parse_function(self) -> FunctionDefinition:
        """Parse function definition"""
        token = self.consume(TokenType.KEYWORD)  # 'def'
        name = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.LPAREN)
        parameters = self.parse_parameter_list()
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.ARROW)
        return_type = self.parse_return_type()
        
        body = self.parse_compound_statement()
        self.consume(TokenType.SEMICOLON)
        
        return FunctionDefinition(
            visibility=None, template_prefix=None,
            name=name, parameters=parameters, return_type=return_type,
            body=body, token=token
        )
    
    def parse_parameter_list(self) -> List[Parameter]:
        """Parse function parameters"""
        parameters = []
        if not self.match(TokenType.RPAREN):
            parameters.append(self.parse_parameter())
            while self.match(TokenType.COMMA):
                self.advance()
                parameters.append(self.parse_parameter())
        return parameters
    
    def parse_parameter(self) -> Parameter:
        """Parse single parameter"""
        type_spec = self.parse_type_specifier()
        token = self.consume(TokenType.IDENTIFIER)
        name = token.value
        
        default_value = None
        if self.match(TokenType.ASSIGN):
            self.advance()
            default_value = self.parse_expression()
        
        return Parameter(type_spec=type_spec, name=name, default_value=default_value, token=token)
    
    def parse_return_type(self) -> ReturnType:
        """Parse return type"""
        never_returns = False
        if self.match(TokenType.NOT):
            never_returns = True
            self.advance()
        
        token = self.current_token
        type_spec = self.parse_type_specifier()
        return ReturnType(type_spec=type_spec, never_returns=never_returns, token=token)
    
    # Object definition
    def parse_object(self) -> ObjectDefinition:
        """Parse object definition"""
        token = self.consume(TokenType.KEYWORD)  # 'object'
        name = self.consume(TokenType.IDENTIFIER).value
        
        inheritance = None
        if self.match(TokenType.COLON):
            inheritance = self.parse_inheritance()
        
        self.consume(TokenType.LBRACE)
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.is_at_end():
            try:
                member = self.parse_object_member()
                if member:
                    members.append(member)
            except ParseError as e:
                print(f"Error parsing object member in '{name}': {e}")
                # More targeted recovery - look for def, object, struct, or type names
                found_recovery = False
                while not self.match(TokenType.RBRACE) and not self.is_at_end():
                    if self.match(TokenType.KEYWORD):
                        if self.current_token.value in ['def', 'object', 'struct']:
                            found_recovery = True
                            break
                    elif self.match(TokenType.IDENTIFIER):
                        # Could be a type name for variable declaration
                        found_recovery = True
                        break
                    self.advance()
                
                if not found_recovery:
                    break
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return ObjectDefinition(
            visibility=None, template_prefix=None,
            name=name, inheritance=inheritance, members=members, token=token
        )
    
    def parse_inheritance(self) -> InheritanceClause:
        """Parse inheritance clause"""
        self.consume(TokenType.COLON)
        specifiers = [self.parse_inheritance_specifier()]
        
        while self.match(TokenType.COMMA):
            self.advance()
            specifiers.append(self.parse_inheritance_specifier())
        
        return InheritanceClause(specifiers=specifiers)
    
    def parse_inheritance_specifier(self) -> InheritanceSpecifier:
        """Parse single inheritance specifier"""
        excluded = False
        if self.match(TokenType.NOT):
            excluded = True
            self.advance()
        
        qualified_id = self.parse_qualified_id()
        return InheritanceSpecifier(qualified_id=qualified_id, excluded=excluded)
    
    def parse_object_member(self) -> Optional[ObjectMember]:
        """Parse object member with better error reporting"""
        if not self.match(TokenType.KEYWORD, TokenType.IDENTIFIER):
            raise ParseError(f"Expected object member, got {self.current_token.type.name if self.current_token else 'EOF'}")
        
        if self.match(TokenType.KEYWORD):
            keyword = self.current_token.value
            if keyword == 'def':
                return self.parse_function()
            elif keyword == 'object':
                return self.parse_object()
            elif keyword == 'struct':
                return self.parse_struct()
            elif keyword in ['void', 'auto', 'signed', 'unsigned', 'data', 'this']:
                return self.parse_variable_declaration()
        
        # Variable declaration with custom type or built-in types from std lib
        return self.parse_variable_declaration()
    
    # Struct definition
    def parse_struct(self) -> StructDefinition:
        """Parse struct definition"""
        token = self.consume(TokenType.KEYWORD)  # 'struct'
        name = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.LBRACE)
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.is_at_end():
            try:
                if self.match(TokenType.KEYWORD) and self.current_token.value == 'struct':
                    members.append(self.parse_struct())
                else:
                    members.append(self.parse_variable_declaration())
            except ParseError as e:
                print(f"Error parsing struct member: {e}")
                self.synchronize()
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return StructDefinition(
            visibility=None, template_prefix=None,
            name=name, members=members, token=token
        )
    
    # Namespace definition
    def parse_namespace(self) -> NamespaceDefinition:
        """Parse namespace definition"""
        token = self.consume(TokenType.KEYWORD)  # 'namespace'
        name = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.LBRACE)
        members = []
        
        while not self.match(TokenType.RBRACE) and not self.is_at_end():
            try:
                if self.match(TokenType.KEYWORD):
                    keyword = self.current_token.value
                    if keyword == 'object':
                        members.append(self.parse_object())
                    elif keyword == 'namespace':
                        members.append(self.parse_namespace())
                    elif keyword == 'def':
                        members.append(self.parse_function())
                    else:
                        members.append(self.parse_variable_declaration())
                else:
                    # Custom type variable declaration
                    members.append(self.parse_variable_declaration())
            except ParseError as e:
                print(f"Error parsing namespace member: {e}")
                self.synchronize()
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return NamespaceDefinition(name=name, members=members, token=token)
    
    # Enum definition
    def parse_enum(self) -> EnumDefinition:
        """Parse enum definition"""
        token = self.consume(TokenType.KEYWORD)  # 'enum'
        name = self.consume(TokenType.IDENTIFIER).value
        
        self.consume(TokenType.LBRACE)
        members = []
        
        if not self.match(TokenType.RBRACE):
            members.append(self.parse_enum_member())
            while self.match(TokenType.COMMA):
                self.advance()
                if not self.match(TokenType.RBRACE):
                    members.append(self.parse_enum_member())
        
        self.consume(TokenType.RBRACE)
        self.consume(TokenType.SEMICOLON)
        
        return EnumDefinition(
            visibility=None, name=name, template_prefix=None,
            members=members, token=token
        )
    
    def parse_enum_member(self) -> EnumMember:
        """Parse enum member"""
        token = self.consume(TokenType.IDENTIFIER)
        name = token.value
        
        associated_types = None
        if self.match(TokenType.LPAREN):
            self.advance()
            associated_types = []
            
            if not self.match(TokenType.RPAREN):
                associated_types.append(self.parse_type_specifier())
                while self.match(TokenType.COMMA):
                    self.advance()
                    associated_types.append(self.parse_type_specifier())
            
            self.consume(TokenType.RPAREN)
        
        return EnumMember(name=name, associated_types=associated_types, token=token)
    
    # Operator definition
    def parse_operator(self) -> OperatorDefinition:
        """Parse operator definition"""
        token = self.consume(TokenType.KEYWORD)  # 'operator'
        
        self.consume(TokenType.LPAREN)
        parameters = self.parse_parameter_list()
        self.consume(TokenType.RPAREN)
        
        self.consume(TokenType.LBRACKET)
        symbol = ""
        while not self.match(TokenType.RBRACKET) and not self.is_at_end():
            symbol += self.current_token.value
            self.advance()
        self.consume(TokenType.RBRACKET)
        
        self.consume(TokenType.ARROW)
        return_type = self.parse_type_specifier()
        
        body = self.parse_compound_statement()
        self.consume(TokenType.SEMICOLON)
        
        return OperatorDefinition(
            parameters=parameters, symbol=symbol, precedence=None,
            return_type=return_type, body=body, token=token
        )
    
    # Variable declaration
    def parse_variable_declaration(self) -> VariableDeclaration:
        """Parse variable declaration"""
        type_spec = self.parse_type_specifier()
        declarators = [self.parse_declarator()]
        
        while self.match(TokenType.COMMA):
            self.advance()
            declarators.append(self.parse_declarator())
        
        token = self.current_token
        self.consume(TokenType.SEMICOLON)
        
        return VariableDeclaration(
            visibility=None, storage_class=None,
            type_spec=type_spec, declarators=declarators, token=token
        )
    
    def parse_declarator(self) -> Declarator:
        """Parse declarator"""
        is_pointer = False
        if self.match(TokenType.MULTIPLY):
            is_pointer = True
            self.advance()
        
        token = self.consume(TokenType.IDENTIFIER)
        name = token.value
        
        array_size = None
        if self.match(TokenType.LBRACKET):
            self.advance()
            if not self.match(TokenType.RBRACKET):
                array_size = self.parse_expression()
            self.consume(TokenType.RBRACKET)
        
        initializer = None
        if self.match(TokenType.ASSIGN):
            initializer = self.parse_initializer()
        
        return Declarator(
            name=name, array_size=array_size, is_pointer=is_pointer,
            function_params=None, initializer=initializer, token=token
        )
    
    def parse_initializer(self) -> Initializer:
        """Parse initializer"""
        self.consume(TokenType.ASSIGN)
        
        if self.match(TokenType.LBRACE):
            return Initializer(value=self.parse_braced_init_list())
        else:
            return Initializer(value=self.parse_expression())
    
    def parse_braced_init_list(self) -> BracedInitList:
        """Parse braced initializer list"""
        token = self.consume(TokenType.LBRACE)
        values = []
        
        if not self.match(TokenType.RBRACE):
            values.append(self.parse_expression())
            while self.match(TokenType.COMMA):
                self.advance()
                if not self.match(TokenType.RBRACE):
                    values.append(self.parse_expression())
        
        self.consume(TokenType.RBRACE)
        return BracedInitList(values=values, token=token)
    
    # Type specifiers
    def parse_type_specifier(self) -> TypeSpecifier:
        """Parse type specifier - fixed to handle all cases including 'this'"""
        if self.match(TokenType.KEYWORD):
            keyword = self.current_token.value
            if keyword in ['void', 'auto', 'this']:  # Added 'this' for constructor return types
                token = self.advance()
                return SimpleTypeSpecifier(name=keyword, token=token)
            elif keyword in ['signed', 'unsigned', 'data']:
                return self.parse_data_type_specifier()
        
        if self.match(TokenType.MULTIPLY):
            return self.parse_pointer_type_specifier()
        
        if self.match(TokenType.IDENTIFIER):
            return self.parse_identifier_type_specifier()
        
        # Add debug info for failed parsing
        current_token_info = f"{self.current_token.type.name}:{self.current_token.value}" if self.current_token else "EOF"
        raise ParseError(f"Expected type specifier, got {current_token_info}")
    
    def parse_identifier_type_specifier(self) -> TypeSpecifier:
        """Parse identifier-based type specifier - fixed to handle member types"""
        name_token = self.advance()
        name = name_token.value
        
        # Handle qualified names (namespace::type)
        if self.match(TokenType.SCOPE):
            identifiers = [name]
            tokens = [name_token]
            
            while self.match(TokenType.SCOPE):
                self.advance()
                tokens.append(self.consume(TokenType.IDENTIFIER))
                identifiers.append(tokens[-1].value)
            
            qualified_id = QualifiedId(identifiers=identifiers, tokens=tokens)
            return SimpleTypeSpecifier(name=qualified_id, token=name_token)
        
        # Handle member types (Object.NestedType) - FIXED
        if self.match(TokenType.DOT):
            # Build the full member type name
            full_name = name
            while self.match(TokenType.DOT):
                self.advance()
                member_token = self.consume(TokenType.IDENTIFIER)
                full_name += "." + member_token.value
            
            return SimpleTypeSpecifier(name=full_name, token=name_token)
        
        # Handle arrays
        if self.match(TokenType.LBRACKET):
            base_type = SimpleTypeSpecifier(name=name, token=name_token)
            return self.parse_array_type_specifier(base_type)
        
        return SimpleTypeSpecifier(name=name, token=name_token)
    
    def parse_pointer_type_specifier(self) -> PointerTypeSpecifier:
        """Parse pointer type specifier"""
        token = self.consume(TokenType.MULTIPLY)
        base_type = self.parse_type_specifier()
        return PointerTypeSpecifier(
            base_type=base_type, cv_qualifiers=[], 
            function_params=None, token=token
        )
    
    def parse_array_type_specifier(self, element_type: TypeSpecifier) -> ArrayTypeSpecifier:
        """Parse array type specifier"""
        token = self.consume(TokenType.LBRACKET)
        size = None
        if not self.match(TokenType.RBRACKET):
            size = self.parse_expression()
        self.consume(TokenType.RBRACKET)
        return ArrayTypeSpecifier(element_type=element_type, size=size, token=token)
    
    def parse_data_type_specifier(self) -> DataTypeSpecifier:
        """Parse data{n} type specifier"""
        signedness = None
        if self.match(TokenType.KEYWORD) and self.current_token.value in ['signed', 'unsigned']:
            signedness = self.current_token.value
            self.advance()
        
        token = self.consume(TokenType.KEYWORD)  # 'data'
        self.consume(TokenType.LBRACE)
        size = self.parse_expression()
        self.consume(TokenType.RBRACE)
        
        return DataTypeSpecifier(signedness=signedness, size=size, token=token)
    
    # Statements
    def parse_compound_statement(self) -> CompoundStatement:
        """Parse compound statement { ... }"""
        token = self.consume(TokenType.LBRACE)
        statements = []
        
        while not self.match(TokenType.RBRACE) and not self.is_at_end():
            try:
                stmt = self.parse_statement()
                if stmt:
                    statements.append(stmt)
            except ParseError as e:
                print(f"Error parsing statement: {e}")
                self.synchronize()
        
        self.consume(TokenType.RBRACE)
        return CompoundStatement(statements=statements, token=token)
    
    def parse_statement(self) -> Statement:
        """Parse statement - FIXED to handle object instantiation correctly"""
        if self.match(TokenType.LBRACE):
            return self.parse_compound_statement()
        
        if self.match(TokenType.KEYWORD):
            keyword = self.current_token.value
            if keyword == 'if':
                return self.parse_if_statement()
            elif keyword == 'while':
                return self.parse_while_statement()
            elif keyword == 'for':
                return self.parse_for_statement()
            elif keyword == 'return':
                return self.parse_return_statement()
            elif keyword == 'break':
                return self.parse_break_statement()
            elif keyword == 'continue':
                return self.parse_continue_statement()
            elif keyword in ['void', 'auto', 'signed', 'unsigned', 'data']:
                return self.parse_variable_declaration()
        
        # Check for object instantiation: Type instanceName(args); or Type instanceName;
        if self.match(TokenType.IDENTIFIER):
            if self.is_object_instantiation():
                return self.parse_object_instantiation()
        
        return self.parse_expression_statement()
    
    def is_object_instantiation(self) -> bool:
        """Check if current position is object instantiation - FIXED"""
        saved_pos = self.position
        try:
            # Type name (possibly qualified or member type)
            self.advance()
            
            # Handle qualified names (namespace::type)
            while self.match(TokenType.SCOPE):
                self.advance()
                if not self.match(TokenType.IDENTIFIER):
                    return False
                self.advance()
            
            # Handle member types (Object.NestedType)
            while self.match(TokenType.DOT):
                self.advance()
                if not self.match(TokenType.IDENTIFIER):
                    return False
                self.advance()
            
            # Instance name
            if not self.match(TokenType.IDENTIFIER):
                return False
            self.advance()
            
            # Constructor call, assignment, or just semicolon (no-arg constructor)
            result = (self.match(TokenType.LPAREN) or 
                     self.match(TokenType.ASSIGN) or 
                     self.match(TokenType.SEMICOLON))
            
            self.position = saved_pos
            self.current_token = self.tokens[self.position]
            return result
            
        except:
            self.position = saved_pos
            self.current_token = self.tokens[self.position]
            return False
    
    def parse_object_instantiation(self) -> VariableDeclaration:
        """Parse object instantiation statement - FIXED"""
        type_spec = self.parse_type_specifier()
        instance_token = self.consume(TokenType.IDENTIFIER)
        instance_name = instance_token.value
        
        initializer = None
        if self.match(TokenType.LPAREN):
            # Constructor call with arguments
            call_expr = self.parse_constructor_call(type_spec, instance_token)
            initializer = Initializer(value=call_expr)
        elif self.match(TokenType.ASSIGN):
            # Assignment initializer
            initializer = self.parse_initializer()
        # If neither, it's a no-argument constructor (just Type instanceName;)
        
        declarator = Declarator(
            name=instance_name, array_size=None, is_pointer=False,
            function_params=None, initializer=initializer, token=instance_token
        )
        
        self.consume(TokenType.SEMICOLON)
        
        return VariableDeclaration(
            visibility=None, storage_class=None,
            type_spec=type_spec, declarators=[declarator], token=instance_token
        )
    
    def parse_constructor_call(self, type_spec: TypeSpecifier, token: Token) -> CallExpression:
        """Parse constructor call arguments - FIXED"""
        self.consume(TokenType.LPAREN)
        arguments = []
        
        if not self.match(TokenType.RPAREN):
            arguments.append(self.parse_expression())
            while self.match(TokenType.COMMA):
                self.advance()
                arguments.append(self.parse_expression())
        
        self.consume(TokenType.RPAREN)
        
        # Create identifier for the type - handle different type spec types
        if isinstance(type_spec.name, str):
            type_name = type_spec.name
        elif isinstance(type_spec.name, QualifiedId):
            type_name = "::".join(type_spec.name.identifiers)
        else:
            type_name = str(type_spec.name)
        
        type_id = Identifier(name=type_name, token=token)
        
        return CallExpression(function=type_id, arguments=arguments, token=token)
    
    def parse_if_statement(self) -> IfStatement:
        """Parse if statement"""
        token = self.consume(TokenType.KEYWORD)  # 'if'
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        
        then_body = self.parse_compound_statement()
        
        else_body = None
        if self.match(TokenType.KEYWORD) and self.current_token.value == 'else':
            self.advance()
            else_body = self.parse_compound_statement()
        
        self.consume(TokenType.SEMICOLON)
        return IfStatement(condition=condition, then_body=then_body, else_body=else_body, token=token)
    
    def parse_while_statement(self) -> WhileStatement:
        """Parse while statement"""
        token = self.consume(TokenType.KEYWORD)  # 'while'
        self.consume(TokenType.LPAREN)
        condition = self.parse_expression()
        self.consume(TokenType.RPAREN)
        body = self.parse_compound_statement()
        self.consume(TokenType.SEMICOLON)
        return WhileStatement(condition=condition, body=body, token=token)
    
    def parse_for_statement(self) -> ForStatement:
        """Parse for statement"""
        token = self.consume(TokenType.KEYWORD)  # 'for'
        self.consume(TokenType.LPAREN)
        
        # Check for range-based for
        if self.match(TokenType.IDENTIFIER):
            saved_pos = self.position
            var_token = self.advance()
            if self.match(TokenType.KEYWORD) and self.current_token.value == 'in':
                # Range-based for
                self.advance()  # 'in'
                iterable = self.parse_expression()
                self.consume(TokenType.RPAREN)
                body = self.parse_compound_statement()
                self.consume(TokenType.SEMICOLON)
                
                return ForStatement(
                    init=None, condition=None, increment=None,
                    iterator_var=var_token.value, iterable=iterable,
                    body=body, token=token
                )
            else:
                # Regular for loop
                self.position = saved_pos
                self.current_token = self.tokens[self.position]
        
        # Regular for loop
        init = self.parse_expression() if not self.match(TokenType.SEMICOLON) else None
        self.consume(TokenType.SEMICOLON)
        
        condition = self.parse_expression() if not self.match(TokenType.SEMICOLON) else None
        self.consume(TokenType.SEMICOLON)
        
        increment = self.parse_expression() if not self.match(TokenType.RPAREN) else None
        self.consume(TokenType.RPAREN)
        
        body = self.parse_compound_statement()
        self.consume(TokenType.SEMICOLON)
        
        return ForStatement(
            init=init, condition=condition, increment=increment,
            iterator_var=None, iterable=None, body=body, token=token
        )
    
    def parse_return_statement(self) -> ReturnStatement:
        """Parse return statement"""
        token = self.consume(TokenType.KEYWORD)  # 'return'
        expression = self.parse_expression() if not self.match(TokenType.SEMICOLON) else None
        self.consume(TokenType.SEMICOLON)
        return ReturnStatement(expression=expression, token=token)
    
    def parse_break_statement(self) -> BreakStatement:
        """Parse break statement"""
        token = self.consume(TokenType.KEYWORD)  # 'break'
        self.consume(TokenType.SEMICOLON)
        return BreakStatement(token=token)
    
    def parse_continue_statement(self) -> ContinueStatement:
        """Parse continue statement"""
        token = self.consume(TokenType.KEYWORD)  # 'continue'
        self.consume(TokenType.SEMICOLON)
        return ContinueStatement(token=token)
    
    def parse_expression_statement(self) -> ExpressionStatement:
        """Parse expression statement"""
        token = self.current_token
        expression = self.parse_expression() if not self.match(TokenType.SEMICOLON) else None
        self.consume(TokenType.SEMICOLON)
        return ExpressionStatement(expression=expression, token=token)
    
    # Expressions (simplified precedence climbing)
    def parse_expression(self) -> Expression:
        """Parse expression"""
        return self.parse_assignment()
    
    def parse_assignment(self) -> Expression:
        """Parse assignment expression"""
        expr = self.parse_logical_or()
        
        if self.match(TokenType.ASSIGN):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_assignment()
            return AssignmentExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_logical_or(self) -> Expression:
        """Parse logical OR"""
        expr = self.parse_logical_and()
        
        while (self.match(TokenType.OR) or 
               (self.match(TokenType.KEYWORD) and self.current_token.value == 'or')):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_logical_and()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_logical_and(self) -> Expression:
        """Parse logical AND"""
        expr = self.parse_equality()
        
        while (self.match(TokenType.AND) or 
               (self.match(TokenType.KEYWORD) and self.current_token.value == 'and')):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_equality()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_equality(self) -> Expression:
        """Parse equality"""
        expr = self.parse_relational()
        
        while self.match(TokenType.EQUAL, TokenType.NOT_EQUAL):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_relational()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_relational(self) -> Expression:
        """Parse relational"""
        expr = self.parse_additive()
        
        while self.match(TokenType.LESS, TokenType.GREATER, TokenType.LESS_EQUAL, TokenType.GREATER_EQUAL):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_additive()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_additive(self) -> Expression:
        """Parse additive"""
        expr = self.parse_multiplicative()
        
        while self.match(TokenType.PLUS, TokenType.MINUS):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_multiplicative()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_multiplicative(self) -> Expression:
        """Parse multiplicative"""
        expr = self.parse_unary()
        
        while self.match(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            operator = self.current_token.value
            token = self.advance()
            right = self.parse_unary()
            expr = BinaryExpression(left=expr, operator=operator, right=right, token=token)
        
        return expr
    
    def parse_unary(self) -> Expression:
        """Parse unary"""
        if self.match(TokenType.NOT, TokenType.MINUS, TokenType.PLUS):
            operator = self.current_token.value
            token = self.advance()
            expr = self.parse_unary()
            return UnaryExpression(operator=operator, operand=expr, prefix=True, token=token)
        
        return self.parse_postfix()
    
    def parse_postfix(self) -> Expression:
        """Parse postfix"""
        expr = self.parse_primary()
        
        while True:
            if self.match(TokenType.DOT):
                token = self.advance()
                member = self.consume(TokenType.IDENTIFIER).value
                expr = MemberExpression(object=expr, member=member, scope_operator=False, token=token)
            elif self.match(TokenType.SCOPE):
                token = self.advance()
                member = self.consume(TokenType.IDENTIFIER).value
                expr = MemberExpression(object=expr, member=member, scope_operator=True, token=token)
            elif self.match(TokenType.LPAREN):
                token = self.advance()
                arguments = []
                if not self.match(TokenType.RPAREN):
                    arguments.append(self.parse_expression())
                    while self.match(TokenType.COMMA):
                        self.advance()
                        arguments.append(self.parse_expression())
                self.consume(TokenType.RPAREN)
                expr = CallExpression(function=expr, arguments=arguments, token=token)
            elif self.match(TokenType.LBRACKET):
                token = self.advance()
                index = self.parse_expression()
                self.consume(TokenType.RBRACKET)
                expr = IndexExpression(array=expr, index=index, token=token)
            else:
                break
        
        return expr
    
    def parse_primary(self) -> Expression:
        """Parse primary expression - FIXED to handle function calls like print()"""
        if self.match(TokenType.INTEGER):
            token = self.advance()
            return IntegerLiteral(value=token.value, token=token)
        
        if self.match(TokenType.FLOAT):
            token = self.advance()
            return FloatingLiteral(value=token.value, token=token)
        
        if self.match(TokenType.STRING):
            token = self.advance()
            return StringLiteral(value=token.value, token=token)
        
        if self.match(TokenType.IDENTIFIER):
            token = self.advance()
            return Identifier(name=token.value, token=token)
        
        if self.match(TokenType.KEYWORD):
            keyword = self.current_token.value
            if keyword == 'this':
                token = self.advance()
                member = None
                if self.match(TokenType.DOT):
                    self.advance()
                    member = self.consume(TokenType.IDENTIFIER).value
                return ThisExpression(member=member, token=token)
            elif keyword == 'super':
                token = self.advance()
                scope_operator = False
                member = None
                
                if self.match(TokenType.DOT):
                    self.advance()
                    member = self.consume(TokenType.IDENTIFIER).value
                elif self.match(TokenType.SCOPE):
                    self.advance()
                    scope_operator = True
                    member = self.consume(TokenType.IDENTIFIER).value
                
                return SuperExpression(member=member, scope_operator=scope_operator, token=token)
        
        if self.match(TokenType.LPAREN):
            self.advance()
            expr = self.parse_expression()
            self.consume(TokenType.RPAREN)
            return expr
        
        if self.match(TokenType.LBRACE):
            return self.parse_braced_literal()
        
        if self.match(TokenType.LBRACKET):
            return self.parse_array_literal()
        
        raise ParseError("Expected expression", self.current_token)
    
    def parse_braced_literal(self) -> Expression:
        """Parse braced literal (dictionary or initializer)"""
        token = self.consume(TokenType.LBRACE)
        
        if self.match(TokenType.RBRACE):
            self.advance()
            return DictionaryLiteral(pairs=[], token=token)
        
        # Try to parse as dictionary first
        saved_pos = self.position
        try:
            key = self.parse_expression()
            if self.match(TokenType.COLON):
                # Dictionary
                self.advance()
                value = self.parse_expression()
                pairs = [KeyValuePair(key=key, value=value, token=token)]
                
                while self.match(TokenType.COMMA):
                    self.advance()
                    if not self.match(TokenType.RBRACE):
                        k = self.parse_expression()
                        self.consume(TokenType.COLON)
                        v = self.parse_expression()
                        pairs.append(KeyValuePair(key=k, value=v, token=self.current_token))
                
                self.consume(TokenType.RBRACE)
                return DictionaryLiteral(pairs=pairs, token=token)
        except:
            pass
        
        # Parse as array initializer
        self.position = saved_pos
        self.current_token = self.tokens[self.position]
        
        elements = []
        elements.append(self.parse_expression())
        
        while self.match(TokenType.COMMA):
            self.advance()
            if not self.match(TokenType.RBRACE):
                elements.append(self.parse_expression())
        
        self.consume(TokenType.RBRACE)
        return ArrayLiteral(elements=elements, token=token)
    
    def parse_array_literal(self) -> ArrayLiteral:
        """Parse array literal"""
        token = self.consume(TokenType.LBRACKET)
        elements = []
        
        if not self.match(TokenType.RBRACKET):
            elements.append(self.parse_expression())
            while self.match(TokenType.COMMA):
                self.advance()
                if not self.match(TokenType.RBRACKET):
                    elements.append(self.parse_expression())
        
        self.consume(TokenType.RBRACKET)
        return ArrayLiteral(elements=elements, token=token)


# Command line interface
if __name__ == "__main__":
    import sys
    import argparse
    import os
    
    def main():
        parser = argparse.ArgumentParser(description='Fixed Flux Language Parser')
        parser.add_argument('file', nargs='?', help='Flux source file to parse')
        parser.add_argument('--debug', '-d', action='store_true', help='Enable debug output')
        
        args = parser.parse_args()
        
        # Get source code
        if not args.file:
            print("No file provided. Using test code...\n")
            source_code = '''
import "std.fx" as std;
using std::io, std::types;

object TestObj
{
    i32 value;
    
    def __init(i32 val) -> this
    {
        this.value = val;
        return this;
    };
};

def main() -> i32
{
    TestObj instance(42);
    TestObj instance2;
    return 0;
};
            '''
        else:
            try:
                if not os.path.exists(args.file):
                    print(f"Error: File '{args.file}' not found.")
                    return 1
                
                with open(args.file, 'r', encoding='utf-8') as f:
                    source_code = f.read()
                    
                print(f"Parsing file: {args.file}")
                print(f"File size: {len(source_code)} characters\n")
                
            except IOError as e:
                print(f"Error reading file '{args.file}': {e}")
                return 1
        
        try:
            # Tokenize
            lexer = FluxLexer(source_code)
            tokens = lexer.tokenize_filter_whitespace()
            
            if args.debug:
                print("Tokens:")
                for i, token in enumerate(tokens[:30]):
                    print(f"  {i}: {token.type.name} = {repr(token.value)}")
                if len(tokens) > 30:
                    print(f"  ... and {len(tokens) - 30} more tokens")
                print()
            
            # Parse
            flux_parser = FluxParser(tokens)
            ast = flux_parser.parse()
            
            # Print AST
            print("AST Structure:")
            printer = ASTPrinter()
            printer.visit(ast)
            
            # Print summary
            declarations = ast.translation_unit.external_declarations
            print(f"\nSummary:")
            print(f"  Total declarations: {len(declarations)}")
            
            for i, decl in enumerate(declarations):
                if isinstance(decl, FunctionDefinition):
                    print(f"  {i+1}. Function: {decl.name}")
                elif isinstance(decl, ObjectDefinition):
                    print(f"  {i+1}. Object: {decl.name} ({len(decl.members)} members)")
                elif isinstance(decl, StructDefinition):
                    print(f"  {i+1}. Struct: {decl.name}")
                elif isinstance(decl, NamespaceDefinition):
                    print(f"  {i+1}. Namespace: {decl.name}")
                elif isinstance(decl, ImportStatement):
                    print(f"  {i+1}. Import: {decl.filename} as {decl.alias}")
                elif isinstance(decl, UsingStatement):
                    print(f"  {i+1}. Using: {len(decl.qualified_ids)} identifiers")
                else:
                    print(f"  {i+1}. {decl.__class__.__name__}")
            
            print(f"\nParsing completed successfully!")
            
        except (LexerError, ParseError) as e:
            print(f"Error: {e}")
            return 1
        except Exception as e:
            print(f"Unexpected error: {e}")
            if args.debug:
                import traceback
                traceback.print_exc()
            return 1
        
        return 0
    
    sys.exit(main())