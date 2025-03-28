namespace std {
    // Constants
    const float PI = 3.14159265358979323846;
    const float E = 2.71828182845904523536;

    // Basic arithmetic functions
    def abs(x: float) -> float {
        return x < 0 ? -x : x;
    };

    def max(x: float, y: float) -> float {
        return x > y ? x : y;
    };

    def min(x: float, y: float) -> float {
        return x < y ? x : y;
    };

    def clamp(x: float, lower: float, upper: float) -> float {
        return max(lower, min(x, upper));
    };

    // Exponential and logarithmic functions
    def exp(x: float) -> float {
        return E ** x;
    };

    def log(x: float) -> float {
        if (x <= 0) {
            throw "math domain error: log(x) requires x > 0";
        };
        // Approximation of natural logarithm
        float result = 0.0;
        while (x >= E) {
            x /= E;
            result += 1.0;
        };
        return result + (x - 1.0) / x; // Small correction for x < E
    };

    def log10(x: float) -> float {
        return log(x) / log(10.0);
    };

    def pow(x: float, y: float) -> float {
        return x ** y;
    };

    def sqrt(x: float) -> float {
        if (x < 0) {
            throw "math domain error: sqrt(x) requires x >= 0";
        };
        float guess = x / 2.0;
        for (int i = 0; i < 10; i++) {
            guess = (guess + x / guess) / 2.0;
        };
        return guess;
    };

    // Trigonometric functions (input in radians)
    def sin(x: float) -> float {
        // Taylor series approximation for sin(x)
        float result = 0.0;
        float term = x;
        int n = 1;
        while (abs(term) > 1e-7) {
            result += term;
            term = -term * x * x / ((2 * n) * (2 * n + 1));
            n++;
        };
        return result;
    };

    def cos(x: float) -> float {
        // Taylor series approximation for cos(x)
        float result = 0.0;
        float term = 1.0;
        int n = 1;
        while (abs(term) > 1e-7) {
            result += term;
            term = -term * x * x / ((2 * n - 1) * (2 * n));
            n++;
        };
        return result;
    };

    def tan(x: float) -> float {
        return sin(x) / cos(x);
    };

    // Hyperbolic functions
    def sinh(x: float) -> float {
        return (exp(x) - exp(-x)) / 2.0;
    };

    def cosh(x: float) -> float {
        return (exp(x) + exp(-x)) / 2.0;
    };

    def tanh(x: float) -> float {
        return sinh(x) / cosh(x);
    };

    // Angular conversion functions
    def radians(degrees: float) -> float {
        return degrees * PI / 180.0;
    };

    def degrees(radians: float) -> float {
        return radians * 180.0 / PI;
    };

    // Rounding functions
    def ceil(x: float) -> float {
        int intPart = (int)x;
        return intPart + (x > intPart ? 1.0 : 0.0);
    };

    def floor(x: float) -> float {
        int intPart = (int)x;
        return intPart - (x < intPart ? 1.0 : 0.0);
    };

    def round(x: float) -> float {
        return floor(x + 0.5);
    };

    // Factorial function
    def factorial(n: int) -> int {
        if (n < 0) {
            throw "math domain error: factorial(n) requires n >= 0";
        };
        int result = 1;
        for (int i = 2; i <= n; i++) {
            result *= i;
        };
        return result;
    };

    // Greatest Common Divisor (GCD)
    def gcd(a: int, b: int) -> int {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        };
        return a;
    };

    // Least Common Multiple (LCM)
    def lcm(a: int, b: int) -> int {
        return abs(a * b) / gcd(a, b);
    };
};