# Flux Programming Language

> **A modern systems programming language that combines the best of C++, Zig, and Python with stricter syntax and powerful metaprogramming capabilities.**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Language](https://img.shields.io/badge/language-Python-blue)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange)]()

---

## 🚀 What is Flux?

Flux is a revolutionary programming language designed for systems programming, performance-critical applications, and complex software architectures. It draws inspiration from C++, Zig, and Python while introducing innovative features for type safety, metaprogramming, and developer productivity.

**Named after flux (solder paste) used in electrical engineering** - programming in Flux is clearly good for your computer! ⚡

---

## ✨ Key Features

### 🎯 **Type System**
- **Variable-width primitive types**: `data{n}` for custom bit-width data types
- **Strict type safety** with compile-time checks
- **Template metaprogramming** with advanced constraints
- **Automatic type inference** with `auto` destructuring

### 🏗️ **Object-Oriented Programming**
- **Multiple inheritance** with inclusion/exclusion syntax
- **Magic methods** for operator overloading
- **Template objects** for generic programming
- **Nested objects** and complex hierarchies

### ⚡ **Advanced Templates**
- **Function templates** with multiple type parameters
- **Object templates** for generic data structures
- **Operator templates** for custom operator definitions
- **Control flow templates**: `for template`, `switch template`
- **Async templates** for concurrent programming

### 🛠️ **Systems Programming**
- **Inline assembly** support
- **Manual memory management** with pointers
- **Volatile and const qualifiers**
- **Direct hardware register access**
- **Zero-cost abstractions**

### 🎨 **Modern Language Features**
- **Interpolated strings**: `i"Hello {}":{name;}`
- **Array comprehensions**: `[x^2 for (x in 1..10)]`
- **Destructuring assignment**: `auto {x, y} = point{x, y}`
- **Custom operators**: Define your own operators like `matrix_mul`
- **Async/await** for concurrent programming

---

## 📋 Language Overview

### Basic Syntax

```flux
import "standard.fx" as std;
using std::io, std::types;

def main() -> int
{
    // Variable declarations
    int x = 42;
    float y = 3.14159;
    data{64} timestamp = getCurrentTime();
    
    // Interpolated strings
    string message = i"Value: {}, Time: {}":{x; timestamp;};
    print(message);
    
    return 0;
};
```

### Object-Oriented Programming

```flux
object Vector3D
{
    float x, y, z;
    
    def __init(float x, float y, float z) -> this
    {
        this.x = x;
        this.y = y; 
        this.z = z;
        return;
    };
    
    def __add(this other) -> this
    {
        return this(this.x + other.x, this.y + other.y, this.z + other.z);
    };
};
```

### Templates and Generics

```flux
template <T> findMax(T[] array) -> T
{
    T max_val = array[0];
    for (T item in array)
    {
        if (item > max_val)
        {
            max_val = item;
        };
    };
    return max_val;
};

// Usage
int[] numbers = [5, 2, 8, 1, 9];
int maximum = findMax<int>(numbers);
```

### Custom Operators

```flux
operator(Matrix a, Matrix b)[matrix_mul] -> Matrix
{
    // Matrix multiplication implementation
    return multiply(a, b);
};

// Usage
Matrix result = matrixA matrix_mul matrixB;
```

---

## 🏗️ Architecture

Flux is implemented as a multi-stage compiler/interpreter with the following components:

```
Source Code (.fx)
       ↓
📝 Lexer (flexer3.py)      → Tokens
       ↓
🌳 Parser (fparser3.py)    → Abstract Syntax Tree
       ↓
🔍 Type Checker (ftyper.py) → Semantically Valid AST
       ↓
📦 LLVM Integration → Compile Flux code and bootstrap fc.fx (the Flux Compiler)
       ↓
⚡ The Flux Capacitor → Compile Flux to native binaries on multiple systems
```

### Current Status

| Component | Status | Description |
|-----------|--------|-------------|
| **Lexer** | ✅ Complete | Tokenizes Flux source code |
| **AST** | ✅ Complete | Comprehensive AST node definitions |
| **Parser** | ✅ Complete | Recursive descent parser for all language features |
| **Type Checker** | 🚧 In Progress | Semantic analysis and type checking |
| **LLVM Integration** | 📅 Planned | Compile Flux code |
| **Native Flux Compiler** | 📅 Planned | Bootstrap the Flux Capacitor (Compiler) |

---

## 🚀 Getting Started

### Prerequisites

- Python 3.8 or higher
- Git

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/flux-lang.git
cd flux-lang

# Run a Flux program
python3 fparser3.py
```

### Running Examples

```bash
# Parse the comprehensive test file
python3 fparser3.py
# Output: Parsing successful! AST has 20 global items

# Try your own Flux code
echo 'def main() -> int { return 42; };' > hello.fx
python3 -c "
from fparser3 import parse_flux_program
with open('hello.fx') as f:
    ast = parse_flux_program(f.read())
print('Parsed successfully!')
"
```

---

## 📚 Language Reference

### Keywords
```
and, as, asm, assert, async, auto, await, break, case, catch, const, continue, data,
def, default, do, else, enum, false, float, for, if, import, in, int, is, namespace,
not, object, operator, or, return, signed, sizeof, struct, super, switch, template,
this, throw, true, try, typeof, unsigned, using, void, volatile, while, xor
```

### Built-in Types
- **Integers**: `int` (platform-dependent), `signed`/`unsigned` qualifiers
- **Floating-point**: `float` (platform-dependent)
- **Characters**: `char`
- **Binary data**: `data{n}` (custom bit-width)
- **Arrays**: `type[]`, `type[][]` (multi-dimensional)
- **Pointers**: `type*`

### Control Flow
- **Conditionals**: `if`/`else if`/`else`
- **Loops**: `while`, `do-while`, `for`, `for-in`
- **Pattern matching**: `switch`/`case`
- **Exception handling**: `try`/`catch`/`throw`

---

## 🎯 Roadmap

### Phase 1: Core Language (Current)
- [x] Lexical analysis
- [x] Syntax parsing
- [x] AST generation
- [ ] Type checking and semantic analysis

### Phase 2: Code Generation
- [ ] Bytecode generation
- [ ] Virtual machine implementation
- [ ] Standard library

### Phase 3: Optimization
- [ ] LLVM backend integration
- [ ] Self-hosting compiler
- [ ] Performance optimizations

### Phase 4: Ecosystem
- [ ] Package manager
- [ ] IDE tooling and language server
- [ ] Documentation generator
- [ ] Testing framework

---

## 🤝 Contributing

We welcome contributions to Flux! Here's how you can help:

### Getting Involved
1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Make your changes** and add tests
4. **Commit your changes**: `git commit -m 'Add amazing feature'`
5. **Push to the branch**: `git push origin feature/amazing-feature`
6. **Open a Pull Request**

### Areas We Need Help
- 🔍 **Type Checker Implementation** - Core semantic analysis
- 📦 **Bytecode Generator** - Code generation backend
- 🖥️ **Virtual Machine** - Runtime execution engine
- 📚 **Documentation** - Language guides and tutorials
- 🧪 **Testing** - Comprehensive test suite
- 🛠️ **Tooling** - IDE support, syntax highlighting

### Development Guidelines
- Follow the existing code style and patterns
- Add comprehensive tests for new features
- Update documentation for language changes
- Ensure all tests pass before submitting PRs

---

## 📖 Examples

Check out the comprehensive language test in [`master_example2.fx`](master_example2.fx) which demonstrates:

- Complex object hierarchies with templates
- Multiple inheritance patterns
- Advanced template metaprogramming
- Custom operator definitions
- Async programming patterns
- Low-level systems programming features

---

## 🔧 Development Setup

```bash
# Development dependencies
pip install -r requirements-dev.txt

# Run tests
python -m pytest tests/

# Type checking
mypy flux/

# Code formatting
black flux/
```

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- Inspired by the design philosophies of C++, Zig, and Python
- Special thanks to the systems programming community
- Built with passion for developer productivity and performance

---

## 📞 Contact

- **GitHub Issues**: [Report bugs and request features](https://github.com/yourusername/flux-lang/issues)
- **Discussions**: [Join the community discussions](https://github.com/yourusername/flux-lang/discussions)

---

<div align="center">

**⚡ Flux - Where Performance Meets Productivity ⚡**

[Getting Started](#-getting-started) • [Documentation](#-language-reference) • [Contributing](#-contributing) • [Examples](#-examples)

</div>
