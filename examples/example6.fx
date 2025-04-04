// Pointer example
import "std.fx" as std;

using std.io.output;

def main() -> !void
{
    int value = 10;
    int* ptr = @value;
    *ptr += 5;
    print(i"Value is now {}":{value;}); // Output: 15
    return 0;
};