#pragma once

#include "token.h"
#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include <vector>
#include <memory>

namespace flux {
namespace lexer {

// Tokenizer class for the Flux language
class Tokenizer {
public:
    // Constructor
    Tokenizer(std::shared_ptr<common::Source> source, common::Arena& arena);
    
    // Destructor
    ~Tokenizer() = default;
    
    // Disable copy and move
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;
    
    // Tokenize the entire source
    std::vector<Token> tokenize();
    
    // Get the next token
    Token nextToken();
    
    // Peek at the next token without consuming it
    Token peekToken();
    
    // Check if we're at the end of the source
    bool isAtEnd() const;
    
    // Get current position
    common::SourcePosition currentPosition() const;
    
    // Get error collector for collecting lexer errors
    common::ErrorCollector& errorCollector() { return errorCollector_; }

private:
    std::shared_ptr<common::Source> source_;
    common::Arena& arena_;
    common::ErrorCollector errorCollector_;
    
    // Current position in source
    size_t current_;
    common::SourcePosition position_;
    
    // State for i-string parsing
    enum class IStringState {
        NONE,
        IN_TEXT,
        IN_EXPRESSION,
        WAITING_FOR_COLON
    };
    IStringState iStringState_;
    int iStringBraceDepth_;
    
    // Helper methods
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool match(const char* str);
    void skipWhitespace();
    void skipComment();
    
    // Position tracking
    void updatePosition(char c);
    common::SourceRange makeRange(const common::SourcePosition& start) const;
    
    // Token creation methods
    Token makeToken(TokenType type, const common::SourcePosition& start);
    Token makeToken(TokenType type, const common::SourcePosition& start, int64_t value);
    Token makeToken(TokenType type, const common::SourcePosition& start, double value);
    Token makeErrorToken(const char* message, const common::SourcePosition& start);
    
    // Lexing methods for different token types
    Token lexNumber(const common::SourcePosition& start);
    Token lexString(const common::SourcePosition& start);
    Token lexIString(const common::SourcePosition& start);
    Token lexIdentifier(const common::SourcePosition& start);
    Token lexBinaryLiteral(const common::SourcePosition& start);
    Token lexOperator(const common::SourcePosition& start);
    
    // I-string specific methods
    Token lexIStringText(const common::SourcePosition& start);
    Token lexIStringExpression(const common::SourcePosition& start);
    
    // Character classification
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
    bool isBinaryDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isWhitespace(char c) const;
    
    // Number parsing helpers
    bool parseInteger(std::string_view text, int64_t& result, int base = 10) const;
    bool parseFloat(std::string_view text, double& result) const;
    
    // String parsing helpers
    std::string parseStringLiteral(std::string_view text) const;
    char parseEscapeSequence(const char*& ptr) const;
    
    // Error reporting
    void reportError(common::ErrorCode code, const char* message, const common::SourcePosition& position);
};

} // namespace lexer
} // namespace flux