#pragma once

#include "token.h"
#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include <vector>
#include <memory>

namespace flux {
namespace lexer {

// Tokenizer class for lexical analysis of Flux source code
class Tokenizer {
public:
    // Constructor
    Tokenizer(std::shared_ptr<common::Source> source, 
              common::Arena& arena = common::Arena::defaultArena());
    
    // Destructor
    ~Tokenizer();
    
    // Tokenize the entire source and return all tokens
    std::vector<Token> tokenizeAll();
    
    // Get the next token (streaming interface)
    Token nextToken();
    
    // Peek at the next token without consuming it
    Token peekToken();
    
    // Check if we've reached the end of the source
    bool isAtEnd() const;
    
    // Get current position in source
    common::SourcePosition getCurrentPosition() const;
    
    // Get the source
    std::shared_ptr<common::Source> getSource() const { return source_; }
    
    // Get error collector
    const common::ErrorCollector& getErrors() const { return errors_; }
    
    // Check if there were any tokenization errors
    bool hasErrors() const { return errors_.hasErrors(); }

private:
    std::shared_ptr<common::Source> source_;
    common::Arena& arena_;
    common::ErrorCollector errors_;
    
    // Current position in the source text
    size_t current_offset_;
    common::SourcePosition current_position_;
    
    // Character reading functions
    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    void advance();
    void advanceBy(size_t count);
    bool isAtEnd(size_t offset) const;
    
    // Position tracking
    void updatePosition(char c);
    common::SourcePosition getPositionAt(size_t offset) const;
    
    // Token creation helpers
    Token makeToken(TokenType type, size_t start_offset, size_t end_offset);
    Token makeToken(TokenType type, size_t start_offset, size_t end_offset, 
                   const std::string& processed_text);
    Token makeErrorToken(const std::string& message, size_t start_offset, size_t end_offset);
    
    // Main tokenization functions
    Token scanToken();
    
    // Specific token scanners
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanCharacter();
    Token scanDataLiteral();
    Token scanIString();
    Token scanLineComment();
    Token scanBlockComment();
    Token scanOperator();
    
    // Helper functions
    void skipWhitespace();
    bool match(char expected);
    bool matchSequence(const char* sequence);
    
    // Number parsing helpers
    Token scanIntegerLiteral(int base, size_t start_offset);
    Token scanFloatLiteral(size_t start_offset);
    bool isValidInBase(char c, int base);
    
    // String parsing helpers
    std::string parseStringContent();
    std::string parseCharacterContent();
    char parseEscapeSequence();
    
    // Data literal parsing
    std::string parseDataContent();
    
    // i-string parsing helpers
    Token parseIStringLiteral();
    
    // Error reporting
    void reportError(common::ErrorCode code, const std::string& message);
    void reportError(common::ErrorCode code, const std::string& message, 
                    const common::SourcePosition& position);
};

} // namespace lexer
} // namespace flux