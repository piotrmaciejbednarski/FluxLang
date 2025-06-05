// Comprehensive Flux Keywords Example
// This file demonstrates every keyword in the Flux language

// 1. import - Import external modules
import "standard.fx" as std;

// 2. using - Using directive for namespaces  
using std::io, std::types;

// 3. namespace - Namespace definition
namespace examples
{
    // 4. object - Object definition
    object SimpleObject
    {
        // 5. int - Integer type
        int value;
        
        // 6. def - Function definition
        def __init(int x) -> this
        {
            // 7. this - Current object reference
            this.value = x;
            // 8. return - Return statement
            return;
        };
        
        // 9. void - Void return type
        def __exit() -> void
        {
            return;
        };
    };
};

// 10. struct - Structure definition  
struct Point
{
    // 11. float - Floating point type
    float x, y;
};

// 12. const - Constant qualifier
const int CONSTANT_VALUE = 42;

// 13. volatile - Volatile qualifier
volatile int shared_variable = 0;

// 14. signed - Signed type qualifier
signed data{32} signed_number;

// 15. unsigned - Unsigned type qualifier  
unsigned data{16} unsigned_number;

// 16. data - Custom data type with bit width
data{8} byte_value;

// 17. template - Template definition
template <T> identity(T x) -> T
{
    return x;
};

// 18. operator - Custom operator definition
operator(int x, int y)[custom_add] -> int
{
    return x + y;
};

// 19. async - Async function template
async template <T> asyncFunction(T value) -> T
{
    return value;
};

// Main function - required entry point
def main() -> int
{
    // 20. true - Boolean true literal
    bool flag1 = true;
    
    // 21. false - Boolean false literal  
    bool flag2 = false;
    
    // 22. auto - Automatic type deduction with destructuring
    Point p = {x = 1.0, y = 2.0};
    auto {a, b} = p{x, y};
    
    // 23. if - Conditional statement
    if (flag1)
    {
        // 24. and - Logical AND
        if (flag1 and flag2)
        {
            // This won't execute
        };
        
        // 25. or - Logical OR  
        if (flag1 or flag2)
        {
            // This will execute
        };
    }
    // 26. else - Else clause
    else
    {
        // Won't execute
    };
    
    // 27. not - Logical NOT
    if (not flag2)
    {
        // This will execute since flag2 is false
    };
    
    // 28. xor - Logical XOR
    if (flag1 xor flag2)
    {
        // This will execute since one is true, one is false
    };
    
    // 29. while - While loop
    int counter = 0;
    while (counter < 3)
    {
        // Demonstrate custom operator usage
        counter = counter custom_add counter;
        
        // 30. continue - Continue statement
        if (counter == 2)
        {
            continue;
        };
        
        // 31. break - Break statement  
        if (counter > 5)
        {
            break;
        };
    };
    
    // 32. do - Do-while loop
    do
    {
        counter--;
    }
    while (counter > 0);
    
    // 33. for - For loop
    for (int i = 0; i < 5; i++)
    {
        // Loop body
    };
    
    // 34. in - For-in loop / membership test
    int[] numbers = [1, 2, 3, 4, 5];
    for (num in numbers)
    {
        // Iterate through array
    };
    
    // 35. switch - Switch statement
    int value = 2;
    switch (value)
    {
        // 36. case - Switch case
        case (1)
        {
            // Handle case 1
        };
        
        case (2)
        {
            // Handle case 2
        };
        
        // 37. default - Default case
        default
        {
            // Default handler
        };
    };
    
    // 38. try - Try block for exception handling
    try
    {
        // 39. throw - Throw exception
        throw 42;
    }
    // 40. catch - Catch exception
    catch (int error)
    {
        // Handle integer exception
    };
    
    // 41. assert - Assertion statement
    assert(1 == 1, "If you see this message, something is fatally wrong with your computer and 1 does not equal 1.");
    
    // 42. sizeof - Size operator
    int size_of_int = sizeof(int);
    
    // 43. typeof - Type operator  
    print(typeof(size_of_int));
    
    // 44. as - Type casting
    float f = 3.14;
    int i = (int)f;
    
    // 45. is - Identity comparison
    if (typeof(f) is float)
    {
        // Type check
    };
    
    // 46. await - Await async operation
    int result = await asyncFunction<int>(100);
    
    // 47. asm - Inline assembly
    asm {
        mov eax, 1
        mov ebx, 0  
        int 0x80
    };
    
    // 48. super - Parent class reference (used in inheritance)
    examples::SimpleObject obj(42);
    
    // 49. enum - Enumeration (if supported)
    // Note: enum might not be fully implemented yet based on spec
    
    return 0;
};

// Additional examples showing inheritance with super
object BaseClass
{
    int base_value;
    
    def __init(int val) -> this
    {
        this.base_value = val;
        return;
    };
    
    def __exit() -> void
    {
        return;
    };
};

object DerivedClass : BaseClass
{
    int derived_value;
    
    def __init(int base_val, int derived_val) -> this
    {
        // Using super to call parent constructor
        BaseClass super_obj(base_val);
        this.derived_value = derived_val;
        return;
    };
    
    def __exit() -> void
    {
        return;
    };
};