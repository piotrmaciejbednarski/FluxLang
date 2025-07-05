# Flux Programming Language
## A Systems Programming Language for the Modern Era

---

## Philosophy

**"Swiss Army Chainsaw"** - Flux is designed to be incredibly powerful and precise, but requires skill to wield effectively. We don't limit your capabilities for your own good. We give you the tools to solve problems efficiently.

Flux eliminates the language decision paralysis:
- Need performance? ✓
- Need rapid prototyping? ✓
- Need bit manipulation? ✓
- Need memory control? ✓
- Need high-level expressiveness? ✓

**Write it in Flux.**

---

## Core Principles

What if we created a language based on everything we know now?

### 1. **Clarity Over Brevity**
Every important construct gets braces. Every statement gets a semicolon. No special cases.

```flux
// Always clear where blocks begin and end
if (condition) 
{
    doSomething();
};

// Even single statements
if (x > 0) { return x; };
```

### 2. **Explicit Over Implicit**
What you write is what happens. No hidden behavior, no magic.

```flux
// Memory management is explicit
int* ptr = malloc(sizeof(int));
*ptr = 42;
(void)ptr;  // Explicit free - dangerous but clear
```

### 3. **Power Without Apology**
You're trusted as a programmer to be competent.

---

## Getting Started

### Your First Program

```flux
import "standard.fx";
using standard::io;

def main() -> int
{
    print("Hello, Flux!");
    return 0;
};
```

### Basic Syntax

#### Variables and Types
```flux
// Standard types
int x = 42;
float pi = 3.14159;
bool isTrue = true;

// Custom bit-width types
unsigned data{8} as byte;
signed data{13:16} as weird_int;  // 13-bit wide, 16-bit aligned

byte myByte = 0xFF;
weird_int myWeird = 1000;
```

Note: `bool` is a 1-bit wide type. It is unlike C++'s bool which requires an entire byte to store.

#### Functions
```flux
def add(int a, int b) -> int
{
    return a + b;
};

// Function overloading
def add(float a, float b) -> float
{
    return a + b;
};

// Templates
def maximum<T>(T a, T b) -> T
{
    return (a > b) ? a : b;
};
```

---

## Key Features

### The `data` Type System
Flux's killer feature - precise control over memory layout:

```flux
// Create custom types with exact bit widths
unsigned data{24:32:0} as rgb_pixel;  // 24-bit, 32-bit aligned, little-endian

// Perfect for hardware registers
struct GPIO_Register
{
    unsigned data{1} as pin0;
    unsigned data{1} as pin1;
    unsigned data{6} as reserved;
    unsigned data{8} as control;
};

// Memory-mapped I/O
volatile const GPIO_Register* gpio = @0x40000000;
gpio.pin0 = 1;  // Set pin 0 high
```

### String Interpolation
Two powerful approaches:

```flux
// f-strings (familiar)
string name = "World";
string msg = f"Hello {name}!";

// i-strings (first-class functions, great for logging)
print(i"User {} logged in at {}" : {
    username;
    getCurrentTime();
});
```

### Objects and Structs
```flux
// Objects have behavior
object Vector2D
{
    float x, y;
    
    def __init(float x, float y) -> this
    {
        this.x = x;
        this.y = y;
        return this;
    };
    
    def length() -> float
    {
        return sqrt(x*x + y*y);
    };
};

// Structs are pure data
struct Point
{
    float x, y;
};

Vector2D vec(3.0, 4.0);
print(f"Length: {vec.length()}");  // 5.0
```

### Memory Management
```flux
// Manual but explicit
int* numbers = malloc(sizeof(int) * 10);
numbers[0] = 42;

// Explicit free
(void)numbers;  // Frees the memory immediately

// Stack allocation
int localVar = 100;
(void)localVar;  // Also works - frees stack memory early
```

### Error Handling
```flux
// Traditional exceptions
object FileError
{
    string message;
    
    def __init(string msg) -> this
    {
        this.message = msg;
        return this;
    };
};

def readFile(string filename) -> string
{
    if (!fileExists(filename))
    {
        FileError error("File not found");
        throw(error);
    };
    // ... read file
};

// Usage
try
{
    string content = readFile("data.txt");
    print(content);
}
catch (FileError e)
{
    print(f"Error: {e.message}");
};
```

### Compile-Time Programming
```flux
// Macros
def PI 3.14159;
def DEBUG_MODE true;

// Compile-time execution
compt
{
    if (def(DEBUG_MODE))
    {
        def LOG_LEVEL 3;
    }
    else
    {
        def LOG_LEVEL 1;
    };
};

// Compile-time functions
compt def factorial(int n) -> int
{
    if (n <= 1) { return 1; };
    return n * factorial(n - 1);
};

// Computed at compile time
const int fact_10 = factorial(10);
```

---

## Advanced Features

### Pointers and References
```flux
int value = 42;
int* ptr = @value;      // Get address
*ptr = 100;             // Dereference
print(value);           // 100

// Function pointers
int (*op)(int, int) = @add;
int result = (*op)(5, 3);  // 8
```

### Templates
```flux
// Function templates
def swap<T>(T* a, T* b) -> void
{
    T temp = *a;
    *a = *b;
    *b = temp;
};

// Object templates
object Container<T>
{
    T* data;
    int size;
    
    def __init(int capacity) -> this
    {
        this.data = malloc(sizeof(T) * capacity);
        this.size = 0;
        return this;
    };
};
```

### Inline Assembly
```flux
def optimizedCopy(void* dest, void* src, int size) -> void
{
    asm
    {
        mov eax, [src]
        mov ebx, [dest]
        mov ecx, [size]
        rep movsb
    };
};
```

### Lambdas
```flux
// Basic lambda
auto square = [](int x) { return x * x; };

// With capture
int multiplier = 10;
auto scale = [multiplier](int x) { return x * multiplier; };

// Usage
int result = square(5);     // 25
int scaled = scale(3);      // 30
```

---

## Best Practices

### 1. Embrace Explicitness
```flux
// Good: Clear intent
if (ptr != void) { *ptr = value; };

// Bad: Implicit behavior
if (ptr) { *ptr = value; };
```

### 2. Use the Type System
```flux
// Good: Precise types
unsigned data{8} as StatusRegister;
unsigned data{16:32} as NetworkPort;

// Less good: Generic types
int status;
int port;
```

### 3. Handle Errors Appropriately
```flux
// For recoverable errors
try
{
    result = riskyOperation();
}
catch (OperationError e)
{
    // Handle gracefully
};

// For programmer errors
assert(ptr != void, "Null pointer passed to function");
```

### 4. Leverage Compile-Time Features
```flux
// Generate lookup tables at compile time
compt
{
    def generateSinTable() -> float[]
    {
        float[] table = new float[360];
        for (int i = 0; i < 360; i++)
        {
            table[i] = sin(i * PI / 180.0);
        };
        return table;
    };
};

const float[] SIN_TABLE = generateSinTable();
```

---

## Common Patterns

### RAII with Objects
```flux
object FileHandle
{
    void* file;
    
    def __init(string filename) -> this
    {
        this.file = fopen(filename, "r");
        if (this.file == void)
        {
            throw("Failed to open file");
        };
        return this;
    };
    
    def __exit() -> void
    {
        if (this.file != void)
        {
            fclose(this.file);
        };
    };
};

// Automatic cleanup
{
    FileHandle handle("data.txt");
    // Use handle...
}  // __exit() called automatically
```

### Bit Manipulation
```flux
// Define custom operators for clarity
def SET_BIT `|
def CLEAR_BIT `&
def TOGGLE_BIT `^^

unsigned data{8} as Register;

Register status = 0x00;
status SET_BIT 0x01;      // Set bit 0
status CLEAR_BIT 0xFE;    // Clear bit 0
status TOGGLE_BIT 0x01;   // Toggle bit 0
```

### Generic Programming
```flux
// Generic container
object Array<T>
{
    T* data;
    int size, capacity;
    
    def push(T item) -> void
    {
        if (size >= capacity)
        {
            resize(capacity * 2);
        };
        data[size++] = item;
    };
    
    def get(int index) -> T
    {
        assert(index >= 0 && index < size, "Index out of bounds");
        return data[index];
    };
};

Array<int> numbers;
numbers.push(42);
numbers.push(100);
```

---

## Performance Considerations

### Memory Layout
```flux
// Tightly packed structs
struct NetworkPacket
{
    unsigned data{8} as type;
    unsigned data{16} as length;
    unsigned data{8}[256] as payload;
};  // Exactly 275 bytes

// Aligned for performance
struct CacheAligned
{
    unsigned data{64:64} as value;  // 64-byte aligned
};
```

### Compile-Time Optimization
```flux
// Pre-compute expensive operations
compt def fibonacci(int n) -> int
{
    if (n <= 1) { return n; };
    return fibonacci(n-1) + fibonacci(n-2);
};

// Computed once at compile time
const int[] FIB_SEQUENCE = [
    fibonacci(0), fibonacci(1), fibonacci(2), 
    fibonacci(3), fibonacci(4), fibonacci(5)
];
```

### Zero-Cost Abstractions
```flux
// High-level interface with zero runtime cost
def forEach<T>(T[] array, void (*func)(T)) -> void
{
    for (int i = 0; i < len(array); i++)
    {
        (*func)(array[i]);
    };
};

// Inlined completely
forEach(numbers, @print);
```

---

## Safety Guidelines

### Memory Safety
```flux
// Always check pointers
if (ptr != void)
{
    *ptr = value;
};

// Use RAII patterns
object SafeBuffer
{
    void* data;
    
    def __init(int size) -> this
    {
        this.data = malloc(size);
        return this;
    };
    
    def __exit() -> void
    {
        if (this.data != void)
        {
            (void)this.data;  // Explicit free
        };
    };
};
```

### Thread Safety
```flux
// Use explicit synchronization
volatile int shared_counter = 0;

def increment() -> void
{
    asm { lock inc [shared_counter] };  // Atomic increment
};
```

---

## Conclusion

Flux gives you the power to write efficient, expressive systems code without artificial limitations. It trusts you to make the right decisions while providing the tools to make those decisions safely and clearly.

The learning curve is steep, but the payoff is immense: **one language that can handle everything from bit manipulation to high-level abstractions, from bare-metal programming to application development.**

Welcome to Flux. Use it wisely.

---

*"Here's a Swiss Army chainsaw. Don't cut your leg off."*