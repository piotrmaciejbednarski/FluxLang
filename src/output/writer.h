#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>

namespace flux {
namespace output {

// Define color constants for terminal output
namespace Color {
    static const std::string_view RESET   = "\033[0m";
    static const std::string_view RED     = "\033[31m";
    static const std::string_view GREEN   = "\033[32m";
    static const std::string_view YELLOW  = "\033[33m";
    static const std::string_view BLUE    = "\033[34m";
    static const std::string_view MAGENTA = "\033[35m";
    static const std::string_view CYAN    = "\033[36m";
    static const std::string_view BOLD    = "\033[1m";
    static const std::string_view ITALIC  = "\033[3m";
}

// Output severity levels
enum class Severity {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Source location information
struct SourceLocation {
    std::string_view filename;
    int line;
    int column;
    std::string_view lineContent;  // The content of the line where the issue occurred
    int highlight_start;           // Starting position of the highlight 
    int highlight_length;          // Length of the highlight
    
    SourceLocation() : line(0), column(0), highlight_start(0), highlight_length(0) {}
    
    SourceLocation(
        std::string_view filename, 
        int line, 
        int column, 
        std::string_view lineContent = "",
        int highlight_start = 0,
        int highlight_length = 0
    ) : filename(filename), line(line), column(column), 
        lineContent(lineContent), highlight_start(highlight_start), 
        highlight_length(highlight_length) {}
};

// Class to handle formatted output for the Flux interpreter
class Writer {
public:
    // Constructor
    Writer(std::ostream& output = std::cout, bool useColors = true);
    
    // Destructor
    ~Writer();

    // Set whether to use colors
    void setUseColors(bool useColors);
    
    // Set output stream
    void setStream(std::ostream& output);
    
    // Output methods for different severity levels
    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message, const SourceLocation& location = SourceLocation());
    void error(std::string_view message, const SourceLocation& location = SourceLocation());
    void fatal(std::string_view message, const SourceLocation& location = SourceLocation());
    
    // Generic output method with severity level
    void write(Severity severity, std::string_view message, 
               const SourceLocation& location = SourceLocation());
    
    // Output source code with highlighting
    void highlightSource(const SourceLocation& location);
    
    // Get a string representation of a severity level
    static std::string_view severityToString(Severity severity);
    
    // Get a color for a severity level
    std::string_view severityToColor(Severity severity) const;

private:
    std::ostream* output_;
    bool useColors_;
    
    // Format the source location for output
    std::string formatLocation(const SourceLocation& location) const;
};

// Global writer instance
extern Writer defaultWriter;

} // namespace output
} // namespace flux