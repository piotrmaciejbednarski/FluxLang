#include "arena.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace flux {
namespace common {

// Block constructor
Arena::Block::Block(size_t size) : size(size), used(0) {
    // Allocate memory
    memory = static_cast<char*>(std::malloc(size));
    if (!memory) {
        throw std::bad_alloc();
    }
}

// Block destructor
Arena::Block::~Block() {
    std::free(memory);
}

// Check if block has enough space
bool Arena::Block::canFit(size_t size, size_t alignment) const {
    size_t alignedUsed = Arena::align(used, alignment);
    return alignedUsed + size <= this->size;
}

// Allocate memory from this block
void* Arena::Block::allocate(size_t size, size_t alignment) {
    size_t alignedUsed = Arena::align(used, alignment);
    void* ptr = memory + alignedUsed;
    used = alignedUsed + size;
    return ptr;
}

// Arena constructor
Arena::Arena(size_t blockSize)
    : currentBlock_(nullptr), blockSize_(blockSize), totalAllocated_(0) {
    // Create initial block
    currentBlock_ = newBlock(blockSize_);
}

// Arena destructor
Arena::~Arena() {
    // Blocks are automatically freed by unique_ptr
}

// Allocate raw memory
void* Arena::allocRaw(size_t size) {
    // Handle large allocations
    if (size > blockSize_ / 2) {
        // Create a custom block for this allocation
        Block* block = newBlock(size);
        return block->allocate(size, alignof(max_align_t));
    }
    
    // Try to allocate from the current block
    size_t alignment = alignof(max_align_t);
    if (!currentBlock_->canFit(size, alignment)) {
        // Create a new block
        currentBlock_ = newBlock(blockSize_);
    }
    
    return currentBlock_->allocate(size, alignment);
}

// Reset arena
void Arena::reset() {
    // Clear all blocks except the first one
    if (!blocks_.empty()) {
        auto firstBlock = std::move(blocks_[0]);
        blocks_.clear();
        blocks_.push_back(std::move(firstBlock));
        
        // Reset first block
        currentBlock_ = blocks_[0].get();
        currentBlock_->used = 0;
    }
    
    totalAllocated_ = 0;
}

// Create a new block
Arena::Block* Arena::newBlock(size_t minSize) {
    size_t blockSize = std::max(blockSize_, minSize);
    auto block = std::make_unique<Block>(blockSize);
    Block* blockPtr = block.get();
    
    blocks_.push_back(std::move(block));
    totalAllocated_ += blockSize;
    
    return blockPtr;
}

// Get default arena
Arena& Arena::defaultArena() {
    static Arena instance;
    return instance;
}

} // namespace common
} // namespace flux