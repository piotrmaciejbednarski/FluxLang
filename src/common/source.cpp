#include "source.h"
#include "error.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstring>

namespace flux {
namespace common {

// Create source from file
std::shared_ptr<Source> Source::fromFile(const std::string& filename, Arena& arena) {
    // Open the file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throwError(ErrorCode::FILE_NOT_FOUND, "Could not open file: " + filename);
    }
    
    // Get file size
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file content into a temporary buffer
    std::string temp_buffer;
    temp_buffer.resize(size);
    if (!file.read(temp_buffer.data(), size)) {
        throwError(ErrorCode::IO_ERROR, "Could not read file: " + filename);
    }
    
    // Copy content to arena
    char* buffer = arena.allocArray<char>(size + 1);
    std::memcpy(buffer, temp_buffer.data(), size);
    buffer[size] = '\0';  // Null-terminate
    
    // Create source
    return std::make_shared<Source>(std::string_view(buffer, size), filename, arena);
}

// Create source from string
std::shared_ptr<Source> Source::fromString(std::string_view text, 
                                           const std::string& name, 
                                           Arena& arena) {
    // Copy text to arena
    size_t size = text.size();
    char* buffer = arena.allocArray<char>(size + 1);
    std::memcpy(buffer, text.data(), size);
    buffer[size] = '\0';  // Null-terminate
    
    // Create source
    return std::make_shared<Source>(std::string_view(buffer, size), name, arena);
}

// Constructor
Source::Source(std::string_view text, const std::string& filename, Arena& arena)
    : text_(text), filename_(filename), arena_(arena) {
    initLineOffsets();
}

// Initialize line offsets
void Source::initLineOffsets() {
    line_offsets_.clear();
    line_offsets_.push_back(0);  // First line starts at offset 0
    
    for (size_t i = 0; i < text_.size(); ++i) {
        if (text_[i] == '\n') {
            line_offsets_.push_back(i + 1);
        }
    }
}

// Get a specific line of text
bool Source::getLine(size_t line_number, std::string_view& line) const {
    if (line_number < 1 || line_number > line_offsets_.size()) {
        return false;
    }
    
    size_t start = line_offsets_[line_number - 1];
    size_t end;
    
    if (line_number < line_offsets_.size()) {
        end = line_offsets_[line_number];
    } else {
        end = text_.size();
    }
    
    // Remove trailing newline if present
    if (end > start && text_[end - 1] == '\n') {
        end--;
    }
    
    // Remove trailing carriage return if present
    if (end > start && text_[end - 1] == '\r') {
        end--;
    }
    
    line = text_.substr(start, end - start);
    return true;
}

// Get a range of text
std::string_view Source::getRange(const SourceRange& range) const {
    size_t start_offset = positionToOffset(range.start);
    size_t end_offset = positionToOffset(range.end);
    
    if (end_offset <= start_offset) {
        return "";
    }
    
    return text_.substr(start_offset, end_offset - start_offset);
}

// Get text for a specific line
std::string_view Source::getLineText(size_t line_number) const {
    std::string_view line;
    if (getLine(line_number, line)) {
        return line;
    }
    return "";
}

// Convert position to offset
size_t Source::positionToOffset(const SourcePosition& position) const {
    if (position.line < 1 || position.line > line_offsets_.size()) {
        return 0;
    }
    
    size_t line_offset = line_offsets_[position.line - 1];
    size_t column = position.column;
    
    // Get the line
    std::string_view line;
    getLine(position.line, line);
    
    // Adjust column if needed
    if (column > line.size()) {
        column = line.size() + 1;
    }
    
    return line_offset + column - 1;
}

// Convert offset to position
SourcePosition Source::offsetToPosition(size_t offset) const {
    if (offset >= text_.size()) {
        offset = text_.size() > 0 ? text_.size() - 1 : 0;
    }
    
    // Find the line containing the offset
    size_t line = findLine(offset);
    size_t line_start = line_offsets_[line - 1];
    
    // Calculate column
    size_t column = offset - line_start + 1;
    
    return {line, column};
}

// Find line number for an offset
size_t Source::findLine(size_t offset) const {
    // Binary search to find the line containing the offset
    auto it = std::upper_bound(line_offsets_.begin(), line_offsets_.end(), offset);
    size_t index = it - line_offsets_.begin();
    
    // Adjust for 1-based line numbers
    return index;
}

} // namespace common
} // namespace flux