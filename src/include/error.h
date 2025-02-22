#ifndef FLUX_ERROR_H
#define FLUX_ERROR_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <iostream>

namespace flux {

enum class ErrorType {
    LEXICAL_ERROR,
    SYNTAX_ERROR,
    TYPE_ERROR,
    RUNTIME_ERROR,
    INTERNAL_ERROR
};

struct SourceLocation {
    std::string_view filename;
    int line;
    int column;
    
    SourceLocation() : filename(""), line(0), column(0) {}
    SourceLocation(std::string_view file, int l, int c) : filename(file), line(l), column(c) {}
};

class Error {
private:
    ErrorType type;
    std::string message;
    SourceLocation location;

public:
    Error(ErrorType type, std::string message, SourceLocation location)
        : type(type), message(std::move(message)), location(location) {}
    
    const std::string& getMessage() const { return message; }
    ErrorType getType() const { return type; }
    const SourceLocation& getLocation() const { return location; }
    
    void report(std::ostream& out = std::cerr) const {
        out << location.filename << ":" << location.line << ":" << location.column << ": ";
        
        switch (type) {
            case ErrorType::LEXICAL_ERROR:  out << "lexical error: "; break;
            case ErrorType::SYNTAX_ERROR:   out << "syntax error: "; break;
            case ErrorType::TYPE_ERROR:     out << "type error: "; break;
            case ErrorType::RUNTIME_ERROR:  out << "runtime error: "; break;
            case ErrorType::INTERNAL_ERROR: out << "internal error: "; break;
        }
        
        out << message << std::endl;
    }
};

class ErrorReporter {
private:
    std::vector<Error> errors;
    
public:
    void reportError(ErrorType type, const std::string& message, const SourceLocation& location) {
        errors.emplace_back(type, message, location);
        errors.back().report();
    }
    
    bool hasErrors() const {
        return !errors.empty();
    }
    
    const std::vector<Error>& getErrors() const {
        return errors;
    }
    
    void clear() {
        errors.clear();
    }
};

// Global error reporter
extern ErrorReporter errorReporter;

} // namespace flux

#endif // FLUX_ERROR_H
