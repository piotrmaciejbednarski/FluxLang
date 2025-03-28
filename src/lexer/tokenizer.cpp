#include "tokenizer.h"
#include <cctype>
#include <unordered_map>

namespace flux {
namespace lexer {

// Constructor with source code
Tokenizer::Tokenizer(std::shared_ptr<common::Source> source)
    : source_(std::move(source)),
      text_(source_->text()),
      position_(0),
      line_(1),
      column_(1) {
}

// Tokenize the entire source and return all tokens
std::vector<Token> Tokenizer::tokenizeAll() {
    std::vector<Token> tokens;
    
    // Reset state
    position_ = 0;
    line_ = 1;
    column_ = 1;
    
    // Tokenize until end of file
    while (true) {
        Token token = nextToken();
        tokens.push_back(token);
        
        if (token.type() == TokenType::END_OF_FILE || token.type() == TokenType::ERROR) {
            break;
        }
    }
    
    return tokens;
}

// Get the next token
Token Tokenizer::nextToken() {
    // Skip whitespace and comments
    skipWhitespaceAndComments();
    
    // Check for end of file
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE);
    }
    
    // Determine token type and scan it
    char c = current();
    
    if (isAlpha(c) || c == '_') {
        return scanIdentifier();
    }
    
    if (isDigit(c)) {
        return scanNumber();
    }
    
    if (c == '"') {
        // Check for i-string
        if (position_ > 0 && text_[position_ - 1] == 'i') {
            // Backtrack to include the 'i'
            position_--;
            column_--;
            return scanIString();
        }
        return scanString();
    }
    
    return scanOperator();
}

// Peek at the next token without consuming it
Token Tokenizer::peekToken() {
    // Save current state
    size_t saved_position = position_;
    size_t saved_line = line_;
    size_t saved_column = column_;
    
    // Get the next token
    Token token = nextToken();
    
    // Restore state
    position_ = saved_position;
    line_ = saved_line;
    column_ = saved_column;
    
    return token;
}

// Get the current position in the source
common::SourcePosition Tokenizer::currentPosition() const {
    return {line_, column_};
}

// Current character
char Tokenizer::current() const {
    if (isAtEnd()) {
        return '\0';
    }
    return text_[position_];
}

// Next character (lookahead)
char Tokenizer::peek() const {
    if (position_ + 1 >= text_.size()) {
        return '\0';
    }
    return text_[position_ + 1];
}

// Next next character (2-character lookahead)
char Tokenizer::peekNext() const {
    if (position_ + 2 >= text_.size()) {
        return '\0';
    }
    return text_[position_ + 2];
}

// Advance to the next character
char Tokenizer::advance() {
    char c = current();
    position_++;
    
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    
    return c;
}

// Check if current character matches expected and advance if it does
bool Tokenizer::match(char expected) {
    if (isAtEnd() || current() != expected) {
        return false;
    }
    
    advance();
    return true;
}

// Check if we've reached the end of the file
bool Tokenizer::isAtEnd() const {
    return position_ >= text_.size();
}

// Skip whitespace and comments
void Tokenizer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = current();
        
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                advance();
                break;
                
            case '/':
                if (peek() == '/') {
                    // Line comment
                    while (!isAtEnd() && current() != '\n') {
                        advance();
                    }
                } else if (peek() == '*') {
                    // Block comment
                    advance(); // Skip '/'
                    advance(); // Skip '*'
                    
                    while (!isAtEnd() && !(current() == '*' && peek() == '/')) {
                        advance();
                    }
                    
                    if (!isAtEnd()) {
                        advance(); // Skip '*'
                        advance(); // Skip '/'
                    }
                } else {
                    return; // Division operator, not a comment
                }
                break;
                
            default:
                return;
        }
    }
}

// Create a token
Token Tokenizer::makeToken(TokenType type) const {
    size_t start_pos = position_;
    size_t length = 0;
    
    // For operators and delimiters, use the appropriate length
    switch (type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::ASTERISK:
        case TokenType::SLASH:
        case TokenType::PERCENT:
        case TokenType::AMPERSAND:
        case TokenType::PIPE:
        case TokenType::CARET:
        case TokenType::TILDE:
        case TokenType::EXCLAMATION:
        case TokenType::EQUAL:
        case TokenType::LESS:
        case TokenType::GREATER:
        case TokenType::DOT:
        case TokenType::QUESTION:
        case TokenType::COLON:
        case TokenType::SEMICOLON:
        case TokenType::COMMA:
        case TokenType::ASTERISK_PTR:
        case TokenType::AMPERSAND_REF:
        case TokenType::LEFT_PAREN:
        case TokenType::RIGHT_PAREN:
        case TokenType::LEFT_BRACE:
        case TokenType::RIGHT_BRACE:
        case TokenType::LEFT_BRACKET:
        case TokenType::RIGHT_BRACKET:
            length = 1;
            break;
            
        case TokenType::PLUS_PLUS:
        case TokenType::MINUS_MINUS:
        case TokenType::EQUAL_EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_EQUAL:
        case TokenType::AMPERSAND_AMPERSAND:
        case TokenType::PIPE_PIPE:
        case TokenType::LESS_LESS:
        case TokenType::GREATER_GREATER:
        case TokenType::PLUS_EQUAL:
        case TokenType::MINUS_EQUAL:
        case TokenType::ASTERISK_EQUAL:
        case TokenType::SLASH_EQUAL:
        case TokenType::PERCENT_EQUAL:
        case TokenType::AMPERSAND_EQUAL:
        case TokenType::PIPE_EQUAL:
        case TokenType::CARET_EQUAL:
        case TokenType::DOUBLE_COLON:
            length = 2;
            break;
            
        case TokenType::DOUBLE_ASTERISK:
        case TokenType::LESS_LESS_EQUAL:
        case TokenType::GREATER_GREATER_EQUAL:
            length = 2;
            break;
            
        case TokenType::DOUBLE_ASTERISK_EQUAL:
            length = 3;
            break;
            
        default:
            // For other token types, calculate length from position
            length = position_ - start_pos;
            break;
    }
    
    // Create token
    common::SourcePosition start = {line_, column_ - length};
    common::SourcePosition end = {line_, column_};
    std::string_view lexeme = text_.substr(start_pos - length, length);
    
    return Token(type, lexeme, start, end);
}

// Create an error token
Token Tokenizer::errorToken(std::string_view message) const {
    common::SourcePosition pos = {line_, column_};
    return Token(TokenType::ERROR, message, pos, pos);
}

// Report an error
void Tokenizer::error(common::ErrorCode code, std::string_view message) {
    common::SourcePosition start = {line_, column_};
    common::SourcePosition end = {line_, column_ + 1};
    auto location = makeSourceLocation(start, end);
    
    errors_.addError(code, message, location);
}

// Process identifier tokens (keywords and identifiers)
Token Tokenizer::scanIdentifier() {
    common::SourcePosition start = getPosition();
    size_t start_pos = position_;
    
    // Consume identifier characters
    while (!isAtEnd() && (isAlphaNumeric(current()) || current() == '_')) {
        advance();
    }
    
    // Extract lexeme
    size_t length = position_ - start_pos;
    std::string_view lexeme = text_.substr(start_pos, length);
    
    // Check for !void (special case)
    if (lexeme == "!void") {
        return Token(TokenType::BANG_VOID, lexeme, start, getPosition());
    }
    
    // Check if it's a keyword
    TokenType type = Token::getKeywordType(lexeme);
    
    // Create token
    return Token(type, lexeme, start, getPosition());
}

// Process number tokens (integer and float literals)
Token Tokenizer::scanNumber() {
    common::SourcePosition start = getPosition();
    size_t start_pos = position_;
    bool is_float = false;
    bool has_exponent = false;
    
    // Consume digits
    while (!isAtEnd() && isDigit(current())) {
        advance();
    }
    
    // Check for decimal point
    if (!isAtEnd() && current() == '.' && isDigit(peek())) {
        is_float = true;
        advance(); // Consume '.'
        
        // Consume fractional part
        while (!isAtEnd() && isDigit(current())) {
            advance();
        }
    }
    
    // Check for exponent notation (e+, e-)
    if (!isAtEnd() && (current() == 'e' || current() == 'E')) {
        if (peek() == '+' || peek() == '-' || isDigit(peek())) {
            is_float = true;
            has_exponent = true;
            advance(); // Consume 'e' or 'E'
            
            // Consume sign if present
            if (!isAtEnd() && (current() == '+' || current() == '-')) {
                advance();
            }
            
            // Consume exponent digits
            if (!isAtEnd() && isDigit(current())) {
                while (!isAtEnd() && isDigit(current())) {
                    advance();
                }
            } else {
                error(common::ErrorCode::INVALID_NUMBER_FORMAT, "Expected digits after exponent");
                return errorToken("Expected digits after exponent");
            }
        }
    }
    
    // Check for hexadecimal suffix 'h'
    if (!isAtEnd() && current() == 'h' && !is_float) {
        advance(); // Consume 'h'
    }
    
    // Extract lexeme
    size_t length = position_ - start_pos;
    std::string_view lexeme = text_.substr(start_pos, length);
    
    // Create token
    TokenType type = is_float ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL;
    return Token(type, lexeme, start, getPosition());
}

// Process string tokens
Token Tokenizer::scanString() {
    common::SourcePosition start = getPosition();
    size_t start_pos = position_;
    
    advance(); // Consume opening quote
    
    // Consume string contents
    while (!isAtEnd() && current() != '"') {
        if (current() == '\\') {
            advance(); // Consume escape character
            
            if (isAtEnd()) {
                error(common::ErrorCode::UNTERMINATED_STRING, "Unterminated string");
                return errorToken("Unterminated string");
            }
            
            // Handle escape sequences
            switch (current()) {
                case '"':
                case '\\':
                case 'n':
                case 't':
                case 'r':
                    advance();
                    break;
                default:
                    error(common::ErrorCode::INVALID_ESCAPE_SEQUENCE, "Invalid escape sequence");
                    return errorToken("Invalid escape sequence");
            }
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        error(common::ErrorCode::UNTERMINATED_STRING, "Unterminated string");
        return errorToken("Unterminated string");
    }
    
    advance(); // Consume closing quote
    
    // Extract lexeme (including quotes)
    size_t length = position_ - start_pos;
    std::string_view lexeme = text_.substr(start_pos, length);
    
    // Create token
    return Token(TokenType::CHAR_LITERAL, lexeme, start, getPosition());
}

// Process injectable string tokens (i"...")
Token Tokenizer::scanIString() {
    common::SourcePosition start = getPosition();
    size_t start_pos = position_;
    
    advance(); // Consume 'i'
    advance(); // Consume opening quote
    
    std::vector<Token> parts;
    
    // Start with the opening token
    parts.push_back(Token(TokenType::ISTRING_START, "i\"", start, getPosition()));
    
    bool in_expression = false;
    size_t expression_start = 0;
    common::SourcePosition expr_start_pos;
    
    // Consume string contents and expressions
    while (!isAtEnd() && (in_expression || current() != '"')) {
        if (!in_expression && current() == '{') {
            // Extract text between expressions
            size_t text_start = start_pos + 2; // Skip i"
            size_t text_length = position_ - text_start;
            
            if (text_length > 0) {
                std::string_view text = text_.substr(text_start, text_length);
                common::SourcePosition text_start_pos = start;
                common::SourcePosition text_end_pos = getPosition();
                parts.push_back(Token(TokenType::ISTRING_MIDDLE, text, text_start_pos, text_end_pos));
            }
            
            // Start of expression
            in_expression = true;
            expression_start = position_;
            expr_start_pos = getPosition();
            
            advance(); // Consume '{'
            
            // Add expression start token
            parts.push_back(Token(TokenType::ISTRING_EXPR_START, "{", expr_start_pos, getPosition()));
        } else if (in_expression && current() == '}') {
            // End of expression
            in_expression = false;
            
            // Extract expression
            size_t expr_length = position_ - expression_start - 1; // Exclude '{'
            
            // Add expression end token
            common::SourcePosition expr_end_pos = getPosition();
            advance(); // Consume '}'
            parts.push_back(Token(TokenType::ISTRING_EXPR_END, "}", expr_end_pos, getPosition()));
            
            // Update start position for next text part
            start_pos = position_;
        } else if (in_expression && current() == '"') {
            // Handle quoted string inside expression
            scanString();
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        error(common::ErrorCode::UNTERMINATED_STRING, "Unterminated injectable string");
        return errorToken("Unterminated injectable string");
    }
    
    if (in_expression) {
        error(common::ErrorCode::UNTERMINATED_STRING, "Unterminated expression in injectable string");
        return errorToken("Unterminated expression in injectable string");
    }
    
    // Extract final text part if any
    size_t text_start = start_pos;
    size_t text_length = position_ - text_start;
    
    if (text_length > 0) {
        std::string_view text = text_.substr(text_start, text_length);
        common::SourcePosition text_start_pos = {line_, column_ - (unsigned int)text_length};
        common::SourcePosition text_end_pos = getPosition();
        parts.push_back(Token(TokenType::ISTRING_MIDDLE, text, text_start_pos, text_end_pos));
    }
    
    advance(); // Consume closing quote
    
    // Add closing token
    parts.push_back(Token(TokenType::ISTRING_END, "\"", {line_, column_ - 1}, getPosition()));
    
    // For now, return just the start token
    // The parser will need to handle the complexity of i-strings
    return parts[0];
}

// Process operator tokens
Token Tokenizer::scanOperator() {
    common::SourcePosition start = getPosition();
    char c = advance();
    
    switch (c) {
        // Single-character tokens
        case '(': return makeToken(TokenType::LEFT_PAREN);
        case ')': return makeToken(TokenType::RIGHT_PAREN);
        case '{': return makeToken(TokenType::LEFT_BRACE);
        case '}': return makeToken(TokenType::RIGHT_BRACE);
        case '[': return makeToken(TokenType::LEFT_BRACKET);
        case ']': return makeToken(TokenType::RIGHT_BRACKET);
        case ',': return makeToken(TokenType::COMMA);
        case '.': return makeToken(TokenType::DOT);
        case ':': return makeToken(TokenType::COLON);
        case ';': return makeToken(TokenType::SEMICOLON);
        case '?': return makeToken(TokenType::QUESTION);
        case '~': return makeToken(TokenType::TILDE);
        
        // Operators that could be one or two characters
        case '+':
            if (match('+')) return makeToken(TokenType::PLUS_PLUS);
            if (match('=')) return makeToken(TokenType::PLUS_EQUAL);
            return makeToken(TokenType::PLUS);
            
        case '-':
            if (match('-')) return makeToken(TokenType::MINUS_MINUS);
            if (match('=')) return makeToken(TokenType::MINUS_EQUAL);
            if (match('>')) return makeToken(TokenType::ARROW);
            return makeToken(TokenType::MINUS);
            
        case '*':
            if (match('*')) {
                if (match('=')) return makeToken(TokenType::DOUBLE_ASTERISK_EQUAL);
                return makeToken(TokenType::DOUBLE_ASTERISK);
            }
            if (match('=')) return makeToken(TokenType::ASTERISK_EQUAL);
            return makeToken(TokenType::ASTERISK);
            
        case '/':
            if (match('=')) return makeToken(TokenType::SLASH_EQUAL);
            return makeToken(TokenType::SLASH);
            
        case '%':
            if (match('=')) return makeToken(TokenType::PERCENT_EQUAL);
            return makeToken(TokenType::PERCENT);
            
        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL_EQUAL);
            return makeToken(TokenType::EQUAL);
            
        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL);
            return makeToken(TokenType::EXCLAMATION);
            
        case '<':
            if (match('<')) {
                if (match('=')) return makeToken(TokenType::LESS_LESS_EQUAL);
                return makeToken(TokenType::LESS_LESS);
            }
            if (match('=')) return makeToken(TokenType::LESS_EQUAL);
            return makeToken(TokenType::LESS);
            
        case '>':
            if (match('>')) {
                if (match('=')) return makeToken(TokenType::GREATER_GREATER_EQUAL);
                return makeToken(TokenType::GREATER_GREATER);
            }
            if (match('=')) return makeToken(TokenType::GREATER_EQUAL);
            return makeToken(TokenType::GREATER);
            
        case '&':
            if (match('&')) return makeToken(TokenType::AMPERSAND_AMPERSAND);
            if (match('=')) return makeToken(TokenType::AMPERSAND_EQUAL);
            return makeToken(TokenType::AMPERSAND);
            
        case '|':
            if (match('|')) return makeToken(TokenType::PIPE_PIPE);
            if (match('=')) return makeToken(TokenType::PIPE_EQUAL);
            return makeToken(TokenType::PIPE);
            
        case '^':
            if (match('=')) return makeToken(TokenType::CARET_EQUAL);
            return makeToken(TokenType::CARET);
    }
    
    // Unrecognized character
    error(common::ErrorCode::INVALID_CHARACTER, "Unexpected character");
    return errorToken("Unexpected character");
}

// Helper methods for scanning
bool Tokenizer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Tokenizer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

// Get current source position
common::SourcePosition Tokenizer::getPosition() const {
    return {line_, column_};
}

// Helper for building source location
output::SourceLocation Tokenizer::makeSourceLocation(
    const common::SourcePosition& start, 
    const common::SourcePosition& end) const {
    // Get the line content
    std::string_view line_content;
    int highlight_start = 0;
    int highlight_length = 0;
    
    if (source_->getLine(start.line, line_content)) {
        // Calculate highlight positions
        highlight_start = start.column - 1;
        highlight_length = end.column - start.column;
    }
    
    return output::SourceLocation(
        source_->filename(),
        start.line,
        start.column,
        line_content,
        highlight_start,
        highlight_length
    );
}

} // namespace lexer
} // namespace flux