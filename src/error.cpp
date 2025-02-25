#include <iostream>
#include <sstream>
#include "include/error.h"

namespace flux {

// FluxError implementation
FluxError::FluxError(const std::string& message)
    : std::runtime_error(message) {}

// LexerError implementation
LexerError::LexerError(const std::string& message, 
                       std::string_view filename, 
                       size_t line, 
                       size_t column)
    : FluxError(message), filename(filename), line(line), column(column) {}

// ParserError implementation
ParserError::ParserError(const std::string& message, 
                         std::string_view filename, 
                         size_t line, 
                         size_t column)
    : FluxError(message), filename(filename), line(line), column(column) {}

// TypeCheckError implementation
TypeCheckError::TypeCheckError(const std::string& message)
    : FluxError(message) {}

// RuntimeError implementation
RuntimeError::RuntimeError(const std::string& message)
    : FluxError(message) {}

// ErrorReporter implementation
class ErrorReporterImpl {
public:
    static std::string currentFilename;
    static bool hasReportedErrors;
    static int lexerErrorCount;
    static int parserErrorCount;
    static int typeCheckErrorCount;
    static int runtimeErrorCount;
};

// Static member definitions
std::string ErrorReporterImpl::currentFilename;
bool ErrorReporterImpl::hasReportedErrors = false;
int ErrorReporterImpl::lexerErrorCount = 0;
int ErrorReporterImpl::parserErrorCount = 0;
int ErrorReporterImpl::typeCheckErrorCount = 0;
int ErrorReporterImpl::runtimeErrorCount = 0;

void ErrorReporter::setSourceContext(std::string_view filename) {
    ErrorReporterImpl::currentFilename = filename;
    ErrorReporterImpl::hasReportedErrors = false;
    ErrorReporterImpl::lexerErrorCount = 0;
    ErrorReporterImpl::parserErrorCount = 0;
    ErrorReporterImpl::typeCheckErrorCount = 0;
    ErrorReporterImpl::runtimeErrorCount = 0;
}

void ErrorReporter::reportLexerError(const std::string& message, 
                                     size_t line, 
                                     size_t column) {
    ErrorReporterImpl::hasReportedErrors = true;
    ErrorReporterImpl::lexerErrorCount++;
    
    std::cerr << "Lexer Error in " << ErrorReporterImpl::currentFilename 
              << " at line " << line << ", column " << column << ": "
              << message << std::endl;
}

void ErrorReporter::reportParserError(const std::string& message, 
                                      size_t line, 
                                      size_t column) {
    ErrorReporterImpl::hasReportedErrors = true;
    ErrorReporterImpl::parserErrorCount++;
    
    std::cerr << "Parser Error in " << ErrorReporterImpl::currentFilename 
              << " at line " << line << ", column " << column << ": "
              << message << std::endl;
}

void ErrorReporter::reportTypeCheckError(const std::string& message) {
    ErrorReporterImpl::hasReportedErrors = true;
    ErrorReporterImpl::typeCheckErrorCount++;
    
    std::cerr << "Type Check Error in " << ErrorReporterImpl::currentFilename 
              << ": " << message << std::endl;
}

void ErrorReporter::reportRuntimeError(const std::string& message) {
    ErrorReporterImpl::hasReportedErrors = true;
    ErrorReporterImpl::runtimeErrorCount++;
    
    std::cerr << "Runtime Error in " << ErrorReporterImpl::currentFilename 
              << ": " << message << std::endl;
}

bool ErrorReporter::hasErrors() {
    return ErrorReporterImpl::hasReportedErrors;
}

void ErrorReporter::reset() {
    ErrorReporterImpl::currentFilename.clear();
    ErrorReporterImpl::hasReportedErrors = false;
    ErrorReporterImpl::lexerErrorCount = 0;
    ErrorReporterImpl::parserErrorCount = 0;
    ErrorReporterImpl::typeCheckErrorCount = 0;
    ErrorReporterImpl::runtimeErrorCount = 0;
}

} // namespace flux
