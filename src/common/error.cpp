#include "error.h"
#include <sstream>

namespace flux {
namespace common {

std::string_view errorCodeToString(ErrorCode code) {
    switch (code) {
        // General errors
        case ErrorCode::NONE:                  return "No error";
        case ErrorCode::INTERNAL_ERROR:        return "Internal error";
        case ErrorCode::FILE_NOT_FOUND:        return "File not found";
        case ErrorCode::IO_ERROR:              return "I/O error";
        
        // Lexer errors
        case ErrorCode::INVALID_CHARACTER:     return "Invalid character";
        case ErrorCode::UNTERMINATED_STRING:   return "Unterminated string";
        case ErrorCode::INVALID_ESCAPE_SEQUENCE: return "Invalid escape sequence";
        case ErrorCode::INVALID_NUMBER_FORMAT: return "Invalid number format";
        
        // Parser errors
        case ErrorCode::UNEXPECTED_TOKEN:      return "Unexpected token";
        case ErrorCode::EXPECTED_IDENTIFIER:   return "Expected identifier";
        case ErrorCode::EXPECTED_EXPRESSION:   return "Expected expression";
        case ErrorCode::EXPECTED_STATEMENT:    return "Expected statement";
        case ErrorCode::EXPECTED_TYPE:         return "Expected type";
        case ErrorCode::EXPECTED_DECLARATION:  return "Expected declaration";
        case ErrorCode::UNMATCHED_PARENTHESIS: return "Unmatched parenthesis";
        case ErrorCode::UNMATCHED_BRACE:       return "Unmatched brace";
        case ErrorCode::UNMATCHED_BRACKET:     return "Unmatched bracket";
        
        // Type checker errors
        case ErrorCode::TYPE_MISMATCH:         return "Type mismatch";
        case ErrorCode::UNDEFINED_IDENTIFIER:  return "Undefined identifier";
        case ErrorCode::UNDEFINED_TYPE:        return "Undefined type";
        case ErrorCode::INCOMPATIBLE_TYPES:    return "Incompatible types";
        case ErrorCode::INVALID_OPERATION:     return "Invalid operation";
        case ErrorCode::INVALID_FUNCTION_CALL: return "Invalid function call";
        
        // Runtime errors
        case ErrorCode::DIVISION_BY_ZERO:      return "Division by zero";
        case ErrorCode::INDEX_OUT_OF_BOUNDS:   return "Index out of bounds";
        case ErrorCode::NULL_REFERENCE:        return "Null reference";
        case ErrorCode::STACK_OVERFLOW:        return "Stack overflow";
        case ErrorCode::UNIMPLEMENTED_FEATURE: return "Unimplemented feature";
        
        default:                               return "Unknown error";
    }
}

Error::Error(ErrorCode code, std::string_view message, const output::SourceLocation& location)
    : code_(code), message_(message), location_(location) {
}

std::string Error::formatted() const {
    std::stringstream ss;
    
    // Format the error with code and message
    ss << errorCodeToString(code_) << ": " << message_;
    
    // Add location information if available
    if (location_.line > 0) {
        if (!location_.filename.empty()) {
            ss << " at " << location_.filename << ":" << location_.line;
            
            if (location_.column > 0) {
                ss << ":" << location_.column;
            }
        } else {
            ss << " at line " << location_.line;
            
            if (location_.column > 0) {
                ss << ", column " << location_.column;
            }
        }
    }
    
    return ss.str();
}

void Error::report(output::Writer& writer) const {
    // Use the appropriate severity based on error code
    output::Severity severity;
    
    if (code_ == ErrorCode::INTERNAL_ERROR || 
        code_ == ErrorCode::STACK_OVERFLOW || 
        code_ == ErrorCode::NULL_REFERENCE) {
        severity = output::Severity::FATAL;
    } else {
        severity = output::Severity::ERROR;
    }
    
    // Report the error through the writer
    writer.write(severity, formatted(), location_);
}

void ErrorCollector::addError(const Error& error) {
    errors_.push_back(error);
}

void ErrorCollector::addError(ErrorCode code, std::string_view message, 
                              const output::SourceLocation& location) {
    errors_.emplace_back(code, message, location);
}

void ErrorCollector::reportErrors(output::Writer& writer) const {
    for (const auto& error : errors_) {
        error.report(writer);
    }
}

[[noreturn]] void throwError(ErrorCode code, std::string_view message, 
                            const output::SourceLocation& location) {
    throw Error(code, message, location);
}

} // namespace common
} // namespace flux