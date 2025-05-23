import "std.fx" as std;
using std.io, std.types;

// Global object definitions
object GlobalObj {
    def __init(int val) -> void {
        this.value = val;
    };
    int value;
};

namespace example {
    object NamespaceObj {
        def __init(string name) -> void {
            this.name = name;
        };
        string name;
    };
};

object ContainerObj {
    object NestedObj {
        def __init() -> void {
            this.data = "nested";
        };
        string data;
    };
};

def main() -> int {
    // Correct: Instantiating objects inside function
    GlobalObj{} globalInstance(42);
    example::NamespaceObj{} namespaceInstance("test");
    ContainerObj{} containerInstance;
    ContainerObj.NestedObj{} nestedInstance;
    
    // Incorrect (would cause error):
    // object InvalidObj {};  // Can't define objects inside functions
    
    // Struct definition is allowed in functions
    struct LocalStruct {
        int x;
        int y;
    };
    
    LocalStruct{} s = {1, 2};
    
    return 0;
};

// More object definitions at global scope
object Parent {
    def greet() -> void {
        print("Hello from Parent");
    };
};

object Child <Parent> {
    def greet() -> void {
        super.Parent.greet();
        print("Hello from Child");
    };
};