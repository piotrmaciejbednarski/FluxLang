#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <cassert>

namespace flux {
namespace common {

// Arena allocator for efficient memory management
class Arena {
public:
    // Default constructor
    Arena(size_t blockSize = DefaultBlockSize);
    
    // Destructor
    ~Arena();
    
    // Disable copy and move
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;
    
    // Allocate memory for a single object
    template<typename T, typename... Args>
    T* alloc(Args&&... args) {
        static_assert(alignof(T) <= alignof(max_align_t), "Alignment too large");
        
        void* memory = allocRaw(sizeof(T));
        return new(memory) T(std::forward<Args>(args)...);
    }
    
    // Allocate memory for an array of objects
    template<typename T>
    T* allocArray(size_t count) {
        static_assert(alignof(T) <= alignof(max_align_t), "Alignment too large");
        
        void* memory = allocRaw(sizeof(T) * count);
        T* array = static_cast<T*>(memory);
        
        // Initialize elements with default constructor
        for (size_t i = 0; i < count; ++i) {
            new(array + i) T();
        }
        
        return array;
    }
    
    // Allocate memory for an array of objects without initialization
    template<typename T>
    T* allocArrayRaw(size_t count) {
        static_assert(alignof(T) <= alignof(max_align_t), "Alignment too large");
        
        void* memory = allocRaw(sizeof(T) * count);
        return static_cast<T*>(memory);
    }
    
    // Allocate raw memory
    void* allocRaw(size_t size);
    
    // Reset arena (deallocate all memory)
    void reset();
    
    // Get total memory allocated
    size_t totalAllocated() const { return totalAllocated_; }
    
    // Get block size
    size_t blockSize() const { return blockSize_; }
    
    // Get default arena
    static Arena& defaultArena();
    
    // Default block size (16 KB)
    static constexpr size_t DefaultBlockSize = 16 * 1024;

private:
    // Memory block
    struct Block {
        char* memory;
        size_t size;
        size_t used;
        
        Block(size_t size);
        ~Block();
        
        // Disable copy and move
        Block(const Block&) = delete;
        Block& operator=(const Block&) = delete;
        Block(Block&&) = delete;
        Block& operator=(Block&&) = delete;
        
        // Check if block has enough space
        bool canFit(size_t size, size_t alignment) const;
        
        // Allocate memory from this block
        void* allocate(size_t size, size_t alignment);
    };
    
    // Vector of memory blocks
    std::vector<std::unique_ptr<Block>> blocks_;
    
    // Current active block
    Block* currentBlock_;
    
    // Block size
    size_t blockSize_;
    
    // Total memory allocated
    size_t totalAllocated_;
    
    // Create a new block
    Block* newBlock(size_t minSize);
    
    // Align pointer to the specified alignment
    static size_t align(size_t n, size_t alignment) {
        return (n + alignment - 1) & ~(alignment - 1);
    }
};

} // namespace common
} // namespace flux