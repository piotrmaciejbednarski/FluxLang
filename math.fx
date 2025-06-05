namespace std
{
    namespace math
    {
        // Mathematical constants
        const float PI = 3.141592653589793;
        const float E = 2.718281828459045;
        const float TAU = 6.283185307179586;
        const float GOLDEN_RATIO = 1.618033988749894;
        
        // Complex number implementation with full operator overloading
        object Complex
        {
            float real;
            float imag;
            
            def __init(float r, float i) -> this
            {
                this.real = r;
                this.imag = i;
                return;
            };
            
            def __init(float r) -> this
            {
                this.real = r;
                this.imag = 0.0;
                return;
            };
            
            def __exit() -> void
            {
                return;
            };
            
            def __add(Complex other) -> Complex
            {
                Complex result(this.real + other.real, this.imag + other.imag);
                return result;
            };
            
            def __sub(Complex other) -> Complex
            {
                Complex result(this.real - other.real, this.imag - other.imag);
                return result;
            };
            
            def __mul(Complex other) -> Complex
            {
                // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
                float new_real = this.real * other.real - this.imag * other.imag;
                float new_imag = this.real * other.imag + this.imag * other.real;
                Complex result(new_real, new_imag);
                return result;
            };
            
            def __div(Complex other) -> Complex
            {
                // Division: (a + bi)/(c + di) = [(ac + bd) + (bc - ad)i] / (c² + d²)
                float denominator = other.real * other.real + other.imag * other.imag;
                float new_real = (this.real * other.real + this.imag * other.imag) / denominator;
                float new_imag = (this.imag * other.real - this.real * other.imag) / denominator;
                Complex result(new_real, new_imag);
                return result;
            };
            
            def magnitude() -> float
            {
                return sqrt(this.real * this.real + this.imag * this.imag);
            };
            
            def phase() -> float
            {
                return atan2(this.imag, this.real);
            };
            
            def conjugate() -> Complex
            {
                Complex result(this.real, -this.imag);
                return result;
            };
        };
        
        // Custom operators for scientific notation (properly with 2 parameters)
        operator(float base, float exponent)[e_plus] -> float
        {
            return base * pow(10.0, exponent);
        };
        
        operator(float base, float exponent)[e_minus] -> float
        {
            return base * pow(10.0, -exponent);
        };
        
        // Custom operators for mathematical operations
        operator(float a, float b)[hypot] -> float
        {
            return sqrt(a * a + b * b);
        };
        
        operator(Complex a, Complex b)[c_exp] -> Complex
        {
            // Complex exponentiation: a^b = e^(b * ln(a))
            if (a.real == 0.0 and a.imag == 0.0)
            {
                Complex result(0.0, 0.0);
                return result;
            };
            
            float ln_magnitude = ln(a.magnitude());
            float ln_phase = a.phase();
            
            // b * ln(a) = b * (ln|a| + i*arg(a))
            float exp_real = b.real * ln_magnitude - b.imag * ln_phase;
            float exp_imag = b.real * ln_phase + b.imag * ln_magnitude;
            
            // e^(exp_real + i*exp_imag)
            float magnitude = exp(exp_real);
            Complex result(magnitude * cos(exp_imag), magnitude * sin(exp_imag));
            return result;
        };
        
        operator(float a, float b)[mod] -> float
        {
            // Proper mathematical modulo (not remainder)
            float result = a - b * floor(a / b);
            return result;
        };
        
        // Template-based mathematical functions for type flexibility
        template <T> abs(T x) -> T
        {
            if (x < (T)0)
            {
                return -x;
            };
            return x;
        };
        
        template <T> min(T a, T b) -> T
        {
            if (a < b)
            {
                return a;
            };
            return b;
        };
        
        template <T> max(T a, T b) -> T
        {
            if (a > b)
            {
                return a;
            };
            return b;
        };
        
        template <T> clamp(T value, T min_val, T max_val) -> T
        {
            if (value < min_val)
            {
                return min_val;
            };
            if (value > max_val)
            {
                return max_val;
            };
            return value;
        };
        
        // Advanced numerical methods using templates
        template <T> newton_raphson(T initial_guess, T target, int max_iterations) -> T
        {
            T x = initial_guess;
            const T epsilon = (T)0.000001;
            
            for (int i = 0; i < max_iterations; i++)
            {
                T fx = x * x - target;  // f(x) = x² - target
                T fpx = (T)2 * x;       // f'(x) = 2x
                
                if (abs<T>(fpx) < epsilon)
                {
                    break;
                };
                
                T new_x = x - fx / fpx;
                
                if (abs<T>(new_x - x) < epsilon)
                {
                    break;
                };
                
                x = new_x;
            };
            
            return x;
        };
        
        // Efficient implementations of core functions
        def sqrt(float x) -> float
        {
            if (x < 0.0)
            {
                return 0.0;
            };
            
            if (x == 0.0 or x == 1.0)
            {
                return x;
            };
            
            return newton_raphson<float>(x / 2.0, x, 50);
        };
        
        def pow(float base, float exponent) -> float
        {
            if (exponent == 0.0)
            {
                return 1.0;
            };
            
            if (base == 0.0)
            {
                return 0.0;
            };
            
            if (base == 1.0)
            {
                return 1.0;
            };
            
            // Handle integer exponents efficiently
            int int_exp = (int)exponent;
            if ((float)int_exp == exponent)
            {
                return integer_power(base, int_exp);
            };
            
            // For fractional exponents: a^b = e^(b * ln(a))
            return exp(exponent * ln(base));
        };
        
        def integer_power(float base, int exponent) -> float
        {
            if (exponent == 0)
            {
                return 1.0;
            };
            
            if (exponent < 0)
            {
                return 1.0 / integer_power(base, -exponent);
            };
            
            // Fast exponentiation by squaring
            float result = 1.0;
            float current_power = base;
            int exp = exponent;
            
            while (exp > 0)
            {
                if (exp % 2 == 1)
                {
                    result = result * current_power;
                };
                current_power = current_power * current_power;
                exp = exp / 2;
            };
            
            return result;
        };
        
        def exp(float x) -> float
        {
            if (x == 0.0)
            {
                return 1.0;
            };
            
            // For large values, use the identity e^x = (e^(x/n))^n
            if (abs<float>(x) > 1.0)
            {
                int n = (int)abs<float>(x) + 1;
                float reduced_x = x / (float)n;
                float result = exp_series(reduced_x);
                return integer_power(result, n);
            };
            
            return exp_series(x);
        };
        
        def exp_series(float x) -> float
        {
            // Taylor series: e^x = 1 + x + x²/2! + x³/3! + ...
            float result = 1.0;
            float term = 1.0;
            const float epsilon = 0.000001;
            
            for (int n = 1; n <= 50; n++)
            {
                term = term * x / (float)n;
                result = result + term;
                
                if (abs<float>(term) < epsilon)
                {
                    break;
                };
            };
            
            return result;
        };
        
        def ln(float x) -> float
        {
            if (x <= 0.0)
            {
                return 0.0;
            };
            
            if (x == 1.0)
            {
                return 0.0;
            };
            
            // For x close to 1, use series expansion
            if (x > 0.5 and x < 1.5)
            {
                float u = x - 1.0;
                return ln_series(u);
            };
            
            // For other values, use the identity ln(x) = 2 * ln(√x)
            if (x > 1.5)
            {
                return 2.0 * ln(sqrt(x));
            }
            else
            {
                return -ln(1.0 / x);
            };
        };
        
        def ln_series(float u) -> float
        {
            // ln(1+u) = u - u²/2 + u³/3 - u⁴/4 + ...
            float result = 0.0;
            float term = u;
            int sign = 1;
            const float epsilon = 0.000001;
            
            for (int n = 1; n <= 50; n++)
            {
                result = result + (float)sign * term / (float)n;
                term = term * u;
                sign = -sign;
                
                if (abs<float>(term / (float)n) < epsilon)
                {
                    break;
                };
            };
            
            return result;
        };
        
        def sin(float x) -> float
        {
            // Normalize to [-2π, 2π]
            x = x mod TAU;
            if (x > PI)
            {
                x = x - TAU;
            };
            if (x < -PI)
            {
                x = x + TAU;
            };
            
            // Taylor series: sin(x) = x - x³/3! + x⁵/5! - ...
            float result = 0.0;
            float term = x;
            int sign = 1;
            const float epsilon = 0.000001;
            
            for (int n = 1; n <= 20; n++)
            {
                result = result + (float)sign * term;
                term = term * x * x / ((float)(2 * n) * (float)(2 * n + 1));
                sign = -sign;
                
                if (abs<float>(term) < epsilon)
                {
                    break;
                };
            };
            
            return result;
        };
        
        def cos(float x) -> float
        {
            // Use identity: cos(x) = sin(π/2 - x)
            return sin(PI / 2.0 - x);
        };
        
        def tan(float x) -> float
        {
            float cos_x = cos(x);
            if (abs<float>(cos_x) < 0.000001)
            {
                return 0.0; // Avoid division by zero
            };
            return sin(x) / cos_x;
        };
        
        def atan2(float y, float x) -> float
        {
            if (x > 0.0)
            {
                return atan(y / x);
            }
            else if (x < 0.0 and y >= 0.0)
            {
                return atan(y / x) + PI;
            }
            else if (x < 0.0 and y < 0.0)
            {
                return atan(y / x) - PI;
            }
            else if (x == 0.0 and y > 0.0)
            {
                return PI / 2.0;
            }
            else if (x == 0.0 and y < 0.0)
            {
                return -PI / 2.0;
            };
            
            return 0.0;
        };
        
        def atan(float x) -> float
        {
            if (abs<float>(x) > 1.0)
            {
                float sign = (x > 0.0) ? 1.0 : -1.0;
                return sign * (PI / 2.0 - atan(1.0 / abs<float>(x)));
            };
            
            // Taylor series: atan(x) = x - x³/3 + x⁵/5 - ...
            float result = 0.0;
            float term = x;
            const float epsilon = 0.000001;
            
            for (int n = 1; n <= 50; n += 2)
            {
                if ((n / 2) % 2 == 0)
                {
                    result = result + term / (float)n;
                }
                else
                {
                    result = result - term / (float)n;
                };
                
                term = term * x * x;
                
                if (abs<float>(term / (float)n) < epsilon)
                {
                    break;
                };
            };
            
            return result;
        };
        
        // Rounding functions
        def floor(float x) -> int
        {
            int int_x = (int)x;
            if (x >= 0.0 or x == (float)int_x)
            {
                return int_x;
            }
            else
            {
                return int_x - 1;
            };
        };
        
        def ceil(float x) -> int
        {
            int int_x = (int)x;
            if (x <= 0.0 or x == (float)int_x)
            {
                return int_x;
            }
            else
            {
                return int_x + 1;
            };
        };
        
        def round(float x) -> int
        {
            if (x >= 0.0)
            {
                return floor(x + 0.5);
            }
            else
            {
                return ceil(x - 0.5);
            };
        };
        
        // Angle conversion functions (not operators since they're unary)
        def to_radians(float degrees) -> float
        {
            return degrees * PI / 180.0;
        };
        
        def to_degrees(float radians) -> float
        {
            return radians * 180.0 / PI;
        };
        
        // Random number generation (volatile is appropriate here - we don't want optimization)
        volatile int rng_state = 1;
        
        def seed_random(int seed) -> void
        {
            rng_state = seed;
            return;
        };
        
        def random() -> float
        {
            // Linear congruential generator
            rng_state = (rng_state * 1664525 + 1013904223) % 2147483647;
            return (float)rng_state / 2147483647.0;
        };
        
        def random_range(float min_val, float max_val) -> float
        {
            return min_val + random() * (max_val - min_val);
        };
        
        def random_int(int min_val, int max_val) -> int
        {
            if (min_val >= max_val)
            {
                return min_val;
            };
            
            int range = max_val - min_val + 1;
            return min_val + (int)(random() * (float)range);
        };
        
        // Advanced matrix operations using templates
        struct template <T> Matrix
        {
            T[][] elements;
            int rows;
            int cols;
        };
        
        template <T> matrix_multiply(Matrix<T> a, Matrix<T> b) -> Matrix<T>
        {
            assert(a.cols == b.rows, "Matrix dimensions incompatible for multiplication");
            
            Matrix<T> result;
            result.rows = a.rows;
            result.cols = b.cols;
            result.elements = [[0 for (j in 0..result.cols)] for (i in 0..result.rows)];
            
            for (int i = 0; i < result.rows; i++)
            {
                for (int j = 0; j < result.cols; j++)
                {
                    T sum = (T)0;
                    for (int k = 0; k < a.cols; k++)
                    {
                        sum = sum + a.elements[i][k] * b.elements[k][j];
                    };
                    result.elements[i][j] = sum;
                };
            };
            
            return result;
        };
    };
};