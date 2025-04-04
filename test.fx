// Comprehensive Flux program using all language keywords

import "std.fx" as std;

// Define basic data types
data{8}[] string;
signed data{32} i32;
unsigned data{32} ui32;
signed data{64} i64;
unsigned data{64} ui64;
signed data{64} float;
unsigned data{64} ufloat;

namespace Flux
{
    // Basic data types
    signed data{32} Integer;
    unsigned data{8} Byte;
    signed data{1} Flag;

    // Enum declaration
    enum Color
    {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
    };

    // Class definition with inheritance
    class BaseWidget
    {
        const Integer VERSION = 1;
        
        struct Dimensions
        {
            Integer width;
            Integer height;
        };
    };

    class Widget<BaseWidget>
    {
        object Logger
        {
            def __init() -> void
            {
                this.enabled = true;
                return;
            };
            
            def log(string message) -> void
            {
                if (this.enabled)
                {
                    return;
                };
                return;
            };
            string x = "";
            Flag enabled;
        };
    };

    // Template function definition
    template <T> max(T a, T b) -> !void
    {
        return (a > b) ? a : b;
    };

    // Custom operator
    operator(Integer x, Integer y)[pow] -> Integer
    {
        Integer result = 1;
        
        for (Integer i = 0; i < y; i += 1)
        {
            result *= x;
        };
        
        return result;
    };

    // Assembly block
    asm {
        // Example assembly code
        mov eax, 1
        mov ebx, 0
        int 0x80
    };
};

// Define a helper function outside any other function
def add(Flux.Integer x, Flux.Integer y) -> Flux.Integer
{
    return x + y;
};

// Define a pointer function outside any other function
def swap(Flux.Integer *a, Flux.Integer *b) -> void
{
    Flux.Integer temp = *a;
    *a = *b;
    *b = temp;
    return;
};

// Using statement at global scope
using Flux;

// Application object at global scope
object Application
{
    def __init() -> void
    {
        this.running = true;
        this.errorCode = 0;
        return;
    };
    
    def process(Integer value) -> Integer
    {
        // Using try-catch
        try
        {
            if (value < 0)
            {
                throw("Negative value");
            };
            
        }
        catch(Error)
        {
            this.errorCode = 1;
            return 0;
        };
    };
    
    def run() -> void
    {
        Integer count = 5;
        
        while (count > 0 and this.running)
        {
            // If-else statement
            if (count == 3)
            {
                continue;
            }
            else if (count == 1)
            {
                break;
            };
            
            count -= 1;
        };
        
        // Do-while loop
        do
        {
            count += 1;
            
            // Logical operators
            if (count > 10 or this.errorCode != 0)
            {
                this.running = false;
            };
            
            // Bitwise operators
            Integer flags = 0x01 | 0x02;
            flags = flags & ~0x01;
            flags = flags ^ 0x04;
            flags = flags << 1;
            flags = flags >> 1;
            
            // Using xor keyword
            Flag test = xor(true,false);
            
            // Using not keyword
            test = not test;
            
            // Empty statement
            ;;;
            
        } while (this.running);
        
        return;
    };
    
    Flag running;
    Integer errorCode;
};

// The main function
def main() -> int
{
    // Object instantiation
    Application{} app;
    
    // Function pointer declaration
    Integer *func_ptr(Integer, Integer) = add;
    
    // Variables and pointers
    Integer a = 5;
    Integer b = 10;
    Integer *ptr_a = @a;
    
    // Call object method
    app.run();
    
    // Using sizeof
    Integer size = sizeof(Integer);
    
    // Type checking with typeof and is
    Flag check = typeof(a) is Integer;
    
    // Ternary operator
    string message = app.running ? "Running" : "Stopped";
    
    // Using op keyword for custom operator
    Integer powered = op<2 pow 3>;
    
    // Super keyword is used in inheritance context
    // This keyword is used in object methods
    
    // Void keyword in return type
    // Return statement at the end
    return 0;
};