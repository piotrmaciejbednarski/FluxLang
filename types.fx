// Standard Type Definitions

namespace standard
{
    namespace types
    {
        global
        {
            // Signed integers
            signed data{8} as i8;
            signed data{16} as i16;
            signed data{32} as i32;
            signed data{64} as i64;
            
            // Unsigned integers
            unsigned data{8} as u8;
            unsigned data{16} as u16;
            unsigned data{32} as u32;
            unsigned data{64} as u64;
            
            // String type (non-OOP)
            unsigned data{8}[] as noopstring;
        };

        // Non-globals, must explicitly reference / "rename" / make global
        
        // Pointer types
        object weak_ptr
        {
            void* ptr;
            
            def __init(void* p) -> this
            {
                this.ptr = p;
                return this;
            };
            
            def __exit() -> void
            {
                (void)this.ptr;
            };
        };

        object string
        {
            noopstr base;

            def __init(noopstr o) -> this
            {
                self.base = o;
                return this;
            };

            def __exit() -> void
            {
                (void)this.base;
            };
        };
    };
};