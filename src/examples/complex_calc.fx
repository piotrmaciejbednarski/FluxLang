// A complex number calculator with operator definitions

struct{
    float{64} real;
    float{64} imag;
} Complex;

// Constructor function for Complex numbers
Complex{} create_complex(float{64} r, float{64} i) {
    Complex{} c;
    c.real = r;
    c.imag = i;
    return c;
};

// Addition of complex numbers
Complex{} add(Complex{} a, Complex{} b) {
    return create_complex(
        a.real + b.real,
        a.imag + b.imag
    );
};

// Multiplication of complex numbers
Complex{} multiply(Complex{} a, Complex{} b) {
    return create_complex(
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    );
};

// Calculate magnitude of a complex number
float{64} magnitude(Complex{} c) {
    return sqrt(c.real * c.real + c.imag * c.imag);
};

// Power function for complex numbers using DeMoivre's formula
Complex{} power(Complex{} base, int{32} exp) {
    if (exp == 0) {
        return create_complex(1.0, 0.0);
    };

    float{64} r = magnitude(base);
    float{64} theta = atan2(base.imag, base.real);
    float{64} new_r = pow(r, exp);
    float{64} new_theta = theta * exp;

    return create_complex(
        new_r * cos(new_theta),
        new_r * sin(new_theta)
    );
};

// Print a complex number in a + bi format
void print_complex(Complex{} c) {
    print(to_string(c.real) + " + " + to_string(c.imag) + "i");
};

// Calculate roots of a quadratic equation ax² + bx + c = 0
void quadratic_solver(float{64} a, float{64} b, float{64} c) {
    float{64} discriminant = b * b - 4.0 * a * c;
    
    if (discriminant > 0.0) {
        float{64} root1 = (-b + sqrt(discriminant)) / (2.0 * a);
        float{64} root2 = (-b - sqrt(discriminant)) / (2.0 * a);
        print("Real roots:");
        print(root1);
        print(root2);
    } else {
        float{64} real_part = -b / (2.0 * a);
        float{64} imag_part = sqrt(-discriminant) / (2.0 * a);
        Complex{} root1 = create_complex(real_part, imag_part);
        Complex{} root2 = create_complex(real_part, -imag_part);
        print("Complex roots:");
        print_complex(root1);
        print_complex(root2);
    };
};

int{32} main(char[] *argc, char[][] *argv) {
    // Test quadratic equation solver
    print("Solving x² + 2x + 5 = 0");
    quadratic_solver(1.0, 2.0, 5.0);
    
    // Test complex number operations
    Complex{} c1 = create_complex(3.0, 4.0);
    Complex{} c2 = create_complex(1.0, 2.0);
    
    print("\nComplex number operations:");
    print("c1 = ");
    print_complex(c1);
    print("c2 = ");
    print_complex(c2);
    
    print("\nAddition:");
    print_complex(add(c1, c2));
    
    print("\nMultiplication:");
    print_complex(multiply(c1, c2));
    
    print("\nc1 to the power of 3:");
    print_complex(power(c1, 3));
    
    return 0;
};
