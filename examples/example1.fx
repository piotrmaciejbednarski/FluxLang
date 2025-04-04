// Function to add two numbers
import "std.fx" as std;

using std.types;

def add(int a, int b) -> int
{
    return a + b;
};

def main() -> int
{
    int result = add(3, 5);
    print(i"3 + 5 = {}":{result;});
    return 0;
};