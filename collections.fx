// Basic Collection Types (Non-Templated Implementation)

import "types.fx";

namespace standard
{
    namespace collections
    {
        // Dynamic array for integers (would need separate implementations for other types)
        object int_array
        {
            i32* data;
            u32 size;
            u32 capacity;
            
            def __init() -> this
            {
                this.size = 0;
                this.capacity = 16;
                this.data = new i32[this.capacity];
                return this;
            };
            
            def __exit() -> void
            {
                (void)this.data;
            };
            
            def push_back(i32 value) -> void
            {
                if (this.size >= this.capacity)
                {
                    this.capacity *= 2;
                    i32* new_data = new i32[this.capacity];
                    for (u32 i = 0; i < this.size; i++)
                    {
                        new_data[i] = this.data[i];
                    };
                    (void)this.data;
                    this.data = new_data;
                };
                
                this.data[this.size] = value;
                this.size++;
            };
            
            def at(u32 index) -> i32
            {
                assert(index < this.size, "Index out of bounds");
                return this.data[index];
            };
        };

        // Similar implementation for float arrays
        object float_array
        {
            f32* data;
            u32 size;
            u32 capacity;
            
            // ... same methods as int_array but for f32 ...
        };

        // String array (array of string pointers)
        object string_array
        {
            string* data;
            u32 size;
            u32 capacity;
            
            // ... same methods but for string ...
        };
    };
};