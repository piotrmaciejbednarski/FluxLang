namespace std {
    // Forward declarations
    struct DecimalSign;
    struct DecimalDigits;
    struct DecimalExponent;
    
    // Core decimal type
    class Decimal {
        // Internal representation structure
        struct DecimalState {
            int{32} sign;      // 1 for positive, -1 for negative
            int{32} precision; // Number of significant digits
            int{32} *digits;   // Dynamic array of digits
            int{32} length;    // Length of digits array
            int{32} exponent;  // Power of 10 exponent
        };

        object Decimal {
            // Max supported precision
            int{32} MAX_PRECISION = 100;
            
            // Instance state
            DecimalState state;
            
            // Constructor from string
            void init(char[] strVal) {
                state.sign = 1;
                state.precision = 0;
                state.length = 0;
                state.exponent = 0;
                
                // Parse string
                parse(strVal);
            };
            
            // Constructor from int
            void init(int{32} intVal) {
                state.sign = intVal >= 0 ? 1 : -1;
                int{32} absVal = Arithmetic.abs(intVal);
                
                // Count digits
                int{32} temp = absVal;
                while (temp > 0) {
                    state.length++;
                    temp = temp / 10;
                };
                
                // Allocate digits
                state.digits = memalloc(int{32}[state.length]);
                
                // Fill digits
                temp = absVal;
                for (int{32} i = state.length - 1; i >= 0; i--) {
                    state.digits[i] = temp % 10;
                    temp = temp / 10;
                };
                
                state.precision = state.length;
                state.exponent = 0;
            };
            
            // Parse string representation
            void parse(char[] str) {
                int{32} i = 0;
                
                // Handle sign
                if (str[0] == '-') {
                    state.sign = -1;
                    i++;
                } else if (str[0] == '+') {
                    i++;
                };
                
                // Count needed space
                int{32} digitCount = 0;
                int{32} decimalPos = -1;
                
                for (; i < length(str); i++) {
                    if (str[i] >= '0' && str[i] <= '9') {
                        digitCount++;
                    } else if (str[i] == '.') {
                        decimalPos = i;
                    };
                };
                
                // Allocate space
                state.digits = memalloc(int{32}[digitCount]);
                state.length = digitCount;
                
                // Fill digits and handle decimal point
                int{32} digitIndex = 0;
                i = state.sign == -1 ? 1 : 0;
                
                for (; i < length(str); i++) {
                    if (str[i] >= '0' && str[i] <= '9') {
                        state.digits[digitIndex++] = int{32}:(str[i] - '0');
                    } else if (str[i] == '.') {
                        state.exponent = -(length(str) - i - 1);
                    };
                };
                
                state.precision = digitCount;
            };
            
            // Addition
            Decimal add(Decimal other) {
                // Handle signs
                if (state.sign != other.state.sign) {
                    other.state.sign = -other.state.sign;
                    return subtract(other);
                };
                
                // Align exponents
                int{32} expDiff = state.exponent - other.state.exponent;
                if (expDiff < 0) {
                    shift_right(-expDiff);
                } else if (expDiff > 0) {
                    other.shift_right(expDiff);
                };
                
                // Add digits
                int{32} maxLen = max(state.length, other.state.length) + 1;
                int{32} *result = memalloc(int{32}[maxLen]);
                int{32} carry = 0;
                
                for (int{32} i = maxLen - 1; i >= 0; i--) {
                    int{32} sum = carry;
                    if (i < state.length) sum += state.digits[i];
                    if (i < other.state.length) sum += other.state.digits[i];
                    
                    result[i] = sum % 10;
                    carry = sum / 10;
                };
                
                // Create result decimal
                Decimal{} res;
                res.state.sign = state.sign;
                res.state.digits = result;
                res.state.length = maxLen;
                res.state.exponent = min(state.exponent, other.state.exponent);
                res.normalize();
                
                return res;
            };
            
            // Subtraction
            Decimal subtract(Decimal other) {
                // TODO: Implement subtraction
                return Decimal{};
            };
            
            // Multiplication
            Decimal multiply(Decimal other) {
                // TODO: Implement multiplication
                return Decimal{};
            };
            
            // Division
            Decimal divide(Decimal other) {
                // TODO: Implement division
                return Decimal{};
            };
            
            // Comparison operators
            bool equals(Decimal other) {
                if (state.sign != other.state.sign) return false;
                if (state.exponent != other.state.exponent) return false;
                if (state.length != other.state.length) return false;
                
                for (int{32} i = 0; i < state.length; i++) {
                    if (state.digits[i] != other.state.digits[i]) return false;
                };
                
                return true;
            };
            
            bool less_than(Decimal other) {
                // TODO: Implement less than
                return false;
            };
            
            // Utility functions
            void normalize() {
                // Remove leading zeros
                int{32} firstNonZero = 0;
                while (firstNonZero < state.length && state.digits[firstNonZero] == 0) {
                    firstNonZero++;
                };
                
                if (firstNonZero > 0) {
                    shift_left(firstNonZero);
                };
                
                // Remove trailing zeros and adjust exponent
                int{32} lastNonZero = state.length - 1;
                while (lastNonZero >= 0 && state.digits[lastNonZero] == 0) {
                    lastNonZero--;
                    state.exponent++;
                };
                
                state.length = lastNonZero + 1;
            };
            
            void shift_left(int{32} n) {
                if (n >= state.length) {
                    state.length = 0;
                    return;
                };
                
                for (int{32} i = 0; i < state.length - n; i++) {
                    state.digits[i] = state.digits[i + n];
                };
                
                state.length -= n;
            };
            
            void shift_right(int{32} n) {
                int{32} *newDigits = memalloc(int{32}[state.length + n]);
                
                // Fill new positions with zeros
                for (int{32} i = 0; i < n; i++) {
                    newDigits[i] = 0;
                };
                
                // Copy old digits
                for (int{32} i = 0; i < state.length; i++) {
                    newDigits[i + n] = state.digits[i];
                };
                
                state.digits = newDigits;
                state.length += n;
                state.exponent -= n;
            };
            
            // String conversion
            char[] toString() {
                // TODO: Implement string conversion
                return "";
            };
        };
    };
};
