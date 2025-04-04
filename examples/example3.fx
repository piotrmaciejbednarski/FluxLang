// Loop and conditional example
import "std.fx" as std;

using std.types;

def main() -> int
{
    for (int i = 0; i < 5; ++i)
    {
        if (i % 2 == 0)
        {
            print(i"{} is even":{i;});
        }
        else
        {
            print(i"{} is odd":{i;});
        };
    };
    return 0;
};