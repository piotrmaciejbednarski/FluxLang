// Sample Flux file to test all language features and keywords
// This file demonstrates every keyword and major syntax element

import "standard.fx" as std;
import "math.fx";

using std::io, std::types;

// Data types and type definitions
signed data{32} i32;
unsigned data{16:32} ui16_aligned;
unsigned data{8}[] string;

// Compile-time macro definitions
def DEBUG_MODE true;
def MAX_SIZE 1024;

// External FFI declarations
extern("C")
{
    def malloc(ui64 size) -> void*;
    def free(void* ptr) -> void;
    def printf(string format) -> int;
};

// Namespace definition
namespace TestNamespace
{
    volatile const int global_counter = 42;
    
    def utility_func(int x, float y) -> bool
    {
        return x > (int)y;
    };
    
    namespace NestedNamespace
    {
        string nested_message = "Hello from nested namespace";
    };
};

// Object with templates and inheritance
object BaseObject
{
    protected
    {
        int base_value;
    };
    
    public
    {
        def __init(int val) -> this
        {
            this.base_value = val;
            return this;
        };
        
        def __exit() -> void
        {
            return void;
        };
        
        virtual def get_value() -> int
        {
            return this.base_value;
        };
    };
};

// Template object with multiple inheritance
object TemplateObject<T,K> : BaseObject
{
    private:
    {
        T template_data;
        K secondary_data;
    };
    
    public:
    {
        def __init<T,K>(int base_val, T data, K sec_data) -> this
        {
            super.__init(base_val);
            this.template_data = data;
            this.secondary_data = sec_data;
            return this;
        };
        
        def __add(TemplateObject<T,K> other) -> TemplateObject<T,K>
        {
            // User-defined addition operator
            return this; // Simplified
        };
        
        def __eq(TemplateObject<T,K> other) -> bool
        {
            return this.template_data == other.template_data;
        };
        
        def __expr() -> string
        {
            return f"TemplateObject({this.template_data})";
        };
    };
};

// Struct definitions with inheritance
struct Point
{
    public:
    {
        float x, y, z;
    };
    
    private:
    {
        bool is_valid;
    };
};

struct Vector3D : Point
{
    float magnitude;
    unsigned data{1} normalized;
};

// Template struct
struct GenericContainer<T>
{
    T* data;
    unsigned data{32} size;
    unsigned data{32} capacity;
};

// Compile-time evaluation block
compt
{
    if (def(DEBUG_MODE))
    {
        // Compile-time debug setup
        def LOG_LEVEL 3;
    }
    else if (!def(DEBUG_MODE))
    {
        def LOG_LEVEL 0;
    };
};

// Volatile template function
volatile def thread_safe_operation<T>(T* ptr) -> T
{
    // Simulated atomic operation
    return *ptr;
};

// Function with all control structures
def comprehensive_test_function(int param1, float param2, bool param3) -> int
{
    string test_msg = "Testing all features";
    
    // Variable declarations with auto
    auto {x, y} = Point{x = 10.5, y = 20.3, z = 5.1};
    
    // i-string interpolation
    string interpolated = i"Values: {} and {}" : {x; y;};
    
    // f-string interpolation  
    string formatted = f"Parameter 1: {param1}, Parameter 2: {param2}";
    
    // All types of loops
    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (i % 2 == 0)
        {
            continue;
        };
        
        if (i > 100)
        {
            break;
        };
    };
    
    // Python-style for loop with range
    for (val in 1..10)
    {
        // Range iteration
    };
    
    // Python-style for loop with collection
    int[] numbers = [1, 2, 3, 4, 5];
    for (num in numbers)
    {
        // Collection iteration
    };
    
    // Array comprehension
    int[] squares = [x ^ 2 for (x in 1..5)];
    int[] evens = [x for (x in 1..20) if (x % 2 == 0)];
    
    // While loop
    int counter = 0;
    while (counter < 10 and param3)
    {
        counter++;
    };
    
    // Do-while loop
    do
    {
        counter--;
    }
    while (counter > 0 or not param3);
    
    // Switch statement
    switch (param1)
    {
        case (0)
        {
            return -1;
        }
        case (1)
        {
            return 1;
        }
        default
        {
            // Continue to match
        };
    };
    
    // Match statement with patterns
    match (param1)
    {
        case (param1 in 1..10)
        {
            // Range matching
        }
        case (param1 in numbers)
        {
            // Collection matching
        }
        default
        {
            // Default case
        };
    };
    
    // All operators demonstration
    int a = 5, b = 3;
    int result = a + b - a * b / a % b;
    result = a ^ b;  // Power
    result += a;
    result -= b;
    result *= 2;
    result /= 3;
    result %= 4;
    result ^= 2;
    
    // Bitwise operations
    result = a & b;
    result = a | b;
    result = a ^^ b;  // XOR
    result = ~a;
    result = a << 2;
    result = a >> 1;
    
    // Bitwise with backtick prefix
    result = a `& b;
    result = a `| b;
    result = a `!& b;  // NAND
    result = a `!| b;  // NOR
    result = a `^^ b;  // Bitwise XOR
    
    // Logical operations
    bool logic_result = (a > b) and (b < 10);
    logic_result = (a == b) or (b != 0);
    logic_result = not logic_result;
    logic_result = (a >= b) && (b <= 10);
    logic_result = (a < b) || (b > 0);
    logic_result = !logic_result;
    logic_result = logic_result xor true;
    
    // Comparison and identity
    logic_result = a is not void;
    logic_result = param2 is float;
    logic_result = param1 in numbers;
    
    // Pointer operations
    int* ptr = @a;
    *ptr = 42;
    ptr++;
    ptr--;
    
    // Type operations
    int size_val = sizeof(int);
    int align_val = alignof(float);
    // Note: typeof would return a type, simplified here
    
    // Conditional operator
    int conditional_result = (a > b) ? a : b;
    
    // Error handling with try/catch/throw/assert
    try
    {
        assert(a > 0, "Value must be positive");
        
        if (a < 0)
        {
            throw("Negative value error");
        };
        
        try
        {
            // Nested try block
            if (b == 0)
            {
                throw("Division by zero");
            };
        }
        catch (string nested_error)
        {
            // Handle nested error
        };
    }
    catch (string error_msg)
    {
        return -1;
    }
    catch (auto generic_error)
    {
        return -2;
    };
    
    // Assembly inline
    asm
    {
        mov eax, 1
        mov ebx, 2
        add eax, ebx
    };
    
    return result;
};

// Template function with multiple constraints
def generic_algorithm<T,K,R>(T input1, K input2) -> R
{
    return (R)(input1 + input2);
};

// Function pointer demonstration
def callback_function(int x, int y) -> int
{
    return x * y;
};

// Object instantiation and virtual calls
def test_objects() -> void
{
    // Regular object instantiation
    BaseObject base_obj(100);
    
    // Template object instantiation
    TemplateObject<int,float> template_obj(50, 42, 3.14);
    
    // Super and virtual instantiation
    super base_from_template();
    virtual::BaseObject virtual_obj();
    
    // Using object methods
    int val = base_obj.get_value();
    string expr_result = template_obj.__expr();
    
    return void;
};

// Struct usage with all data types
def test_data_types() -> bool
{
    // Primitive types
    bool boolean_val = true;
    int integer_val = 42;
    float float_val = 3.14159;
    char character_val = 'A';
    void void_val;
    
    // Custom data types
    signed data{64} big_signed = 0x123456789ABCDEF0;
    unsigned data{13:16} weird_aligned = 0b1101011010110;
    
    // Arrays
    int[] int_array = [1, 2, 3, 4, 5];
    float[] float_array = [1.0, 2.5, 3.7];
    
    // Strings (which are byte arrays)
    string message = "Hello World!";
    char single_char = "X";
    
    // Struct initialization
    Point point_val = {x = 1.0, y = 2.0, z = 3.0};
    Vector3D vector_val = {x = 0.0, y = 1.0, z = 0.0, magnitude = 1.0, normalized = 1};
    
    // Generic container
    GenericContainer<int> int_container;
    
    // Function pointers
    int (*operation)(int,int) = @callback_function;
    int (*func_array[])(int,int) = {@callback_function};
    
    // Pointer arithmetic
    int* ptr = @int_array[0];
    ptr += 2;
    int value = *ptr;
    
    return boolean_val;
};

// Enum-like structure (since enum is a keyword but not fully specified)
namespace Colors
{
    const int RED = 1;
    const int GREEN = 2;
    const int BLUE = 3;
};

// Main function - required by Flux
def main() -> int
{
    // Test all major features
    int test_result = comprehensive_test_function(10, 2.5, true);
    
    // Test objects
    test_objects();
    
    // Test data types
    bool data_test = test_data_types();
    
    // Use namespace members
    bool util_result = TestNamespace::utility_func(5, 3.0);
    string nested_msg = TestNamespace::NestedNamespace::nested_message;
    
    // Template function usage
    float template_result = generic_algorithm<int,int,float>(10, 20);
    
    // External function call (would need actual linking)
    // printf(f"Test completed with result: {test_result}");
    
    // Final return
    return test_result;
};