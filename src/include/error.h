#ifndef FLUX_ERROR_H
#define FLUX_ERROR_H

#include <string>
#include <stdexcept>
#include <string_view>

namespace flux {

// Base class for Flux-specific errors
class FluxError : public std::runtime_error {
public:
    FluxError(const std::string& message);
};

// Lexer-specific error
class LexerError : public FluxError {
public:
    LexerError(const std::string& message, 
               std::string_view filename, 
               size_t line, 
               size_t column);

    std::string_view getFilename() const { return filename; }
    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

private:
    std::string_view filename;
    size_t line;
    size_t column;
};

// Parser-specific error
class ParserError : public FluxError {
public:
    ParserError(const std::string& message, 
                std::string_view filename, 
                size_t line, 
                size_t column);

    std::string_view getFilename() const { return filename; }
    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

private:
    std::string_view filename;
    size_t line;
    size_t column;
};

// Type checking error
class TypeCheckError : public FluxError {
public:
    TypeCheckError(const std::string& message);
};

// Runtime error
class RuntimeError : public FluxError {
public:
    RuntimeError(const std::string& message);
};

// Utility error reporting functions
class ErrorReporter {
public:
    // Set the current source file context
    static void setSourceContext(std::string_view filename);

    // Report a lexer error
    static void reportLexerError(const std::string& message, 
                                 size_t line, 
                                 size_t column);

    // Report a parser error
    static void reportParserError(const std::string& message, 
                                  size_t line, 
                                  size_t column);

    // Report a type checking error
    static void reportTypeCheckError(const std::string& message);

    // Report a runtime error
    static void reportRuntimeError(const std::string& message);

    // Check if any errors have been reported
    static bool hasErrors();

    // Clear error state
    static void reset();

private:
    // Prevent instantiation
    ErrorReporter() = delete;
};

} // namespace flux

#endif // FLUX_ERROR_H
