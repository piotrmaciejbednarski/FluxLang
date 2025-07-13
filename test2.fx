// Comprehensive Flux Program - Demonstrating All Keywords
// This program showcases every keyword in the Flux language

import "standard.fx";
using standard::io, standard::types;

// Data types and alignment demonstrations
unsigned data{8} as byte;
signed data{16:16} as word;
unsigned data{32::0} as dword;

// Macro definitions using 'def'
def PI 3.14159;
def MAX_SIZE 1024;
def DEBUG_MODE true;

// Compile-time block with 'compt'
compt
{
    if (def(DEBUG_MODE))
    {
        def LOG_LEVEL 2;
    }
    else
    {
        def LOG_LEVEL 0;
    };
};

// External FFI block with 'extern'
extern("C")
{
    def printf(unsigned data{8}[] format, ...) -> int;
    def malloc(unsigned data{64} size) -> void*;
    def free(void* ptr) -> void;
};

// Namespace demonstration
namespace MathUtils
{
    // Template function using 'def' with templates
    def max<T>(T a, T b) -> T
    {
        return (a > b) ? a : b;
    };
    
    // Const expression function
    const def factorial(int n) -> int
    {
        if (n <= 1) { return 1; };
        return n * factorial(n - 1);
    };
};

// Contract definitions
contract ValidRange(int value)
{
    assert(value >= 0 and value <= 100, "Value must be between 0 and 100");
};

contract NonZero(int value)
{
    assert(value != 0, "Value cannot be zero");
};

// Struct with inheritance and data types
struct BaseData
{
    public
    {
        unsigned data{8} as status;
        signed data{32} as value;
    };
    
    private
    {
        bool initialized;
    };
};

struct ExtendedData : BaseData
{
    volatile unsigned data{16} as flags;
    const float pi = PI;
};

// Object with inheritance, templates, and magic methods
object Animal
{
    protected
    {
        unsigned data{8}[] as name;
        int age;
    };
    
    public
    {
        def __init(unsigned data{8}[] n, int a) -> this
        {
            this.name = n;
            this.age = a;
            return this;
        };
        
        def __exit() -> void
        {
            return void;
        };
        
        def __expr() -> unsigned data{8}[]
        {
            return this.name;
        };
        
        virtual def speak() -> void
        {
            print("Animal makes a sound");
        };
    };
};

// Inheritance with 'virtual' and 'super'
object Dog : Animal
{
    private
    {
        unsigned data{8}[] as breed;
    };
    
    public
    {
        def __init(unsigned data{8}[] n, int a, unsigned data{8}[] b) -> this
        {
            super.__init(n, a);
            this.breed = b;
            return this;
        };
        
        def speak() -> void
        {
            print(f"Dog {this.name} barks!");
        };
        
        def getBreed() -> unsigned data{8}[]
        {
            return this.breed;
        };
    };
};

// Template object
object Container<T>
{
    private
    {
        T[] items;
        int count;
    };
    
    public
    {
        def __init() -> this
        {
            this.count = 0;
            return this;
        };
        
        def add(T item) -> void
        {
            this.items[this.count] = item;
            this.count++;
        };
        
        def get(int index) -> T
        {
            if (index < 0 or index >= this.count)
            {
                throw("Index out of bounds");
            };
            return this.items[index];
        };
        
        def size() -> int
        {
            return this.count;
        };
    };
};

// Operator overloading
operator (int a, int b)[xor] -> int
{
    return a ^ b;
};

// Function with contracts
def divide(int a, int b) -> int : ValidRange, NonZero
{
    return a / b;
} : ValidRange;

// Function demonstrating most control flow keywords
def demonstrateControlFlow(int[] numbers) -> void
{
    // For loop - C++ style
    for (int i = 0; i < sizeof(numbers) / sizeof(int); i++)
    {
        // Switch statement with case and default
        switch (numbers[i] % 3)
        {
            case (0)
            {
                print("Divisible by 3");
                if (numbers[i] == 0)
                {
                    continue;
                };
            }
            case (1)
            {
                print("Remainder 1");
            }
            default
            {
                print("Remainder 2");
                if (numbers[i] > 10)
                {
                    break;
                };
            };
        };
    };
    
    // For loop - Python style
    for (num in numbers)
    {
        if (num < 0)
        {
            continue;
        }
        else if (num > 100)
        {
            break;
        }
        else
        {
            print(f"Processing: {num}");
        };
    };
    
    // While loop
    int counter = 0;
    while (counter < 5)
    {
        counter++;
        if (counter == 3)
        {
            continue;
        };
        print(f"Counter: {counter}");
    };
    
    // Do-while loop
    int x = 0;
    do
    {
        x++;
        print(f"Do-while: {x}");
    }
    while (x < 3);
};

// Function with try-catch and throw
def errorDemo() -> void
{
    try
    {
        int result = divide(10, 0);
        print(f"Result: {result}");
    }
    catch (unsigned data{8}[] error)
    {
        print(f"Caught error: {error}");
        throw("Re-throwing error");
    };
};

// Function using pointers and address-of operator
def pointerDemo() -> void
{
    int value = 42;
    int* ptr = @value;
    
    print(f"Value: {value}");
    print(f"Pointer value: {*ptr}");
    
    *ptr = 100;
    print(f"Modified value: {value}");
    
    // Type checking
    if (typeof(ptr) == int*)
    {
        print("Pointer type confirmed");
    };
    
    // Size and alignment
    print(f"Size of int: {sizeof(int)}");
    print(f"Alignment of int: {alignof(int)}");
};

// Function with auto destructuring
def destructuringDemo() -> void
{
    ExtendedData data = {status = 1, value = 42, flags = 0xFF};
    
    // Auto destructuring
    auto {s, v} = data{status, value};
    print(f"Status: {s}, Value: {v}");
};

// Function with inline assembly
def asmDemo() -> void
{
    int result = 0;
    asm
    {
        mov eax, 42
        mov result, eax
    };
    print(f"Assembly result: {result}");
};

// Function using all string interpolation types
def stringDemo() -> void
{
    unsigned data{8}[] name = "World";
    int count = 5;
    
    // F-string
    unsigned data{8}[] greeting = f"Hello, {name}!";
    print(greeting);
    
    // I-string
    print(i"Count is {} and name is {}" : {count; name;});
    
    // Range demonstration
    int[] range_array = [x for (x in 1..10) if (x % 2 == 0)];
    for (val in range_array)
    {
        print(f"Even number: {val}");
    };
};

// Assertion demonstration
def assertDemo(int value) -> void
{
    assert(value > 0, "Value must be positive");
    assert(value < 1000, "Value must be less than 1000");
    print(f"Assertion passed for value: {value}");
};

// Main function demonstrating everything
def main() -> int
{
    print("=== Flux Keywords Demonstration ===");
    
    // Object instantiation and usage
    Dog myDog("Buddy", 3, "Golden Retriever");
    print(f"Dog: {myDog}");
    myDog.speak();
    
    // Template usage
    Container<int> intContainer;
    intContainer.add(1);
    intContainer.add(2);
    intContainer.add(3);
    
    print(f"Container size: {intContainer.size()}");
    
    // Control flow demo
    int[] testNumbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    demonstrateControlFlow(testNumbers);
    
    // Error handling
    try
    {
        errorDemo();
    }
    catch (unsigned data{8}[] error)
    {
        print(f"Main caught: {error}");
    };
    
    // Pointer operations
    pointerDemo();
    
    // Destructuring
    destructuringDemo();
    
    // String interpolation
    stringDemo();
    
    // Assertions
    try
    {
        assertDemo(50);
        assertDemo(-5);  // This will fail
    }
    catch (unsigned data{8}[] error)
    {
        print(f"Assertion failed: {error}");
    };
    
    // Namespace usage
    int maxVal = MathUtils::max<int>(10, 20);
    print(f"Max value: {maxVal}");
    
    // Boolean literals
    bool isTrue = true;
    bool isFalse = false;
    
    if (isTrue and not isFalse)
    {
        print("Boolean logic works!");
    };
    
    // XOR operator
    int xorResult = 5 xor 3;
    print(f"5 XOR 3 = {xorResult}");
    
    // Void literal check
    void* nullPtr = void;
    if (nullPtr == void)
    {
        print("Void literal works!");
    };
    
    // Assembly demo (commented out for safety)
    // asmDemo();
    
    print("=== All Flux keywords demonstrated! ===");
    
    return 0;
};