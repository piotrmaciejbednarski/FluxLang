#include "tokenizer.h"
#include <cctype>
#include <cstring>
#include <charconv>

namespace flux {
namespace lexer {

Tokenizer::Tokenizer(std::shared_ptr<common::Source> source, common::Arena& arena)
    : source_(source), arena_(arena), current_(0), position_(1, 1),
      iStringState_(IStringState::NONE), iStringBraceDepth_(0) {
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        if (token.type != TokenType::WHITESPACE && token.type != TokenType::COMMENT) {
            tokens.push_back(token);
        }
        if (token.type == TokenType::ERROR) {
            break;
        }
    }
    
    // Add EOF token
    common::SourcePosition eofPos = position_;
    tokens.emplace_back(TokenType::END_OF_FILE, "", makeRange(eofPos));
    
    return tokens;
}

Token Tokenizer::nextToken() {
    if (iStringState_ != IStringState::NONE) {
        return lexIStringExpression(position_);
    }
    
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, position_);
    }
    
    common::SourcePosition start = position_;
    char c = advance();
    
    // Numbers
    if (isDigit(c)) {
        current_--; // Back up
        position_ = start;
        return lexNumber(start);
    }
    
    // Identifiers and keywords
    if (isAlpha(c) || c == '_') {
        current_--; // Back up
        position_ = start;
        return lexIdentifier(start);
    }
    
    // Strings
    if (c == '"') {
        current_--; // Back up
        position_ = start;
        return lexString(start);
    }
    
    // I-strings
    if (c == 'i' && peek() == '"') {
        current_--; // Back up
        position_ = start;
        return lexIString(start);
    }
    
    // Comments - handle before other operators
    if (c == '/' && peek() == '/') {
        current_--; // Back up
        position_ = start;
        skipComment();
        return makeToken(TokenType::COMMENT, start);
    }
    
    // Single character tokens and operators
    switch (c) {
        // Simple punctuation
        case '(':  return makeToken(TokenType::LEFT_PAREN, start);
        case ')':  return makeToken(TokenType::RIGHT_PAREN, start);
        case '{':  return makeToken(TokenType::LEFT_BRACE, start);
        case '}':  return makeToken(TokenType::RIGHT_BRACE, start);
        case '[':  return makeToken(TokenType::LEFT_BRACKET, start);
        case ']':  return makeToken(TokenType::RIGHT_BRACKET, start);
        case ';':  return makeToken(TokenType::SEMICOLON, start);
        case ',':  return makeToken(TokenType::COMMA, start);
        case '.':  return makeToken(TokenType::DOT, start);
        case '?':  return makeToken(TokenType::QUESTION, start);
        case '~':  return makeToken(TokenType::BITWISE_NOT, start);
        case '@':  return makeToken(TokenType::ADDRESS_OF, start);
    }
    
    // Multi-character operators
    current_--; // Back up
    position_ = start;
    return lexOperator(start);
}

Token Tokenizer::peekToken() {
    size_t savedCurrent = current_;
    common::SourcePosition savedPosition = position_;
    IStringState savedIStringState = iStringState_;
    int savedIStringBraceDepth = iStringBraceDepth_;
    
    Token token = nextToken();
    
    current_ = savedCurrent;
    position_ = savedPosition;
    iStringState_ = savedIStringState;
    iStringBraceDepth_ = savedIStringBraceDepth;
    
    return token;
}

bool Tokenizer::isAtEnd() const {
    return current_ >= source_->text().size();
}

common::SourcePosition Tokenizer::currentPosition() const {
    return position_;
}

char Tokenizer::peek() const {
    if (isAtEnd()) return '\0';
    return source_->text()[current_];
}

char Tokenizer::peekNext() const {
    if (current_ + 1 >= source_->text().size()) return '\0';
    return source_->text()[current_ + 1];
}

char Tokenizer::advance() {
    if (isAtEnd()) return '\0';
    char c = source_->text()[current_++];
    updatePosition(c);
    return c;
}

bool Tokenizer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_->text()[current_] != expected) return false;
    advance();
    return true;
}

bool Tokenizer::match(const char* str) {
    size_t len = std::strlen(str);
    if (current_ + len > source_->text().size()) return false;
    
    for (size_t i = 0; i < len; i++) {
        if (source_->text()[current_ + i] != str[i]) return false;
    }
    
    // Advance past the matched string
    for (size_t i = 0; i < len; i++) {
        advance();
    }
    return true;
}

void Tokenizer::skipWhitespace() {
    while (!isAtEnd() && isWhitespace(peek())) {
        advance();
    }
}

void Tokenizer::skipComment() {
    // We should be positioned at the first '/'
    if (peek() == '/' && peekNext() == '/') {
        advance(); // consume first '/'
        advance(); // consume second '/'
        
        // Read until end of line
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
        // Don't consume the newline - let the main loop handle it
    }
}

void Tokenizer::updatePosition(char c) {
    if (c == '\n') {
        position_.line++;
        position_.column = 1;
    } else {
        position_.column++;
    }
}

common::SourceRange Tokenizer::makeRange(const common::SourcePosition& start) const {
    return common::SourceRange(start, position_);
}

Token Tokenizer::makeToken(TokenType type, const common::SourcePosition& start) {
    size_t startOffset = source_->positionToOffset(start);
    size_t endOffset = source_->positionToOffset(position_);
    std::string_view text = source_->text().substr(startOffset, endOffset - startOffset);
    return Token(type, text, makeRange(start));
}

Token Tokenizer::makeToken(TokenType type, const common::SourcePosition& start, int64_t value) {
    size_t startOffset = source_->positionToOffset(start);
    size_t endOffset = source_->positionToOffset(position_);
    std::string_view text = source_->text().substr(startOffset, endOffset - startOffset);
    return Token(type, text, makeRange(start), value);
}

Token Tokenizer::makeToken(TokenType type, const common::SourcePosition& start, double value) {
    size_t startOffset = source_->positionToOffset(start);
    size_t endOffset = source_->positionToOffset(position_);
    std::string_view text = source_->text().substr(startOffset, endOffset - startOffset);
    return Token(type, text, makeRange(start), value);
}

Token Tokenizer::makeErrorToken(const char* message, const common::SourcePosition& start) {
    reportError(common::ErrorCode::INVALID_CHARACTER, message, start);
    return Token(TokenType::ERROR, message, makeRange(start));
}

Token Tokenizer::lexNumber(const common::SourcePosition& start) {
    bool hasDecimal = false;
    bool hasExponent = false;
    
    // Consume digits
    while (isDigit(peek())) {
        advance();
    }
    
    // Look for decimal point
    if (peek() == '.' && isDigit(peekNext())) {
        hasDecimal = true;
        advance(); // consume '.'
        while (isDigit(peek())) {
            advance();
        }
    }
    
    // Look for exponent
    if (peek() == 'e' || peek() == 'E') {
        hasExponent = true;
        advance(); // consume 'e' or 'E'
        if (peek() == '+' || peek() == '-') {
            advance(); // consume sign
        }
        while (isDigit(peek())) {
            advance();
        }
    }
    
    size_t startOffset = source_->positionToOffset(start);
    size_t endOffset = source_->positionToOffset(position_);
    std::string_view text = source_->text().substr(startOffset, endOffset - startOffset);
    
    if (hasDecimal || hasExponent) {
        double value;
        if (parseFloat(text, value)) {
            return makeToken(TokenType::FLOAT_LITERAL, start, value);
        } else {
            return makeErrorToken("Invalid float format", start);
        }
    } else {
        int64_t value;
        if (parseInteger(text, value)) {
            return makeToken(TokenType::INTEGER_LITERAL, start, value);
        } else {
            return makeErrorToken("Invalid integer format", start);
        }
    }
}

Token Tokenizer::lexString(const common::SourcePosition& start) {
    advance(); // consume opening '"'
    
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance(); // consume '\'
            if (isAtEnd()) {
                return makeErrorToken("Unterminated string", start);
            }
            advance(); // consume escaped character
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        return makeErrorToken("Unterminated string", start);
    }
    
    advance(); // consume closing '"'
    return makeToken(TokenType::STRING_LITERAL, start);
}

Token Tokenizer::lexIString(const common::SourcePosition& start) {
    advance(); // consume 'i'
    advance(); // consume '"'
    
    iStringState_ = IStringState::IN_TEXT;
    return makeToken(TokenType::I_STRING_START, start);
}

Token Tokenizer::lexIdentifier(const common::SourcePosition& start) {
    while (isAlphaNumeric(peek()) || peek() == '_') {
        advance();
    }
    
    size_t startOffset = source_->positionToOffset(start);
    size_t endOffset = source_->positionToOffset(position_);
    std::string_view text = source_->text().substr(startOffset, endOffset - startOffset);
    
    TokenType type = keywordTable.lookup(text);
    return makeToken(type, start);
}

Token Tokenizer::lexBinaryLiteral(const common::SourcePosition& start) {
    advance(); // consume '{'
    
    bool foundBinaryDigits = false;
    bool hasOnlyBinaryAndCommas = true;
    
    while (!isAtEnd() && peek() != '}') {
        char c = peek();
        if (isBinaryDigit(c)) {
            foundBinaryDigits = true;
            advance();
        } else if (c == ',' || isWhitespace(c)) {
            advance();
        } else {
            // Found non-binary, non-comma, non-whitespace character
            hasOnlyBinaryAndCommas = false;
            break;
        }
    }
    
    // Only treat as binary literal if:
    // 1. We found actual binary digits (0 or 1)
    // 2. Everything inside was binary digits, commas, or whitespace
    // 3. We reached the closing brace
    if (!foundBinaryDigits || !hasOnlyBinaryAndCommas || peek() != '}') {
        // This is not a binary literal, treat '{' as a regular left brace
        // Reset position to just after the opening brace
        current_ = source_->positionToOffset(start) + 1;
        position_ = start;
        updatePosition('{');
        return makeToken(TokenType::LEFT_BRACE, start);
    }
    
    advance(); // consume '}'
    return makeToken(TokenType::BINARY_LITERAL, start);
}

Token Tokenizer::lexOperator(const common::SourcePosition& start) {
    char c = advance();
    
    switch (c) {
        case '+':
            if (match('+')) return makeToken(TokenType::INCREMENT, start);
            if (match('=')) return makeToken(TokenType::PLUS_ASSIGN, start);
            return makeToken(TokenType::PLUS, start);
            
        case '-':
            if (match('-')) return makeToken(TokenType::DECREMENT, start);
            if (match('=')) return makeToken(TokenType::MINUS_ASSIGN, start);
            if (match('>')) return makeToken(TokenType::ARROW, start);
            return makeToken(TokenType::MINUS, start);
            
        case '*':
            if (match('*')) {
                if (match('=')) return makeToken(TokenType::POWER_ASSIGN, start);
                return makeToken(TokenType::POWER, start);
            }
            if (match('=')) return makeToken(TokenType::MULTIPLY_ASSIGN, start);
            return makeToken(TokenType::MULTIPLY, start);
            
        case '/':
            if (match('=')) return makeToken(TokenType::DIVIDE_ASSIGN, start);
            return makeToken(TokenType::DIVIDE, start);
            
        case '%':
            if (match('=')) return makeToken(TokenType::MODULO_ASSIGN, start);
            return makeToken(TokenType::MODULO, start);
            
        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL, start);
            return makeToken(TokenType::ASSIGN, start);
            
        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL, start);
            return makeToken(TokenType::LOGICAL_NOT, start);
            
        case '<':
            if (match('<')) {
                if (match('=')) return makeToken(TokenType::SHIFT_LEFT_ASSIGN, start);
                return makeToken(TokenType::SHIFT_LEFT, start);
            }
            if (match('=')) return makeToken(TokenType::LESS_EQUAL, start);
            return makeToken(TokenType::LESS_THAN, start);
            
        case '>':
            if (match('>')) {
                if (match('=')) return makeToken(TokenType::SHIFT_RIGHT_ASSIGN, start);
                return makeToken(TokenType::SHIFT_RIGHT, start);
            }
            if (match('=')) return makeToken(TokenType::GREATER_EQUAL, start);
            return makeToken(TokenType::GREATER_THAN, start);
            
        case '&':
            if (match('&')) return makeToken(TokenType::LOGICAL_AND, start);
            if (match('=')) return makeToken(TokenType::BITWISE_AND_ASSIGN, start);
            return makeToken(TokenType::BITWISE_AND, start);
            
        case '|':
            if (match('|')) return makeToken(TokenType::LOGICAL_OR, start);
            if (match('=')) return makeToken(TokenType::BITWISE_OR_ASSIGN, start);
            return makeToken(TokenType::BITWISE_OR, start);
            
        case '^':
            if (match('=')) return makeToken(TokenType::BITWISE_XOR_ASSIGN, start);
            return makeToken(TokenType::BITWISE_XOR, start);
            
        case ':':
            if (match(':')) return makeToken(TokenType::SCOPE_RESOLUTION, start);
            if (iStringState_ == IStringState::WAITING_FOR_COLON && match('{')) {
                iStringState_ = IStringState::IN_EXPRESSION;
                iStringBraceDepth_ = 1;
                return makeToken(TokenType::I_STRING_EXPR_START, start);
            }
            return makeToken(TokenType::COLON, start);
            
        default:
            return makeErrorToken("Unexpected character", start);
    }
}

Token Tokenizer::lexIStringText(const common::SourcePosition& start) {
    while (!isAtEnd() && peek() != '"' && peek() != '{') {
        if (peek() == '\\') {
            advance(); // consume '\'
            if (isAtEnd()) {
                return makeErrorToken("Unterminated i-string", start);
            }
            advance(); // consume escaped character
        } else {
            advance();
        }
    }
    
    if (peek() == '"') {
        advance(); // consume '"'
        iStringState_ = IStringState::NONE;
        return makeToken(TokenType::I_STRING_END, start);
    } else if (peek() == '{') {
        advance(); // consume '{'
        if (peek() == '}') {
            advance(); // consume '}'
            // Empty placeholder, continue as text
            return lexIStringText(start);
        } else {
            current_--; // back up
            position_.column--;
            iStringState_ = IStringState::WAITING_FOR_COLON;
            return makeToken(TokenType::I_STRING_TEXT, start);
        }
    }
    
    return makeToken(TokenType::I_STRING_TEXT, start);
}

Token Tokenizer::lexIStringExpression(const common::SourcePosition& start) {
    if (iStringState_ == IStringState::IN_TEXT) {
        return lexIStringText(start);
    }
    
    if (iStringState_ == IStringState::IN_EXPRESSION) {
        // Handle brace counting for nested expressions
        char c = peek();
        if (c == '{') {
            iStringBraceDepth_++;
        } else if (c == '}') {
            iStringBraceDepth_--;
            if (iStringBraceDepth_ == 0) {
                advance(); // consume '}'
                if (match(';')) {
                    iStringState_ = IStringState::IN_TEXT;
                    return makeToken(TokenType::I_STRING_EXPR_END, start);
                } else {
                    return makeErrorToken("Expected ';' after i-string expression", start);
                }
            }
        }
        
        // Parse normal token within expression
        iStringState_ = IStringState::NONE;
        Token token = nextToken();
        iStringState_ = IStringState::IN_EXPRESSION;
        return token;
    }
    
    return makeErrorToken("Invalid i-string state", start);
}

bool Tokenizer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Tokenizer::isHexDigit(char c) const {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Tokenizer::isBinaryDigit(char c) const {
    return c == '0' || c == '1';
}

bool Tokenizer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Tokenizer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Tokenizer::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool Tokenizer::parseInteger(std::string_view text, int64_t& result, int base) const {
    const char* start = text.data();
    const char* end = text.data() + text.size();
    
    auto [ptr, ec] = std::from_chars(start, end, result, base);
    return ec == std::errc{} && ptr == end;
}

bool Tokenizer::parseFloat(std::string_view text, double& result) const {
    const char* start = text.data();
    const char* end = text.data() + text.size();
    
    auto [ptr, ec] = std::from_chars(start, end, result);
    return ec == std::errc{} && ptr == end;
}

std::string Tokenizer::parseStringLiteral(std::string_view text) const {
    std::string result;
    result.reserve(text.size());
    
    const char* ptr = text.data() + 1; // Skip opening quote
    const char* end = text.data() + text.size() - 1; // Skip closing quote
    
    while (ptr < end) {
        if (*ptr == '\\') {
            result += parseEscapeSequence(ptr);
        } else {
            result += *ptr++;
        }
    }
    
    return result;
}

char Tokenizer::parseEscapeSequence(const char*& ptr) const {
    ++ptr; // Skip backslash
    
    switch (*ptr++) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '"': return '"';
        case '\'': return '\'';
        case '0': return '\0';
        default:
            --ptr; // Back up
            return *ptr++; // Return the character as-is
    }
}

void Tokenizer::reportError(common::ErrorCode code, const char* message, const common::SourcePosition& position) {
    // Convert position to output location
    output::SourceLocation location(
        source_->filename(),
        static_cast<int>(position.line),
        static_cast<int>(position.column)
    );
    
    errorCollector_.addError(code, message, location);
}

}
}