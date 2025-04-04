// Object with a method
import "std.fx" as std;

using std.io.output;
using std.types;

object Greeter
{
    def sayHello(string name) -> string
    {
        return i"Hello, {}!":{name;};
    };
};

def main() -> int
{
    Greeter{} greeter;
    print(greeter.sayHello("Flux"));
    return 0;
};