#!/usr/bin/env python3
"""
Flux Language Compiler - A Python implementation using Lark parser and LLVM
Loads grammar from paste.txt and compiles Flux language to executable code.
"""

import sys
import os
from typing import Dict, List, Any, Optional, Union
from lark import Lark, Transformer, v_args, Token, Tree
from llvmlite import ir, binding
import llvmlite.binding as llvm
import ctypes

class FluxType:
    """Represents a Flux type"""
    def __init__(self, name: str, llvm_type: ir.Type):
        self.name = name
        self.llvm_type = llvm_type

class FluxSymbol:
    """Represents a symbol in the symbol table"""
    def __init__(self, name: str, flux_type: FluxType, llvm_value: Any = None):
        self.name = name
        self.flux_type = flux_type
        self.llvm_value = llvm_value

class FluxCompiler(Transformer):
    """Transforms Flux AST to LLVM IR"""
    
    def __init__(self):
        self.module = ir.Module(name="flux_module")
        self.builder = None
        self.current_function = None
        self.symbols = {}  # Symbol table
        self.types = self._init_types()
        self.global_vars = {}  # Global variables
        
    def _init_types(self):
        """Initialize built-in types"""
        return {
            'int': FluxType('int', ir.IntType(32)),
            'float': FluxType('float', ir.FloatType()),
            'char': FluxType('char', ir.IntType(8)),
            'bool': FluxType('bool', ir.IntType(1)),
            'void': FluxType('void', ir.VoidType()),
            'voidtype': FluxType('voidtype', ir.VoidType()),
        }
    
    def _get_llvm_type(self, type_name: str) -> ir.Type:
        """Get LLVM type from Flux type name"""
        if type_name in self.types:
            return self.types[type_name].llvm_type
        raise ValueError(f"Unknown type: {type_name}")
    
    def _get_token_value(self, item):
        """Extract value from token or tree node"""
        if isinstance(item, Token):
            return item.value
        elif isinstance(item, Tree):
            return self.visit(item)
        else:
            return str(item)
    
    def _extract_tokens_from_tree(self, tree):
        """Extract all tokens from a tree recursively"""
        if isinstance(tree, Token):
            return [tree]
        elif isinstance(tree, Tree):
            tokens = []
            for child in tree.children:
                tokens.extend(self._extract_tokens_from_tree(child))
            return tokens
        else:
            return []
    
    # Top-level rules
    def program(self, items):
        """Handle program node"""
        for item in items:
            if item is not None:
                self.visit(item)
        return self.module
    
    def top_level_statement(self, items):
        """Handle top-level statement"""
        if items:
            return self.visit(items[0])
        return None
    
    # Variable declarations
    def global_variable_declaration(self, items):
        """Handle global variable declaration"""
        return self.visit(items[0])
    
    def variable_declaration(self, items):
        """Handle variable declaration"""
        type_node = items[0]
        declarator_list = items[1]
        
        # Get type name - handle both Tree and direct string
        if isinstance(type_node, Tree):
            type_name = self.visit(type_node)
        else:
            type_name = str(type_node)
        
        # Process declarators
        declarators = self.visit(declarator_list)
        
        # Rest of the method remains the same...
        # Handle single declarator vs list
        if not isinstance(declarators, list):
            declarators = [declarators]
        
        for declarator in declarators:
            if isinstance(declarator, tuple):
                var_name, init_expr = declarator
                init_val = init_expr
            else:
                var_name = declarator
                init_val = None
            
            if self.builder is None:
                # Global variable
                var_type = self._get_llvm_type(type_name)
                if init_val is not None:
                    global_var = ir.GlobalVariable(self.module, var_type, name=var_name)
                    global_var.initializer = init_val
                else:
                    global_var = ir.GlobalVariable(self.module, var_type, name=var_name)
                    global_var.initializer = ir.Constant(var_type, 0)
                
                self.global_vars[var_name] = global_var
            else:
                # Local variable
                var_type = self._get_llvm_type(type_name)
                var_ptr = self.builder.alloca(var_type, name=var_name)
                
                if init_val is not None:
                    self.builder.store(init_val, var_ptr)
                
                self.symbols[var_name] = FluxSymbol(
                    var_name,
                    self.types[type_name],
                    var_ptr
                )
        
        return None
    
    def variable_declarator_list(self, items):
        """Handle variable declarator list"""
        return [self.visit(item) for item in items]
    
    def variable_declarator(self, items):
        """Handle single variable declarator"""
        if len(items) == 1:
            return self.visit(items[0])
        else:
            # Variable with initializer
            var_name = self.visit(items[0])
            init_expr = self.visit(items[1])  # Process the assignment expression
            return (var_name, init_expr)
    
    # Types
    def primitive_type(self, items):
        """Handle primitive type"""
        if not items:
            return 'int'  # Default fallback
        
        item = items[0]
        if isinstance(item, Token):
            return item.value
        elif isinstance(item, str):
            return item
        else:
            return str(item)
    
    def type_specifier(self, items):
        """Handle type specifier"""
        # Handle both direct type names and type nodes
        if isinstance(items[0], str):
            return items[0]
        return self.visit(items[0])

    def ret(self, value=None):
        """Generate a return instruction"""
        if value is None:
            return self.builder.ret_void()
        else:
            return self.builder.ret(value)
    
    # Function definitions
    def function_definition(self, items):
        """Handle function definition"""
        # Extract function components
        func_name = None
        return_type = 'int'  # Default return type
        body = None
        
        for item in items:
            if isinstance(item, Token) and item.type == 'IDENTIFIER':
                func_name = item.value
            elif isinstance(item, Tree) and item.data == 'function_body':
                body = item
            elif isinstance(item, (str, Tree)) and (isinstance(item, str) or item.data in ['primitive_type', 'type_specifier']):
                return_type = self.visit(item) if isinstance(item, Tree) else item

        # Create function
        llvm_return_type = self._get_llvm_type(return_type)
        func_type = ir.FunctionType(llvm_return_type, [])
        func = ir.Function(self.module, func_type, name=func_name)

        # Create entry block and set builder
        entry_block = func.append_basic_block(name="entry")
        old_builder = self.builder
        self.builder = ir.IRBuilder(entry_block)

        # Process function body
        if body:
            self.visit(body)

        # Ensure function has terminator
        if not self.builder.block.is_terminated:
            if llvm_return_type == ir.VoidType():
                self.builder.ret_void()
            else:
                self.builder.ret(ir.Constant(llvm_return_type, 0))

        # Restore previous builder
        self.builder = old_builder
        return func
    
    def function_body(self, items):
        """Handle function body"""
        for item in items:
            if item is not None:
                self.visit(item)
    
    # Statements
    def statement(self, items):
        """Handle statement"""
        if items:
            return self.visit(items[0])
        return None
    
    def return_statement(self, items):
        """Handle return statement"""
        if len(items) > 0:
            ret_val = self.visit(items[0])
            self.builder.ret(ret_val)
        else:
            self.builder.ret_void()

    def int_type(self, items):
        """Handle int type"""
        return 'int'

    def float_type(self, items):
        """Handle float type"""
        return 'float'

    def char_type(self, items):
        """Handle char type"""
        return 'char'

    def bool_type(self, items):
        """Handle bool type"""
        return 'bool'

    # Expressions - create a chain of delegation
    def assignment_expression(self, items):
        """Handle assignment expression"""
        return self.visit(items[0])
    
    def conditional_expression(self, items):
        """Handle conditional expression"""
        return self.visit(items[0])
    
    def logical_or_expression(self, items):
        """Handle logical OR expression"""
        return self.visit(items[0])
    
    def logical_and_expression(self, items):
        """Handle logical AND expression"""
        return self.visit(items[0])
    
    def inclusive_or_expression(self, items):
        """Handle inclusive OR expression"""
        return self.visit(items[0])
    
    def exclusive_or_expression(self, items):
        """Handle exclusive OR expression"""
        return self.visit(items[0])
    
    def and_expression(self, items):
        """Handle AND expression"""
        return self.visit(items[0])
    
    def equality_expression(self, items):
        """Handle equality expression"""
        return self.visit(items[0])
    
    def relational_expression(self, items):
        """Handle relational expression"""
        return self.visit(items[0])
    
    def shift_expression(self, items):
        """Handle shift expression"""
        return self.visit(items[0])
    
    def additive_expression(self, items):
        """Handle additive expression"""
        return self.visit(items[0])
    
    def multiplicative_expression(self, items):
        """Handle multiplicative expression"""
        return self.visit(items[0])
    
    def cast_expression(self, items):
        """Handle cast expression"""
        return self.visit(items[0])
    
    def unary_expression(self, items):
        """Handle unary expression"""
        return self.visit(items[0])
    
    def postfix_expression(self, items):
        """Handle postfix expression"""
        return self.visit(items[0])
    
    def primary_expression(self, items):
        """Handle primary expression"""
        return self.visit(items[0])
    
    # Literals
    def literal(self, items):
        """Handle literal values"""
        if not items:
            raise ValueError("Empty literal")
        
        item = items[0]
        
        # Handle case where we already have an LLVM constant
        if hasattr(item, 'type') and isinstance(item.type, ir.IntType):
            return item
        
        # Handle tokens and other cases
        if isinstance(item, Token):
            if item.type == 'NUMBER':
                return ir.Constant(ir.IntType(32), int(item.value))
            elif item.type == 'FLOAT':
                return ir.Constant(ir.FloatType(), float(item.value))
            elif item.type == 'STRING':
                return ir.Constant(ir.IntType(8), ord(item.value[1]))
            elif item.type == 'CHAR':
                return ir.Constant(ir.IntType(8), ord(item.value[1]))
            elif item.value == 'true':
                return ir.Constant(ir.IntType(1), 1)
            elif item.value == 'false':
                return ir.Constant(ir.IntType(1), 0)
        
        # Handle as string value
        value = str(item)
        if value.isdigit():
            return ir.Constant(ir.IntType(32), int(value))
        elif value in ['true', 'false']:
            return ir.Constant(ir.IntType(1), value == 'true')
        
        raise ValueError(f"Unknown literal: {item}")
    
    # Handle terminals (tokens) that reach the transformer
    def IDENTIFIER(self, token):
        """Handle identifier tokens"""
        return token.value
    
    def NUMBER(self, token):
        """Handle number tokens"""
        return ir.Constant(ir.IntType(32), int(token.value))
    
    def FLOAT(self, token):
        """Handle float tokens"""
        return ir.Constant(ir.FloatType(), float(token.value))

    def visit(self, node):
        """Visit a node in the parse tree"""
        if isinstance(node, Tree):
            return self.transform(node)
        elif isinstance(node, Token):
            return node.value
        return node

def load_grammar(filename: str) -> str:
    """Load grammar from file"""
    try:
        with open(filename, 'r') as f:
            return f.read()
    except FileNotFoundError:
        print(f"Error: Grammar file '{filename}' not found.")
        sys.exit(1)

def compile_flux(source_code: str, grammar_file: str = 'flux_lark.bnf.txt') -> ir.Module:
    """Compile Flux source code to LLVM IR"""
    # Load grammar
    grammar = load_grammar(grammar_file)
    
    # Create parser
    parser = Lark(grammar, parser='lalr', start='program')
    
    # Parse source code
    try:
        tree = parser.parse(source_code)
        print("Parse tree:")
        print(tree.pretty())
    except Exception as e:
        print(f"Parse error: {e}")
        return None
    
    # Create compiler and transform to LLVM
    compiler = FluxCompiler()
    try:
        module = compiler.transform(tree)
        return module
    except Exception as e:
        print(f"Compilation error: {e}")
        import traceback
        traceback.print_exc()
        return None

def execute_llvm_module(module: ir.Module):
    """Execute LLVM module using JIT compilation"""
    # Initialize LLVM
    llvm.initialize()
    llvm.initialize_native_target()
    llvm.initialize_native_asmprinter()
    
    # Create execution engine
    target = llvm.Target.from_default_triple()
    target_machine = target.create_target_machine()
    
    # Create module and add our IR
    llvm_module = llvm.parse_assembly(str(module))
    
    # Create execution engine
    with llvm.create_mcjit_compiler(llvm_module, target_machine) as ee:
        ee.finalize_object()
        
        # Look for main function
        try:
            main_func = ee.get_function_address("main")
            print(f"Executing main function at address: {hex(main_func)}")
            
            # Create callable function
            func_type = ctypes.CFUNCTYPE(ctypes.c_int)
            main_callable = func_type(main_func)
            
            # Execute
            result = main_callable()
            print(f"Program returned: {result}")
            return result
        except Exception as e:
            print(f"Execution error: {e}")
            return None

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python3 ./fc.py <source_file>")
        sys.exit(1)
    
    source_file = sys.argv[1]
    
    # Read source code
    try:
        with open(source_file, 'r') as f:
            source_code = f.read()
    except FileNotFoundError:
        print(f"Error: Source file '{source_file}' not found.")
        sys.exit(1)
    
    print(f"Compiling {source_file}...")
    
    # Compile
    module = compile_flux(source_code)
    if module is None:
        print("Compilation failed.")
        sys.exit(1)
    
    print("Generated LLVM IR:")
    print(str(module))
    
    # Execute
    print("\nExecuting...")
    execute_llvm_module(module)

if __name__ == "__main__":
    # Example usage if run directly
    example_code = """
    def main() -> int {
        int x = 10;
        int y = 20;
        int result = x + y;
        return result;
    };
    """
    
    if len(sys.argv) == 1:
        print("Example compilation:")
        module = compile_flux(example_code)
        if module:
            print("Generated LLVM IR:")
            print(str(module))
            print("\nExecuting...")
            execute_llvm_module(module)
    else:
        main()