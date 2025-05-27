import "std.fx" as std;
using std::io, std::types;

// Global object definitions
object GlobalObj
{
    i32 value;

    def __init(i32 val) -> this
    {
        this.value = val;
        return this;
    };

    def __exit() -> void
    {
        return;
    };
};

namespace example
{
    object NamespaceObj
    {
        string name;

        def __init(string name) -> this
        {
            this.name = name;
            return this;
        };

        def __exit() -> void
        {
            return;
        };
    };
};

object ContainerObj
{
    object NestedObj
    {
        string buf;

        def __init() -> this
        {
            this.buf = "nested";
            return this;
        };

        def __exit() -> void
        {
            return;
        };
    };
};


struct myStruct
{
    int x, y;
};

def main() -> i32
{
    // Correct: Instantiating objects inside function
    GlobalObj globalInstance(42);
    example::NamespaceObj namespaceInstance("test");
    ContainerObj containerInstance;
    ContainerObj.NestedObj nestedInstance;
    
    myStruct s = {1, 2};
    
    return 0;
};

// More object definitions at global scope
object Parent
{
    def greet() -> void
    {
        print("Hello from Parent");
        return;
    };
};

object Child : Parent
{
    def greet() -> void
    {
        super.Parent.greet();
        print("Hello from Child");
        return;
    };
};