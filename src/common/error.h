#pragma once

#include "source.h"
#include "../output/writer.h"
#include <string>
#include <string_view>
#include <vector>
#include <exception>
#include <memory>

namespace flux {
namespace common {

// Error codes for the Flux interpreter
enum class ErrorCode {
    // General errors
    NONE,
    INTERNAL_ERROR,
    FILE_NOT_FOUND,
    IO_ERROR,
    
    // Lexer errors
    INVALID_CHARACTER,
    UNTERMINATED_STRING,
    INVALID_ESCAPE_SEQUENCE,
    INVALID_NUMBER_FORMAT,
    INVALID_SECTION_ATTRIBUTE,
    INVALID_ADDRESS_SPECIFIER,
    
    // Parser errors
    PARSER_ERROR,         // General parser error
    UNEXPECTED_TOKEN,
    EXPECTED_IDENTIFIER,
    EXPECTED_EXPRESSION,
    EXPECTED_STATEMENT,
    EXPECTED_TYPE,
    EXPECTED_DECLARATION,
    UNMATCHED_PARENTHESIS,
    UNMATCHED_BRACE,
    UNMATCHED_BRACKET,
    
    // Type checker errors
    TYPE_MISMATCH,
    UNDEFINED_IDENTIFIER,
    UNDEFINED_TYPE,
    INCOMPATIBLE_TYPES,
    INVALID_OPERATION,
    INVALID_FUNCTION_CALL,
    
    // Runtime errors
    DIVISION_BY_ZERO,
    INDEX_OUT_OF_BOUNDS,
    NULL_REFERENCE,
    STACK_OVERFLOW,
    UNIMPLEMENTED_FEATURE
};

// Convert an error code to a string
std::string_view errorCodeToString(ErrorCode code);

// Base class for all Flux errors
class Error : public std::exception {
public:
    Error(ErrorCode code, std::string_view message, const output::SourceLocation& location = output::SourceLocation());
    
    // Get the error code
    ErrorCode code() const { return code_; }
    
    // Get the error message
    const std::string& message() const { return message_; }
    
    // Get the source location
    const output::SourceLocation& location() const { return location_; }
    
    // Get a formatted error message
    std::string formatted() const;
    
    // std::exception interface
    const char* what() const noexcept override { return message_.c_str(); }
    
    // Report the error to the output
    void report(output::Writer& writer = output::defaultWriter) const;

private:
    ErrorCode code_;
    std::string message_;
    output::SourceLocation location_;
};

// Class for collecting multiple errors
class ErrorCollector {
public:
    ErrorCollector() = default;
    
    // Add an error to the collection
    void addError(const Error& error);
    void addError(ErrorCode code, std::string_view message, const output::SourceLocation& location = output::SourceLocation());
    
    // Create and add an error to the collection
    template<typename... Args>
    void error(ErrorCode code, std::string_view message, Args&&... args) {
        addError(Error(code, message, std::forward<Args>(args)...));
    }
    
    // Check if there are any errors
    bool hasErrors() const { return !errors_.empty(); }
    
    // Get the number of errors
    size_t errorCount() const { return errors_.size(); }
    
    // Get the collected errors
    const std::vector<Error>& errors() const { return errors_; }
    
    // Report all errors to the output
    void reportErrors(output::Writer& writer = output::defaultWriter) const;
    
    // Clear all errors
    void clear() { errors_.clear(); }

private:
    std::vector<Error> errors_;
};

// Function to create and throw an error
[[noreturn]] void throwError(ErrorCode code, std::string_view message, const output::SourceLocation& location = output::SourceLocation());

// Error handling macros
#define FLUX_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            flux::common::throwError(flux::common::ErrorCode::INTERNAL_ERROR, message); \
        } \
    } while (0)

#define FLUX_UNREACHABLE(message) \
    flux::common::throwError(flux::common::ErrorCode::INTERNAL_ERROR, "Unreachable code reached: " message)

} // namespace common
} // namespace flux