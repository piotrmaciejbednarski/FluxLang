#!/usr/bin/env python3
"""
Flux Compiler with Hello World ASM Support
Handles:
  - def main() -> int { return 0; }
  - asm { "syscall" } with proper constraints
"""
from dataclasses import dataclass
from typing import List, Union, Optional
from flexer import FluxLexer, TokenType, Token
from fparser import *
from fast import *