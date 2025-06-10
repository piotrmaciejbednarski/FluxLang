// ================================================================
// FLUX MATHEMATICS LIBRARY
// math.fx - Comprehensive mathematical functions and constants
// ================================================================

import "types.fx";

namespace standard
{
    namespace math
    {
        using standard::types::basic;
        
        // ================================================================
        // MATHEMATICAL CONSTANTS
        // ================================================================
        
        namespace constants
        {
            // Fundamental constants
            float PI = 3.14159265358979323846;
            float E = 2.71828182845904523536;
            float EULER_GAMMA = 0.57721566490153286061;
            float PHI = 1.61803398874989484820;  // Golden ratio
            float SQRT2 = 1.41421356237309504880;
            float SQRT3 = 1.73205080756887729353;
            float SQRT5 = 2.23606797749978969641;
            float LN2 = 0.69314718055994530942;
            float LN10 = 2.30258509299404568402;
            float LOG10E = 0.43429448190325182765;
            float LOG2E = 1.44269504088896338700;
            
            // Derived constants
            float PI_2 = 1.57079632679489661923;      // π/2
            float PI_4 = 0.78539816339744830962;      // π/4
            float PI_6 = 0.52359877559829887308;      // π/6
            float PI_3 = 1.04719755119659774615;      // π/3
            float TWO_PI = 6.28318530717958647693;    // 2π
            float SQRT_PI = 1.77245385090551602730;   // √π
            float INV_PI = 0.31830988618379067154;    // 1/π
            float INV_SQRT_PI = 0.56418958354775628695; // 1/√π
            
            // Physical and mathematical constants
            float CATALAN = 0.91596559417721901505;   // Catalan's constant
            float APERY = 1.20205690315959428540;     // Apéry's constant (ζ(3))
            float FEIGENBAUM_DELTA = 4.66920160910299067185; // Feigenbaum constant δ
            float FEIGENBAUM_ALPHA = 2.50290787509589282228; // Feigenbaum constant α
            
            // Machine epsilon values
            float FLOAT_EPSILON = 1.19209290e-7;
            float DOUBLE_EPSILON = 2.2204460492503131e-16;
            
            // Infinity and NaN representations
            float INFINITY = 1.0 / 0.0;
            float NEG_INFINITY = -1.0 / 0.0;
            float NAN = 0.0 / 0.0;
            
            // Conversion factors
            float DEG_TO_RAD = 0.01745329251994329577; // π/180
            float RAD_TO_DEG = 57.2957795130823208768; // 180/π
        };
        
        // ================================================================
        // BASIC MATHEMATICAL FUNCTIONS
        // ================================================================
        
        namespace basic
        {
            // Absolute value functions
            template <T> abs(T x) -> T
            {
                return (x < 0) ? -x : x;
            };
            
            def fabs(float x) -> float
            {
                return abs<float>(x);
            };
            
            // Sign function
            template <T> sign(T x) -> T
            {
                if (x > 0) { return 1; };
                if (x < 0) { return -1; };
                return 0;
            };
            
            // Min/Max functions
            template <T> min(T a, T b) -> T
            {
                return (a < b) ? a : b;
            };
            
            template <T> max(T a, T b) -> T
            {
                return (a > b) ? a : b;
            };
            
            template <T> clamp(T value, T min_val, T max_val) -> T
            {
                if (value < min_val) { return min_val; };
                if (value > max_val) { return max_val; };
                return value;
            };
            
            // Square root using Newton-Raphson method
            def sqrt(float x) -> float
            {
                if (x < 0.0) { return constants::NAN; };
                if (x == 0.0) { return 0.0; };
                
                float guess = x / 2.0;
                float prev_guess = 0.0;
                
                while (abs(guess - prev_guess) > constants::FLOAT_EPSILON)
                {
                    prev_guess = guess;
                    guess = (guess + x / guess) / 2.0;
                };
                
                return guess;
            };
            
            // Cube root
            def cbrt(float x) -> float
            {
                if (x == 0.0) { return 0.0; };
                
                bool negative = (x < 0.0);
                x = abs(x);
                
                float guess = x / 3.0;
                float prev_guess = 0.0;
                
                while (abs(guess - prev_guess) > constants::FLOAT_EPSILON)
                {
                    prev_guess = guess;
                    guess = (2.0 * guess + x / (guess * guess)) / 3.0;
                };
                
                return negative ? -guess : guess;
            };
            
            // Nth root
            def nthroot(float x, int n) -> float
            {
                if (n == 0) { return constants::NAN; };
                if (n == 1) { return x; };
                if (n == 2) { return sqrt(x); };
                if (n == 3) { return cbrt(x); };
                
                if (x < 0.0 and (n % 2 == 0)) { return constants::NAN; };
                
                bool negative = (x < 0.0);
                x = abs(x);
                
                float guess = x / (float)n;
                float prev_guess = 0.0;
                float fn = (float)n;
                
                while (abs(guess - prev_guess) > constants::FLOAT_EPSILON)
                {
                    prev_guess = guess;
                    float power_n_minus_1 = 1.0;
                    for (int i = 0; i < n - 1; i++)
                    {
                        power_n_minus_1 *= guess;
                    };
                    guess = ((fn - 1.0) * guess + x / power_n_minus_1) / fn;
                };
                
                return negative ? -guess : guess;
            };
            
            // Power function using exponentiation by squaring
            def pow(float base, int exponent) -> float
            {
                if (exponent == 0) { return 1.0; };
                if (exponent == 1) { return base; };
                if (base == 0.0) { return 0.0; };
                
                bool negative_exp = (exponent < 0);
                exponent = abs(exponent);
                
                float result = 1.0;
                float current_power = base;
                
                while (exponent > 0)
                {
                    if (exponent % 2 == 1)
                    {
                        result *= current_power;
                    };
                    current_power *= current_power;
                    exponent /= 2;
                };
                
                return negative_exp ? (1.0 / result) : result;
            };
            
            // Power function for real exponents
            def powf(float base, float exponent) -> float
            {
                if (base == 0.0)
                {
                    return (exponent > 0.0) ? 0.0 : constants::INFINITY;
                };
                if (base == 1.0) { return 1.0; };
                if (exponent == 0.0) { return 1.0; };
                if (exponent == 1.0) { return base; };
                
                // Use exp(exponent * ln(base))
                return exponential::exp(exponent * exponential::ln(base));
            };
            
            // Floor function
            def floor(float x) -> float
            {
                int int_part = (int)x;
                return (x < 0.0 and x != (float)int_part) ? (float)(int_part - 1) : (float)int_part;
            };
            
            // Ceiling function
            def ceil(float x) -> float
            {
                int int_part = (int)x;
                return (x > 0.0 and x != (float)int_part) ? (float)(int_part + 1) : (float)int_part;
            };
            
            // Round function
            def round(float x) -> float
            {
                return floor(x + 0.5);
            };
            
            // Truncate function
            def trunc(float x) -> float
            {
                return (float)((int)x);
            };
            
            // Fractional part
            def fract(float x) -> float
            {
                return x - trunc(x);
            };
            
            // Modulo function for floats
            def fmod(float x, float y) -> float
            {
                if (y == 0.0) { return constants::NAN; };
                return x - y * trunc(x / y);
            };
            
            // Remainder function
            def remainder(float x, float y) -> float
            {
                if (y == 0.0) { return constants::NAN; };
                float quotient = round(x / y);
                return x - y * quotient;
            };
            
            // GCD and LCM
            def gcd(int a, int b) -> int
            {
                a = abs(a);
                b = abs(b);
                while (b != 0)
                {
                    int temp = b;
                    b = a % b;
                    a = temp;
                };
                return a;
            };
            
            def lcm(int a, int b) -> int
            {
                if (a == 0 or b == 0) { return 0; };
                return abs(a * b) / gcd(a, b);
            };
            
            // Linear interpolation
            def lerp(float a, float b, float t) -> float
            {
                return a + t * (b - a);
            };
            
            // Smooth step function
            def smoothstep(float edge0, float edge1, float x) -> float
            {
                float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
                return t * t * (3.0 - 2.0 * t);
            };
        };
        
        // ================================================================
        // TRIGONOMETRIC FUNCTIONS
        // ================================================================
        
        namespace trigonometric
        {
            // Normalize angle to [-π, π]
            def normalize_angle(float angle) -> float
            {
                while (angle > constants::PI)
                {
                    angle -= constants::TWO_PI;
                };
                while (angle < -constants::PI)
                {
                    angle += constants::TWO_PI;
                };
                return angle;
            };
            
            // Sine function using Taylor series
            def sin(float x) -> float
            {
                x = normalize_angle(x);
                
                float result = x;
                float term = x;
                float x_squared = x * x;
                
                for (int n = 1; n <= 20; n++)
                {
                    term *= -x_squared / ((2 * n) * (2 * n + 1));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Cosine function using Taylor series
            def cos(float x) -> float
            {
                x = normalize_angle(x);
                
                float result = 1.0;
                float term = 1.0;
                float x_squared = x * x;
                
                for (int n = 1; n <= 20; n++)
                {
                    term *= -x_squared / ((2 * n - 1) * (2 * n));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Tangent function
            def tan(float x) -> float
            {
                float cos_x = cos(x);
                if (basic::abs(cos_x) < constants::FLOAT_EPSILON)
                {
                    return constants::INFINITY;
                };
                return sin(x) / cos_x;
            };
            
            // Cotangent function
            def cot(float x) -> float
            {
                float sin_x = sin(x);
                if (basic::abs(sin_x) < constants::FLOAT_EPSILON)
                {
                    return constants::INFINITY;
                };
                return cos(x) / sin_x;
            };
            
            // Secant function
            def sec(float x) -> float
            {
                float cos_x = cos(x);
                if (basic::abs(cos_x) < constants::FLOAT_EPSILON)
                {
                    return constants::INFINITY;
                };
                return 1.0 / cos_x;
            };
            
            // Cosecant function
            def csc(float x) -> float
            {
                float sin_x = sin(x);
                if (basic::abs(sin_x) < constants::FLOAT_EPSILON)
                {
                    return constants::INFINITY;
                };
                return 1.0 / sin_x;
            };
            
            // Arcsine function using iterative method
            def asin(float x) -> float
            {
                if (x < -1.0 or x > 1.0) { return constants::NAN; };
                if (x == -1.0) { return -constants::PI_2; };
                if (x == 1.0) { return constants::PI_2; };
                if (x == 0.0) { return 0.0; };
                
                // Use Newton-Raphson method
                float guess = x;
                float prev_guess = 0.0;
                
                for (int i = 0; i < 50; i++)
                {
                    prev_guess = guess;
                    float sin_guess = sin(guess);
                    float cos_guess = cos(guess);
                    guess = guess - (sin_guess - x) / cos_guess;
                    
                    if (basic::abs(guess - prev_guess) < constants::FLOAT_EPSILON) { break; };
                };
                
                return guess;
            };
            
            // Arccosine function
            def acos(float x) -> float
            {
                if (x < -1.0 or x > 1.0) { return constants::NAN; };
                return constants::PI_2 - asin(x);
            };
            
            // Arctangent function using series expansion
            def atan(float x) -> float
            {
                if (x > 1.0)
                {
                    return constants::PI_2 - atan(1.0 / x);
                };
                if (x < -1.0)
                {
                    return -constants::PI_2 - atan(1.0 / x);
                };
                
                float result = x;
                float term = x;
                float x_squared = x * x;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= -x_squared;
                    float next_term = term / (2 * n + 1);
                    result += next_term;
                    if (basic::abs(next_term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Two-argument arctangent function
            def atan2(float y, float x) -> float
            {
                if (x > 0.0)
                {
                    return atan(y / x);
                };
                if (x < 0.0 and y >= 0.0)
                {
                    return atan(y / x) + constants::PI;
                };
                if (x < 0.0 and y < 0.0)
                {
                    return atan(y / x) - constants::PI;
                };
                if (x == 0.0 and y > 0.0)
                {
                    return constants::PI_2;
                };
                if (x == 0.0 and y < 0.0)
                {
                    return -constants::PI_2;
                };
                return 0.0; // x == 0 and y == 0
            };
            
            // Convert degrees to radians
            def deg_to_rad(float degrees) -> float
            {
                return degrees * constants::DEG_TO_RAD;
            };
            
            // Convert radians to degrees
            def rad_to_deg(float radians) -> float
            {
                return radians * constants::RAD_TO_DEG;
            };
            
            // Versine function
            def versin(float x) -> float
            {
                return 1.0 - cos(x);
            };
            
            // Coversine function
            def coversin(float x) -> float
            {
                return 1.0 - sin(x);
            };
            
            // Haversine function
            def haversin(float x) -> float
            {
                return versin(x) / 2.0;
            };
            
            // Exsecant function
            def exsec(float x) -> float
            {
                return sec(x) - 1.0;
            };
            
            // Excosecant function
            def excsc(float x) -> float
            {
                return csc(x) - 1.0;
            };
        };
        
        // ================================================================
        // EXPONENTIAL AND LOGARITHMIC FUNCTIONS
        // ================================================================
        
        namespace exponential
        {
            // Natural exponential function using Taylor series
            def exp(float x) -> float
            {
                if (x > 700.0) { return constants::INFINITY; };
                if (x < -700.0) { return 0.0; };
                
                float result = 1.0;
                float term = 1.0;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= x / (float)n;
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Exponential function base 2
            def exp2(float x) -> float
            {
                return exp(x * constants::LN2);
            };
            
            // Exponential function base 10
            def exp10(float x) -> float
            {
                return exp(x * constants::LN10);
            };
            
            // exp(x) - 1 (more accurate for small x)
            def expm1(float x) -> float
            {
                if (basic::abs(x) < 1e-5)
                {
                    return x + x * x / 2.0 + x * x * x / 6.0;
                };
                return exp(x) - 1.0;
            };
            
            // Natural logarithm using series expansion
            def ln(float x) -> float
            {
                if (x <= 0.0) { return constants::NAN; };
                if (x == 1.0) { return 0.0; };
                
                // Use the fact that ln(x) = 2 * ln((x-1)/(x+1)) for x near 1
                if (x > 0.5 and x < 2.0)
                {
                    float y = (x - 1.0) / (x + 1.0);
                    float y_squared = y * y;
                    float result = 2.0 * y;
                    float term = y;
                    
                    for (int n = 1; n <= 50; n++)
                    {
                        term *= y_squared;
                        float next_term = 2.0 * term / (2 * n + 1);
                        result += next_term;
                        if (basic::abs(next_term) < constants::FLOAT_EPSILON) { break; };
                    };
                    
                    return result;
                };
                
                // For x outside [0.5, 2], use ln(x) = ln(x/2^k) + k*ln(2)
                int k = 0;
                while (x > 2.0)
                {
                    x /= 2.0;
                    k++;
                };
                while (x < 0.5)
                {
                    x *= 2.0;
                    k--;
                };
                
                return ln(x) + (float)k * constants::LN2;
            };
            
            // Logarithm base 2
            def log2(float x) -> float
            {
                return ln(x) / constants::LN2;
            };
            
            // Logarithm base 10
            def log10(float x) -> float
            {
                return ln(x) / constants::LN10;
            };
            
            // Logarithm with arbitrary base
            def log(float x, float base) -> float
            {
                return ln(x) / ln(base);
            };
            
            // ln(1 + x) (more accurate for small x)
            def log1p(float x) -> float
            {
                if (x <= -1.0) { return constants::NAN; };
                if (basic::abs(x) < 1e-5)
                {
                    return x - x * x / 2.0 + x * x * x / 3.0;
                };
                return ln(1.0 + x);
            };
            
            // Power function using logarithms
            def pow(float base, float exponent) -> float
            {
                if (base == 0.0)
                {
                    return (exponent > 0.0) ? 0.0 : constants::INFINITY;
                };
                if (base == 1.0) { return 1.0; };
                if (exponent == 0.0) { return 1.0; };
                if (exponent == 1.0) { return base; };
                
                if (base < 0.0)
                {
                    // Handle negative base
                    int int_exp = (int)exponent;
                    if ((float)int_exp == exponent)
                    {
                        // Integer exponent
                        return basic::pow(base, int_exp);
                    };
                    return constants::NAN; // Complex result
                };
                
                return exp(exponent * ln(base));
            };
            
            // Hypot function: sqrt(x^2 + y^2) avoiding overflow
            def hypot(float x, float y) -> float
            {
                x = basic::abs(x);
                y = basic::abs(y);
                
                if (x < y)
                {
                    float temp = x;
                    x = y;
                    y = temp;
                };
                
                if (x == 0.0) { return 0.0; };
                
                float ratio = y / x;
                return x * basic::sqrt(1.0 + ratio * ratio);
            };
            
            // 3D hypot function
            def hypot3(float x, float y, float z) -> float
            {
                return basic::sqrt(x * x + y * y + z * z);
            };
        };
        
        // ================================================================
        // HYPERBOLIC FUNCTIONS
        // ================================================================
        
        namespace hyperbolic
        {
            // Hyperbolic sine
            def sinh(float x) -> float
            {
                if (basic::abs(x) > 700.0)
                {
                    return (x > 0.0) ? constants::INFINITY : constants::NEG_INFINITY;
                };
                
                float exp_x = exponential::exp(x);
                float exp_neg_x = exponential::exp(-x);
                return (exp_x - exp_neg_x) / 2.0;
            };
            
            // Hyperbolic cosine
            def cosh(float x) -> float
            {
                if (basic::abs(x) > 700.0)
                {
                    return constants::INFINITY;
                };
                
                float exp_x = exponential::exp(x);
                float exp_neg_x = exponential::exp(-x);
                return (exp_x + exp_neg_x) / 2.0;
            };
            
            // Hyperbolic tangent
            def tanh(float x) -> float
            {
                if (x > 20.0) { return 1.0; };
                if (x < -20.0) { return -1.0; };
                
                float exp_2x = exponential::exp(2.0 * x);
                return (exp_2x - 1.0) / (exp_2x + 1.0);
            };
            
            // Hyperbolic cotangent
            def coth(float x) -> float
            {
                if (x == 0.0) { return constants::INFINITY; };
                return 1.0 / tanh(x);
            };
            
            // Hyperbolic secant
            def sech(float x) -> float
            {
                return 1.0 / cosh(x);
            };
            
            // Hyperbolic cosecant
            def csch(float x) -> float
            {
                if (x == 0.0) { return constants::INFINITY; };
                return 1.0 / sinh(x);
            };
            
            // Inverse hyperbolic sine
            def asinh(float x) -> float
            {
                return exponential::ln(x + basic::sqrt(x * x + 1.0));
            };
            
            // Inverse hyperbolic cosine
            def acosh(float x) -> float
            {
                if (x < 1.0) { return constants::NAN; };
                return exponential::ln(x + basic::sqrt(x * x - 1.0));
            };
            
            // Inverse hyperbolic tangent
            def atanh(float x) -> float
            {
                if (basic::abs(x) >= 1.0) { return constants::NAN; };
                return 0.5 * exponential::ln((1.0 + x) / (1.0 - x));
            };
            
            // Inverse hyperbolic cotangent
            def acoth(float x) -> float
            {
                if (basic::abs(x) <= 1.0) { return constants::NAN; };
                return 0.5 * exponential::ln((x + 1.0) / (x - 1.0));
            };
            
            // Inverse hyperbolic secant
            def asech(float x) -> float
            {
                if (x <= 0.0 or x > 1.0) { return constants::NAN; };
                return exponential::ln((1.0 + basic::sqrt(1.0 - x * x)) / x);
            };
            
            // Inverse hyperbolic cosecant
            def acsch(float x) -> float
            {
                if (x == 0.0) { return constants::NAN; };
                return exponential::ln((1.0 + basic::sqrt(1.0 + x * x)) / basic::abs(x));
            };
        };
        
        // ================================================================
        // STATISTICAL FUNCTIONS
        // ================================================================
        
        namespace statistical
        {
            // Factorial function
            def factorial(int n) -> float
            {
                if (n < 0) { return constants::NAN; };
                if (n == 0 or n == 1) { return 1.0; };
                
                float result = 1.0;
                for (int i = 2; i <= n; i++)
                {
                    result *= (float)i;
                };
                return result;
            };
            
            // Double factorial
            def double_factorial(int n) -> float
            {
                if (n < 0) { return constants::NAN; };
                if (n == 0 or n == 1) { return 1.0; };
                
                float result = 1.0;
                for (int i = n; i > 0; i -= 2)
                {
                    result *= (float)i;
                };
                return result;
            };
            
            // Binomial coefficient
            def binomial(int n, int k) -> float
            {
                if (k < 0 or k > n) { return 0.0; };
                if (k == 0 or k == n) { return 1.0; };
                
                // Use symmetry property
                if (k > n - k) { k = n - k; };
                
                float result = 1.0;
                for (int i = 0; i < k; i++)
                {
                    result *= (float)(n - i) / (float)(i + 1);
                };
                return result;
            };
            
            // Gamma function using Lanczos approximation
            def gamma(float x) -> float
            {
                if (x < 0.0)
                {
                    return constants::PI / (trigonometric::sin(constants::PI * x) * gamma(1.0 - x));
                };
                if (x == 0.0) { return constants::INFINITY; };
                if (x == 1.0 or x == 2.0) { return 1.0; };
                
                // Lanczos coefficients for g = 7
                float coeff[] = {
                    0.99999999999980993,
                    676.5203681218851,
                    -1259.1392167224028,
                    771.32342877765313,
                    -176.61502916214059,
                    12.507343278686905,
                    -0.13857109526572012,
                    9.9843695780195716e-6,
                    1.5056327351493116e-7
                };
                
                if (x < 0.5)
                {
                    return constants::PI / (trigonometric::sin(constants::PI * x) * gamma(1.0 - x));
                };
                
                x -= 1.0;
                float a = coeff[0];
                for (int i = 1; i < 9; i++)
                {
                    a += coeff[i] / (x + (float)i);
                };
                
                float t = x + 7.5;
                return basic::sqrt(2.0 * constants::PI) * basic::pow(t, x + 0.5) * exponential::exp(-t) * a;
            };
            
            // Log gamma function
            def lgamma(float x) -> float
            {
                return exponential::ln(basic::abs(gamma(x)));
            };
            
            // Beta function
            def beta(float a, float b) -> float
            {
                return gamma(a) * gamma(b) / gamma(a + b);
            };
            
            // Error function using series expansion
            def erf(float x) -> float
            {
                if (x == 0.0) { return 0.0; };
                if (x > 5.0) { return 1.0; };
                if (x < -5.0) { return -1.0; };
                
                float result = 0.0;
                float term = x;
                float x_squared = x * x;
                
                for (int n = 0; n <= 50; n++)
                {
                    result += term / (2 * n + 1);
                    term *= -x_squared / (float)(n + 1);
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return 2.0 / basic::sqrt(constants::PI) * result;
            };
            
            // Complementary error function
            def erfc(float x) -> float
            {
                return 1.0 - erf(x);
            };
            
            // Inverse error function (approximation)
            def erfinv(float x) -> float
            {
                if (basic::abs(x) >= 1.0) { return constants::NAN; };
                if (x == 0.0) { return 0.0; };
                
                float a = 0.147;
                float ln_term = exponential::ln(1.0 - x * x);
                float term1 = 2.0 / (constants::PI * a) + ln_term / 2.0;
                float term2 = ln_term / a;
                
                float sqrt_term = basic::sqrt(term1 * term1 - term2);
                return basic::sign(x) * basic::sqrt(sqrt_term - term1);
            };
            
            // Normal distribution probability density function
            def normal_pdf(float x, float mean, float std_dev) -> float
            {
                if (std_dev <= 0.0) { return constants::NAN; };
                
                float normalized = (x - mean) / std_dev;
                return exponential::exp(-0.5 * normalized * normalized) / (std_dev * basic::sqrt(2.0 * constants::PI));
            };
            
            // Normal distribution cumulative distribution function
            def normal_cdf(float x, float mean, float std_dev) -> float
            {
                if (std_dev <= 0.0) { return constants::NAN; };
                
                float normalized = (x - mean) / (std_dev * basic::sqrt(2.0));
                return 0.5 * (1.0 + erf(normalized));
            };
        };
        
        // ================================================================
        // SPECIAL FUNCTIONS
        // ================================================================
        
        namespace special
        {
            // Bessel function of the first kind, order 0
            def j0(float x) -> float
            {
                if (x == 0.0) { return 1.0; };
                
                float result = 1.0;
                float term = 1.0;
                float x_squared = x * x;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= -x_squared / (4.0 * (float)(n * n));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Bessel function of the first kind, order 1
            def j1(float x) -> float
            {
                if (x == 0.0) { return 0.0; };
                
                float result = x / 2.0;
                float term = x / 2.0;
                float x_squared = x * x;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= -x_squared / (4.0 * (float)(n * (n + 1)));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Bessel function of the second kind, order 0 (approximation)
            def y0(float x) -> float
            {
                if (x <= 0.0) { return constants::NEG_INFINITY; };
                
                // Use asymptotic expansion for large x
                if (x > 8.0)
                {
                    float sqrt_term = basic::sqrt(2.0 / (constants::PI * x));
                    float phase = x - constants::PI_4;
                    return sqrt_term * trigonometric::sin(phase);
                };
                
                // Use series for small x (simplified)
                return (2.0 / constants::PI) * (exponential::ln(x / 2.0) + constants::EULER_GAMMA) * j0(x);
            };
            
            // Bessel function of the second kind, order 1 (approximation)
            def y1(float x) -> float
            {
                if (x <= 0.0) { return constants::NEG_INFINITY; };
                
                // Use asymptotic expansion for large x
                if (x > 8.0)
                {
                    float sqrt_term = basic::sqrt(2.0 / (constants::PI * x));
                    float phase = x - 3.0 * constants::PI_4;
                    return sqrt_term * trigonometric::sin(phase);
                };
                
                // Use series for small x (simplified)
                return (2.0 / constants::PI) * (exponential::ln(x / 2.0) + constants::EULER_GAMMA) * j1(x) - 2.0 / (constants::PI * x);
            };
            
            // Modified Bessel function of the first kind, order 0
            def i0(float x) -> float
            {
                float result = 1.0;
                float term = 1.0;
                float x_squared = x * x;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= x_squared / (4.0 * (float)(n * n));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Modified Bessel function of the first kind, order 1
            def i1(float x) -> float
            {
                if (x == 0.0) { return 0.0; };
                
                float result = x / 2.0;
                float term = x / 2.0;
                float x_squared = x * x;
                
                for (int n = 1; n <= 50; n++)
                {
                    term *= x_squared / (4.0 * (float)(n * (n + 1)));
                    result += term;
                    if (basic::abs(term) < constants::FLOAT_EPSILON) { break; };
                };
                
                return result;
            };
            
            // Elliptic integral of the first kind (complete)
            def elliptic_k(float k) -> float
            {
                if (basic::abs(k) >= 1.0) { return constants::INFINITY; };
                
                float a = 1.0;
                float b = basic::sqrt(1.0 - k * k);
                float c = k;
                
                while (basic::abs(c) > constants::FLOAT_EPSILON)
                {
                    float a_new = (a + b) / 2.0;
                    float b_new = basic::sqrt(a * b);
                    c = (a - b) / 2.0;
                    a = a_new;
                    b = b_new;
                };
                
                return constants::PI_2 / a;
            };
            
            // Elliptic integral of the second kind (complete)
            def elliptic_e(float k) -> float
            {
                if (basic::abs(k) >= 1.0) { return 1.0; };
                
                float a = 1.0;
                float b = basic::sqrt(1.0 - k * k);
                float c = k;
                float sum = 2.0;
                float power = 1.0;
                
                while (basic::abs(c) > constants::FLOAT_EPSILON)
                {
                    float a_new = (a + b) / 2.0;
                    float b_new = basic::sqrt(a * b);
                    power *= 2.0;
                    sum -= power * c * c;
                    c = (a - b) / 2.0;
                    a = a_new;
                    b = b_new;
                };
                
                return constants::PI_4 * sum / a;
            };
            
            // Riemann zeta function (for s > 1)
            def zeta(float s) -> float
            {
                if (s <= 1.0) { return constants::NAN; };
                
                float result = 0.0;
                for (int n = 1; n <= 1000; n++)
                {
                    result += 1.0 / basic::pow((float)n, (int)s);
                };
                return result;
            };
            
            // Digamma function (derivative of log gamma)
            def digamma(float x) -> float
            {
                if (x <= 0.0) { return constants::NAN; };
                
                float result = -constants::EULER_GAMMA;
                
                for (int n = 1; n <= 100; n++)
                {
                    result += x / ((float)n * ((float)n + x));
                };
                
                return result;
            };
            
            // Polygamma function (nth derivative of digamma)
            def polygamma(int n, float x) -> float
            {
                if (n < 0 or x <= 0.0) { return constants::NAN; };
                if (n == 0) { return digamma(x); };
                
                float result = 0.0;
                float sign = (n % 2 == 0) ? 1.0 : -1.0;
                
                for (int k = 0; k <= 100; k++)
                {
                    float term = statistical::factorial(n) / basic::pow(x + (float)k, n + 1);
                    result += term;
                };
                
                return sign * result;
            };
        };
        
        // ================================================================
        // CALCULUS-RELATED FUNCTIONS
        // ================================================================
        
        namespace calculus
        {
            // Numerical derivative using central difference
            template <T> derivative(T (*func)(T), T x, T h) -> T
            {
                return ((*func)(x + h) - (*func)(x - h)) / (2.0 * h);
            };
            
            // Numerical derivative with default step size
            template <T> derivative(T (*func)(T), T x) -> T
            {
                T h = basic::sqrt(constants::FLOAT_EPSILON) * basic::max(basic::abs(x), 1.0);
                return derivative<T>(func, x, h);
            };
            
            // Second derivative
            template <T> second_derivative(T (*func)(T), T x, T h) -> T
            {
                return ((*func)(x + h) - 2.0 * (*func)(x) + (*func)(x - h)) / (h * h);
            };
            
            // Second derivative with default step size
            template <T> second_derivative(T (*func)(T), T x) -> T
            {
                T h = basic::pow(constants::FLOAT_EPSILON, 1.0/3.0) * basic::max(basic::abs(x), 1.0);
                return second_derivative<T>(func, x, h);
            };
            
            // Numerical integration using Simpson's rule
            template <T> integrate_simpson(T (*func)(T), T a, T b, int n) -> T
            {
                if (n % 2 != 0) { n++; }; // Ensure even number of intervals
                
                T h = (b - a) / (T)n;
                T result = (*func)(a) + (*func)(b);
                
                // Add odd-indexed terms (coefficient 4)
                for (int i = 1; i < n; i += 2)
                {
                    result += 4.0 * (*func)(a + (T)i * h);
                };
                
                // Add even-indexed terms (coefficient 2)
                for (int i = 2; i < n; i += 2)
                {
                    result += 2.0 * (*func)(a + (T)i * h);
                };
                
                return result * h / 3.0;
            };
            
            // Numerical integration using trapezoidal rule
            template <T> integrate_trapezoid(T (*func)(T), T a, T b, int n) -> T
            {
                T h = (b - a) / (T)n;
                T result = ((*func)(a) + (*func)(b)) / 2.0;
                
                for (int i = 1; i < n; i++)
                {
                    result += (*func)(a + (T)i * h);
                };
                
                return result * h;
            };
            
            // Adaptive quadrature integration
            template <T> integrate_adaptive(T (*func)(T), T a, T b, T tolerance) -> T
            {
                T c = (a + b) / 2.0;
                T h = b - a;
                
                T fa = (*func)(a);
                T fb = (*func)(b);
                T fc = (*func)(c);
                
                T s1 = h * (fa + 4.0 * fc + fb) / 6.0; // Simpson's rule
                
                T d = (a + c) / 2.0;
                T e = (c + b) / 2.0;
                T fd = (*func)(d);
                T fe = (*func)(e);
                
                T s2 = h * (fa + 4.0 * fd + 2.0 * fc + 4.0 * fe + fb) / 12.0;
                
                if (basic::abs(s2 - s1) <= tolerance)
                {
                    return s2;
                };
                
                return integrate_adaptive<T>(func, a, c, tolerance / 2.0) + 
                       integrate_adaptive<T>(func, c, b, tolerance / 2.0);
            };
            
            // Newton-Raphson root finding
            template <T> find_root_newton(T (*func)(T), T (*func_prime)(T), T initial_guess, T tolerance, int max_iterations) -> T
            {
                T x = initial_guess;
                
                for (int i = 0; i < max_iterations; i++)
                {
                    T fx = (*func)(x);
                    T fpx = (*func_prime)(x);
                    
                    if (basic::abs(fpx) < tolerance) { break; }; // Derivative too small
                    
                    T x_new = x - fx / fpx;
                    
                    if (basic::abs(x_new - x) < tolerance) { return x_new; };
                    
                    x = x_new;
                };
                
                return x;
            };
            
            // Bisection method root finding
            template <T> find_root_bisection(T (*func)(T), T a, T b, T tolerance, int max_iterations) -> T
            {
                T fa = (*func)(a);
                T fb = (*func)(b);
                
                if (fa * fb >= 0.0) { return constants::NAN; }; // No root in interval
                
                for (int i = 0; i < max_iterations; i++)
                {
                    T c = (a + b) / 2.0;
                    T fc = (*func)(c);
                    
                    if (basic::abs(fc) < tolerance or (b - a) / 2.0 < tolerance)
                    {
                        return c;
                    };
                    
                    if (fa * fc < 0.0)
                    {
                        b = c;
                        fb = fc;
                    }
                    else
                    {
                        a = c;
                        fa = fc;
                    };
                };
                
                return (a + b) / 2.0;
            };
            
            // Secant method root finding
            template <T> find_root_secant(T (*func)(T), T x0, T x1, T tolerance, int max_iterations) -> T
            {
                for (int i = 0; i < max_iterations; i++)
                {
                    T f0 = (*func)(x0);
                    T f1 = (*func)(x1);
                    
                    if (basic::abs(f1 - f0) < tolerance) { break; };
                    
                    T x2 = x1 - f1 * (x1 - x0) / (f1 - f0);
                    
                    if (basic::abs(x2 - x1) < tolerance) { return x2; };
                    
                    x0 = x1;
                    x1 = x2;
                };
                
                return x1;
            };
            
            // Taylor series approximation
            template <T> taylor_series(T (*func)(T), T (*derivatives[])(T), T center, T x, int order) -> T
            {
                T result = (*func)(center);
                T power = 1.0;
                T factorial = 1.0;
                
                for (int n = 1; n <= order; n++)
                {
                    power *= (x - center);
                    factorial *= (T)n;
                    result += (*derivatives[n-1])(center) * power / factorial;
                };
                
                return result;
            };
            
            // Gradient descent optimization
            template <T> gradient_descent(T (*func)(T), T (*grad)(T), T initial, T learning_rate, T tolerance, int max_iterations) -> T
            {
                T x = initial;
                
                for (int i = 0; i < max_iterations; i++)
                {
                    T gradient = (*grad)(x);
                    T x_new = x - learning_rate * gradient;
                    
                    if (basic::abs(x_new - x) < tolerance) { return x_new; };
                    
                    x = x_new;
                };
                
                return x;
            };
        };
        
        // ================================================================
        // NUMBER THEORY FUNCTIONS
        // ================================================================
        
        namespace number_theory
        {
            // Check if a number is prime
            def is_prime(int n) -> bool
            {
                if (n < 2) { return false; };
                if (n == 2) { return true; };
                if (n % 2 == 0) { return false; };
                
                for (int i = 3; i * i <= n; i += 2)
                {
                    if (n % i == 0) { return false; };
                };
                
                return true;
            };
            
            // Next prime number
            def next_prime(int n) -> int
            {
                if (n < 2) { return 2; };
                
                n++;
                while (!is_prime(n))
                {
                    n++;
                };
                
                return n;
            };
            
            // Prime factorization (returns first factor)
            def prime_factor(int n) -> int
            {
                if (n < 2) { return n; };
                if (n % 2 == 0) { return 2; };
                
                for (int i = 3; i * i <= n; i += 2)
                {
                    if (n % i == 0) { return i; };
                };
                
                return n; // n is prime
            };
            
            // Euler's totient function
            def euler_phi(int n) -> int
            {
                if (n <= 0) { return 0; };
                
                int result = n;
                
                for (int p = 2; p * p <= n; p++)
                {
                    if (n % p == 0)
                    {
                        while (n % p == 0)
                        {
                            n /= p;
                        };
                        result -= result / p;
                    };
                };
                
                if (n > 1)
                {
                    result -= result / n;
                };
                
                return result;
            };
            
            // Möbius function
            def mobius(int n) -> int
            {
                if (n <= 0) { return 0; };
                if (n == 1) { return 1; };
                
                int prime_count = 0;
                int temp_n = n;
                
                for (int p = 2; p * p <= temp_n; p++)
                {
                    int count = 0;
                    while (temp_n % p == 0)
                    {
                        temp_n /= p;
                        count++;
                    };
                    
                    if (count > 1) { return 0; }; // Square factor
                    if (count == 1) { prime_count++; };
                };
                
                if (temp_n > 1) { prime_count++; }; // temp_n is prime
                
                return (prime_count % 2 == 0) ? 1 : -1;
            };
            
            // Sum of divisors function
            def sigma(int n) -> int
            {
                if (n <= 0) { return 0; };
                
                int sum = 0;
                for (int i = 1; i * i <= n; i++)
                {
                    if (n % i == 0)
                    {
                        sum += i;
                        if (i != n / i)
                        {
                            sum += n / i;
                        };
                    };
                };
                
                return sum;
            };
            
            // Number of divisors function
            def tau(int n) -> int
            {
                if (n <= 0) { return 0; };
                
                int count = 0;
                for (int i = 1; i * i <= n; i++)
                {
                    if (n % i == 0)
                    {
                        count++;
                        if (i != n / i)
                        {
                            count++;
                        };
                    };
                };
                
                return count;
            };
            
            // Modular exponentiation
            def mod_pow(int base, int exp, int mod) -> int
            {
                if (mod == 1) { return 0; };
                
                int result = 1;
                base = base % mod;
                
                while (exp > 0)
                {
                    if (exp % 2 == 1)
                    {
                        result = (result * base) % mod;
                    };
                    exp = exp / 2;
                    base = (base * base) % mod;
                };
                
                return result;
            };
            
            // Modular inverse using extended Euclidean algorithm
            def mod_inverse(int a, int mod) -> int
            {
                int old_r = a;
                int r = mod;
                int old_s = 1;
                int s = 0;
                
                while (r != 0)
                {
                    int quotient = old_r / r;
                    int temp_r = r;
                    r = old_r - quotient * r;
                    old_r = temp_r;
                    
                    int temp_s = s;
                    s = old_s - quotient * s;
                    old_s = temp_s;
                };
                
                if (old_r > 1) { return -1; }; // No inverse exists
                if (old_s < 0) { old_s += mod; };
                
                return old_s;
            };
            
            // Jacobi symbol
            def jacobi(int a, int n) -> int
            {
                if (n <= 0 or n % 2 == 0) { return 0; };
                
                a = a % n;
                int result = 1;
                
                while (a != 0)
                {
                    while (a % 2 == 0)
                    {
                        a /= 2;
                        if (n % 8 == 3 or n % 8 == 5)
                        {
                            result = -result;
                        };
                    };
                    
                    int temp = a;
                    a = n;
                    n = temp;
                    
                    if (a % 4 == 3 and n % 4 == 3)
                    {
                        result = -result;
                    };
                    
                    a = a % n;
                };
                
                return (n == 1) ? result : 0;
            };
            
            // Legendre symbol
            def legendre(int a, int p) -> int
            {
                if (!is_prime(p) or p == 2) { return 0; };
                return jacobi(a, p);
            };
        };
        
        // ================================================================
        // RANDOM NUMBER UTILITIES
        // ================================================================
        
        namespace random
        {
            // Linear congruential generator state
            object lcg_state
            {
                unsigned data{64} state;
                
                def __init() -> this
                {
                    this.state = 1;
                    return this;
                };
                
                def __init(unsigned data{64} seed) -> this
                {
                    this.state = seed;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
            };
            
            // Global RNG state
            lcg_state global_rng();
            
            // Seed the random number generator
            def seed(unsigned data{64} seed_value) -> void
            {
                global_rng.state = seed_value;
                return void;
            };
            
            // Generate random integer using LCG
            def rand_int() -> unsigned data{32}
            {
                global_rng.state = global_rng.state * 6364136223846793005 + 1442695040888963407;
                return (unsigned data{32})(global_rng.state >> 32);
            };
            
            // Generate random float in [0, 1)
            def rand_float() -> float
            {
                return (float)rand_int() / (float)0xFFFFFFFF;
            };
            
            // Generate random float in [min, max)
            def rand_range(float min_val, float max_val) -> float
            {
                return min_val + rand_float() * (max_val - min_val);
            };
            
            // Generate random integer in [min, max]
            def rand_int_range(int min_val, int max_val) -> int
            {
                if (min_val > max_val) { return min_val; };
                return min_val + (int)(rand_int() % (max_val - min_val + 1));
            };
            
            // Box-Muller transform for normal distribution
            def rand_normal(float mean, float std_dev) -> float
            {
                // Using Box-Muller transform
                static bool has_spare = false;
                static float spare;
                
                if (has_spare)
                {
                    has_spare = false;
                    return spare * std_dev + mean;
                };
                
                has_spare = true;
                float u = rand_float();
                float v = rand_float();
                
                float mag = std_dev * basic::sqrt(-2.0 * exponential::ln(u));
                spare = mag * trigonometric::cos(constants::TWO_PI * v);
                
                return mag * trigonometric::sin(constants::TWO_PI * v) + mean;
            };
            
            // Exponential distribution
            def rand_exponential(float lambda) -> float
            {
                if (lambda <= 0.0) { return constants::NAN; };
                return -exponential::ln(1.0 - rand_float()) / lambda;
            };
            
            // Gamma distribution (using Marsaglia-Tsang method for shape >= 1)
            def rand_gamma(float shape, float scale) -> float
            {
                if (shape <= 0.0 or scale <= 0.0) { return constants::NAN; };
                
                if (shape < 1.0)
                {
                    // Use rejection method
                    return rand_gamma(shape + 1.0, scale) * basic::pow(rand_float(), 1.0 / shape);
                };
                
                // Marsaglia-Tsang method
                float d = shape - 1.0 / 3.0;
                float c = 1.0 / basic::sqrt(9.0 * d);
                
                while (true)
                {
                    float x = rand_normal(0.0, 1.0);
                    float v = 1.0 + c * x;
                    
                    if (v <= 0.0) { continue; };
                    
                    v = v * v * v;
                    float u = rand_float();
                    
                    if (u < 1.0 - 0.0331 * x * x * x * x)
                    {
                        return d * v * scale;
                    };
                    
                    if (exponential::ln(u) < 0.5 * x * x + d * (1.0 - v + exponential::ln(v)))
                    {
                        return d * v * scale;
                    };
                };
            };
            
            // Beta distribution
            def rand_beta(float alpha, float beta) -> float
            {
                if (alpha <= 0.0 or beta <= 0.0) { return constants::NAN; };
                
                float x = rand_gamma(alpha, 1.0);
                float y = rand_gamma(beta, 1.0);
                
                return x / (x + y);
            };
        };
        
        // ================================================================
        // UTILITY FUNCTIONS
        // ================================================================
        
        // Check for NaN
        def isnan(float x) -> bool
        {
            return x != x;
        };
        
        // Check for infinity
        def isinf(float x) -> bool
        {
            return basic::abs(x) == constants::INFINITY;
        };
        
        // Check for finite value
        def isfinite(float x) -> bool
        {
            return !isnan(x) and !isinf(x);
        };
        
        // Check for normal number (not zero, subnormal, infinite, or NaN)
        def isnormal(float x) -> bool
        {
            return isfinite(x) and x != 0.0;
        };
        
        // Next representable value
        def nextafter(float x, float y) -> float
        {
            if (isnan(x) or isnan(y)) { return constants::NAN; };
            if (x == y) { return x; };
            
            // This is a simplified implementation
            if (x < y)
            {
                return x + constants::FLOAT_EPSILON;
            }
            else
            {
                return x - constants::FLOAT_EPSILON;
            };
        };
        
        // Copy sign
        def copysign(float magnitude, float sign) -> float
        {
            return (sign >= 0.0) ? basic::abs(magnitude) : -basic::abs(magnitude);
        };
        
        // Floating-point manipulation
        def ldexp(float x, int exp) -> float
        {
            return x * basic::pow(2.0, exp);
        };
        
        def frexp(float value, int* exp) -> float
        {
            if (value == 0.0)
            {
                *exp = 0;
                return 0.0;
            };
            
            *exp = 0;
            float abs_value = basic::abs(value);
            
            while (abs_value >= 2.0)
            {
                abs_value /= 2.0;
                (*exp)++;
            };
            
            while (abs_value < 1.0)
            {
                abs_value *= 2.0;
                (*exp)--;
            };
            
            return copysign(abs_value, value);
        };
        
        def modf(float value, float* iptr) -> float
        {
            *iptr = basic::trunc(value);
            return value - *iptr;
        };
        
        // Comparison functions
        def fdim(float x, float y) -> float
        {
            return basic::max(x - y, 0.0);
        };
        
        def fmax(float x, float y) -> float
        {
            if (isnan(x)) { return y; };
            if (isnan(y)) { return x; };
            return basic::max(x, y);
        };
        
        def fmin(float x, float y) -> float
        {
            if (isnan(x)) { return y; };
            if (isnan(y)) { return x; };
            return basic::min(x, y);
        };
        
        // Fused multiply-add
        def fma(float x, float y, float z) -> float
        {
            return x * y + z;
        };
    };
};