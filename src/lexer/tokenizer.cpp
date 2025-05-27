#include "tokenizer.h"
#include <cctype>
#include <algorithm>
#include <cstring>

namespace flux {
namespace lexer {

Tokenizer::Tokenizer(std::shared_ptr<common::Source> source, common::Arena& arena)
    : source_(source), arena_(arena), current_offset_(0), current_position_(1, 1) {
}

Tokenizer::~Tokenizer() {
}

std::vector<Token> Tokenizer::tokenizeAll() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        
        // Filter out whitespace and comments by default
        if (!token.shouldFilter()) {
            tokens.push_back(token);
        }
        
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    
    return tokens;
}

Token Tokenizer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, current_offset_, current_offset_);
    }
    
    return scanToken();
}

Token Tokenizer::peekToken() {
    // Save current state
    size_t saved_offset = current_offset_;
    common::SourcePosition saved_position = current_position_;
    
    // Get next token
    Token token = nextToken();
    
    // Restore state
    current_offset_ = saved_offset;
    current_position_ = saved_position;
    
    return token;
}

bool Tokenizer::isAtEnd() const {
    return current_offset_ >= source_->text().size();
}

common::SourcePosition Tokenizer::getCurrentPosition() const {
    return current_position_;
}

char Tokenizer::currentChar() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source_->text()[current_offset_];
}

char Tokenizer::peekChar(size_t offset) const {
    size_t pos = current_offset_ + offset;
    if (pos >= source_->text().size()) {
        return '\0';
    }
    return source_->text()[pos];
}

void Tokenizer::advance() {
    if (!isAtEnd()) {
        updatePosition(currentChar());
        current_offset_++;
    }
}

void Tokenizer::advanceBy(size_t count) {
    for (size_t i = 0; i < count && !isAtEnd(); ++i) {
        advance();
    }
}

bool Tokenizer::isAtEnd(size_t offset) const {
    return current_offset_ + offset >= source_->text().size();
}

void Tokenizer::updatePosition(char c) {
    if (c == '\n') {
        current_position_.line++;
        current_position_.column = 1;
    } else {
        current_position_.column++;
    }
}

common::SourcePosition Tokenizer::getPositionAt(size_t offset) const {
    return source_->offsetToPosition(offset);
}

Token Tokenizer::makeToken(TokenType type, size_t start_offset, size_t end_offset) {
    std::string_view text = source_->text().substr(start_offset, end_offset - start_offset);
    common::SourcePosition pos = getPositionAt(start_offset);
    return Token(type, text, pos);
}

Token Tokenizer::makeToken(TokenType type, size_t start_offset, size_t end_offset, 
                          const std::string& processed_text) {
    Token token = makeToken(type, start_offset, end_offset);
    token.processed_text = processed_text;
    return token;
}

Token Tokenizer::makeErrorToken(const std::string& message, size_t start_offset, size_t end_offset) {
    Token token = makeToken(TokenType::ERROR, start_offset, end_offset);
    token.processed_text = message;
    reportError(common::ErrorCode::INVALID_CHARACTER, message, getPositionAt(start_offset));
    return token;
}

Token Tokenizer::scanToken() {
    size_t start_offset = current_offset_;
    char c = currentChar();
    
    // Single character tokens
    switch (c) {
        case '(':
            advance();
            return makeToken(TokenType::PAREN_OPEN, start_offset, current_offset_);
        case ')':
            advance();
            return makeToken(TokenType::PAREN_CLOSE, start_offset, current_offset_);
        case '{':
            advance();
            return makeToken(TokenType::BRACE_OPEN, start_offset, current_offset_);
        case '}':
            advance();
            return makeToken(TokenType::BRACE_CLOSE, start_offset, current_offset_);
        case '[':
            advance();
            return makeToken(TokenType::BRACKET_OPEN, start_offset, current_offset_);
        case ']':
            advance();
            return makeToken(TokenType::BRACKET_CLOSE, start_offset, current_offset_);
        case ';':
            advance();
            return makeToken(TokenType::SEMICOLON, start_offset, current_offset_);
        case ',':
            advance();
            return makeToken(TokenType::COMMA, start_offset, current_offset_);
        case '?':
            advance();
            return makeToken(TokenType::CONDITIONAL, start_offset, current_offset_);
        case '~':
            advance();
            return makeToken(TokenType::BITWISE_NOT, start_offset, current_offset_);
        case '@':
            advance();
            return makeToken(TokenType::ADDRESS_OF, start_offset, current_offset_);
        case '\n':
            advance();
            return makeToken(TokenType::NEWLINE, start_offset, current_offset_);
    }
    
    // Multi-character tokens and operators
    if (c == '/') {
        if (peekChar() == '/') {
            return scanLineComment();
        } else if (peekChar() == '*') {
            return scanBlockComment();
        } else {
            return scanOperator();
        }
    }
    
    // String literals
    if (c == '"') {
        return scanString();
    }
    
    // Character literals
    if (c == '\'') {
        return scanCharacter();
    }
    
    // i-string literals
    if (c == 'i' && peekChar() == '"') {
        return scanIString();
    }
    
    // Numbers
    if (isDigit(c)) {
        return scanNumber();
    }
    
    // Identifiers and keywords
    if (isIdentifierStart(c)) {
        return scanIdentifierOrKeyword();
    }
    
    // Operators
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || 
        c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || 
        c == '|' || c == '^' || c == ':' || c == '.') {
        return scanOperator();
    }
    
    // Unknown character
    advance();
    return makeErrorToken("Unexpected character", start_offset, current_offset_);
}

Token Tokenizer::scanIdentifierOrKeyword() {
    size_t start_offset = current_offset_;
    
    // Read identifier
    while (isIdentifierContinue(currentChar())) {
        advance();
    }
    
    std::string_view text = source_->text().substr(start_offset, current_offset_ - start_offset);
    TokenType type = getKeywordType(text);
    
    return makeToken(type, start_offset, current_offset_);
}

Token Tokenizer::scanNumber() {
    size_t start_offset = current_offset_;
    
    // Check for hex, binary, or octal prefix
    if (currentChar() == '0') {
        char next = peekChar();
        if (next == 'x' || next == 'X') {
            advanceBy(2); // Skip "0x"
            return scanIntegerLiteral(16, start_offset);
        } else if (next == 'b' || next == 'B') {
            advanceBy(2); // Skip "0b"
            return scanIntegerLiteral(2, start_offset);
        } else if (next == 'o' || next == 'O') {
            advanceBy(2); // Skip "0o"
            return scanIntegerLiteral(8, start_offset);
        }
    }
    
    // Decimal number (could be int or float)
    while (isDigit(currentChar())) {
        advance();
    }
    
    // Check for decimal point
    if (currentChar() == '.' && isDigit(peekChar())) {
        return scanFloatLiteral(start_offset);
    }
    
    // Check for exponent
    if (currentChar() == 'e' || currentChar() == 'E') {
        return scanFloatLiteral(start_offset);
    }
    
    return scanIntegerLiteral(10, start_offset);
}

Token Tokenizer::scanIntegerLiteral(int base, size_t start_offset) {
    // Read digits in the specified base
    while (isValidInBase(currentChar(), base)) {
        advance();
    }
    
    std::string_view text = source_->text().substr(start_offset, current_offset_ - start_offset);
    Token token = makeToken(TokenType::INTEGER_LITERAL, start_offset, current_offset_);
    
    try {
        if (base == 16) {
            token.integer_value = parseIntegerLiteral(text.substr(2), 16); // Skip "0x"
        } else if (base == 2) {
            token.integer_value = parseIntegerLiteral(text.substr(2), 2); // Skip "0b"
        } else if (base == 8) {
            token.integer_value = parseIntegerLiteral(text.substr(2), 8); // Skip "0o"
        } else {
            token.integer_value = parseIntegerLiteral(text, base);
        }
    } catch (...) {
        return makeErrorToken("Invalid integer literal", start_offset, current_offset_);
    }
    
    return token;
}

Token Tokenizer::scanFloatLiteral(size_t start_offset) {
    // Handle decimal point
    if (currentChar() == '.') {
        advance();
        while (isDigit(currentChar())) {
            advance();
        }
    }
    
    // Handle exponent
    if (currentChar() == 'e' || currentChar() == 'E') {
        advance();
        if (currentChar() == '+' || currentChar() == '-') {
            advance();
        }
        while (isDigit(currentChar())) {
            advance();
        }
    }
    
    std::string_view text = source_->text().substr(start_offset, current_offset_ - start_offset);
    Token token = makeToken(TokenType::FLOAT_LITERAL, start_offset, current_offset_);
    
    try {
        token.float_value = parseFloatLiteral(text);
    } catch (...) {
        return makeErrorToken("Invalid float literal", start_offset, current_offset_);
    }
    
    return token;
}

bool Tokenizer::isValidInBase(char c, int base) {
    switch (base) {
        case 2: return isBinaryDigit(c);
        case 8: return isOctalDigit(c);
        case 10: return isDigit(c);
        case 16: return isHexDigit(c);
        default: return false;
    }
}

Token Tokenizer::scanString() {
    size_t start_offset = current_offset_;
    advance(); // Skip opening quote
    
    std::string content;
    while (!isAtEnd() && currentChar() != '"') {
        if (currentChar() == '\\') {
            advance();
            if (isAtEnd()) {
                return makeErrorToken("Unterminated string literal", start_offset, current_offset_);
            }
            content += parseEscapeSequence();
        } else if (currentChar() == '\n') {
            return makeErrorToken("Unterminated string literal", start_offset, current_offset_);
        } else {
            content += currentChar();
            advance();
        }
    }
    
    if (isAtEnd()) {
        return makeErrorToken("Unterminated string literal", start_offset, current_offset_);
    }
    
    advance(); // Skip closing quote
    
    return makeToken(TokenType::STRING_LITERAL, start_offset, current_offset_, content);
}

Token Tokenizer::scanCharacter() {
    size_t start_offset = current_offset_;
    advance(); // Skip opening quote
    
    if (isAtEnd() || currentChar() == '\n') {
        return makeErrorToken("Unterminated character literal", start_offset, current_offset_);
    }
    
    char c;
    if (currentChar() == '\\') {
        advance();
        if (isAtEnd()) {
            return makeErrorToken("Unterminated character literal", start_offset, current_offset_);
        }
        c = parseEscapeSequence();
    } else {
        c = currentChar();
        advance();
    }
    
    if (isAtEnd() || currentChar() != '\'') {
        return makeErrorToken("Unterminated character literal", start_offset, current_offset_);
    }
    
    advance(); // Skip closing quote
    
    std::string content(1, c);
    Token token = makeToken(TokenType::CHARACTER_LITERAL, start_offset, current_offset_, content);
    token.integer_value = static_cast<long long>(c);
    
    return token;
}

char Tokenizer::parseEscapeSequence() {
    char c = currentChar();
    advance();
    
    switch (c) {
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case '\\': return '\\';
        case '"': return '"';
        case '\'': return '\'';
        case '0': return '\0';
        case 'x': {
            // Hex escape \xHH
            if (!isAtEnd() && isHexDigit(currentChar())) {
                char hex1 = currentChar();
                advance();
                if (!isAtEnd() && isHexDigit(currentChar())) {
                    char hex2 = currentChar();
                    advance();
                    char hex_str[3] = {hex1, hex2, '\0'};
                    return static_cast<char>(std::strtol(hex_str, nullptr, 16));
                }
            }
            return 'x'; // Invalid hex escape
        }
        case 'u': {
            // Unicode escape \uHHHH
            char hex_chars[4];
            bool valid = true;
            for (int i = 0; i < 4; ++i) {
                if (isAtEnd() || !isHexDigit(currentChar())) {
                    valid = false;
                    break;
                }
                hex_chars[i] = currentChar();
                advance();
            }
            if (valid) {
                char hex_str[5] = {hex_chars[0], hex_chars[1], hex_chars[2], hex_chars[3], '\0'};
                int unicode_value = std::strtol(hex_str, nullptr, 16);
                if (unicode_value < 128) {
                    return static_cast<char>(unicode_value);
                }
            }
            return 'u'; // Invalid unicode escape
        }
        default:
            return c; // Unknown escape, return as literal
    }
}

Token Tokenizer::scanDataLiteral() {
    size_t start_offset = current_offset_;
    advance(); // Skip opening brace
    
    std::string bits;
    while (!isAtEnd() && currentChar() != '}') {
        if (currentChar() == '0' || currentChar() == '1') {
            bits += currentChar();
            advance();
        } else if (currentChar() == ',' || isWhitespace(currentChar())) {
            advance(); // Skip separator or whitespace
        } else {
            return makeErrorToken("Invalid character in data literal", start_offset, current_offset_);
        }
    }
    
    if (isAtEnd()) {
        return makeErrorToken("Unterminated data literal", start_offset, current_offset_);
    }
    
    advance(); // Skip closing brace
    
    return makeToken(TokenType::DATA_LITERAL, start_offset, current_offset_, bits);
}

Token Tokenizer::scanIString() {
    size_t start_offset = current_offset_;
    advance(); // Skip 'i'
    
    // Now we expect a string literal
    Token string_token = scanString();
    if (string_token.type != TokenType::STRING_LITERAL) {
        return string_token; // Return the error
    }
    
    // Check for colon
    if (currentChar() != ':') {
        return makeErrorToken("Expected ':' after i-string", start_offset, current_offset_);
    }
    advance();
    
    // Skip opening brace
    if (currentChar() != '{') {
        return makeErrorToken("Expected '{' after i-string ':'", start_offset, current_offset_);
    }
    advance();
    
    // Read expression list (simplified for now - just read until closing brace)
    std::string expressions;
    int brace_count = 1;
    while (!isAtEnd() && brace_count > 0) {
        if (currentChar() == '{') {
            brace_count++;
        } else if (currentChar() == '}') {
            brace_count--;
        }
        if (brace_count > 0) {
            expressions += currentChar();
        }
        advance();
    }
    
    if (brace_count > 0) {
        return makeErrorToken("Unterminated i-string expression list", start_offset, current_offset_);
    }
    
    // Skip closing semicolon
    if (currentChar() == ';') {
        advance();
    }
    
    Token token = makeToken(TokenType::I_STRING_LITERAL, start_offset, current_offset_, 
                           string_token.processed_text);
    // Store expressions in a separate field or process them later
    return token;
}

Token Tokenizer::scanLineComment() {
    size_t start_offset = current_offset_;
    
    // Skip "//"
    advanceBy(2);
    
    // Read until end of line
    while (!isAtEnd() && currentChar() != '\n') {
        advance();
    }
    
    return makeToken(TokenType::LINE_COMMENT, start_offset, current_offset_);
}

Token Tokenizer::scanBlockComment() {
    size_t start_offset = current_offset_;
    
    // Skip "/*"
    advanceBy(2);
    
    // Read until "*/"
    while (!isAtEnd()) {
        if (currentChar() == '*' && peekChar() == '/') {
            advanceBy(2);
            break;
        }
        advance();
    }
    
    return makeToken(TokenType::BLOCK_COMMENT, start_offset, current_offset_);
}

Token Tokenizer::scanOperator() {
    size_t start_offset = current_offset_;
    char c = currentChar();
    
    advance();
    
    switch (c) {
        case '+':
            if (match('+')) return makeToken(TokenType::INCREMENT, start_offset, current_offset_);
            if (match('=')) return makeToken(TokenType::ASSIGN_ADD, start_offset, current_offset_);
            return makeToken(TokenType::PLUS, start_offset, current_offset_);
            
        case '-':
            if (match('-')) return makeToken(TokenType::DECREMENT, start_offset, current_offset_);
            if (match('=')) return makeToken(TokenType::ASSIGN_SUB, start_offset, current_offset_);
            if (match('>')) return makeToken(TokenType::ARROW, start_offset, current_offset_);
            return makeToken(TokenType::MINUS, start_offset, current_offset_);
            
        case '*':
            if (match('*')) {
                if (match('=')) return makeToken(TokenType::ASSIGN_POW, start_offset, current_offset_);
                return makeToken(TokenType::POWER, start_offset, current_offset_);
            }
            if (match('=')) return makeToken(TokenType::ASSIGN_MUL, start_offset, current_offset_);
            return makeToken(TokenType::MULTIPLY, start_offset, current_offset_);
            
        case '/':
            if (match('=')) return makeToken(TokenType::ASSIGN_DIV, start_offset, current_offset_);
            return makeToken(TokenType::DIVIDE, start_offset, current_offset_);
            
        case '%':
            if (match('=')) return makeToken(TokenType::ASSIGN_MOD, start_offset, current_offset_);
            return makeToken(TokenType::MODULO, start_offset, current_offset_);
            
        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL, start_offset, current_offset_);
            return makeToken(TokenType::ASSIGN, start_offset, current_offset_);
            
        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL, start_offset, current_offset_);
            return makeToken(TokenType::LOGICAL_NOT, start_offset, current_offset_);
            
        case '<':
            if (match('<')) {
                if (match('=')) return makeToken(TokenType::ASSIGN_SHL, start_offset, current_offset_);
                return makeToken(TokenType::SHIFT_LEFT, start_offset, current_offset_);
            }
            if (match('=')) return makeToken(TokenType::LESS_EQUAL, start_offset, current_offset_);
            return makeToken(TokenType::LESS_THAN, start_offset, current_offset_);
            
        case '>':
            if (match('>')) {
                if (match('=')) return makeToken(TokenType::ASSIGN_SHR, start_offset, current_offset_);
                return makeToken(TokenType::SHIFT_RIGHT, start_offset, current_offset_);
            }
            if (match('=')) return makeToken(TokenType::GREATER_EQUAL, start_offset, current_offset_);
            return makeToken(TokenType::GREATER_THAN, start_offset, current_offset_);
            
        case '&':
            if (match('&')) return makeToken(TokenType::LOGICAL_AND, start_offset, current_offset_);
            if (match('=')) return makeToken(TokenType::ASSIGN_AND, start_offset, current_offset_);
            return makeToken(TokenType::BITWISE_AND, start_offset, current_offset_);
            
        case '|':
            if (match('|')) return makeToken(TokenType::LOGICAL_OR, start_offset, current_offset_);
            if (match('=')) return makeToken(TokenType::ASSIGN_OR, start_offset, current_offset_);
            return makeToken(TokenType::BITWISE_OR, start_offset, current_offset_);
            
        case '^':
            if (match('^')) {
                if (match('=')) return makeToken(TokenType::ASSIGN_XOR, start_offset, current_offset_);
                return makeToken(TokenType::BITWISE_XOR, start_offset, current_offset_);
            }
            if (match('=')) return makeToken(TokenType::ASSIGN_EXP, start_offset, current_offset_);
            return makeToken(TokenType::EXPONENT, start_offset, current_offset_);
            
        case ':':
            if (match(':')) return makeToken(TokenType::SCOPE_RESOLUTION, start_offset, current_offset_);
            return makeToken(TokenType::COLON, start_offset, current_offset_);
            
        case '.':
            if (match('.')) return makeToken(TokenType::RANGE, start_offset, current_offset_);
            return makeToken(TokenType::MEMBER_ACCESS, start_offset, current_offset_);
            
        default:
            return makeErrorToken("Unknown operator", start_offset, current_offset_);
    }
}

void Tokenizer::skipWhitespace() {
    while (!isAtEnd() && isWhitespace(currentChar())) {
        advance();
    }
}

bool Tokenizer::match(char expected) {
    if (isAtEnd() || currentChar() != expected) {
        return false;
    }
    advance();
    return true;
}

bool Tokenizer::matchSequence(const char* sequence) {
    size_t len = std::strlen(sequence);
    for (size_t i = 0; i < len; ++i) {
        if (isAtEnd(i) || peekChar(i) != sequence[i]) {
            return false;
        }
    }
    advanceBy(len);
    return true;
}

void Tokenizer::reportError(common::ErrorCode code, const std::string& message) {
    reportError(code, message, current_position_);
}

void Tokenizer::reportError(common::ErrorCode code, const std::string& message, 
                           const common::SourcePosition& position) {
    output::SourceLocation location(
        source_->filename(),
        static_cast<int>(position.line),
        static_cast<int>(position.column)
    );
    errors_.addError(code, message, location);
}

} // namespace lexer
} // namespace flux