# Flux Language Documentation

## Table of Contents
1. Language Overview
2. Basic Syntax 
3. Type System
4. Memory Model
5. Object System
6. Standard Library
7. Operator Reference
8. Complete Grammar
9. Decay Rules

## Language Overview
Flux is a statically-typed systems programming language combining C++'s low-level control, Zig's bit-width feature, and Python's readability.

Example program:
```
import "standard.fx" as std;
using std::io;

def main() -> i32
{
    print("Hello World!");
    return 0;
};
```

## Basic Syntax

### Program Structure
All Flux programs require a main function:
```
def main() -> i32
{
    // Program entry point
    return 0;
};
```

### Variables
Variable declaration:
```
i32 count = 0;
f64 pi = 3.14159;
string name = "Flux";
```

### Control Flow
Conditionals:
```
if (x > 0)
{
    print("Positive");
}
else
{
    print("Non-positive");
};
```

Loops:
```
for (i in 0..10)
{
    print(i);
};
```

## Type System

### Primitive Types

`data`: Binary data type

### Complex Types
Struct:
```
struct Point
{
    i32 x;
    i32 y;
};
```

Object:
```
object Counter
{
    i32 count = 0;
    
    def increment() -> void
    {
        this.count++;
    };
};
```

## Memory Model

### Pointers
Pointer operations:
```
i32 x = 42;
i32* ptr = @x;
*ptr = 100;
```

### Memory Management
Manual control:
```
data{1024} buffer;        // 1KB buffer
void* raw = (void)buffer; // Null memory
```

## Object System

### Objects
```
object Vehicle
{
    string name;
    
    def __init(string n) -> this
    {
        this.name = n;
        return;
    };

    def __exit() -> void
    {
        return;
    };
};
```

### Inheritance
```
object Car : Vehicle
{
    i32 wheels = 4;
    
    def __init() -> this
    {
        super.Vehicle("Sedan");
        return;
    };
};
```

## Standard Library

### Core Modules
```
import "standard.fx" as std;
using std::io, std::math;
```

### Common Functions
```
print("Message");
i32 max = math.max(10, 20);
```

## Operator Reference

### Arithmetic
```
i32 sum = 5 + 3;
f64 div = 10.0 / 3.0;
```

### Bitwise
```
i32 flags = 0b1010 | 0b1100;
```

# Flux Language Decay Rules

Based on C++ decay rules, adapted for Flux's type system:

### 1. Array-to-Pointer Decay
- **Rule**: Array types decay to pointer types when used in expressions (except when operand of `sizeof`, `typeof`, address-of `@`, or when initializing array references)
- **Example**: `i32[] arr` → `i32*` when passed to functions or used in pointer arithmetic

### 2. Function-to-Function Pointer Decay
- **Rule**: Function names decay to function pointers when used in expressions (except when operand of address-of `@`)
- **Example**: `def foo() -> void {}` → function name `foo` decays to `void*()`

### 3. Template Function Decay
- **Rule**: Template function names decay to template function pointers
- **Example**: `template <T> myFunc(T x) -> T` → `template*<T> (T)`

### 4. Object Instance Decay
- **Rule**: Object instances decay to pointers when passed to functions expecting pointers
- **Example**: `MyObject obj` → `MyObject*` in pointer contexts

### 5. Data Type Decay
- **Rule**: Specific `data{n}` types decay to generic `data` type in generic contexts
- **Example**: `data{32} x` → `data` when type information is lost

### 6. Const/Volatile Qualifier Decay
- **Rule**: Top-level const and volatile qualifiers are ignored in function parameter types
- **Example**: `const i32` parameter decays to `i32` for function matching

### 7. Reference Collapse (for auto deduction)
- **Rule**: When `auto` is used, reference-to-reference collapses
- **Example**: `auto& x = someRef` where `someRef` is already a reference

### 8. String Literal Decay
- **Rule**: String literals decay to `data{8}*` (pointer to bytes)
- **Example**: `"hello"` → `data{8}*`

### 9. Binary Literal Decay
- **Rule**: Binary literals `{0,1,0,1}` decay to `data{n}` where n is the bit count
- **Example**: `{0,1,0,1,0,0,0,1}` → `data{8}`

### 10. Template Parameter Decay
- **Rule**: Template type parameters undergo standard decay when instantiated
- **Example**: In `template <T> func(T x)`, if `T` is `i32[]`, it decays to `i32*`

### 11. Namespace Scope Decay
- **Rule**: Qualified names decay to unqualified names in their scope
- **Example**: Within namespace `std`, `std::io` can decay to `io`

### 12. Void Pointer Decay
- **Rule**: All pointer types can decay to `void*` for generic storage
- **Example**: `i32*` → `void*`, `MyObject*` → `void*`

### 13. Inheritance Decay (Upcasting)
- **Rule**: Derived object pointers decay to base object pointers
- **Example**: `ChildObject*` → `ParentObject*` (implicit upcast)

### 14. Array Comprehension Decay
- **Rule**: Array comprehensions decay to their element type arrays
- **Example**: `[x for x in range]` → appropriate array type based on expression type

### Application Order
1. Template instantiation decay (if applicable)
2. Array/function decay
3. Const/volatile decay
4. Type promotion/conversion decay
5. Pointer decay (if needed)

These decay rules ensure type safety while allowing flexible usage patterns similar to C++,
but adapted for Flux's unique features like the `data` type system and object model.