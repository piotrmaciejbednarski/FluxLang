#include "include/lexer.h"
#include <cctype>
#include <iostream>

namespace flux {

// Initialize the static keywords map
// Ensure the keywords map in lexer.cpp properly maps these tokens
const std::unordered_map<std::string_view, TokenType> Lexer::keywords = {
    {"object", TokenType::OBJECT},
    {"asm", TokenType::ASM},
    {"and", TokenType::AND},
    {"assert", TokenType::ASSERT},
    {"break", TokenType::BREAK},
    {"case", TokenType::CASE},
    {"catch", TokenType::CATCH},
    {"string", TokenType::STRING},
    {"class", TokenType::CLASS},
    {"const", TokenType::CONST},
    {"continue", TokenType::CONTINUE},
    {"default", TokenType::DEFAULT},
    {"delete", TokenType::DELETE},
    {"do", TokenType::DO},
    {"else", TokenType::ELSE},
    {"enum", TokenType::ENUM},
    {"false", TokenType::FALSE},
    {"float", TokenType::FLOAT},
    {"for", TokenType::FOR},
    {"function", TokenType::FUNCTION},
    {"if", TokenType::IF},
    {"import", TokenType::IMPORT},
    {"int", TokenType::INT},
    {"is", TokenType::IS},
    {"lambda", TokenType::LAMBDA},
    {"memalloc", TokenType::MEMALLOC},
    {"namespace", TokenType::NAMESPACE},
    {"new", TokenType::NEW},
    {"not", TokenType::NOT},
    {"nullptr", TokenType::NULLPTR},
    {"operator", TokenType::OPERATOR},
    {"or", TokenType::OR},
    {"print", TokenType::PRINT},
    {"require", TokenType::REQUIRE},
    {"return", TokenType::RETURN},
    {"signed", TokenType::SIGNED},
    {"sizeof", TokenType::SIZEOF},
    {"struct", TokenType::STRUCT},
    {"super", TokenType::SUPER},
    {"switch", TokenType::SWITCH},
    {"this", TokenType::THIS},
    {"throw", TokenType::THROW},
    {"true", TokenType::TRUE},
    {"try", TokenType::TRY},
    {"typedef", TokenType::TYPEDEF},
    {"union", TokenType::UNION},
    {"unsigned", TokenType::UNSIGNED},
    {"using", TokenType::USING},
    {"void", TokenType::VOID},
    {"while", TokenType::WHILE},
    {"xor", TokenType::XOR},
    {"bool", TokenType::BOOL}  // Added bool
};

Lexer::Lexer(std::string_view source) 
    : source(source), line(1), column(1) {
    current = this->source.begin();
    start = current;
}

std::vector<Token> Lexer::scanTokens() {
    tokens.clear();
    
    while (!isAtEnd()) {
        // Beginning of the next token
        start = current;
        scanToken();
    }
    
    // Add EOF token
    tokens.emplace_back(TokenType::END_OF_FILE, "", line, column);
    return tokens;
}

void Lexer::reset(std::string_view newSource) {
    source = newSource;
    current = source.begin();
    start = current;
    line = 1;
    column = 1;
    tokens.clear();
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    
    char c = *current;
    ++current;
    ++column;
    
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (*current != expected) return false;
    
    ++current;
    ++column;
    return true;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return *current;
}

char Lexer::peekNext() const {
    if (current + 1 >= source.end()) return '\0';
    return *(current + 1);
}

bool Lexer::isAtEnd() const {
    return current >= source.end();
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                line++;
                column = 1;
                advance();
                break;
            default:
                return;
        }
    }
}

void Lexer::scanToken() {
    skipWhitespace();
    
    if (isAtEnd()) return;
    
    char c = advance();
    std::cout << "Processing character: '" << c << "' (ASCII: " << (int)c << ")" << std::endl;
    
    // Check for identifiers
    if (std::isalpha(c) || c == '_') {
        current--; // Go back one character
        column--;
        scanIdentifier();
        return;
    }
    
    // Check for numbers
    if (std::isdigit(c)) {
        current--; // Go back one character
        column--;
        scanNumber();
        return;
    }
    
    // Handle all other tokens
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
        case ';': addToken(TokenType::SEMICOLON); break;
        case '?': addToken(TokenType::QUESTION); break;
        case '@': addToken(TokenType::ADDRESS_OF); break;
        case '~': addToken(TokenType::BIT_NOT); break;
        
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
                addToken(TokenType::LEFT_SHIFT);
            } else {
                addToken(TokenType::LESS);
            }
            break;
        case '>': 
            if (match('=')) {
                addToken(TokenType::GREATER_EQUAL);
            } else if (match('>')) {
                addToken(TokenType::RIGHT_SHIFT);
            } else {
                addToken(TokenType::GREATER);
            }
            break;
        case '+': 
            if (match('=')) {
                addToken(TokenType::PLUS_EQUAL);
            } else if (match('+')) {
                addToken(TokenType::INCREMENT);
            } else {
                addToken(TokenType::PLUS);
            }
            break;
        case '-': 
            if (match('=')) {
                addToken(TokenType::MINUS_EQUAL);
            } else if (match('-')) {
                addToken(TokenType::DECREMENT);
            } else if (match('>')) {
                addToken(TokenType::ARROW);
            } else {
                addToken(TokenType::MINUS);
            }
            break;
        case '*': 
            if (match('=')) {
                addToken(TokenType::STAR_EQUAL);
            } else if (match('*')) {
                addToken(TokenType::EXPONENT);
            } else {
                addToken(TokenType::STAR);
            }
            break;
        case '/': 
            if (match('/')) {
                // Comment until end of line
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                // Multi-line comment
                while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
                    if (peek() == '\n') {
                        line++;
                        column = 1;
                    }
                    advance();
                }
                
                if (isAtEnd()) {
                    addError("Unterminated comment.");
                    return;
                }
                
                // Consume the closing */
                advance(); // *
                advance(); // /
            } else if (match('=')) {
                addToken(TokenType::SLASH_EQUAL);
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case '%': 
            addToken(match('=') ? TokenType::MODULO_EQUAL : TokenType::MODULO); 
            break;
        case '&': 
            if (match('=')) {
                addToken(TokenType::AND_EQUAL);
            } else if (match('&')) {
                addToken(TokenType::LOGICAL_AND);
            } else {
                addToken(TokenType::BIT_AND);
            }
            break;
        case '|': 
            if (match('=')) {
                addToken(TokenType::OR_EQUAL);
            } else if (match('|')) {
                addToken(TokenType::LOGICAL_OR);
            } else {
                addToken(TokenType::BIT_OR);
            }
            break;
        case '^': 
            addToken(match('=') ? TokenType::XOR_EQUAL : TokenType::BIT_XOR); 
            break;
        case ':':
            addToken(match(':') ? TokenType::DOUBLE_COLON : TokenType::COLON);
            break;
            
        // String literals
        case '"': 
            scanString(); 
            break;
        case '\'': 
            scanCharacter(); 
            break;
        case 'i':
            if (peek() == '"') {
                advance(); // Consume the "
                addToken(TokenType::INTERP_START);
                scanStringInterpolation();
            } else {
                current--; // Go back to the 'i'
                column--;
                scanIdentifier();
            }
            break;
            
        default:
            // Unexpected character
            addError("Unexpected character.");
            break;
    }
}

void Lexer::scanIdentifier() {
    // Skip any leading whitespace before recording the start
    while (std::isspace(peek())) {
        advance();
        start = current;
    }
    
    while (std::isalnum(peek()) || peek() == '_') advance();
    
    // Get the identifier text directly from source
    std::string_view rawText(source.data() + (start - source.begin()), current - start);
    
    // Create a clean version for keyword lookup
    std::string cleanText;
    for (char c : rawText) {
        if (!std::isspace(c)) {
            cleanText += c;
        }
    }
    
    // Check if the identifier is a keyword
    auto it = keywords.find(cleanText);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    
    // Add the token with the original text
    tokens.emplace_back(type, rawText, line, column - (current - start));
}
void Lexer::scanNumber() {
    bool isFloat = false;
    
    // Integer part
    while (std::isdigit(peek())) advance();
    
    // Fractional part
    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); // Consume the '.'
        
        while (std::isdigit(peek())) advance();
    }
    
    // Exponent part
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance(); // Consume the 'e' or 'E'
        
        if (peek() == '+' || peek() == '-') advance();
        
        if (!std::isdigit(peek())) {
            addError("Invalid number format: expected digit after exponent.");
            return;
        }
        
        while (std::isdigit(peek())) advance();
    }
    
    // Check for bit width specification like int{32}
    if (peek() == '{') {
        advance(); // Consume the '{'
        
        while (std::isdigit(peek())) advance();
        
        if (peek() != '}') {
            addError("Invalid bit width specification: expected '}'.");
            return;
        }
        
        advance(); // Consume the '}'
    }
    
    std::string_view text = std::string_view(source.data() + (start - source.begin()), 
                                            current - start);
    
    Token token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, 
                text, line, column - (current - start));
    
    // Convert the literal value
    try {
        if (isFloat) {
            token.floatValue = std::stod(std::string(text));
        } else {
            token.intValue = std::stoll(std::string(text));
        }
    } catch (const std::exception& e) {
        addError("Number conversion error: " + std::string(e.what()));
        return;
    }
    
    tokens.push_back(token);
}

void Lexer::scanString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 1;
        }
        
        // Handle escape sequences
        if (peek() == '\\' && !isAtEnd()) {
            advance(); // Consume the backslash
            advance(); // Consume the escaped character
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        addError("Unterminated string.");
        return;
    }
    
    advance(); // The closing "
    
    // Extract the string value (excluding the quotes)
    std::string_view text = std::string_view(source.data() + (start - source.begin()) + 1, 
                                           (current - start) - 2);
    
    tokens.emplace_back(TokenType::STRING_LITERAL, text, line, 
                      column - (current - start));
}

void Lexer::scanCharacter() {
    // A character literal contains exactly one character, possibly escaped
    if (peek() == '\\') {
        advance(); // Consume the backslash
        advance(); // Consume the escaped character
    } else if (peek() != '\'') {
        advance(); // Consume the character
    } else {
        addError("Empty character literal.");
        return;
    }
    
    if (peek() != '\'') {
        addError("Character literal must contain exactly one character.");
        while (peek() != '\'' && !isAtEnd()) advance();
    }
    
    if (isAtEnd()) {
        addError("Unterminated character literal.");
        return;
    }
    
    advance(); // The closing '
    
    // Extract the character value (excluding the quotes)
    std::string_view text = std::string_view(source.data() + (start - source.begin()) + 1, 
                                           (current - start) - 2);
    
    tokens.emplace_back(TokenType::CHAR_LITERAL, text, line, 
                      column - (current - start));
}

void Lexer::scanStringInterpolation() {
    // Parse until we find the end of the interpolation string
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 1;
        }
        
        // Handle escape sequences
        if (peek() == '\\' && !isAtEnd()) {
            advance(); // Consume the backslash
            advance(); // Consume the escaped character
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        addError("Unterminated string interpolation.");
        return;
    }
    
    advance(); // The closing "
    
    // Now we should find the :{
    if (peek() == ':' && peekNext() == '{') {
        advance(); // Consume :
        advance(); // Consume {
        addToken(TokenType::INTERP_END);
        
        // Now the interpolation expression until we find a closing ;}
        int braceCount = 1; // We've already consumed one {
        
        while (braceCount > 0 && !isAtEnd()) {
            if (peek() == '{') {
                braceCount++;
            } else if (peek() == '}') {
                braceCount--;
                if (braceCount == 0) {
                    if (peekNext() == ';') {
                        advance(); // Consume }
                        advance(); // Consume ;
                        addToken(TokenType::INTERP_CLOSE);
                        return;
                    }
                }
            } else if (peek() == '\n') {
                line++;
                column = 1;
            }
            
            advance();
        }
        
        addError("Unterminated string interpolation expression.");
    } else {
        addError("Expected ':' followed by '{' after interpolation string.");
    }
}

void Lexer::addToken(TokenType type) {
    std::string_view text = std::string_view(source.data() + (start - source.begin()), 
                                           current - start);
    
    // Trim leading and trailing whitespace
    while (!text.empty() && std::isspace(text.front())) {
        text = text.substr(1);
    }
    while (!text.empty() && std::isspace(text.back())) {
        text = text.substr(0, text.length() - 1);
    }
    
    tokens.emplace_back(type, text, line, column - (current - start));
}

void Lexer::addError(const std::string& message) {
    // Create an error token
    tokens.emplace_back(TokenType::ERROR, message, line, column);
    
    // Report to error system
    errorReporter.reportError(ErrorType::LEXICAL_ERROR, message, 
                             SourceLocation("<source>", line, column));
}

} // namespace flux
