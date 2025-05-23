// Arena Allocator Interface
namespace memory {
    object Arena {
        def __init(int size) -> void;
        def __exit() -> void;
        
        def allocate(int size) -> void*;
        def reset() -> void;
        def remaining() -> int;
        
        int size;
        int used;
        void* buffer;
    };
    
    // Helper functions
    def create_arena(int size) -> Arena*;
    def destroy_arena(Arena* arena) -> void;
};

import "arena.fx" as arena;
using memory;

object memory::Arena {
    def __init(int size) -> void {
        this.size = size;
        this.used = 0;
        this.buffer = (void*)malloc(size);
        if (this.buffer is void) {
            throw("Failed to allocate arena memory");
        };
    };

    def __exit() -> void {
        free(this.buffer);
        this.buffer = void;
    };

    def allocate(int size) -> void* {
        // Align to 8-byte boundary
        int aligned_size = (size + 7) & ~7;
        
        if (this.used + aligned_size > this.size) {
            return void; // Allocation failed
        };
        
        void* ptr = this.buffer + this.used;
        this.used += aligned_size;
        return ptr;
    };

    def reset() -> void {
        this.used = 0;
        // Optionally could zero memory
        // memset(this.buffer, 0, this.size);
    };

    def remaining() -> int {
        return this.size - this.used;
    };
};

// Helper function implementations
def memory::create_arena(int size) -> Arena* {
    Arena{}(size) new_arena;
    return @new_arena;
};

def memory::destroy_arena(Arena* arena) -> void {
    free(arena);
};