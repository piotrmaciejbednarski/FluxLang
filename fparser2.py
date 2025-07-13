#!/usr/bin/env python3
"""
Flux Language Parser - fparser.py

Complete recursive descent parser for the Flux programming language.
Transforms token stream from flexer.py into AST using fast.py node definitions.

Usage:
    python3 fparser.py file.fx          # Basic parsing
    python3 fparser.py file.fx -v       # Verbose output with AST details
    python3 fparser.py file.fx -a       # Show AST tree structure
    python3 fparser.py file.fx -c       # Count AST nodes
"""

import sys
from typing import List, Optional, Union, Dict, Tuple, Any
from dataclasses import dataclass

# Import the lexer and AST modules
from flexer import FluxLexer, Token, TokenType
from fast import *

class ParseError(Exception):
    """Exception raised when parsing fails"""
    def __init__(self, message: str, token: Optional[Token] = None):
        self.message = message
        self.token = token
        super().__init__(self.format_message())
    
    def format_message(self) -> str:
        if self.token:
            return f"Parse error at line {self.token.line}, column {self.token.column}: {self.message}"
        return f"Parse error: {self.message}"

class FluxParser:
    """Expectation-based parser for Flux language"""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.current = 0
        self.length = len(tokens)
    
    def current_token(self) -> Optional[Token]:
        """Get current token"""
        if self.current >= self.length:
            return None
        return self.tokens[self.current]
    
    def peek_token(self, offset: int = 1) -> Optional[Token]:
        """Peek ahead at token"""
        pos = self.current + offset
        if pos >= self.length:
            return None
        return self.tokens[pos]
    
    def advance(self) -> Token:
        """Advance to next token and return current"""
        token = self.current_token()
        if self.current < self.length:
            self.current += 1
        return token
    
    # Core expectation method
    def expect(self, token_type: TokenType, message: str = "") -> Token:
        """Expect specific token type - consumes and returns the token"""
        current = self.current_token()
        if not current:
            raise ParseError(f"Expected {token_type.name} but reached end of file")
        
        if current.type != token_type:
            if not message:
                message = f"Expected {token_type.name}, got {current.type.name}"
            raise ParseError(message, current)
        
        return self.advance()
    
    # ============================================================================
    # Top-level parsing
    # ============================================================================
    
    def parse(self) -> Program:
        """Parse the entire program"""
        program = Program()
        
        while not self.match(TokenType.EOF):
            try:
                global_item = self.parse_global_item()
                if global_item:
                    program.global_items.append(global_item)
            except ParseError as e:
                print(f"Error: {e}", file=sys.stderr)
                self.synchronize()
        
        return program

    