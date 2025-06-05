// string.fx - Standard String Library for Flux

namespace std
{
    object string
    {
        // Internal storage - array of 8-bit unsigned data
        unsigned data{8}[] internal_data;
        
        // Magic methods
        def __new() -> this
        {
            this.internal_data = [];
            return;
        }
        
        def __init(unsigned data{8}[] initial = []) -> this
        {
            this.internal_data = initial;
            return;
        }
        
        def __exit() -> void
        {
            this.internal_data = void;
            return;
        }
        
        // Conversion magic methods
        def __expr() -> this
        {
            return this.internal_data;
        }
        
        def __eq(this other) -> bool
        {
            if (this.len() != other.len())
                return false;
                
            for (int i = 0; i < this.len(); i++)
            {
                if (this.internal_data[i] != other.internal_data[i])
                    return false;
            }
            return true;
        }
        
        // Operator overloading
        def __add(this other) -> this
        {
            this result;
            result.internal_data = this.internal_data + other.internal_data;
            return result;
        }
        
        def __iadd(this other) -> this
        {
            this.internal_data = this.internal_data + other.internal_data;
            return this;
        }
        
        // Properties
        def len() -> int
        {
            return sizeof(this.internal_data) / 8;
        }
        
        // Accessors
        def at(int index) -> char
        {
            if (index < 0 || index >= this.length())
            {
                throw "Index out of bounds";
            }
            return (char)this.internal_data[index];
        }
        t
        def setChar(int index, unsigned data{8} c) -> void
        {
            if (index < 0 || index >= this.length())
            {
                throw "Index out of bounds";
            }
            this.internal_data[index] = c;
            return;
        }
        
        // String operations
        def substring(int start, int end = -1) -> this
        {
            int len = this.len();
            if (end == -1) end = len;
            if (start < 0 || end > len || start > end)
            {
                throw "Invalid substring range";
            }
            return (this)(this.internal_data[start..end]);
        }
        
        def find(this pattern) -> int
        {
            for (int i = 0; i <= this.len() - pattern.len(); i++)
            {
                bool found = true;
                for (int j = 0; j < pattern.len(); j++)
                {
                    if (this.internal_data[i + j] != pattern.internal_data[j])
                    {
                        found = false;
                        break;
                    }
                }
                if (found) {return i;};
            }
            return -1;
        }
        
        def toUpper() -> this
        {
            this result;
            result.internal_data = [];
            for (byte in this.internal_data)
            {
                if (byte >= 'a' && byte <= 'z')
                {
                    result.internal_data += byte - 32;
                }
                else
                {
                    result.internal_data += byte;
                }
            }
            return result;
        }
        
        // Static constructor methods
        def fromInt(int value) -> this
        {
            if (value == 0) {return "0";};
            this result;
            bool negative = value < 0;
            if (negative) {value *= -1;};
            
            while (value > 0)
            {
                result.internal_data = [('0' + (value % 10))] + result.internal_data;
                value /= 10;
            }
            
            if (negative)
            {
                result.internal_data = ['-'] + result.internal_data;
            }
            return result;
        }
        
        static def fromFloat(float value, int precision = 6) -> this
        {
            // Actual implementation would use floating point conversion
            return (this)"0.0";
        }
    };

    object StringBuilder
    {
        string[] parts;
        int total_length = 0;
        
        def __init() -> this
        {
            this.parts = [];
            return this;
        }
        
        def append(string s) -> void
        {
            this.parts += s;
            this.total_length += s.length();
            return;
        }
        
        def toString() -> string
        {
            string result;
            unsigned data{8}[] combined;
            combined.length = this.total_length;
            
            int position = 0;
            for (part in this.parts)
            {
                for (int i = 0; i < part.len(); i++)
                {
                    combined[position] = part.at(i);
                    position++;
                }
            }
            
            result.internal_data = combined;
            return result;
        }
    };
};