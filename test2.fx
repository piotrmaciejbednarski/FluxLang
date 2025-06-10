// Simple Flux Program - No Standard Library Required
// This program demonstrates basic Flux language features without external dependencies

// Simple function to add two integers
def add(int x, int y) -> int
{
    return x + y;
};

// Function to calculate factorial
def factorial(int n) -> int
{
    if (n <= 1)
    {
        return 1;
    }
    else
    {
        return n * factorial(n - 1);
    };
};

// Simple struct for 2D point
struct Point
{
    int x;
    int y;
};

// Simple object with basic methods
object Calculator
{
    int result;
    
    def __init() -> this
    {
        this.result = 0;
        return this;
    };
    
    def __exit() -> void
    {
        return;
    };
    
    def multiply(int a, int b) -> int
    {
        return a * b;
    };
    
    def divide(int a, int b) -> int
    {
        if (b != 0)
        {
            return a / b;
        }
        else
        {
            return 0;
        };
    };
};

// Main function - required entry point
def main() -> int
{
    // Basic variable declarations
    int a = 10;
    int b = 5;
    float pi = 3.14159;
    bool is_valid = true;
    
    // Basic arithmetic
    int sum = add(a, b);
    int product = a * b;
    int difference = a - b;
    
    // Control flow - if/else
    int max_value;
    if (a > b)
    {
        max_value = a;
    }
    else
    {
        max_value = b;
    };
    
    // Loop example - calculate factorial of 5
    int fact_result = factorial(5);
    
    // For loop example
    int counter = 0;
    for (int i = 0; i < 10; i++)
    {
        counter = counter + i;
    };
    
    // While loop example
    int countdown = 5;
    while (countdown > 0)
    {
        countdown = countdown - 1;
    };
    
    // Struct usage
    Point origin;
    origin.x = 0;
    origin.y = 0;
    
    Point target;
    target.x = a;
    target.y = b;
    
    // Object usage
    Calculator calc;
    int calc_result = calc.multiply(a, b);
    int div_result = calc.divide(a, b);
    
    // Array usage
    int[] numbers = [1, 2, 3, 4, 5];
    int first_number = numbers[0];
    int last_number = numbers[4];
    
    // Type checking
    bool is_int = (typeof(a) is int);
    bool is_float = (typeof(pi) is float);
    
    // Size calculations
    int int_size = sizeof(int);
    int point_size = sizeof(Point);
    
    // Conditional expression
    int final_result = (sum > product) ? sum : product;
    
    // Bitwise operations
    data{32} bits = 0b11110000111100001111000011110000;
    data{32} shifted = bits << 2;
    
    // Cast example
    float result_as_float = (float)final_result;
    int truncated = (int)pi;
    
    // Return success code
    return 0;
};