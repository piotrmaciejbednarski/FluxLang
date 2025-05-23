import "std.fx" as std;
using std.types, std.io, std.exceptions;

// Global namespace for organizational purposes
namespace example {
    // Base object template
    object template <T> BaseShape {
        T x;
        T y;
        
        def __init(T x, T y) -> void {
            this.x = x;
            this.y = y;
        };
        
        def area() -> T {
            return 0;
        };
    };

    // Derived object using inheritance
    object template <T> Rectangle <BaseShape<T>> {
        T width;
        T height;
        
        def __init(T x, T y, T w, T h) -> void {
            super.BaseShape<T>.__init(x, y);
            this.width = w;
            this.height = h;
        };
        
        def area() -> T {
            return this.width * this.height;
        };
    };

    // Another derived object
    object template <T> Circle <BaseShape<T>> {
        T radius;
        
        def __init(T x, T y, T r) -> void {
            super.BaseShape<T>.__init(x, y);
            this.radius = r;
        };
        
        def area() -> T {
            return (this.radius ** 2) * 3.14159;
        };
    };
};

// Custom operator for vector addition
operator(int[] a, int[] b)[v+] -> int[] {
    if (a.len() != b.len()) {
        throw("Vector dimensions don't match"){__exception = std.exceptions.math;};
    };
    
    int[] result = [];
    for (i in 0..a.len()) {
        result.append(a[i] + b[i]);
    };
    return result;
};

// Hardware register simulation
namespace hw {
    struct GPIORegister {
        data {32} control;
        data {32} status;
        data {32} data_reg;
    };
    
    volatile const GPIORegister* GPIO = @0x40000000;
};

// Function template example
template <T> max(T a, T b) -> T {
    return (a > b) ? a : b;
};

// Polymorphic function example
def print_area(example::BaseShape<float> shape) -> void {
    print(i"Area: {}":{shape.area();});
};

// Main program
def main() -> int {
    // Object instantiation
    example::Rectangle<float>{} rect(1.5, 2.5, 3.0, 4.0);
    example::Circle<float>{} circle(0.0, 0.0, 5.0);
    
    // Custom operator usage
    int[] vec1 = [1, 2, 3];
    int[] vec2 = [4, 5, 6];
    int[] vec3 = vec1 v+ vec2;  // [5, 7, 9]
    
    // Hardware register access
    hw::GPIO.control = 0x1;
    
    // Function template usage
    float max_val = max(3.14, 2.71);
    
    // Polymorphic function call
    print_area(rect);
    print_area(circle);
    
    // Pointer examples
    float* p_max = @max_val;
    example::BaseShape<float>* p_shape = @rect;
    
    // I-string example
    print(i"Vector sum: {} | Max value: {} | Shape area: {}":
          {
              vec3;
              *p_max;
              p_shape->area();
          };
    );
    
    // Exception handling
    try {
        int[] bad_vec = [1, 2];
        bad_vec v+ vec1;  // Will throw
    }
    catch (__exception as e) {
        print(i"Caught exception: {}":{e;});
    };
    
    // Self-modifying code example
    def make_incrementer(int n) -> (()->int) {
        signed data{8}[] code = [
            0x55,                         // push rbp
            0x48, 0x89, 0xE5,             // mov rbp, rsp
            0x89, 0x7D, 0xFC,             // mov [rbp-4], edi
            0x8B, 0x45, 0xFC,             // mov eax, [rbp-4]
            0x05, n, 0x00, 0x00, 0x00,    // add eax, n
            0x5D,                         // pop rbp
            0xC3                          // ret
        ];
        return (()->int)code;
    };
    
    (()->int) inc5 = make_incrementer(5);
    print(i"Self-modifying code result: {}":{inc5(10);});  // 15
    
    // Bit manipulation
    byte b = {0, 1, 0, 1, 0, 1, 0, 1};  // 85
    b>>;                                 // {0, 0, 1, 0, 1, 0, 1, 0} (42)
    print(i"Bit-shifted byte: {}":{(int)b;});
    
    // Dictionary example
    dict shapes = {
        "rectangle": rect,
        "circle": circle
    };
    
    for (name,shape in shapes) {
        print(i"{} area: {}":{name; shape.area();});
    };
    
    return 0;
};