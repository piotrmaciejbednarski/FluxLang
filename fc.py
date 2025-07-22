"""
Flux Compiler with LLVM IR Generation

Usage:
    python fc.py input.fx > output.ll
"""

import sys
from pathlib import Path
from llvmlite import ir
from flexer import FluxLexer
from fparser import FluxParser, ParseError
from fast import Program
from typing import Optional

class FluxCompiler:
    def __init__(self):
      self.module = ir.Module(name="flux_module")
      self.module.triple = "x86_64-pc-linux-gnu"  # Adjust as needed
      self.module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
    
    def compile_file(self, filename: str) -> str:
        try:
            with open(filename, 'r') as f:
                source = f.read()
            
            lexer = FluxLexer(source)
            tokens = lexer.tokenize()
            
            parser = FluxParser(tokens)
            ast = parser.parse()
            
            self.module = ast.codegen(self.module)
            
            return str(self.module)
        
        except FileNotFoundError:
            print(f"Error: File '{filename}' not found", file=sys.stderr)
            sys.exit(1)
        except ParseError as e:
            print(f"Parse error: {e}", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Compilation error: {e}", file=sys.stderr)
            sys.exit(1)

def main():
    if len(sys.argv) != 2:
        print("Usage: python fc.py input.fx > output.ll", file=sys.stderr)
        sys.exit(1)
    
    input_file = sys.argv[1]
    if not input_file.endswith('.fx'):
        print("Error: Input file must have .fx extension", file=sys.stderr)
        sys.exit(1)
    
    compiler = FluxCompiler()
    llvm_ir = compiler.compile_file(input_file)
    print(llvm_ir)

if __name__ == "__main__":
    main()