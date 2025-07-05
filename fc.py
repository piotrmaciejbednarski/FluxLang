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
        
    def _init_types(self):
        """Initialize built-in types"""
        return {
            'int': FluxType('int', ir.IntType(32)),
            'float': FluxType('float', ir.FloatType()),
            'char': FluxType('char', ir.IntType(8)),
            'bool': FluxType('bool', ir.IntType(1)),
            'void': FluxType('void', ir.VoidType()),
        }
    
    def _get_llvm_type(self, type_name: str) -> ir.Type:
        """Get LLVM type from Flux type name"""
        if type_name in self.types:
            return self.types[type_name].llvm_type
        raise ValueError(f"Unknown type: {type_name}")
    
    def program(self, items):
        """Handle program node"""
        for item in items:
            pass  # Items are processed as they're encountered
        return self.module
    
    def function_definition(self, items):
        """Handle function definition"""
        # Parse function components
        volatile = False
        idx = 0
        
        if items[idx] == "volatile":
            volatile = True
            idx += 1
        
        # Skip 'def' keyword
        idx += 1
        
        func_name = str(items[idx])
        idx += 1
        
        # Skip template params if present
        if idx < len(items) and isinstance(items[idx], list):
            idx += 1
        
        # Skip opening paren
        idx += 1
        
        # Parse parameters
        params = []
        if idx < len(items) and isinstance(items[idx], list):
            params = items[idx]
            idx += 1
        
        # Skip closing paren and arrow
        idx += 2
        
        # Get return type
        return_type = self._get_llvm_type(str(items[idx]))
        idx += 1
        
        # Create function type
        param_types = [self._get_llvm_type(p[0]) for p in params] if params else []
        func_type = ir.FunctionType(return_type, param_types)
        
        # Create function
        func = ir.Function(self.module, func_type, name=func_name)
        self.current_function = func
        
        # Create entry block
        block = func.append_basic_block(name="entry")
        self.builder = ir.IRBuilder(block)
        
        # Add parameters to symbol table
        for i, (param_type, param_name) in enumerate(params or []):
            param_val = func.args[i]
            param_val.name = param_name
            self.symbols[param_name] = FluxSymbol(
                param_name, 
                self.types[param_type], 
                param_val
            )
        
        # Process function body (statements between braces)
        body_start = idx + 1  # Skip opening brace
        statements = []
        brace_count = 1
        
        for i in range(body_start, len(items)):
            if items[i] == "{":
                brace_count += 1
            elif items[i] == "}":
                brace_count -= 1
                if brace_count == 0:
                    break
            elif brace_count == 1:
                statements.append(items[i])
        
        # Process statements
        for stmt in statements:
            if stmt is not None:
                self.visit(stmt)
        
        # Add return void if no return statement
        if not self.builder.block.is_terminated:
            if return_type == ir.VoidType():
                self.builder.ret_void()
            else:
                # Return zero/default value
                if isinstance(return_type, ir.IntType):
                    self.builder.ret(ir.Constant(return_type, 0))
                elif isinstance(return_type, ir.FloatType):
                    self.builder.ret(ir.Constant(return_type, 0.0))
        
        return func
    
    def parameter_list(self, items):
        """Handle parameter list"""
        return items
    
    def parameter(self, items):
        """Handle single parameter"""
        type_spec = str(items[0])
        param_name = str(items[1])
        return (type_spec, param_name)
    
    def variable_declaration(self, items):
        """Handle variable declaration"""
        type_spec = str(items[0])
        declarators = items[1] if len(items) > 1 else []
        
        for declarator in declarators:
            if isinstance(declarator, tuple):
                var_name, init_expr = declarator
            else:
                var_name = str(declarator)
                init_expr = None
            
            # Allocate variable
            var_type = self._get_llvm_type(type_spec)
            var_ptr = self.builder.alloca(var_type, name=var_name)
            
            # Initialize if provided
            if init_expr is not None:
                init_val = self.visit(init_expr)
                self.builder.store(init_val, var_ptr)
            
            # Add to symbol table
            self.symbols[var_name] = FluxSymbol(
                var_name,
                self.types[type_spec],
                var_ptr
            )
    
    def declarator_list(self, items):
        """Handle declarator list"""
        return items
    
    def declarator(self, items):
        """Handle single declarator"""
        if len(items) == 1:
            return str(items[0])
        else:
            # Variable with initializer
            var_name = str(items[0])
            init_expr = items[2]  # Skip '=' token
            return (var_name, init_expr)
    
    def return_statement(self, items):
        """Handle return statement"""
        if len(items) > 1:  # Has return value
            ret_val = self.visit(items[1])
            self.builder.ret(ret_val)
        else:
            self.builder.ret_void()
    
    def if_statement(self, items):
        """Handle if statement"""
        condition = self.visit(items[2])  # Skip 'if' and '('
        
        # Create blocks
        then_block = self.current_function.append_basic_block(name="if_then")
        else_block = self.current_function.append_basic_block(name="if_else")
        merge_block = self.current_function.append_basic_block(name="if_merge")
        
        # Branch based on condition
        self.builder.cbranch(condition, then_block, else_block)
        
        # Generate then block
        self.builder.position_at_end(then_block)
        self.visit(items[4])  # Then statement
        if not self.builder.block.is_terminated:
            self.builder.branch(merge_block)
        
        # Generate else block
        self.builder.position_at_end(else_block)
        if len(items) > 6:  # Has else clause
            self.visit(items[6])  # Else statement
        if not self.builder.block.is_terminated:
            self.builder.branch(merge_block)
        
        # Continue from merge block
        self.builder.position_at_end(merge_block)
    
    def while_statement(self, items):
        """Handle while statement"""
        # Create blocks
        cond_block = self.current_function.append_basic_block(name="while_cond")
        body_block = self.current_function.append_basic_block(name="while_body")
        exit_block = self.current_function.append_basic_block(name="while_exit")
        
        # Jump to condition
        self.builder.branch(cond_block)
        
        # Generate condition block
        self.builder.position_at_end(cond_block)
        condition = self.visit(items[2])  # Skip 'while' and '('
        self.builder.cbranch(condition, body_block, exit_block)
        
        # Generate body block
        self.builder.position_at_end(body_block)
        self.visit(items[4])  # Body statement
        if not self.builder.block.is_terminated:
            self.builder.branch(cond_block)
        
        # Continue from exit block
        self.builder.position_at_end(exit_block)
    
    def assignment_expression(self, items):
        """Handle assignment expression"""
        if len(items) == 1:
            return self.visit(items[0])
        
        # Assignment: identifier = expression
        var_name = str(items[0])
        expr_val = self.visit(items[2])  # Skip '=' token
        
        if var_name in self.symbols:
            var_ptr = self.symbols[var_name].llvm_value
            self.builder.store(expr_val, var_ptr)
            return expr_val
        else:
            raise ValueError(f"Undefined variable: {var_name}")
    
    def additive_expression(self, items):
        """Handle addition and subtraction"""
        if len(items) == 1:
            return self.visit(items[0])
        
        left = self.visit(items[0])
        op = str(items[1])
        right = self.visit(items[2])
        
        if op == '+':
            return self.builder.add(left, right)
        elif op == '-':
            return self.builder.sub(left, right)
    
    def multiplicative_expression(self, items):
        """Handle multiplication, division, and modulo"""
        if len(items) == 1:
            return self.visit(items[0])
        
        left = self.visit(items[0])
        op = str(items[1])
        right = self.visit(items[2])
        
        if op == '*':
            return self.builder.mul(left, right)
        elif op == '/':
            return self.builder.sdiv(left, right)
        elif op == '%':
            return self.builder.srem(left, right)
    
    def equality_expression(self, items):
        """Handle equality and inequality"""
        if len(items) == 1:
            return self.visit(items[0])
        
        left = self.visit(items[0])
        op = str(items[1])
        right = self.visit(items[2])
        
        if op == '==':
            return self.builder.icmp_signed('==', left, right)
        elif op == '!=':
            return self.builder.icmp_signed('!=', left, right)
    
    def relational_expression(self, items):
        """Handle relational operators"""
        if len(items) == 1:
            return self.visit(items[0])
        
        left = self.visit(items[0])
        op = str(items[1])
        right = self.visit(items[2])
        
        if op == '<':
            return self.builder.icmp_signed('<', left, right)
        elif op == '>':
            return self.builder.icmp_signed('>', left, right)
        elif op == '<=':
            return self.builder.icmp_signed('<=', left, right)
        elif op == '>=':
            return self.builder.icmp_signed('>=', left, right)
    
    def unary_expression(self, items):
        """Handle unary expressions"""
        if len(items) == 1:
            return self.visit(items[0])
        
        op = str(items[0])
        operand = self.visit(items[1])
        
        if op == '+':
            return operand
        elif op == '-':
            return self.builder.neg(operand)
        elif op == '!':
            return self.builder.not_(operand)
    
    def identifier(self, items):
        """Handle identifier"""
        name = str(items[0])
        if name in self.symbols:
            symbol = self.symbols[name]
            # Load value if it's a pointer (variable)
            if isinstance(symbol.llvm_value.type, ir.PointerType):
                return self.builder.load(symbol.llvm_value)
            else:
                return symbol.llvm_value
        else:
            raise ValueError(f"Undefined identifier: {name}")
    
    def integer_literal(self, items):
        """Handle integer literal"""
        return ir.Constant(ir.IntType(32), int(items[0]))
    
    def float_literal(self, items):
        """Handle float literal"""
        return ir.Constant(ir.FloatType(), float(items[0]))
    
    def boolean_literal(self, items):
        """Handle boolean literal"""
        value = str(items[0]) == 'true'
        return ir.Constant(ir.IntType(1), value)
    
    def primitive_type(self, items):
        """Handle primitive type"""
        return str(items[0])
    
    def type_specifier(self, items):
        """Handle type specifier"""
        return self.visit(items[0])

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