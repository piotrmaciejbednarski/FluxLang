// Scientific notation operators
operator(int{32}, int{32})[e+]
{
    return x * pow(10, y);
};

operator(int{32}, int{32})[e-]
{
    return x * pow(10, -y);
};

operator(int{64}, int{32})[e+]
{
    return x * pow(10, y);
};

operator(int{64}, int{32})[e-]
{
    return x * pow(10, -y);
};

operator(float{32}, int{32})[e+]
{
    return x * pow(10.0, float{32}:y);
};

operator(float{32}, int{32})[e-]
{
    return x * pow(10.0, float{32}:-y);
};

operator(float{64}, int{32})[e+]
{
    return x * pow(10.0, float{64}:y);
};

operator(float{64}, int{32})[e-]
{
    return x * pow(10.0, float{64}:-y);
};

namespace std
{
    class Math 
    {
        object Constants
        {
            const float{64} PI = 3.14159265358979323846;
            const float{64} E = 2.71828182845904523536;
            const float{64} SQRT2 = 1.41421356237309504880;
            const float{64} LN2 = 0.69314718055994530942;
            const float{64} LN10 = 2.30258509299404568402;
            const float{64} LOG2E = 1.44269504088896340736;
            const float{64} LOG10E = 0.43429448190325182765;
            const float{64} PI_2 = 1.57079632679489661923;
            const float{64} PI_4 = 0.78539816339744830962;
            const float{64} INF = 1.0 / 0.0;
            const float{64} NAN = 0.0 / 0.0;
            const float{64} EPSILON = 2.2e-16;
            const float{64} MAX_VALUE = 1.7e+308;
            const float{64} MIN_VALUE = 5.0e-324;
        };

        object Complex
        {
            struct ComplexNum
            {
                float{64} real;
                float{64} imag;
            };

            ComplexNum create(float{64} real, float{64} imag)
            {
                ComplexNum num;
                num.real = real;
                num.imag = imag;
                return num;
            };

            ComplexNum add(ComplexNum a, ComplexNum b)
            {
                return create(a.real + b.real, a.imag + b.imag);
            };

            ComplexNum subtract(ComplexNum a, ComplexNum b)
            {
                return create(a.real - b.real, a.imag - b.imag);
            };

            ComplexNum multiply(ComplexNum a, ComplexNum b)
            {
                return create(
                    a.real * b.real - a.imag * b.imag,
                    a.real * b.imag + a.imag * b.real
                );
            };

            ComplexNum divide(ComplexNum a, ComplexNum b)
            {
                float{64} denom = b.real * b.real + b.imag * b.imag;
                return create(
                    (a.real * b.real + a.imag * b.imag) / denom,
                    (a.imag * b.real - a.real * b.imag) / denom
                );
            };

            float{64} magnitude(ComplexNum z)
            {
                return sqrt(z.real * z.real + z.imag * z.imag);
            };

            float{64} phase(ComplexNum z)
            {
                return atan2(z.imag, z.real);
            };

            ComplexNum exp(ComplexNum z)
            {
                float{64} r = exp(z.real);
                return create(r * cos(z.imag), r * sin(z.imag));
            };

            ComplexNum log(ComplexNum z)
            {
                return create(log(magnitude(z)), phase(z));
            };

            ComplexNum pow(ComplexNum base, ComplexNum exp)
            {
                return exp(multiply(log(base), exp));
            };
        };

        object BasicOps
        {
            int{32} abs(int{32} x)
            {
                return x < 0 ? -x : x;
            };

            float{64} abs(float{64} x)
            {
                return x < 0.0 ? -x : x;
            };

            float{64} floor(float{64} x)
            {
                return float{64}:int{64}:x;
            };

            float{64} ceil(float{64} x)
            {
                float{64} f = floor(x);
                return f == x ? f : f + 1.0;
            };

            float{64} round(float{64} x)
            {
                return floor(x + 0.5);
            };

            float{64} min(float{64} a, float{64} b)
            {
                return a < b ? a : b;
            };

            float{64} max(float{64} a, float{64} b)
            {
                return a > b ? a : b;
            };

            float{64} clamp(float{64} x, float{64} min, float{64} max)
            {
                return x < min ? min : (x > max ? max : x);
            };
        };

        object Exponential
        {
            float{64} exp(float{64} x)
            {
                if (x == 0.0) 
                    return 1.0;
                
                float{64} sum = 1.0;
                float{64} term = 1.0;
                float{64} n = 1.0;
                
                while (abs(term) > Constants.EPSILON)
                {
                    term = term * x / n;
                    sum = sum + term;
                    n = n + 1.0;
                };
                
                return sum;
            };

            float{64} log(float{64} x)
            {
                if (x <= 0.0) 
                    return Constants.NAN;
                if (x == 1.0) 
                    return 0.0;
                
                float{64} y = (x - 1.0) / (x + 1.0);
                float{64} y2 = y * y;
                float{64} sum = y;
                float{64} term = y;
                float{64} n = 1.0;
                
                while (abs(term) > Constants.EPSILON)
                {
                    n = n + 2.0;
                    term = term * y2 * (n - 2.0) / n;
                    sum = sum + term;
                };
                
                return 2.0 * sum;
            };

            float{64} pow(float{64} x, float{64} y)
            {
                return exp(y * log(x));
            };

            float{64} sqrt(float{64} x)
            {
                if (x < 0.0) 
                    return Constants.NAN;
                if (x == 0.0) 
                    return 0.0;
                
                float{64} guess = x / 2.0;
                float{64} prev;
                
                do
                {
                    prev = guess;
                    guess = (guess + x / guess) / 2.0;
                } while (abs(guess - prev) > Constants.EPSILON);
                
                return guess;
            };
        };

        object Trigonometry
        {
            float{64} sin(float{64} x)
            {
                x = x % (2.0 * Constants.PI);
                
                float{64} sum = 0.0;
                float{64} term = x;
                float{64} n = 1.0;
                int{32} sign = 1;
                
                while (abs(term) > Constants.EPSILON)
                {
                    sum = sum + sign * term;
                    sign = -sign;
                    n = n + 2.0;
                    term = term * x * x / (n * (n - 1.0));
                };
                
                return sum;
            };

            float{64} cos(float{64} x)
            {
                x = x % (2.0 * Constants.PI);
                
                float{64} sum = 1.0;
                float{64} term = 1.0;
                float{64} n = 0.0;
                int{32} sign = 1;
                
                while (abs(term) > Constants.EPSILON)
                {
                    n = n + 2.0;
                    sign = -sign;
                    term = term * x * x / (n * (n - 1.0));
                    sum = sum + sign * term;
                };
                
                return sum;
            };

            float{64} tan(float{64} x)
            {
                float{64} c = cos(x);
                if (abs(c) < Constants.EPSILON) 
                    return Constants.INF;
                return sin(x) / c;
            };

            float{64} asin(float{64} x)
            {
                if (abs(x) > 1.0) 
                    return Constants.NAN;
                return atan2(x, sqrt(1.0 - x * x));
            };

            float{64} acos(float{64} x)
            {
                if (abs(x) > 1.0) 
                    return Constants.NAN;
                return atan2(sqrt(1.0 - x * x), x);
            };

            float{64} atan(float{64} x)
            {
                if (x == 0.0) 
                    return 0.0;
                if (x > 0.0) 
                    return atan2(x, 1.0);
                return atan2(x, -1.0);
            };

            float{64} atan2(float{64} y, float{64} x)
            {
                if (x == 0.0)
                {
                    if (y > 0.0) 
                        return Constants.PI_2;
                    if (y < 0.0) 
                        return -Constants.PI_2;
                    return 0.0;
                };
                
                float{64} atan = atan(y / x);
                
                if (x < 0.0)
                {
                    if (y >= 0.0) 
                        return atan + Constants.PI;
                    return atan - Constants.PI;
                };
                
                return atan;
            };

            float{64} sinh(float{64} x)
            {
                float{64} e = exp(x);
                return (e - 1.0/e) / 2.0;
            };

            float{64} cosh(float{64} x)
            {
                float{64} e = exp(x);
                return (e + 1.0/e) / 2.0;
            };

            float{64} tanh(float{64} x)
            {
                float{64} e = exp(2.0 * x);
                return (e - 1.0) / (e + 1.0);
            };
        };

        object Vectors
        {
            struct Vector3
            {
                float{64} x;
                float{64} y;
                float{64} z;
            };

            Vector3 create(float{64} x, float{64} y, float{64} z)
            {
                Vector3 v;
                v.x = x;
                v.y = y;
                v.z = z;
                return v;
            };

            Vector3 add(Vector3 a, Vector3 b)
            {
                return create(a.x + b.x, a.y + b.y, a.z + b.z);
            };

            Vector3 subtract(Vector3 a, Vector3 b)
            {
                return create(a.x - b.x, a.y - b.y, a.z - b.z);
            };

            Vector3 scale(Vector3 v, float{64} s)
            {
                return create(v.x * s, v.y * s, v.z * s);
            };

            float{64} dot(Vector3 a, Vector3 b)
            {
                return a.x * b.x + a.y * b.y + a.z * b.z;
            };

            Vector3 cross(Vector3 a, Vector3 b)
            {
                return create(
                    a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x
                );
            };

            float{64} magnitude(Vector3 v)
            {
                return sqrt(dot(v, v));
            };

            Vector3 normalize(Vector3 v)
            {
                float{64} mag = magnitude(v);
                if (mag < Constants.EPSILON) 
                    return v;
                return scale(v, 1.0 / mag);
            };
        };

        object Statistics
        {
            float{64} mean(float{64}[] values)
            {
                float{64} sum = 0.0;
                for (int{32} i = 0; i < length(values); i++)
                {
                    sum = sum + values[i];
                };
                return sum / float{64}:length(values);
            };

            float{64} variance(float{64}[] values)
            {
                float{64} m = mean(values);
                float{64} sum = 0.0;
                
                for (int{32} i = 0; i < length(values); i++)
                {
                    float{64} diff = values[i] - m;
                    sum = sum + diff * diff;
                };
                
                return sum / float{64}:length(values);
            };

            float{64} stddev(float{64}[] values)
            {
                return sqrt(variance(values));
            };

            float{64} median(float{64}[] values)
            {
                int{32} len = length(values);
                if (len == 0) 
                    return Constants.NAN;
                
                if (len % 2 == 0)
                {
                    return (values[len/2 - 1] + values[len/2]) / 2.0;
                }
                else
                {
                    return values[len/2];
                };
            };
        };
    };
};

// Main function (required by Flux)
int{32} main() 
{
    // Example usage of the math library
    float{64} pi = std::Math::Constants.PI;
    print(i"Pi is approximately {pi}":{pi;});
    
    // Example of using complex numbers
    std::Math::Complex::ComplexNum c1 = std::Math::Complex::create(3.0, 4.0);
    float{64} mag = std::Math::Complex::magnitude(c1);
    print(i"Magnitude of 3+4i is {mag}":{mag;});
    
    return 0;
};
