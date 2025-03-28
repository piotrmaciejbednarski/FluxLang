#pragma once

#include "token.h"
#include "../common/source.h"
#include "../common/error.h"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <utility>

namespace flux {
namespace lexer {

// Class to tokenize Flux source code
class Tokenizer {
public:
    // Constructor with source code
    Tokenizer(std::shared_ptr<common::Source> source);
    
    // Tokenize the entire source and return all tokens
    std::vector<Token> tokenizeAll();
    
    // Get the next token
    Token nextToken();
    
    // Peek at the next token without consuming it
    Token peekToken();
    
    // Get the current position in the source
    common::SourcePosition currentPosition() const;
    
    // Get any errors that occurred during tokenization
    const common::ErrorCollector& errors() const { return errors_; }
    
    // Check if there were errors during tokenization
    bool hasErrors() const { return errors_.hasErrors(); }

private:
    std::shared_ptr<common::Source> source_;  // Source code
    std::string_view text_;                   // Source text view
    size_t position_;                         // Current position in source
    size_t line_;                             // Current line
    size_t column_;                           // Current column
    common::ErrorCollector errors_;           // Error collector
    
    // Current character
    char current() const;
    
    // Next character (lookahead)
    char peek() const;
    
    // Next next character (2-character lookahead)
    char peekNext() const;
    
    // Advance to the next character
    char advance();
    
    // Check if current character matches expected and advance if it does
    bool match(char expected);
    
    // Check if we've reached the end of the file
    bool isAtEnd() const;
    
    // Skip whitespace and comments
    void skipWhitespaceAndComments();
    
    // Create a token
    Token makeToken(TokenType type) const;
    
    // Create an error token
    Token errorToken(std::string_view message) const;
    
    // Report an error
    void error(common::ErrorCode code, std::string_view message);
    
    // Process tokens
    Token scanToken();
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
    Token scanIString(); // Injectable strings
    Token scanOperator();
    
    // Helper methods for scanning
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Get current source position
    common::SourcePosition getPosition() const;
    
    // Helper for building source location
    output::SourceLocation makeSourceLocation(
        const common::SourcePosition& start, 
        const common::SourcePosition& end) const;
};

} // namespace lexer
} // namespace flux