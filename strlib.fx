namespace std {
    class String {
        // Core string operations object
        object StringOps {
            // Get string length - note: this is just a wrapper around built-in length()
            int{32} length(string str) {
                return length(str);
            };

            // String comparison
            int{32} compare(string a, string b) {
                if (a < b) return -1;
                if (a > b) return 1;
                return 0;
            };

            // String concatenation - note: + is native for strings
            string concat(string a, string b) {
                return a + b;
            };

            // String substring
            string substring(string str, int{32} start, int{32} end) {
                if (start < 0 || end > length(str) || start > end) {
                    throw("Invalid substring indices");
                };
                
                // Use native array slicing
                return str[start:end];
            };
        };

        // String conversion object
        object Convert {
            // Integer to string
            string fromInt(int{32} value) {
                if (value == 0) {
                    return "0";
                };
                
                string result = "";
                int{32} isNegative = value < 0;
                
                if (isNegative) {
                    value = -value;
                };
                
                while (value > 0) {
                    int{32} digit = value % 10;
                    // Convert digit to string and concatenate
                    result = string:digit + result;
                    value = value / 10;
                };
                
                if (isNegative) {
                    result = "-" + result;
                };
                
                return result;
            };

            // Float to string with precision
            string fromFloat(float{64} value, int{32} precision) {
                if (value == 0.0) {
                    return "0";
                };
                
                int{32} isNegative = value < 0.0;
                if (isNegative) value = -value;
                
                // Handle integer part
                int{32} intPart = int{32}:value;
                float{64} fracPart = value - float{64}:intPart;
                
                // Convert integer part
                string result = fromInt(intPart);
                
                if (precision <= 0) {
                    return result;
                };
                
                result = result + ".";
                
                // Convert fractional part
                for (int{32} i = 0; i < precision; i++) {
                    fracPart = fracPart * 10.0;
                    int{32} digit = int{32}:fracPart;
                    result = result + string:digit;
                    fracPart = fracPart - float{64}:digit;
                };
                
                if (isNegative) {
                    result = "-" + result;
                };
                
                return result;
            };

            // String to integer
            int{32} toInt(string str) {
                int{32} result = 0;
                int{32} i = 0;
                int{32} sign = 1;
                
                if (str[0] == '-') {
                    sign = -1;
                    i = i + 1;
                };
                
                while (i < length(str)) {
                    if (str[i] >= '0' && str[i] <= '9') {
                        result = result * 10 + int{32}:str[i] - int{32}:'0';
                    } else {
                        throw("Invalid integer string");
                    };
                    i = i + 1;
                };
                
                return sign * result;
            };

            // String to float
            float{64} toFloat(string str) {
                float{64} result = 0.0;
                int{32} i = 0;
                int{32} sign = 1;
                
                if (str[0] == '-') {
                    sign = -1;
                    i = i + 1;
                };
                
                // Parse integer part
                while (i < length(str) && str[i] != '.') {
                    if (str[i] >= '0' && str[i] <= '9') {
                        result = result * 10.0 + float{64}:(int{32}:str[i] - int{32}:'0');
                    } else {
                        throw("Invalid float string");
                    };
                    i = i + 1;
                };
                
                // Parse fractional part
                if (i < length(str) && str[i] == '.') {
                    i = i + 1;
                    float{64} fraction = 0.1;
                    while (i < length(str)) {
                        if (str[i] >= '0' && str[i] <= '9') {
                            result = result + fraction * float{64}:(int{32}:str[i] - int{32}:'0');
                            fraction = fraction * 0.1;
                        } else {
                            throw("Invalid float string");
                        };
                        i = i + 1;
                    };
                };
                
                return sign * result;
            };
        };

        // String search and manipulation object
        object Search {
            // Find first occurrence of substring
            int{32} find(string str, string substr) {
                int{32} strLen = length(str);
                int{32} subLen = length(substr);
                
                if (subLen > strLen) return -1;
                
                for (int{32} i = 0; i <= strLen - subLen; i++) {
                    if (str[i:i+subLen] == substr) {
                        return i;
                    };
                };
                
                return -1;
            };

            // Replace all occurrences of substring
            string replace(string str, string old, string new) {
                int{32} pos = find(str, old);
                
                if (pos == -1) {
                    return str;
                };
                
                string result = str[0:pos] + new;
                pos = pos + length(old);
                
                while (pos < length(str)) {
                    int{32} nextPos = find(str[pos:], old);
                    if (nextPos == -1) {
                        result = result + str[pos:];
                        break;
                    };
                    result = result + str[pos:pos+nextPos] + new;
                    pos = pos + nextPos + length(old);
                };
                
                return result;
            };

            // Split string into array by delimiter
            string[] split(string str, string delim) {
                string[] result = [];
                int{32} start = 0;
                int{32} pos = 0;
                
                while ((pos = find(str[start:], delim)) != -1) {
                    result = result + [str[start:start+pos]];
                    start = start + pos + length(delim);
                };
                
                // Add remaining part
                if (start < length(str)) {
                    result = result + [str[start:]];
                };
                
                return result;
            };
        };
    };
};
