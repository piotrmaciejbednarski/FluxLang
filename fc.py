"""
Flux Compiler with Full Toolchain Integration
"""

import sys
import os
import subprocess
from pathlib import Path
from llvmlite import ir
from flexer import FluxLexer
from fparser import FluxParser, ParseError
from fast import Program

class FluxCompiler:
    def __init__(self, /, verbosity: int = None):
        self.verbosity = int(verbosity)
        self.module = ir.Module(name="flux_module")
        self.module.triple = "x86_64-pc-linux-gnu"
        #self.module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
        self.temp_files = []

    def compile_file(self, filename: str, output_bin: str = None) -> str:
        try:
            # 1. Parse and generate LLVM IR
            with open(filename, 'r') as f:
                source = f.read()
            
            lexer = FluxLexer(source)
            tokens = lexer.tokenize()

            if self.verbosity == 0:
                print(tokens)
            
            parser = FluxParser(tokens)
            ast = parser.parse()

            if self.verbosity == 1:
                print(ast)
            
            self.module = ast.codegen(self.module)
            llvm_ir = str(self.module)

            if self.verbosity == 2:
                print(llvm_ir)
            
            # Create temp directory
            base_name = Path(filename).stem
            temp_dir = Path(f"flux_build_{base_name}")
            temp_dir.mkdir(exist_ok=True)
            
            # 2. Generate LLVM IR file
            ll_file = temp_dir / f"{base_name}.ll"
            with open(ll_file, 'w') as f:
                f.write(llvm_ir)
            self.temp_files.append(ll_file)
            
            # 3. Compile to assembly
            asm_file = temp_dir / f"{base_name}.s"
            subprocess.run([
                "llc",
                "-O2",               # Enable optimizations
                str(ll_file),
                "-o", str(asm_file)
            ], check=True)
            self.temp_files.append(asm_file)

            if self.verbosity == 3:
                with open(asm_file, "r") as f:
                    f.seek(0)
                    print(f.read())
            
            # 4. Assemble to object file
            obj_file = temp_dir / f"{base_name}.o"
            subprocess.run([
                "as",
                "--64",              # Force 64-bit mode
                str(asm_file),
                "-o", str(obj_file)
            ], check=True)
            self.temp_files.append(obj_file)

            if self.verbosity == 4:
                print(tokens)
                print(ast)
                print(llvm_ir)
                with open(asm_file, "r") as f:
                    f.seek(0)
                    print(f.read())
            
            # 5. Link executable
            output_bin = output_bin or f"./{base_name}"
            subprocess.run([
                "gcc",
                "-no-pie",          # Disable position-independent executable
                str(obj_file),
                "-o", output_bin
            ], check=True)
            
            print(f"Successfully built: {output_bin}")
            return output_bin
            
        except Exception as e:
            self.cleanup()
            print(f"Compilation failed: {e}", file=sys.stderr)
            sys.exit(1)
    
    def cleanup(self):
        """Remove temporary files"""
        for f in self.temp_files:
            try:
                if os.path.exists(f):
                    os.remove(f)
            except:
                pass

def main():
    if len(sys.argv) < 2:
        print("Usage: python fc.py input.fx [output_binary] ...arguments...\n\n")
        print("\tArguments:\n")
        print("\t\t-vX\tVerbose output. X = 0..4\n")
        print("\t\t\t\t0: Tokens")
        print("\t\t\t\t1: AST")
        print("\t\t\t\t2: LLVM IR")
        print("\t\t\t\t3: ASM")
        print("\t\t\t\t4: Everything")
        sys.exit(1)

    input_file = None
    output_bin = None

    if len(sys.argv) == 2:
        input_file = sys.argv[1]
        output_bin = sys.argv[2] if len(sys.argv) > 2 else None

    verbosity = None

    if len(sys.argv) > 2:
        input_file = sys.argv[1]
        output_bin = sys.argv[2] if len(sys.argv) > 2 else None
        for arg in sys.argv:
            match (arg.lower()[0:2]):
                case ("-v"):
                    match (arg.lower()[2:]):
                        case ("0"):
                            verbosity = int(arg[2:])
                        case ("1"):
                            verbosity = int(arg[2:])
                        case ("2"):
                            verbosity = int(arg[2:])
                        case ("3"):
                            verbosity = int(arg[2:])
                        case ("4"):
                            verbosity = int(arg[2:])
    
    if not input_file.endswith('.fx'):
        print("Error: Input file must have .fx extension", file=sys.stderr)
        sys.exit(1)
    
    compiler = FluxCompiler(verbosity=verbosity)
    try:
        binary_path = compiler.compile_file(input_file, output_bin)
        print(f"Executable created at: {binary_path}")
    finally:
        compiler.cleanup()

if __name__ == "__main__":
    main()