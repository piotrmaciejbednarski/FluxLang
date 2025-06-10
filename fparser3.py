#!/usr/bin/env python3
"""
Flux Parser (fparser3.py) - FIXED VERSION 
Fixed template call vs template type declaration detection in is_likely_variable_declaration
"""

from fast3 import *
from flexer3 import Lexer, Token
from typing import List, Optional, Union, Any


class ParseError(Exception):
    """Parser exception for syntax errors"""
    def __init__(self, message: str, line: int = 0, col: int = 0):
        self.message = message
        self.line = line
        self.col = col
        super().__init__(f"Parse error at {line}:{col}: {message}")


class Parser:
    """Recursive descent parser for Flux"""
    
    def __init__(self, tokens: List[str], token_positions: List[tuple] = None):
        self.tokens = tokens
        self.current = 0
        self.current_token = self.tokens[0] if tokens else None
        self.token_positions = token_positions or [(1, 1)] * len(tokens)  # (line, col) for each token
        self.source_code = ""
        self.lexer = None
        self.literal_values = {}  # Store actual literal values by position
        self.in_lookahead = False  # Flag to prevent token stream modifications during lookahead
    
    def advance(self) -> str:
        """Move to the next token"""
        if self.current < len(self.tokens) - 1:
            self.current += 1
            self.current_token = self.tokens[self.current]
        return self.current_token
    
    def get_current_position(self):
        """Get the current line and column for the current token"""
        if self.current < len(self.token_positions):
            return self.token_positions[self.current]
        return (1, 1)
    
    @property
    def line(self) -> int:
        """Get current line number"""
        line, _ = self.get_current_position()
        return line
    
    @property 
    def col(self) -> int:
        """Get current column number"""
        _, col = self.get_current_position()
        return col
    
    def error(self, message: str):
        """Raise a parse error with proper position information"""
        line, col = self.get_current_position()
        raise ParseError(message, line, col)
    
    def peek(self, offset: int = 1) -> Optional[str]:
        """Look ahead at the next token without advancing"""
        peek_pos = self.current + offset
        if peek_pos < len(self.tokens):
            return self.tokens[peek_pos]
        return None
    
    def split_rshift_if_needed(self) -> bool:
        """Split RSHIFT into two GT tokens if current token is RSHIFT. Returns True if split occurred."""
        if self.current_token == "RSHIFT" and not self.in_lookahead:
            # Split RSHIFT into two GT tokens
            self.tokens[self.current] = "GT"
            self.tokens.insert(self.current + 1, "GT")
            # Adjust token positions if available
            if self.current < len(self.token_positions):
                current_pos = self.token_positions[self.current]
                self.token_positions.insert(self.current + 1, current_pos)
            self.current_token = "GT"
            return True
        return False
    
    def match(self, *expected_tokens: str) -> bool:
        """Check if current token matches any of the expected tokens"""
        # Handle >> as > in template contexts
        if "GT" in expected_tokens and self.current_token == "RSHIFT":
            return True
        if self.current_token in expected_tokens:
            return True
        return False
    
    def expect(self, expected_token: str) -> str:
        """Expect a specific token and advance, or raise error"""
        # Handle >> as two > tokens in template contexts
        if expected_token == "GT":
            self.split_rshift_if_needed()
        
        if self.current_token != expected_token:
            self.error(f"Expected '{expected_token}', got '{self.current_token}'")
        token = self.current_token
        self.advance()
        return token
    
    def expect_literal_value(self, expected_token: str) -> str:
        """Expect a literal token and return its actual value"""
        if self.current_token != expected_token:
            raise ParseError(f"Expected '{expected_token}', got '{self.current_token}'", self.line, self.col)
        
        # Extract actual value from token position
        value = self.get_literal_value(self.current)
        self.advance()
        return value
    
    def get_literal_value(self, token_pos: int) -> str:
        """Extract the actual literal value from the source code"""
        # This is a simplified approach that parses literals from the source
        from flexer3 import token_list
        
        # For data{32} parsing, we know we're looking for the number inside braces
        if token_pos < len(token_list) and self.tokens[token_pos] == "INT_LITERAL":
            # Find all numbers in source and map by position
            import re
            
            # Look for the specific pattern of numbers in data{} declarations
            data_pattern = r'data\s*\{\s*(\d+)\s*\}'
            data_matches = re.findall(data_pattern, self.source_code)
            
            # Also find standalone numbers
            number_pattern = r'\b(\d+)\b'
            all_numbers = re.findall(number_pattern, self.source_code)
            
            # For data type parsing, return the first data{} number found
            if data_matches:
                return data_matches[0]
            elif all_numbers:
                # Return first number found (fallback)
                return all_numbers[0]
                
        return "0"
    
    def consume(self, expected_token: str) -> bool:
        """Consume token if it matches, return True if consumed"""
        # Handle >> as two > tokens in template contexts
        if expected_token == "GT":
            self.split_rshift_if_needed()
        
        if self.current_token == expected_token:
            self.advance()
            return True
        return False
    
    def handle_rshift_in_templates(self):
        """Handle >> tokens in template contexts by splitting them into two > tokens"""
        if self.current_token == "RSHIFT":
            # Split RSHIFT into two GT tokens
            self.tokens[self.current] = "GT"
            self.tokens.insert(self.current + 1, "GT")
            # Adjust token positions if available
            if self.current < len(self.token_positions):
                current_pos = self.token_positions[self.current]
                self.token_positions.insert(self.current + 1, current_pos)
            self.current_token = "GT"
    
    def at_end(self) -> bool:
        """Check if we're at the end of tokens"""
        return self.current_token == "== END OF FILE ==" or \
               self.current_token == "EOF" or \
               self.current >= len(self.tokens) - 1
    
    def is_template_instantiation_lookahead(self) -> bool:
        """
        Look ahead to determine if '<' starts a template instantiation.
        This looks for the pattern: < type_list > where type_list is comma-separated types.
        """
        if not self.match("LT"):
            return False
        
        # Save current position
        saved_pos = self.current
        saved_token = self.current_token
        self.in_lookahead = True
        
        try:
            self.advance()  # consume LT
            
            # Try to parse type list
            if not self.is_type_specifier():
                return False
            
            # Parse first type
            self.parse_type_spec()
            
            # Handle comma-separated types
            while self.consume("DELIM_COMMA"):
                if not self.is_type_specifier():
                    return False
                self.parse_type_spec()
            
            # Should end with GT
            result = self.match("GT")
            return result
            
        except:
            return False
        finally:
            # Always restore position and clear lookahead flag
            self.current = saved_pos
            self.current_token = saved_token
            self.in_lookahead = False

    def is_template_call_lookahead(self) -> bool:
        """
        Look ahead to determine if this is a template call with : syntax
        Pattern: identifier < type_list > [( args )] : expression
        The function call part is optional
        """
        if not (self.current_token.startswith("IDENTIFIER_") and self.peek(1) == "LT"):
            return False
            
        # Save current position
        saved_pos = self.current
        saved_token = self.current_token
        self.in_lookahead = True
        
        try:
            self.advance()  # consume identifier
            self.advance()  # consume LT
            
            # Try to parse type list
            if not self.is_type_specifier():
                return False
            
            # Parse first type
            self.parse_type_spec()
            
            # Handle comma-separated types
            while self.consume("DELIM_COMMA"):
                if not self.is_type_specifier():
                    return False
                self.parse_type_spec()
            
            # Should end with GT
            if not self.match("GT"):
                return False
            self.advance()  # consume GT
            
            # FIXED: Handle optional function call syntax
            if self.match("DELIM_L_PARENTHESIS"):
                self.advance()  # consume (
                # Skip over arguments - we don't need to parse them precisely for lookahead
                paren_count = 1
                while paren_count > 0 and not self.at_end():
                    if self.match("DELIM_L_PARENTHESIS"):
                        paren_count += 1
                    elif self.match("DELIM_R_PARENTHESIS"):
                        paren_count -= 1
                    self.advance()
            
            # Check if followed by colon (template call syntax)
            return self.match("DELIM_COLON")
            
        except:
            return False
        finally:
            # Always restore position and clear lookahead flag
            self.current = saved_pos
            self.current_token = saved_token
            self.in_lookahead = False
    
    # Main parsing entry point
    def parse(self) -> ProgramNode:
        """Parse the entire program"""
        print("Beginning parse phase...")
        global_items = []
        
        while not self.at_end():
            item = self.parse_global_item()
            if item:
                global_items.append(item)
        
        print("Parse phase completed.")
        return ProgramNode(global_items, self.line, self.col)
    
    def parse_global_item(self) -> Optional[GlobalItemNode]:
            """Parse a global item"""
            if self.match("KW_IMPORT"):
                return self.parse_import_stmt()
            elif self.match("KW_USING"):
                return self.parse_using_stmt()
            elif self.match("KW_NAMESPACE"):
                return self.parse_namespace_def()
            elif self.match("KW_DEF"):
                return self.parse_function_def()
            elif self.match("KW_OBJECT") and self.peek(1) == "KW_TEMPLATE":
                # FIXED: Handle object templates
                return self.parse_object_template(is_volatile=False)
            elif self.match("KW_OBJECT"):
                return self.parse_object_def()
            elif self.match("KW_STRUCT") and self.peek(1) == "KW_TEMPLATE":
                return self.parse_struct_template(is_volatile=False)
            elif self.match("KW_STRUCT"):
                return self.parse_struct_def()
            elif self.match("KW_TEMPLATE"):
                return self.parse_template_def()
            elif self.match("KW_VOLATILE"):
                # Check if this is a volatile template or volatile variable declaration
                next_token = self.peek(1)
                if (next_token == "KW_TEMPLATE" or 
                    next_token == "KW_ASYNC" or 
                    next_token == "KW_OPERATOR" or
                    next_token == "KW_FOR" or
                    next_token == "KW_SWITCH" or
                    next_token == "KW_OBJECT" or
                    next_token == "KW_STRUCT"):
                    # It's a template definition - parse_template_def handles volatile internally
                    return self.parse_template_def()
                else:
                    # It's a variable declaration with volatile qualifier
                    var_decl = self.parse_variable_decl()
                    self.expect("DELIM_SEMICOLON")
                    return var_decl
            elif self.match("KW_SWITCH") and self.peek(1) == "KW_TEMPLATE":
                return self.parse_switch_template()
            elif self.match("KW_SWITCH"):
                return self.parse_switch_stmt()
            elif self.match("KW_ASYNC"):
                # Check what follows async
                if self.peek(1) == "KW_TEMPLATE":
                    return self.parse_template_def()  # async template
                elif self.peek(1) == "KW_DEF":
                    return self.parse_async_function_def()  # async def
                else:
                    self.error("Expected 'template' or 'def' after 'async'")
            elif self.match("KW_OPERATOR"):
                if self.peek(1) == "KW_TEMPLATE":
                    return self.parse_template_def()  # parse_template_def handles operator templates
                else:
                    return self.parse_operator_def()
            elif self.match("KW_FOR") and self.peek(1) == "KW_TEMPLATE":
                return self.parse_template_def()  # parse_template_def handles for templates
            else:
                # Try parsing as variable declaration
                if self.is_type_specifier():
                    var_decl = self.parse_variable_decl()
                    self.expect("DELIM_SEMICOLON")
                    return var_decl
                else:
                    self.error(f"Unexpected token in global scope: {self.current_token}")
    
    def parse_import_stmt(self) -> ImportStmtNode:
        """Parse import statement"""
        self.expect("KW_IMPORT")
        module_path = self.expect("STRING_LITERAL")
        
        alias = None
        if self.consume("KW_AS"):
            alias = self.expect_identifier()
        
        self.expect("DELIM_SEMICOLON")
        return ImportStmtNode(module_path, alias, self.line, self.col)
    
    def parse_using_stmt(self) -> UsingStmtNode:
        """Parse using statement"""
        self.expect("KW_USING")
        using_list = self.parse_using_list()
        self.expect("DELIM_SEMICOLON")
        return UsingStmtNode(using_list, self.line, self.col)
    
    def parse_using_list(self) -> List[NamespaceAccessNode]:
        """Parse comma-separated list of namespace accesses"""
        using_list = [self.parse_namespace_access()]
        
        while self.consume("DELIM_COMMA"):
            using_list.append(self.parse_namespace_access())
        
        return using_list
    
    def parse_namespace_access(self) -> NamespaceAccessNode:
        """Parse namespace access (e.g., std::io)"""
        identifiers = [self.expect_identifier()]
        
        while self.consume("DELIM_SCOPE"):
            identifiers.append(self.expect_identifier())
        
        return NamespaceAccessNode(identifiers, self.line, self.col)
    
    def parse_namespace_def(self) -> NamespaceDefNode:
        """Parse namespace definition - can contain any global item except imports/using"""
        self.expect("KW_NAMESPACE")
        name = self.expect_identifier()
        self.expect("DELIM_L_BRACE")
        
        body = []
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            item = self.parse_namespace_item()
            if item:
                body.append(item)
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        return NamespaceDefNode(name, body, self.line, self.col)

    def parse_namespace_item(self) -> Optional[GlobalItemNode]:
        """Parse a namespace item (same as global items except imports/using)"""
        if self.match("KW_NAMESPACE"):
            return self.parse_namespace_def()
        elif self.match("KW_DEF"):
            return self.parse_function_def()
        elif self.match("KW_OBJECT") and self.peek(1) == "KW_TEMPLATE":
            return self.parse_object_template(is_volatile=False)
        elif self.match("KW_OBJECT"):
            return self.parse_object_def()
        elif self.match("KW_STRUCT") and self.peek(1) == "KW_TEMPLATE":
            return self.parse_struct_template(is_volatile=False)
        elif self.match("KW_STRUCT"):
            return self.parse_struct_def()
        elif self.match("KW_TEMPLATE"):
            return self.parse_template_def()
        elif self.match("KW_VOLATILE"):
            # Check if this is a volatile template or volatile variable declaration
            next_token = self.peek(1)
            if (next_token == "KW_TEMPLATE" or 
                next_token == "KW_ASYNC" or 
                next_token == "KW_OPERATOR" or
                next_token == "KW_FOR" or
                next_token == "KW_SWITCH" or
                next_token == "KW_OBJECT" or
                next_token == "KW_STRUCT"):
                return self.parse_template_def()
            else:
                # Variable declaration with volatile qualifier
                var_decl = self.parse_variable_decl()
                self.expect("DELIM_SEMICOLON")
                return var_decl
        elif self.match("KW_ASYNC"):
            # Check what follows async
            if self.peek(1) == "KW_TEMPLATE":
                return self.parse_template_def()  # async template
            elif self.peek(1) == "KW_DEF":
                return self.parse_async_function_def()  # async def
            else:
                self.error("Expected 'template' or 'def' after 'async'")
        elif self.match("KW_OPERATOR"):
            if self.peek(1) == "KW_TEMPLATE":
                return self.parse_template_def()  # operator template
            else:
                return self.parse_operator_def()
        elif self.match("KW_FOR") and self.peek(1) == "KW_TEMPLATE":
            return self.parse_template_def()  # for template
        else:
            # Try parsing as variable declaration
            if self.is_type_specifier():
                var_decl = self.parse_variable_decl()
                self.expect("DELIM_SEMICOLON")
                return var_decl
            else:
                self.error(f"Unexpected token in namespace: {self.current_token}")

    def parse_function_def(self) -> FunctionDefNode:
        """Parse function definition"""
        self.expect("KW_DEF")
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return FunctionDefNode(name, parameters, return_type, body, self.line, self.col)
    
    def parse_async_function_def(self) -> FunctionDefNode:
        """Parse async function definition"""
        self.expect("KW_ASYNC")
        self.expect("KW_DEF")
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        # Note: For now, we're using FunctionDefNode for async functions
        # In a full implementation, you might want an AsyncFunctionDefNode
        # but this works with the current AST structure
        return FunctionDefNode(name, parameters, return_type, body, self.line, self.col)
        """Parse function definition"""
        self.expect("KW_DEF")
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return FunctionDefNode(name, parameters, return_type, body, self.line, self.col)
    
    def parse_parameter_list(self) -> List[ParameterNode]:
        """Parse function parameter list"""
        parameters = [self.parse_parameter()]
        
        while self.consume("DELIM_COMMA"):
            parameters.append(self.parse_parameter())
        
        return parameters
    
    def parse_parameter(self) -> ParameterNode:
        """Parse function parameter"""
        type_spec = self.parse_type_spec()
        name = self.expect_identifier()
        return ParameterNode(type_spec, name, self.line, self.col)
    
    def parse_object_def(self) -> ObjectDefNode:
        """Parse object definition"""
        self.expect("KW_OBJECT")
        name = self.expect_identifier()
        
        # Check for forward declaration
        if self.consume("DELIM_SEMICOLON"):
            return ObjectDefNode(name, None, None, True, self.line, self.col)
        
        inheritance = None
        if self.consume("DELIM_COLON"):
            inheritance = self.parse_inheritance()
        
        self.expect("DELIM_L_BRACE")
        body = []
        
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            if self.match("KW_OBJECT"):
                # Handle nested object - parse it but store as a special object member
                nested_obj = self.parse_nested_object_member()
                if nested_obj:
                    body.append(nested_obj)
            else:
                member = self.parse_object_member()
                if member:
                    body.append(member)
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return ObjectDefNode(name, inheritance, body, False, self.line, self.col)
    
    def parse_nested_object_member(self) -> ObjectMemberNode:
        """Parse nested object as an object member (workaround for AST limitation)"""
        # Parse the nested object
        nested_obj = self.parse_nested_object_def()
        
        # Create a wrapper that makes it compatible with ObjectMemberNode
        # For now, we'll create a variable declaration that represents the nested object
        # This is a workaround - ideally the AST should support nested objects directly
        object_type = BaseTypeNode(nested_obj.name, None, self.line, self.col)
        var_init = VariableInitNode(nested_obj.name + "_nested", None, None, self.line, self.col)
        return VariableDeclNode(object_type, [var_init], False, None, None, None, self.line, self.col)
    
    def parse_nested_object_def(self) -> ObjectDefNode:
        """Parse nested object definition (same as regular object but doesn't expect to be at global scope)"""
        self.expect("KW_OBJECT")
        name = self.expect_identifier()
        
        inheritance = None
        if self.consume("DELIM_COLON"):
            inheritance = self.parse_inheritance()
        
        self.expect("DELIM_L_BRACE")
        body = []
        
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            member = self.parse_object_member()
            if member:
                body.append(member)
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return ObjectDefNode(name, inheritance, body, False, self.line, self.col)
    
    def parse_inheritance(self) -> InheritanceNode:
        """Parse inheritance specification"""
        inheritance_list = [self.parse_inheritance_item()]
        
        while self.consume("DELIM_COMMA"):
            inheritance_list.append(self.parse_inheritance_item())
        
        return InheritanceNode(inheritance_list, self.line, self.col)
    
    def parse_inheritance_item(self) -> InheritanceItemNode:
        """Parse single inheritance item - FIXED to handle qualified names"""
        is_excluded = self.consume("NOT")
        
        # Handle qualified names (e.g., SystemCore::BaseSystem)
        name_parts = [self.expect_identifier()]
        
        while self.consume("DELIM_SCOPE"):
            name_parts.append(self.expect_identifier())
        
        # Join the parts to form the full qualified name
        full_name = "::".join(name_parts)
        
        return InheritanceItemNode(full_name, is_excluded, self.line, self.col)
    
    def parse_object_member(self) -> Optional[ObjectMemberNode]:
        """Parse object member (variable, method, template, or nested object)"""
        if self.match("KW_DEF"):
            return self.parse_method_def()
        elif self.match("KW_ASYNC") and self.peek(1) == "KW_DEF":
            # FIXED: Handle async methods within objects
            return self.parse_async_method_def()
        elif self.match("KW_OBJECT"):
            # Handle nested objects (both regular and template)
            return self.parse_nested_object_def()
        elif self.match("KW_ASYNC") and self.peek(1) == "KW_TEMPLATE":
            # Handle async template methods within objects
            return self.parse_async_template_in_object()
        elif self.match("KW_TEMPLATE"):
            # This is a function template within the object, not an object template
            # Handle template methods within objects
            return self.parse_function_template_in_object()
        elif self.match("KW_VOLATILE") and self.peek(1) == "KW_TEMPLATE":
            # Handle volatile template methods within objects
            is_volatile = True
            self.advance()  # consume KW_VOLATILE
            return self.parse_function_template_in_object(is_volatile)
        elif self.match("KW_VOLATILE") and self.peek(1) == "KW_ASYNC" and self.peek(2) == "KW_TEMPLATE":
            # Handle volatile async template methods within objects
            is_volatile = True
            self.advance()  # consume KW_VOLATILE
            return self.parse_async_template_in_object(is_volatile)
        elif self.match("KW_VOLATILE") and self.peek(1) == "KW_ASYNC" and self.peek(2) == "KW_DEF":
            # Handle volatile async methods within objects
            is_volatile = True
            self.advance()  # consume KW_VOLATILE
            return self.parse_async_method_def(is_volatile)
        elif self.is_type_specifier():
            var_decl = self.parse_variable_decl()
            self.expect("DELIM_SEMICOLON")
            return var_decl
        else:
            self.error(f"Expected object member, got {self.current_token}")

    def parse_async_method_def(self, is_volatile: bool = False) -> MethodDefNode:
        """Parse async method definition within an object"""
        self.expect("KW_ASYNC")
        self.expect("KW_DEF")
        
        # Check for magic method
        name = self.expect_identifier()
        is_magic = name.startswith("__")
        
        self.expect("DELIM_L_PARENTHESIS")
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        # Note: Using MethodDefNode for async methods
        # In a full implementation, you might want an AsyncMethodDefNode
        return MethodDefNode(name, parameters, return_type, body, is_magic, self.line, self.col)

    def parse_async_template_in_object(self, is_volatile: bool = False) -> AsyncTemplateNode:
        """Parse an async template method within an object"""
        self.expect("KW_ASYNC")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return AsyncTemplateNode(template_params, name, parameters, return_type, body, is_volatile, self.line, self.col)
    
    def parse_function_template_in_object(self, is_volatile: bool = False) -> FunctionTemplateNode:
        """Parse a function template within an object"""
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return FunctionTemplateNode(template_params, name, parameters, return_type, body, is_volatile, self.line, self.col)
    
    def parse_nested_object_def(self) -> ObjectDefNode:
        """Parse nested object definition (can be regular object or object template)"""
        self.expect("KW_OBJECT")
        
        # Check if this is an object template
        if self.consume("KW_TEMPLATE"):
            self.expect("LT")
            template_params = self.parse_template_params()
            self.expect("GT")
            
            name = self.expect_identifier()
            
            inheritance = None
            if self.consume("DELIM_COLON"):
                inheritance = self.parse_inheritance()
            
            self.expect("DELIM_L_BRACE")
            body = []
            
            while not self.match("DELIM_R_BRACE") and not self.at_end():
                member = self.parse_object_member()
                if member:
                    body.append(member)
            
            self.expect("DELIM_R_BRACE")
            self.expect("DELIM_SEMICOLON")
            
            # Return an ObjectTemplateNode
            return ObjectTemplateNode(template_params, name, inheritance, body, False, self.line, self.col)
        else:
            # Regular nested object
            name = self.expect_identifier()
            
            inheritance = None
            if self.consume("DELIM_COLON"):
                inheritance = self.parse_inheritance()
            
            self.expect("DELIM_L_BRACE")
            body = []
            
            while not self.match("DELIM_R_BRACE") and not self.at_end():
                member = self.parse_object_member()
                if member:
                    body.append(member)
            
            self.expect("DELIM_R_BRACE")
            self.expect("DELIM_SEMICOLON")
            
            return ObjectDefNode(name, inheritance, body, False, self.line, self.col)
    
    def parse_method_def(self) -> MethodDefNode:
        """Parse method definition"""
        self.expect("KW_DEF")
        
        # Check for magic method
        name = self.expect_identifier()
        is_magic = name.startswith("__")
        
        self.expect("DELIM_L_PARENTHESIS")
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return MethodDefNode(name, parameters, return_type, body, is_magic, self.line, self.col)
    
    def parse_struct_def(self) -> StructDefNode:
        """Parse struct definition"""
        if self.peek(1) == "KW_TEMPLATE":
            return self.parse_struct_template(is_volatile=False)
        self.expect("KW_STRUCT")
        name = self.expect_identifier()
        self.expect("DELIM_L_BRACE")
        
        body = []
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            var_decl = self.parse_variable_decl()
            self.expect("DELIM_SEMICOLON")
            body.append(StructMemberNode(var_decl, self.line, self.col))
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return StructDefNode(name, body, self.line, self.col)
    
    def parse_template_def(self) -> TemplateDefNode:
        """Parse template definition - FIXED"""
        is_volatile = self.consume("KW_VOLATILE")
        
        # Handle async template sequence
        if self.consume("KW_ASYNC"):
            self.expect("KW_TEMPLATE")
            self.expect("LT")
            template_params = self.parse_template_params()
            self.expect("GT")
            return self.parse_async_template_body(template_params, is_volatile)
        
        elif self.match("KW_TEMPLATE"):
            self.advance()
            self.expect("LT")
            template_params = self.parse_template_params()
            self.expect("GT")
            
            # Check what follows the template parameters
            if self.is_identifier():
                # Function template
                return self.parse_function_template(template_params, is_volatile)
            else:
                self.error("Expected identifier after template parameters")
        
        elif self.match("KW_OBJECT"):
            return self.parse_object_template(is_volatile)
        elif self.match("KW_STRUCT"):
            return self.parse_struct_template(is_volatile)
        elif self.match("KW_OPERATOR"):
            return self.parse_operator_template(is_volatile)
        elif self.match("KW_FOR"):
            return self.parse_for_template(is_volatile)
        elif self.match("KW_SWITCH"):
            return self.parse_switch_template(is_volatile)
        else:
            self.error("Expected template definition")
    
    def parse_template_params(self) -> List[TemplateParamNode]:
        """Parse template parameters"""
        params = [TemplateParamNode(self.expect_identifier(), self.line, self.col)]
        
        while self.consume("DELIM_COMMA"):
            params.append(TemplateParamNode(self.expect_identifier(), self.line, self.col))
        
        # Handle >> as two > tokens when closing nested templates
        self.handle_rshift_in_templates()
        
        return params
    
    def parse_function_template(self, template_params: List[TemplateParamNode], is_volatile: bool) -> FunctionTemplateNode:
        """Parse function template"""
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return FunctionTemplateNode(template_params, name, parameters, return_type, body, is_volatile, self.line, self.col)
    
    def parse_object_template(self, is_volatile: bool) -> ObjectTemplateNode:
        """Parse object template"""
        self.expect("KW_OBJECT")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        
        inheritance = None
        if self.consume("DELIM_COLON"):
            inheritance = self.parse_inheritance()
        
        self.expect("DELIM_L_BRACE")
        body = []
        
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            member = self.parse_object_member()
            if member:
                body.append(member)
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return ObjectTemplateNode(template_params, name, inheritance, body, is_volatile, self.line, self.col)
    
    def parse_struct_template(self, is_volatile: bool) -> StructTemplateNode:
        """Parse struct template"""
        self.expect("KW_STRUCT")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_BRACE")
        
        body = []
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            var_decl = self.parse_variable_decl()
            self.expect("DELIM_SEMICOLON")
            body.append(StructMemberNode(var_decl, self.line, self.col))
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return StructTemplateNode(template_params, name, body, is_volatile, self.line, self.col)
    
    def parse_operator_template(self, is_volatile: bool) -> OperatorTemplateNode:
        """Parse operator template"""
        self.expect("KW_OPERATOR")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        self.expect("DELIM_L_PARENTHESIS")
        parameters = self.parse_parameter_list()
        self.expect("DELIM_R_PARENTHESIS")
        
        self.expect("DELIM_L_BRACKET")
        custom_op = self.expect_identifier()
        self.expect("DELIM_R_BRACKET")
        
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return OperatorTemplateNode(template_params, parameters, custom_op, return_type, body, is_volatile, self.line, self.col)
    
    def parse_for_template(self, is_volatile: bool) -> ForTemplateNode:
        """Parse for template"""
        self.expect("KW_FOR")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        for_init = self.parse_for_init()
        self.expect("DELIM_SEMICOLON")
        condition = self.parse_expression()
        self.expect("DELIM_SEMICOLON")
        for_update = self.parse_for_update()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_COLON")
        
        data_param = TemplateParamNode(self.expect_identifier(), self.line, self.col)
        self.expect("MULTIPLICATION")  # *
        
        self.expect("DELIM_L_BRACE")
        body = self.parse_statement_list()
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return ForTemplateNode(template_params, name, for_init, condition, for_update, data_param, body, is_volatile, self.line, self.col)
    
    def parse_async_template_body(self, template_params: List[TemplateParamNode], is_volatile: bool) -> AsyncTemplateNode:
        """Parse async template body after template parameters have been parsed"""
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return AsyncTemplateNode(template_params, name, parameters, return_type, body, is_volatile, self.line, self.col)
    
    def parse_async_template(self, is_volatile: bool) -> AsyncTemplateNode:
        """Parse async template"""
        self.expect("KW_ASYNC")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        
        parameters = []
        if not self.match("DELIM_R_PARENTHESIS"):
            parameters = self.parse_parameter_list()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return AsyncTemplateNode(template_params, name, parameters, return_type, body, is_volatile, self.line, self.col)
    
    def parse_switch_template(self, is_volatile: bool) -> SwitchTemplateNode:
        """Parse switch template"""
        self.expect("KW_SWITCH")
        self.expect("KW_TEMPLATE")
        self.expect("LT")
        template_params = self.parse_template_params()
        self.expect("GT")
        
        name = self.expect_identifier()
        self.expect("DELIM_L_PARENTHESIS")
        parameters = self.parse_parameter_list()
        self.expect("DELIM_R_PARENTHESIS")
        
        data_param = None
        if self.consume("DELIM_COLON"):
            data_param = TemplateParamNode(self.expect_identifier(), self.line, self.col)
            self.expect("MULTIPLICATION")  # *
        
        self.expect("DELIM_L_BRACE")
        cases = self.parse_case_list()
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return SwitchTemplateNode(template_params, name, parameters, data_param, cases, is_volatile, self.line, self.col)
    
    def parse_operator_def(self) -> OperatorDefNode:
        """Parse custom operator definition"""
        self.expect("KW_OPERATOR")
        self.expect("DELIM_L_PARENTHESIS")
        parameters = self.parse_parameter_list()
        self.expect("DELIM_R_PARENTHESIS")
        
        self.expect("DELIM_L_BRACKET")
        custom_op = self.expect_identifier()
        self.expect("DELIM_R_BRACKET")
        
        self.expect("RETURN_ARROW")
        return_type = self.parse_type_spec()
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return OperatorDefNode(parameters, custom_op, return_type, body, self.line, self.col)
    
    def parse_type_spec(self) -> TypeSpecNode:
        """Parse type specification - FIXED array handling"""
        # Handle multiple type qualifiers
        qualifiers = []
        while self.match("KW_CONST", "KW_VOLATILE", "KW_SIGNED", "KW_UNSIGNED"):
            qualifiers.append(self.current_token)
            self.advance()
        
        base_type = self.parse_base_type()
        
        # Handle pointer and array types
        type_spec = base_type
        
        while True:
            if self.consume("MULTIPLICATION"):  # Pointer
                type_spec = PointerTypeNode(type_spec, self.line, self.col)
            elif self.consume("DELIM_L_BRACKET"):
                # FIXED: Handle variable-sized arrays and multi-dimensional arrays
                if self.match("DELIM_L_BRACKET"):  # Check for 2D array
                    self.advance()  # consume second [
                    if not self.match("DELIM_R_BRACKET"):
                        # Variable-sized 2D array [expr][expr]
                        size_expr1 = self.parse_expression()
                    self.expect("DELIM_R_BRACKET")
                    self.expect("DELIM_L_BRACKET")
                    if not self.match("DELIM_R_BRACKET"):
                        # Variable-sized 2D array [expr][expr]
                        size_expr2 = self.parse_expression()
                    self.expect("DELIM_R_BRACKET")
                    type_spec = ArrayTypeNode(type_spec, 2, self.line, self.col)
                else:  # 1D array
                    # FIXED: Check if there's a size expression inside brackets
                    if not self.match("DELIM_R_BRACKET"):
                        # Variable-sized array [expr] - parse the expression but don't store it for now
                        # In a full implementation, ArrayTypeNode would store the size expression
                        size_expr = self.parse_expression()
                    self.expect("DELIM_R_BRACKET")
                    type_spec = ArrayTypeNode(type_spec, 1, self.line, self.col)
            else:
                break
        
        # Apply qualifiers in reverse order (rightmost qualifier is innermost)
        for qualifier in reversed(qualifiers):
            type_spec = QualifiedTypeNode(qualifier, type_spec, self.line, self.col)
        
        return type_spec
    
    def parse_base_type(self) -> TypeSpecNode:
        """Parse base type"""
        if self.match("KW_INT"):
            self.advance()
            return BaseTypeNode("int", None, self.line, self.col)
        elif self.match("KW_FLOAT"):
            self.advance()
            return BaseTypeNode("float", None, self.line, self.col)
        elif self.match("KW_VOID"):
            self.advance()
            return BaseTypeNode("void", None, self.line, self.col)
        elif self.match("KW_THIS"):
            self.advance()
            return BaseTypeNode("this", None, self.line, self.col)
        elif self.match("KW_DATA"):
            self.advance()
            self.expect("DELIM_L_BRACE")
            # Parse an expression (could be literal, sizeof(), or any expression)
            width_expr = self.parse_expression()
            self.expect("DELIM_R_BRACE")
            return BaseTypeNode("data", width_expr, self.line, self.col)
        elif self.current_token.startswith("IDENTIFIER_") and "CHAR" in self.current_token:
            self.advance()
            return BaseTypeNode("char", None, self.line, self.col)
        elif self.current_token.startswith("IDENTIFIER_") and "BOOL" in self.current_token:
            self.advance()
            return BaseTypeNode("bool", None, self.line, self.col)
        elif self.current_token.startswith("IDENTIFIER_"):
            # Handle qualified identifiers (namespace:: or object.member)
            name_parts = [self.expect_identifier()]
            
            # Check for qualification (either :: or .)
            while self.match("DELIM_SCOPE", "DOT"):
                if self.consume("DELIM_SCOPE"):
                    name_parts.append("::" + self.expect_identifier())
                elif self.consume("DOT"):
                    name_parts.append("." + self.expect_identifier())
            
            # Join the parts to form the full qualified name
            full_name = "".join(name_parts)
            
            # Check for template instantiation
            if self.consume("LT"):
                type_args = self.parse_type_list()
                self.expect("GT")
                return TemplateTypeNode(full_name, type_args, self.line, self.col)
            else:
                return BaseTypeNode(full_name, None, self.line, self.col)
        else:
            self.error(f"Expected type specifier, got {self.current_token}")
    
    def parse_type_list(self) -> List[TypeSpecNode]:
        """Parse comma-separated list of types"""
        types = [self.parse_type_spec()]
        
        while self.consume("DELIM_COMMA"):
            types.append(self.parse_type_spec())
        
        return types
    
    def parse_statement_list(self) -> List[StatementNode]:
        """Parse list of statements"""
        statements = []
        
        while not self.match("DELIM_R_BRACE") and not self.at_end():
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        
        return statements
    
    def parse_statement(self) -> Optional[StatementNode]:
        """Parse a statement"""
        if self.match("KW_IF"):
            return self.parse_if_stmt()
        elif self.match("KW_WHILE"):
            return self.parse_while_stmt()
        elif self.match("KW_DO"):
            return self.parse_do_while_stmt()
        elif self.match("KW_FOR"):
            return self.parse_for_stmt()
        elif self.match("KW_SWITCH"):
            return self.parse_switch_stmt()
        elif self.match("KW_BREAK"):
            return self.parse_break_stmt()
        elif self.match("KW_CONTINUE"):
            return self.parse_continue_stmt()
        elif self.match("KW_RETURN"):
            return self.parse_return_stmt()
        elif self.match("KW_TRY"):
            return self.parse_try_catch_stmt()
        elif self.match("KW_THROW"):
            return self.parse_throw_stmt()
        elif self.match("KW_ASSERT"):
            return self.parse_assert_stmt()
        elif self.match("KW_ASM"):
            return self.parse_asm_stmt()
        elif self.match("DELIM_L_BRACE"):
            return self.parse_block_stmt()
        elif self.match("KW_AUTO"):
            return self.parse_auto_variable_decl()
        elif self.is_likely_variable_declaration():
            # Parse as variable declaration only if it looks like one
            var_decl = self.parse_variable_decl()
            self.expect("DELIM_SEMICOLON")
            return var_decl
        else:
            # Expression statement (including function calls, assignments, etc.)
            expr = self.parse_expression()
            self.expect("DELIM_SEMICOLON")
            return ExpressionStmtNode(expr, self.line, self.col)
    
    def parse_if_stmt(self) -> IfStmtNode:
        """Parse if statement"""
        self.expect("KW_IF")
        self.expect("DELIM_L_PARENTHESIS")
        condition = self.parse_expression()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        then_body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        
        else_clause = None
        if self.consume("KW_ELSE"):
            if self.match("KW_IF"):
                # Parse else-if without expecting semicolon
                else_if = self.parse_if_stmt_no_semicolon()
                else_clause = ElseClauseNode(else_if, None, self.line, self.col)
            else:
                self.expect("DELIM_L_BRACE")
                else_body = self.parse_statement_list()
                self.expect("DELIM_R_BRACE")
                else_clause = ElseClauseNode(None, else_body, self.line, self.col)
        
        self.expect("DELIM_SEMICOLON")
        return IfStmtNode(condition, then_body, else_clause, self.line, self.col)

    def parse_if_stmt_no_semicolon(self) -> IfStmtNode:
        """Parse if statement without expecting a semicolon at the end (for else-if chains)"""
        self.expect("KW_IF")
        self.expect("DELIM_L_PARENTHESIS")
        condition = self.parse_expression()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        then_body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        
        else_clause = None
        if self.consume("KW_ELSE"):
            if self.match("KW_IF"):
                # Recursively parse else-if without expecting semicolon
                else_if = self.parse_if_stmt_no_semicolon()
                else_clause = ElseClauseNode(else_if, None, self.line, self.col)
            else:
                self.expect("DELIM_L_BRACE")
                else_body = self.parse_statement_list()
                self.expect("DELIM_R_BRACE")
                else_clause = ElseClauseNode(None, else_body, self.line, self.col)
        
        # No semicolon expected here - only the outermost if gets the semicolon
        return IfStmtNode(condition, then_body, else_clause, self.line, self.col)
    
    def parse_while_stmt(self) -> WhileStmtNode:
        """Parse while statement"""
        self.expect("KW_WHILE")
        self.expect("DELIM_L_PARENTHESIS")
        condition = self.parse_expression()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return WhileStmtNode(condition, body, self.line, self.col)
    
    def parse_do_while_stmt(self) -> DoWhileStmtNode:
        """Parse do-while statement"""
        self.expect("KW_DO")
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("KW_WHILE")
        self.expect("DELIM_L_PARENTHESIS")
        condition = self.parse_expression()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_SEMICOLON")
        
        return DoWhileStmtNode(body, condition, self.line, self.col)
    
    def parse_for_stmt(self) -> ForStmtNode:
        """Parse for statement"""
        self.expect("KW_FOR")
        self.expect("DELIM_L_PARENTHESIS")
        
        # Check for for-in loop (including destructuring)
        if self.is_identifier():
            # Look ahead to see if this is a for-in loop
            saved_pos = self.current
            iterator_vars = [self.expect_identifier()]
            
            # Check for multiple iterator variables (destructuring)
            while self.consume("DELIM_COMMA"):
                iterator_vars.append(self.expect_identifier())
            
            if self.consume("KW_IN"):
                # This is a for-in loop
                iterable = self.parse_expression()
                self.expect("DELIM_R_PARENTHESIS")
                self.expect("DELIM_L_BRACE")
                
                body = self.parse_statement_list()
                
                self.expect("DELIM_R_BRACE")
                self.expect("DELIM_SEMICOLON")
                
                # For now, pass the first iterator variable and store others in a list
                # The AST would need to be extended to properly support multiple iterator vars
                return ForStmtNode(None, None, None, body, iterator_vars[0], iterable, self.line, self.col)
            else:
                # Not a for-in loop, restore position and continue with traditional for loop
                self.current = saved_pos
                self.current_token = self.tokens[self.current]
        
        # Traditional for loop
        for_init = self.parse_for_init()
        self.expect("DELIM_SEMICOLON")
        condition = None
        if not self.match("DELIM_SEMICOLON"):
            condition = self.parse_expression()
        self.expect("DELIM_SEMICOLON")
        for_update = self.parse_for_update()
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return ForStmtNode(for_init, condition, for_update, body, None, None, self.line, self.col)
    
    def parse_for_init(self) -> Optional[ForInitNode]:
        """Parse for loop initialization"""
        if self.match("DELIM_SEMICOLON"):
            return None
        elif self.is_type_specifier():
            var_decl = self.parse_variable_decl()
            return ForInitNode(var_decl, None, self.line, self.col)
        else:
            expr = self.parse_expression()
            return ForInitNode(None, expr, self.line, self.col)
    
    def parse_for_update(self) -> Optional[ForUpdateNode]:
        """Parse for loop update"""
        if self.match("DELIM_R_PARENTHESIS"):
            return None
        else:
            expr = self.parse_expression()
            return ForUpdateNode(expr, self.line, self.col)
    
    def parse_switch_stmt(self) -> SwitchStmtNode:
        """Parse switch statement"""
        self.expect("KW_SWITCH")
        self.expect("DELIM_L_PARENTHESIS")
        expression = self.parse_expression()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        cases = self.parse_case_list()
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return SwitchStmtNode(expression, cases, self.line, self.col)
    
    def parse_case_list(self) -> List[CaseItemNode]:
        """Parse case list"""
        cases = []
        
        while self.match("KW_CASE", "KW_DEFAULT") and not self.at_end():
            cases.append(self.parse_case_item())
        
        return cases
    
    def parse_case_item(self) -> CaseItemNode:
        """Parse case item"""
        if self.consume("KW_CASE"):
            self.expect("DELIM_L_PARENTHESIS")
            case_value = None
            if not self.match("DELIM_R_PARENTHESIS"):
                case_value = self.parse_expression()
            self.expect("DELIM_R_PARENTHESIS")
            self.expect("DELIM_L_BRACE")
            
            body = self.parse_statement_list()
            
            self.expect("DELIM_R_BRACE")
            self.expect("DELIM_SEMICOLON")
            
            return CaseItemNode(case_value, body, False, self.line, self.col)
        
        elif self.consume("KW_DEFAULT"):
            self.expect("DELIM_L_BRACE")
            body = self.parse_statement_list()
            self.expect("DELIM_R_BRACE")
            self.expect("DELIM_SEMICOLON")
            
            return CaseItemNode(None, body, True, self.line, self.col)
        
        else:
            self.error("Expected case or default")
    
    def parse_break_stmt(self) -> BreakStmtNode:
        """Parse break statement"""
        self.expect("KW_BREAK")
        self.expect("DELIM_SEMICOLON")
        return BreakStmtNode(self.line, self.col)
    
    def parse_continue_stmt(self) -> ContinueStmtNode:
        """Parse continue statement"""
        self.expect("KW_CONTINUE")
        self.expect("DELIM_SEMICOLON")
        return ContinueStmtNode(self.line, self.col)
    
    def parse_return_stmt(self) -> ReturnStmtNode:
        """Parse return statement"""
        self.expect("KW_RETURN")
        expression = None
        if not self.match("DELIM_SEMICOLON"):
            expression = self.parse_expression()
        self.expect("DELIM_SEMICOLON")
        return ReturnStmtNode(expression, self.line, self.col)
    
    def parse_try_catch_stmt(self) -> TryCatchStmtNode:
        """Parse try-catch statement"""
        self.expect("KW_TRY")
        self.expect("DELIM_L_BRACE")
        
        try_body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        
        catch_clauses = []
        while self.match("KW_CATCH"):
            catch_clauses.append(self.parse_catch_clause())
        
        if not catch_clauses:
            self.error("Expected at least one catch clause")
        
        self.expect("DELIM_SEMICOLON")
        return TryCatchStmtNode(try_body, catch_clauses, self.line, self.col)
    
    def parse_catch_clause(self) -> CatchClauseNode:
        """Parse catch clause"""
        self.expect("KW_CATCH")
        self.expect("DELIM_L_PARENTHESIS")
        parameter = self.parse_parameter()
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_L_BRACE")
        
        body = self.parse_statement_list()
        
        self.expect("DELIM_R_BRACE")
        
        return CatchClauseNode(parameter, body, self.line, self.col)
    
    def parse_throw_stmt(self) -> ThrowStmtNode:
        """Parse throw statement"""
        self.expect("KW_THROW")
        expression = self.parse_expression()
        self.expect("DELIM_SEMICOLON")
        return ThrowStmtNode(expression, self.line, self.col)
    
    def parse_assert_stmt(self) -> AssertStmtNode:
        """Parse assert statement with optional message"""
        self.expect("KW_ASSERT")
        self.expect("DELIM_L_PARENTHESIS")
        expression = self.parse_expression()
        
        # Optional message parameter
        message = None
        if self.consume("DELIM_COMMA"):
            message = self.parse_expression()  # This should handle i-strings
        
        self.expect("DELIM_R_PARENTHESIS")
        self.expect("DELIM_SEMICOLON")
        return AssertStmtNode(expression, message, self.line, self.col)
    
    def parse_asm_stmt(self) -> AsmStmtNode:
        """Parse inline assembly statement"""
        self.expect("KW_ASM")
        self.expect("DELIM_L_BRACE")
        
        # Read assembly code until closing brace
        asm_code = ""
        brace_count = 1
        while brace_count > 0 and not self.at_end():
            if self.current_token == "DELIM_L_BRACE":
                brace_count += 1
            elif self.current_token == "DELIM_R_BRACE":
                brace_count -= 1
            
            if brace_count > 0:
                asm_code += self.current_token + " "
            
            self.advance()
        
        self.expect("DELIM_SEMICOLON")
        return AsmStmtNode(asm_code.strip(), self.line, self.col)
    
    def parse_block_stmt(self) -> BlockStmtNode:
        """Parse block statement"""
        self.expect("DELIM_L_BRACE")
        statements = self.parse_statement_list()
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        return BlockStmtNode(statements, self.line, self.col)
    
    def parse_variable_decl(self) -> VariableDeclNode:
        """Parse variable declaration"""
        type_spec = self.parse_type_spec()
        variables = self.parse_variable_list()
        return VariableDeclNode(type_spec, variables, False, None, None, None, self.line, self.col)
    
    def parse_auto_variable_decl(self) -> VariableDeclNode:
        """Parse auto variable declaration with destructuring"""
        self.expect("KW_AUTO")
        self.expect("DELIM_L_BRACE")
        
        target_vars = []
        target_vars.append(self.expect_identifier())
        while self.consume("DELIM_COMMA"):
            target_vars.append(self.expect_identifier())
        
        self.expect("DELIM_R_BRACE")
        self.expect("ASSIGNMENT")
        
        source_expr = self.parse_expression()
        
        self.expect("DELIM_L_BRACE")
        
        field_names = []
        field_names.append(self.expect_identifier())
        while self.consume("DELIM_COMMA"):
            field_names.append(self.expect_identifier())
        
        self.expect("DELIM_R_BRACE")
        self.expect("DELIM_SEMICOLON")
        
        return VariableDeclNode(None, [], True, target_vars, source_expr, field_names, self.line, self.col)
    
    def parse_variable_list(self) -> List[VariableInitNode]:
        """Parse comma-separated variable list"""
        variables = [self.parse_variable_init()]
        
        while self.consume("DELIM_COMMA"):
            variables.append(self.parse_variable_init())
        
        return variables
    
    def parse_variable_init(self) -> VariableInitNode:
        """Parse variable initialization"""
        name = self.expect_identifier()
        
        if self.consume("ASSIGNMENT"):
            initializer = self.parse_expression()
            return VariableInitNode(name, initializer, None, self.line, self.col)
        elif self.consume("DELIM_L_PARENTHESIS"):
            constructor_args = []
            if not self.match("DELIM_R_PARENTHESIS"):
                constructor_args = self.parse_argument_list()
            self.expect("DELIM_R_PARENTHESIS")
            return VariableInitNode(name, None, constructor_args, self.line, self.col)
        else:
            return VariableInitNode(name, None, None, self.line, self.col)
    
    def parse_expression(self) -> ExpressionNode:
        """Parse expression (assignment level)"""
        return self.parse_assignment_expr()
    
    def parse_assignment_expr(self) -> ExpressionNode:
        """Parse assignment expression"""
        expr = self.parse_conditional_expr()
        
        if self.match("ASSIGNMENT", "ADDITION_ASSIGNMENT", "SUBTRACTION_ASSIGNMENT",
                     "MULTIPLICATION_ASSIGNMENT", "DIVISION_ASSIGNMENT", "MODULO_ASSIGNMENT",
                     "EXPONENTIATION_ASSIGNMENT", "LSHIFT_ASSIGNMENT", "RSHIFT_ASSIGNMENT",
                     "BAND_ASSIGNMENT", "BNAND_ASSIGNMENT", "BOR_ASSIGNMENT", "BNOR_ASSIGNMENT",
                     "BXOR_ASSIGNMENT", "BNOT_ASSIGNMENT"):
            operator = self.current_token
            self.advance()
            right = self.parse_assignment_expr()
            return AssignmentExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_conditional_expr(self) -> ExpressionNode:
        """Parse conditional (ternary) expression"""
        expr = self.parse_logical_or_expr()
        
        if self.consume("QUESTION"):
            true_expr = self.parse_expression()
            self.expect("DELIM_COLON")
            false_expr = self.parse_conditional_expr()
            return ConditionalExprNode(expr, true_expr, false_expr, self.line, self.col)
        
        return expr
    
    def parse_logical_or_expr(self) -> ExpressionNode:
        """Parse logical OR expression"""
        expr = self.parse_logical_and_expr()
        
        while self.match("KW_OR", "OR", "NOR"):
            operator = self.current_token
            self.advance()
            right = self.parse_logical_and_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_logical_and_expr(self) -> ExpressionNode:
        """Parse logical AND expression"""
        expr = self.parse_bitwise_or_expr()
        
        while self.match("KW_AND", "AND", "NAND"):
            operator = self.current_token
            self.advance()
            right = self.parse_bitwise_or_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_bitwise_or_expr(self) -> ExpressionNode:
        """Parse bitwise OR expression"""
        expr = self.parse_bitwise_xor_expr()
        
        while self.match("BOR", "BNOR"):
            operator = self.current_token
            self.advance()
            right = self.parse_bitwise_xor_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_bitwise_xor_expr(self) -> ExpressionNode:
        """Parse bitwise XOR expression"""
        expr = self.parse_bitwise_and_expr()
        
        while self.match("XOR", "KW_XOR", "BXOR", "BXNOR"):
            operator = self.current_token
            self.advance()
            right = self.parse_bitwise_and_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_bitwise_and_expr(self) -> ExpressionNode:
        """Parse bitwise AND expression"""
        expr = self.parse_identity_expr()
        
        while self.match("BAND", "BNAND", "BXAND", "BXNAND"):
            operator = self.current_token
            self.advance()
            right = self.parse_identity_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_identity_expr(self) -> ExpressionNode:
        """Parse identity expression - FIXED to handle 'is' operator with types"""
        expr = self.parse_equality_expr()
        
        while True:
            if self.match("KW_IS"):
                # Handle 'is' operator specially - right side should be a type
                operator = self.current_token
                self.advance()
                # Parse right side as type specification and wrap in special literal
                type_spec = self.parse_type_spec()
                type_expr = LiteralExprNode(type_spec, "type", self.line, self.col)
                expr = BinaryOpExprNode(expr, operator, type_expr, self.line, self.col)
            elif self.match("KW_AS"):
                # 'as' is also a type-related operator
                operator = self.current_token
                self.advance()
                # Parse right side as type specification
                type_spec = self.parse_type_spec()
                type_expr = LiteralExprNode(type_spec, "type", self.line, self.col)
                expr = BinaryOpExprNode(expr, operator, type_expr, self.line, self.col)
            elif self.match("KW_NOT") and self.peek() != "KW_IN":
                # Only handle 'not' if it's not followed by 'in' (let relational handle 'not in')
                operator = self.current_token
                self.advance()
                right = self.parse_equality_expr()
                expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
            else:
                break
        
        return expr
    
    def parse_equality_expr(self) -> ExpressionNode:
        """Parse equality expression"""
        expr = self.parse_relational_expr()
        
        while self.match("EQ", "NEQ"):
            operator = self.current_token
            self.advance()
            right = self.parse_relational_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_relational_expr(self) -> ExpressionNode:
        """Parse relational expression - FIXED to handle 'not in' as compound operator"""
        expr = self.parse_shift_expr()
        
        while True:
            if self.match("LT", "LTEQ", "GT", "GTEQ", "KW_IN"):
                operator = self.current_token
                self.advance()
                right = self.parse_shift_expr()
                expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
            elif self.match("KW_NOT") and self.peek() == "KW_IN":
                # Handle "not in" as a compound operator
                self.advance()  # consume "not"
                self.advance()  # consume "in"
                operator = "not in"
                right = self.parse_shift_expr()
                expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
            else:
                break
        
        return expr
    
    def parse_shift_expr(self):
        expr = self.parse_range_expr()
        while self.match("LSHIFT", "RSHIFT"):
            op = self.current_token
            self.advance()
            right = self.parse_range_expr()
            expr = BinaryOpExprNode(expr, op, right, self.line, self.col)
        return expr
    
    def parse_additive_expr(self) -> ExpressionNode:
        """Parse additive expression"""
        expr = self.parse_multiplicative_expr()
        
        while self.match("ADDITION", "SUBTRACTION"):
            operator = self.current_token
            self.advance()
            right = self.parse_multiplicative_expr()
            expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_multiplicative_expr(self) -> ExpressionNode:
        """Parse multiplicative expression"""
        expr = self.parse_exponential_expr()
        
        while True:
            if self.match("MULTIPLICATION", "DIVISION", "MODULO"):
                operator = self.current_token
                self.advance()
                right = self.parse_exponential_expr()
                expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
            elif self.current_token.startswith("IDENTIFIER_"):
                # Custom operator (identifier used as binary operator)
                operator = self.expect_identifier()
                right = self.parse_exponential_expr()
                expr = BinaryOpExprNode(expr, operator, right, self.line, self.col)
            else:
                break
        
        return expr
    
    def parse_exponential_expr(self) -> ExpressionNode:
        """Parse exponential expression"""
        expr = self.parse_cast_expr()
        
        if self.match("EXPONENTIATION"):
            operator = self.current_token
            self.advance()
            right = self.parse_exponential_expr()  # Right associative
            return BinaryOpExprNode(expr, operator, right, self.line, self.col)
        
        return expr
    
    def parse_cast_expr(self) -> ExpressionNode:
        """Parse cast expression"""
        if self.match("DELIM_L_PARENTHESIS"):
            # Look ahead to see if this is a cast
            saved_pos = self.current
            saved_token = self.current_token
            
            self.advance()  # consume (
            
            # Try to parse as a type specification
            is_cast = False
            try:
                self.parse_type_spec()
                if self.match("DELIM_R_PARENTHESIS"):
                    is_cast = True
            except:
                is_cast = False
            
            # Restore position to before the (
            self.current = saved_pos
            self.current_token = saved_token
            
            if is_cast:
                # Parse as cast expression
                self.expect("DELIM_L_PARENTHESIS")
                target_type = self.parse_type_spec()
                self.expect("DELIM_R_PARENTHESIS")
                expr = self.parse_cast_expr()
                return CastExprNode(target_type, expr, self.line, self.col)
        
        return self.parse_unary_expr()
    
    def parse_unary_expr(self) -> ExpressionNode:
        """Parse unary expression"""
        if self.match("INCREMENT", "DECREMENT", "ADDITION", "SUBTRACTION", "NOT", 
                     "ADDRESS_OF", "MULTIPLICATION", "KW_NOT", "KW_AWAIT"):
            operator = self.current_token
            self.advance()
            expr = self.parse_unary_expr()
            return UnaryOpExprNode(operator, expr, False, self.line, self.col)
        elif self.match("KW_SIZEOF", "KW_TYPEOF"):
            # Special handling for sizeof and typeof - they can take type specifiers
            operator = self.current_token
            self.advance()
            
            if self.consume("DELIM_L_PARENTHESIS"):
                # Check if this is a type specifier or expression
                if self.is_type_specifier():
                    # Parse as type specifier
                    type_spec = self.parse_type_spec()
                    self.expect("DELIM_R_PARENTHESIS")
                    # Create a special literal node to represent the type
                    type_literal = LiteralExprNode(type_spec, "type", self.line, self.col)
                    return UnaryOpExprNode(operator, type_literal, False, self.line, self.col)
                else:
                    # Parse as expression
                    expr = self.parse_expression()
                    self.expect("DELIM_R_PARENTHESIS")
                    return UnaryOpExprNode(operator, expr, False, self.line, self.col)
            else:
                # No parentheses, parse as regular unary operator on expression
                expr = self.parse_unary_expr()
                return UnaryOpExprNode(operator, expr, False, self.line, self.col)
        
        return self.parse_postfix_expr()
    
    def parse_postfix_expr(self) -> ExpressionNode:
        """Parse postfix expression - FIXED to handle template instantiation and template calls"""
        expr = self.parse_primary_expr()
        
        while True:
            if self.consume("DELIM_L_BRACKET"):
                index = self.parse_expression()
                self.expect("DELIM_R_BRACKET")
                expr = ArrayAccessExprNode(expr, index, self.line, self.col)
            elif self.consume("DELIM_L_PARENTHESIS"):
                arguments = []
                if not self.match("DELIM_R_PARENTHESIS"):
                    arguments = self.parse_argument_list()
                self.expect("DELIM_R_PARENTHESIS")
                expr = FunctionCallExprNode(expr, arguments, self.line, self.col)
                
                # Check for template call syntax: template<types>(args) : data_param
                if self.consume("DELIM_COLON"):
                    data_param = self.parse_unary_expr()  # Parse the data parameter (usually @variable or *variable)
                    # Create a special template call - use FunctionCallExprNode with special marker
                    # We'll store the data parameter in a way that indicates this is a template call
                    template_call = FunctionCallExprNode(expr, [data_param], self.line, self.col)
                    expr = template_call
                    
            elif self.consume("DOT") or self.consume("DELIM_DOT"):
                member = self.expect_identifier()
                expr = MemberAccessExprNode(expr, member, False, self.line, self.col)
                
                # FIXED: Check for template instantiation after member access
                if self.match("LT") and self.is_template_instantiation_lookahead():
                    self.advance()  # consume LT
                    type_args = self.parse_type_list()
                    self.expect("GT")
                    # Create template instantiation expression with the member access as base
                    expr = TemplateInstantiationExprNode(f"{expr.__class__.__name__}.{member}", type_args, self.line, self.col)
                    
            elif self.consume("DELIM_SCOPE"):
                member = self.expect_identifier()
                expr = MemberAccessExprNode(expr, member, True, self.line, self.col)
                
                # FIXED: Check for template instantiation after scope access
                if self.match("LT") and self.is_template_instantiation_lookahead():
                    self.advance()  # consume LT
                    type_args = self.parse_type_list()
                    self.expect("GT")
                    # Create template instantiation expression with the scope access as base
                    expr = TemplateInstantiationExprNode(f"{expr.__class__.__name__}::{member}", type_args, self.line, self.col)
                    
            elif self.match("INCREMENT", "DECREMENT"):
                operator = self.current_token
                self.advance()
                expr = UnaryOpExprNode(operator, expr, True, self.line, self.col)
            else:
                break
        
        # FIXED: Handle template call syntax: template<types> : data_param
        # This handles cases like: matrixProcessor<Matrix<float, int, int>, float> : @transformation_matrix;
        if self.match("DELIM_COLON") and isinstance(expr, TemplateInstantiationExprNode):
            self.advance()  # consume DELIM_COLON
            data_param = self.parse_unary_expr()  # Parse the data parameter (usually @variable or *variable)
            # Create a function call that represents the template call with data parameter
            expr = FunctionCallExprNode(expr, [data_param], self.line, self.col)
        
        return expr
    
    def parse_primary_expr(self) -> ExpressionNode:
        """Parse primary expression - FIXED to handle type keywords as identifiers"""
        if self.match("INT_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "int", self.line, self.col)
        elif self.match("FLOAT_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "float", self.line, self.col)
        elif self.match("HEX_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "int", self.line, self.col)  # Hex literals are integers
        elif self.match("BIN_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "data", self.line, self.col)  # Binary literals are data
        elif self.match("OCT_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "int", self.line, self.col)  # Octal literals are integers
        elif self.match("STRING_LITERAL"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "string", self.line, self.col)
        elif self.match("I_STRING"):
            # Handle I_STRING first, before other checks
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "string", self.line, self.col)
        elif self.match("KW_TRUE", "KW_FALSE"):
            value = self.current_token
            self.advance()
            return LiteralExprNode(value, "bool", self.line, self.col)
        elif self.match("KW_THIS"):
            self.advance()
            return ThisExprNode(self.line, self.col)
        elif self.match("KW_SUPER"):
            # Handle super keyword for parent class access
            self.advance()
            return IdentifierExprNode("super", self.line, self.col)
        elif self.current_token == "IDENTIFIER__":
            self.advance()
            return IteratorExprNode(self.line, self.col)
        elif self.current_token.startswith("IDENTIFIER_"):
            name = self.expect_identifier()
            
            # Check for template instantiation - use lookahead to distinguish from comparison
            if self.match("LT") and self.is_template_instantiation_lookahead():
                self.advance()  # consume LT
                type_args = self.parse_type_list()
                self.expect("GT")
                return TemplateInstantiationExprNode(name, type_args, self.line, self.col)
            else:
                return IdentifierExprNode(name, self.line, self.col)
        # FIXED: Handle type keywords as identifiers in expression contexts
        elif self.match("KW_INT", "KW_FLOAT", "KW_VOID", "KW_DATA"):
            # Type keywords can be used as identifiers in certain expression contexts
            # like: typeof(x) is float, sizeof(int), etc.
            type_name = self.current_token.replace("KW_", "")
            self.advance()
            return IdentifierExprNode(type_name, self.line, self.col)
        elif self.consume("DELIM_L_BRACE"):
            # Handle brace initialization (e.g., {1, 2} for structs)
            elements = []
            if not self.match("DELIM_R_BRACE"):
                elements = self.parse_expression_list()
            self.expect("DELIM_R_BRACE")
            return ArrayLiteralExprNode(elements, self.line, self.col)  # Reuse ArrayLiteral for brace init
        elif self.consume("DELIM_L_PARENTHESIS"):
            expr = self.parse_expression()
            self.expect("DELIM_R_PARENTHESIS")
            return expr
        elif self.consume("DELIM_L_BRACKET"):
            # Try to detect array comprehension
            expr = self.parse_expression()

            if self.match("KW_FOR"):
                self.advance()
                self.expect("DELIM_L_PARENTHESIS")
                iterator_var = self.expect_identifier()
                self.expect("KW_IN")
                iterable_expr = self.parse_expression()
                self.expect("DELIM_R_PARENTHESIS")

                # Optional `if` clause
                condition_expr = None
                if self.consume("KW_IF"):
                    self.expect("DELIM_L_PARENTHESIS")
                    condition_expr = self.parse_expression()
                    self.expect("DELIM_R_PARENTHESIS")

                self.expect("DELIM_R_BRACKET")
                return ArrayComprehensionExprNode(
                    expr, iterator_var, iterable_expr, condition_expr,
                    self.line, self.col
                )
            else:
                # Standard array literal
                elements = [expr]
                while self.consume("DELIM_COMMA"):
                    elements.append(self.parse_expression())
                self.expect("DELIM_R_BRACKET")
                return ArrayLiteralExprNode(elements, self.line, self.col)
        else:
            self.error(f"Unexpected token in expression: {self.current_token}")
    
    def parse_i_string_expr(self) -> IStringExprNode:
        """Parse interpolated string expression"""
        self.expect("I_STRING")
        # Parse the i-string format from the lexer
        # This is a simplified version - in practice, you'd parse the actual format
        string_literal = "placeholder"
        expressions = []
        return IStringExprNode(string_literal, expressions, self.line, self.col)
    
    def parse_expression_list(self) -> List[ExpressionNode]:
        """Parse comma-separated expression list"""
        if self.match("DELIM_R_BRACKET", "DELIM_R_PARENTHESIS", "DELIM_R_BRACE"):
            return []
        
        expressions = [self.parse_expression()]
        
        while self.consume("DELIM_COMMA"):
            expressions.append(self.parse_expression())
        
        return expressions
    
    def parse_argument_list(self) -> List[ExpressionNode]:
        """Parse function call argument list"""
        return self.parse_expression_list()

    def parse_range_expr(self) -> ExpressionNode:
        expr = self.parse_additive_expr()
        while self.match("RANGE"):
            self.advance()
            right = self.parse_additive_expr()
            expr = RangeExprNode(expr, right, self.line, self.col)
        return expr
    
    # Helper methods
    def is_type_specifier(self) -> bool:
        """Check if current token starts a type specifier"""
        return self.match("KW_INT", "KW_FLOAT", "KW_VOID", "KW_DATA", "KW_CONST", 
                         "KW_VOLATILE", "KW_SIGNED", "KW_UNSIGNED") or \
               self.current_token.startswith("IDENTIFIER_")
    
    def is_likely_variable_declaration(self) -> bool:
        """Check if current position looks like a variable declaration using lookahead - FIXED"""
        # Definite type keywords - ALWAYS variable declarations
        if self.current_token in ["KW_INT", "KW_FLOAT", "KW_VOID", "KW_DATA", 
                                 "KW_CONST", "KW_VOLATILE", "KW_SIGNED", "KW_UNSIGNED"]:
            return True
        
        # For identifiers, use lookahead to distinguish declarations from expressions
        if self.current_token.startswith("IDENTIFIER_"):
            # FIXED: Check for template call patterns first
            if self.is_template_call_lookahead():
                return False  # This is a template call, not a declaration
            
            next_token = self.peek(1)
            
            # If followed by another identifier, likely declaration: Type var
            if next_token and next_token.startswith("IDENTIFIER_"):
                third_token = self.peek(2)
                if third_token in ["ASSIGNMENT", "DELIM_SEMICOLON", "DELIM_COMMA", "DELIM_L_PARENTHESIS"]:
                    return True
                # Avoid custom operators like "counter custom_add counter"
                if (third_token and 
                    (third_token.startswith("IDENTIFIER_") or 
                     third_token in ["INT_LITERAL", "FLOAT_LITERAL", "STRING_LITERAL"])):
                    return False
                return True
            
            # Type followed by pointer/array syntax: Type* var, Type[] var
            if next_token in ["MULTIPLICATION", "DELIM_L_BRACKET"]:
                return True
                
            # Qualified types: namespace::Type var
            if next_token == "DELIM_SCOPE":
                return True
                
            # Template types: Type<T> var (but not template calls: Type<T> : data)
            if next_token == "LT":
                # Use the more sophisticated lookahead to check if this is a template call
                if self.is_template_call_lookahead():
                    return False  # Template call, not declaration
                return True  # Template type declaration
                
            # Member types: Container.Type var
            if next_token in ["DOT", "DELIM_DOT"]:
                member_type = self.peek(2)
                var_name = self.peek(3)
                if (member_type and member_type.startswith("IDENTIFIER_") and
                    var_name and var_name.startswith("IDENTIFIER_")):
                    return True
            
            return False
        
        return False
    
    def is_identifier(self) -> bool:
        """Check if current token is an identifier"""
        return self.current_token.startswith("IDENTIFIER_")
    
    def expect_identifier(self) -> str:
        """Expect an identifier and return its name"""
        if not self.is_identifier():
            self.error(f"Expected identifier, got {self.current_token}")
        
        # Extract identifier name from token
        identifier = self.current_token.replace("IDENTIFIER_", "")
        self.advance()
        return identifier


def parse(source_code: str) -> ProgramNode:
    """Parse Flux source code and return AST"""
    # Tokenize and capture position information
    lexer = Lexer(source_code)
    
    # Monkey patch the Token class to capture positions
    from flexer3 import Token, token_list
    original_init = Token.__init__
    token_positions = []
    
    def new_init(self, t_index, arg):
        original_init(self, t_index, arg)
        # Capture position when token is created
        token_positions.append((lexer.line, lexer.col))
    
    Token.__init__ = new_init
    
    # Clear and lex
    token_list.clear()
    lexer.lex()
    
    # Restore original Token.__init__
    Token.__init__ = original_init
    
    # Extract the properly mapped token names
    actual_tokens = [token.token for token in token_list]
    
    # Parse with position information
    parser = Parser(actual_tokens, token_positions)
    parser.source_code = source_code
    parser.lexer = lexer
    return parser.parse()


if __name__ == "__main__":
    # Test with a Flux program from file
    try:
        with open("./master_example2.fx", "r") as file:
            source_code = file.read()
        
        ast = parse(source_code)
        print("Parsing successful!")
        print(f"AST has {len(ast.global_items)} global items")
        
        # Print some basic info about the AST
        for i, item in enumerate(ast.global_items):
            print(f"Global item {i}: {type(item).__name__}")
            
    except FileNotFoundError:
        print("Error: master_example2.fx file not found")
    except ParseError as e:
        print(f"Parse error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")