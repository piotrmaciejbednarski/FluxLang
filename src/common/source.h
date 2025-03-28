#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>
#include "arena.h"

namespace flux {
namespace common {

// Position in source code (line and column)
struct SourcePosition {
    size_t line;
    size_t column;
    
    SourcePosition() : line(1), column(1) {}
    SourcePosition(size_t l, size_t c) : line(l), column(c) {}
    
    bool operator==(const SourcePosition& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator!=(const SourcePosition& other) const {
        return !(*this == other);
    }
    
    bool operator<(const SourcePosition& other) const {
        return line < other.line || (line == other.line && column < other.column);
    }
    
    bool operator<=(const SourcePosition& other) const {
        return *this < other || *this == other;
    }
    
    bool operator>(const SourcePosition& other) const {
        return !(*this <= other);
    }
    
    bool operator>=(const SourcePosition& other) const {
        return !(*this < other);
    }
};

// Range in source code (start and end positions)
struct SourceRange {
    SourcePosition start;
    SourcePosition end;
    
    SourceRange() = default;
    SourceRange(const SourcePosition& s, const SourcePosition& e) : start(s), end(e) {}
    
    bool contains(const SourcePosition& pos) const {
        return start <= pos && pos <= end;
    }
    
    bool overlaps(const SourceRange& other) const {
        return contains(other.start) || contains(other.end) ||
               other.contains(start) || other.contains(end);
    }
};

// Class to represent source code
class Source {
public:
    // Create source from file
    static std::shared_ptr<Source> fromFile(const std::string& filename, Arena& arena);
    
    // Create source from string
    static std::shared_ptr<Source> fromString(std::string_view text, 
                                              const std::string& name = "<string>", 
                                              Arena& arena = Arena::defaultArena());
    
    // Constructor
    Source(std::string_view text, const std::string& filename, Arena& arena);
    
    // Get the complete text
    std::string_view text() const { return text_; }
    
    // Get the filename
    std::string_view filename() const { return filename_; }
    
    // Get a specific line of text
    bool getLine(size_t line_number, std::string_view& line) const;
    
    // Get a range of text
    std::string_view getRange(const SourceRange& range) const;
    
    // Get text for a specific line
    std::string_view getLineText(size_t line_number) const;
    
    // Convert position to offset
    size_t positionToOffset(const SourcePosition& position) const;
    
    // Convert offset to position
    SourcePosition offsetToPosition(size_t offset) const;
    
    // Get the number of lines
    size_t lineCount() const { return line_offsets_.size(); }

private:
    std::string_view text_;           // The complete source text
    std::string filename_;            // The filename or source name
    std::vector<size_t> line_offsets_; // Offsets of each line in the text
    Arena& arena_;                    // Arena for memory management

    // Initialize line offsets
    void initLineOffsets();
    
    // Find line number for an offset
    size_t findLine(size_t offset) const;
};

} // namespace common
} // namespace flux