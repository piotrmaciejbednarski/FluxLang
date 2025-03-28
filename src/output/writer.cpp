#include "../output/writer.h"
#include <iomanip>
#include <sstream>

namespace flux {
namespace output {

// Initialize the default writer
Writer defaultWriter;

Writer::Writer(std::ostream& output, bool useColors)
    : output_(&output), useColors_(useColors) {
}

Writer::~Writer() {
}

void Writer::setUseColors(bool useColors) {
    useColors_ = useColors;
}

void Writer::setStream(std::ostream& output) {
    output_ = &output;
}

void Writer::debug(std::string_view message) {
    write(Severity::DEBUG, message);
}

void Writer::info(std::string_view message) {
    write(Severity::INFO, message);
}

void Writer::warning(std::string_view message, const SourceLocation& location) {
    write(Severity::WARNING, message, location);
}

void Writer::error(std::string_view message, const SourceLocation& location) {
    write(Severity::ERROR, message, location);
}

void Writer::fatal(std::string_view message, const SourceLocation& location) {
    write(Severity::FATAL, message, location);
}

void Writer::write(Severity severity, std::string_view message, const SourceLocation& location) {
    // Get color for the severity level
    std::string_view color = severityToColor(severity);
    
    // Format and output the message header with severity and location
    std::string severityStr(severityToString(severity));
    
    // Start with color if enabled
    if (useColors_) {
        *output_ << color;
    }
    
    // Output severity
    *output_ << "[" << severityStr << "] ";
    
    // Reset color
    if (useColors_) {
        *output_ << Color::RESET;
    }
    
    // Output message
    *output_ << message;
    
    // Output location information if provided
    if (location.line > 0) {
        *output_ << "\n" << formatLocation(location);
        highlightSource(location);
    }
    
    // End with newline
    *output_ << std::endl;
}

void Writer::highlightSource(const SourceLocation& location) {
    // Skip if no line content is provided
    if (location.lineContent.empty()) {
        return;
    }
    
    // Output the line content
    *output_ << "  | " << location.lineContent << "\n";
    
    // Calculate and output the highlight markers
    std::string markers(location.lineContent.length(), ' ');
    
    // Fill highlight area with carets
    int highlight_end = location.highlight_start + location.highlight_length;
    for (int i = location.highlight_start; 
         i < highlight_end && i < static_cast<int>(markers.length()); 
         ++i) {
        markers[i] = '^';
    }
    
    // Output the markers with color if enabled
    if (useColors_) {
        *output_ << Color::RED;
    }
    
    *output_ << "  | " << markers;
    
    if (useColors_) {
        *output_ << Color::RESET;
    }
    
    *output_ << std::endl;
}

std::string Writer::formatLocation(const SourceLocation& location) const {
    std::stringstream ss;
    
    if (!location.filename.empty()) {
        ss << "  --> " << location.filename << ":" << location.line;
        if (location.column > 0) {
            ss << ":" << location.column;
        }
    } else {
        ss << "  at line " << location.line;
        if (location.column > 0) {
            ss << ", column " << location.column;
        }
    }
    
    return ss.str();
}

std::string_view Writer::severityToString(Severity severity) {
    switch (severity) {
        case Severity::DEBUG:   return "DEBUG";
        case Severity::INFO:    return "INFO";
        case Severity::WARNING: return "WARNING";
        case Severity::ERROR:   return "ERROR";
        case Severity::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string_view Writer::severityToColor(Severity severity) const {
    if (!useColors_) {
        return Color::RESET;
    }
    
    switch (severity) {
        case Severity::DEBUG:   return Color::CYAN;
        case Severity::INFO:    return Color::GREEN;
        case Severity::WARNING: return Color::YELLOW;
        case Severity::ERROR:   return Color::RED;
        case Severity::FATAL:   return Color::BOLD;  // Just using BOLD for FATAL to avoid string_view concatenation issues
        default:                return Color::RESET;
    }
}

} // namespace output
} // namespace flux