/**
 * @file lexer.cpp
 * @brief Implementation of the Flux lexer
 */

#include "lexer.hpp"
#include <sstream>
#include <cctype>

namespace flux {

// Initialize the keywords map
const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"object", TokenType::OBJECT},
    {"when", TokenType::WHEN},
    {"asm", TokenType::ASM},
    {"async", TokenType::ASYNC},
    {"await", TokenType::AWAIT},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"bitand", TokenType::BITAND},
    {"bitor", TokenType::BITOR},
    {"break", TokenType::BREAK},
    {"case", TokenType::CASE},
    {"catch", TokenType::CATCH},
    {"char", TokenType::CHAR_TYPE},
    {"class", TokenType::CLASS},
    {"const", TokenType::CONST},
    {"continue", TokenType::CONTINUE},
    {"default", TokenType::DEFAULT},
    {"delete", TokenType::DELETE},
    {"do", TokenType::DO},
    {"else", TokenType::ELSE},
    {"enum", TokenType::ENUM},
    {"false", TokenType::FALSE},
    {"float", TokenType::FLOAT_TYPE},
    {"for", TokenType::FOR},
    {"function", TokenType::FUNCTION},
    {"goto", TokenType::GOTO},
    {"if", TokenType::IF},
    {"import", TokenType::IMPORT},
    {"in", TokenType::IN},
    {"is", TokenType::IS},
    {"int", TokenType::INT},
    {"lock", TokenType::LOCK},
    {"__lock", TokenType::DUNDER_LOCK},
    {"lock__", TokenType::LOCK_DUNDER},
    {"namespace", TokenType::NAMESPACE},
    {"new", TokenType::NEW},
    {"not", TokenType::NOT},
    {"nullptr", TokenType::NULL_LITERAL},
    {"operator", TokenType::OPERATOR},
    {"print", TokenType::PRINT},
    {"input", TokenType::INPUT},
    {"return", TokenType::RETURN},
    {"sizeof", TokenType::SIZEOF},
    {"struct", TokenType::STRUCT},
    {"switch", TokenType::SWITCH},
    {"this", TokenType::THIS},
    {"throw", TokenType::THROW},
    {"true", TokenType::TRUE},
    {"try", TokenType::TRY},
    {"typedef", TokenType::TYPEDEF},
    {"union", TokenType::UNION},
    {"using", TokenType::USING},
    {"void", TokenType::VOID},
    {"volatile", TokenType::VOLATILE},
    {"while", TokenType::WHILE},
    {"bool", TokenType::BOOL},
    {"xor", TokenType::XOR}
};

Lexer::Lexer(std::string source) : source(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start = current;
        scanToken();
    }
    
    tokens.emplace_back(TokenType::END_OF_FILE, "", line);
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        // Single character tokens
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case '.': addToken(TokenType::DOT); break;
        case '+': 
            if (match('=')) {
                addToken(TokenType::PLUS_EQUAL);
            } else {
                addToken(TokenType::PLUS);
            }
            break;
        case '-': 
            if (match('=')) {
                addToken(TokenType::MINUS_EQUAL);
            } else if (match('>')) {
                addToken(TokenType::ARROW);
            } else {
                addToken(TokenType::MINUS);
            }
            break;
        case '*': 
            if (match('=')) {
                addToken(TokenType::STAR_EQUAL);
            } else {
                addToken(TokenType::STAR);
            }
            break;
        case '%': 
            if (match('=')) {
                addToken(TokenType::PERCENT_EQUAL);
            } else {
                addToken(TokenType::PERCENT);
            }
            break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case ':': 
            if (match(':')) {
                addToken(TokenType::SCOPE_RESOLUTION);
            } else {
                addToken(TokenType::COLON);
            }
            break;
        case '@': addToken(TokenType::AT); break;
        case '~': addToken(TokenType::TILDE); break;
        
        // One or two character tokens
        case '!':
            addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
            break;
        case '=':
            addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            break;
        case '<':
            if (match('=')) {
                addToken(TokenType::LESS_EQUAL);
            } else if (match('<')) {
                addToken(TokenType::SHIFT_LEFT);
            } else {
                addToken(TokenType::LESS);
            }
            break;
        case '>':
            if (match('=')) {
                addToken(TokenType::GREATER_EQUAL);
            } else if (match('>')) {
                addToken(TokenType::SHIFT_RIGHT);
            } else {
                addToken(TokenType::GREATER);
            }
            break;
        case '&':
            if (match('=')) {
                addToken(TokenType::BITAND_EQUAL);
            } else {
                addToken(TokenType::BITAND);
            }
            break;
        case '|':
            if (match('=')) {
                addToken(TokenType::BITOR_EQUAL);
            } else {
                addToken(TokenType::BITOR);
            }
            break;
        case '^':
            if (match('=')) {
                addToken(TokenType::XOR_EQUAL);
            } else {
                addToken(TokenType::XOR);
            }
            break;
            
        // Slash or comments
        case '/':
            if (match('/')) {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else if (match('*')) {
                // Multi-line comment
                while (!isAtEnd() && !(peek() == '*' && peekNext() == '/')) {
                    if (peek() == '\n') line++;
                    advance();
                }
                
                // Consume the closing "*/"
                if (!isAtEnd()) {
                    advance(); // *
                    advance(); // /
                } else {
                    error("Unterminated multi-line comment");
                }
            } else if (match('=')) {
                addToken(TokenType::SLASH_EQUAL);
            } else {
                addToken(TokenType::SLASH);
            }
            break;
            
        // Whitespace
        case ' ':
        case '\r':
        case '\t':
            // Ignore whitespace
            break;
        case '\n':
            line++;
            break;
            
        // String literals
        case '"': string(); break;
        
        // Character literals
        case '\'': character(); break;
        
        // Interpolated strings
        case 'i':
            if (peek() == '"') {
                advance(); // Consume the "
                interpolatedString();
            } else {
                // Treat as identifier
                identifier();
            }
            break;
            
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                std::stringstream ss;
                ss << "Unexpected character: " << c;
                error(ss.str());
            }
            break;
    }
}

bool Lexer::isAtEnd() const {
    return current >= static_cast<int>(source.length());
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= static_cast<int>(source.length())) return '\0';
    return source[current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source[current] != expected) {
        return false;
    }
    
    current++;
    return true;
}

void Lexer::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, line);
}

void Lexer::string() {
    std::string value;
    
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
        } else if (peek() == '\\') {
            advance(); // Consume the backslash
            value += processEscapeSequence();
            continue;
        }
        
        value += advance();
    }
    
    if (isAtEnd()) {
        error("Unterminated string");
        return;
    }
    
    // The closing ".
    advance();
    
    // Add the token with the string value
    tokens.emplace_back(TokenType::STRING, "\"" + value + "\"", line);
}

void Lexer::interpolatedString() {
    std::string value;
    
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
        } else if (peek() == '\\') {
            advance(); // Consume the backslash
            value += processEscapeSequence();
            continue;
        }
        
        value += advance();
    }
    
    if (isAtEnd()) {
        error("Unterminated interpolated string");
        return;
    }
    
    // The closing ".
    advance();
    
    // Add the token for the interpolated string
    tokens.emplace_back(TokenType::INTERPOLATED_STRING_START, "i\"" + value + "\"", line);
}

void Lexer::character() {
    char value;
    
    if (peek() == '\\') {
        advance(); // Consume the backslash
        value = processEscapeSequence();
    } else if (peek() == '\'' || peek() == '\n') {
        error("Empty character literal");
        return;
    } else {
        value = advance();
    }
    
    if (peek() != '\'') {
        error("Character literal must contain exactly one character");
        // Try to recover
        while (peek() != '\'' && !isAtEnd() && peek() != '\n') {
            advance();
        }
    }
    
    if (isAtEnd() || peek() == '\n') {
        error("Unterminated character literal");
        return;
    }
    
    // The closing '.
    advance();
    
    // Add the token with the character value
    std::string charStr = "'";
    charStr += value;
    charStr += "'";
    tokens.emplace_back(TokenType::CHAR, charStr, line);
}

void Lexer::number() {
    // Check for hex, octal, or binary literals
    if (peek() == '0' && (peekNext() == 'x' || peekNext() == 'X' || 
                           peekNext() == 'b' || peekNext() == 'B' ||
                           isDigit(peekNext()))) {
        advance(); // Consume the '0'
        processSpecialNumber(advance()); // Consume and process the prefix
        return;
    }
    
    // Regular decimal number
    while (isDigit(peek())) {
        advance();
    }
    
    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the "."
        advance();
        
        while (isDigit(peek())) {
            advance();
        }
        
        // Look for an exponent part
        if (peek() == 'e' || peek() == 'E') {
            advance();
            
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            
            if (!isDigit(peek())) {
                error("Invalid exponent in float literal");
                return;
            }
            
            while (isDigit(peek())) {
                advance();
            }
        }
        
        // Look for a type suffix (f, F, l, L)
        if (peek() == 'f' || peek() == 'F' || peek() == 'l' || peek() == 'L') {
            advance();
        }
        
        // Float literal
        std::string text = source.substr(start, current - start);
        tokens.emplace_back(TokenType::FLOAT, text, line);
    } else {
        // Look for an integer suffix (u, U, l, L)
        if (peek() == 'u' || peek() == 'U') {
            advance();
            if (peek() == 'l' || peek() == 'L') {
                advance();
                if (peek() == 'l' || peek() == 'L') {
                    advance();
                }
            }
        } else if (peek() == 'l' || peek() == 'L') {
            advance();
            if (peek() == 'l' || peek() == 'L') {
                advance();
            }
            if (peek() == 'u' || peek() == 'U') {
                advance();
            }
        }
        
        // Integer literal
        std::string text = source.substr(start, current - start);
        tokens.emplace_back(TokenType::INTEGER, text, line);
    }
}

void Lexer::identifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    // See if the identifier is a reserved word
    std::string text = source.substr(start, current - start);
    
    auto it = keywords.find(text);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    
    addToken(type);
}

char Lexer::processEscapeSequence() {
    switch (peek()) {
        case 'n': advance(); return '\n';
        case 'r': advance(); return '\r';
        case 't': advance(); return '\t';
        case '\\': advance(); return '\\';
        case '\'': advance(); return '\'';
        case '"': advance(); return '"';
        case '0': advance(); return '\0';
        case 'x': {
            // Hexadecimal escape sequence
            advance(); // Consume 'x'
            
            if (!isxdigit(peek()) || !isxdigit(peekNext())) {
                error("Invalid hexadecimal escape sequence");
                return '?';
            }
            
            std::string hex;
            hex += advance();
            hex += advance();
            
            return static_cast<char>(std::stoi(hex, nullptr, 16));
        }
        default:
            error("Invalid escape sequence");
            return '?';
    }
}

void Lexer::processSpecialNumber(char prefix) {
    switch (prefix) {
        case 'x':
        case 'X': {
            // Hexadecimal
            if (!isxdigit(peek())) {
                error("Invalid hexadecimal literal");
                return;
            }
            
            while (isxdigit(peek())) {
                advance();
            }
            
            // Look for integer suffix
            if (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L') {
                if (peek() == 'u' || peek() == 'U') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                        if (peek() == 'l' || peek() == 'L') {
                            advance();
                        }
                    }
                } else if (peek() == 'l' || peek() == 'L') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                    }
                    if (peek() == 'u' || peek() == 'U') {
                        advance();
                    }
                }
            }
            
            std::string text = source.substr(start, current - start);
            tokens.emplace_back(TokenType::INTEGER, text, line);
            break;
        }
        case 'b':
        case 'B': {
            // Binary
            if (peek() != '0' && peek() != '1') {
                error("Invalid binary literal");
                return;
            }
            
            while (peek() == '0' || peek() == '1') {
                advance();
            }
            
            // Look for integer suffix
            if (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L') {
                if (peek() == 'u' || peek() == 'U') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                        if (peek() == 'l' || peek() == 'L') {
                            advance();
                        }
                    }
                } else if (peek() == 'l' || peek() == 'L') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                    }
                    if (peek() == 'u' || peek() == 'U') {
                        advance();
                    }
                }
            }
            
            std::string text = source.substr(start, current - start);
            tokens.emplace_back(TokenType::INTEGER, text, line);
            break;
        }
        default: {
            // Octal
            while (isDigit(peek()) && peek() < '8') {
                advance();
            }
            
            // Look for integer suffix
            if (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L') {
                if (peek() == 'u' || peek() == 'U') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                        if (peek() == 'l' || peek() == 'L') {
                            advance();
                        }
                    }
                } else if (peek() == 'l' || peek() == 'L') {
                    advance();
                    if (peek() == 'l' || peek() == 'L') {
                        advance();
                    }
                    if (peek() == 'u' || peek() == 'U') {
                        advance();
                    }
                }
            }
            
            std::string text = source.substr(start, current - start);
            tokens.emplace_back(TokenType::INTEGER, text, line);
            break;
        }
    }
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

void Lexer::error(const std::string& message) {
    throw LexerError(message, line);
}

} // namespace flux
