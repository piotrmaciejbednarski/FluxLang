#include "include/lexer.h"
#include <cctype>
#include <algorithm>
#include <limits>
#include <iostream>
#include <iomanip>

namespace flux {

// Initialize the keyword map
const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"object", TokenType::KW_OBJECT},
    {"asm", TokenType::KW_ASM},
    {"and", TokenType::KW_AND},
    {"assert", TokenType::KW_ASSERT},
    {"break", TokenType::KW_BREAK},
    {"case", TokenType::KW_CASE},
    {"catch", TokenType::KW_CATCH},
    {"char", TokenType::KW_CHAR},
    {"class", TokenType::KW_CLASS},
    {"const", TokenType::KW_CONST},
    {"continue", TokenType::KW_CONTINUE},
    {"default", TokenType::KW_DEFAULT},
    {"delete", TokenType::KW_DELETE},
    {"do", TokenType::KW_DO},
    {"else", TokenType::KW_ELSE},
    {"enum", TokenType::KW_ENUM},
    {"false", TokenType::KW_FALSE},
    {"float", TokenType::KW_FLOAT},
    {"for", TokenType::KW_FOR},
    {"if", TokenType::KW_IF},
    {"import", TokenType::KW_IMPORT},
    {"int", TokenType::KW_INT},
    {"is", TokenType::KW_IS},
    {"lambda", TokenType::KW_LAMBDA},
    {"memalloc", TokenType::KW_MEMALLOC},
    {"namespace", TokenType::KW_NAMESPACE},
    {"new", TokenType::KW_NEW},
    {"not", TokenType::KW_NOT},
    {"nullptr", TokenType::NULL_LITERAL},
    {"operator", TokenType::KW_OPERATOR},
    {"or", TokenType::KW_OR},
    {"require", TokenType::KW_REQUIRE},
    {"return", TokenType::KW_RETURN},
    {"signed", TokenType::KW_SIGNED},
    {"sizeof", TokenType::KW_SIZEOF},
    {"struct", TokenType::KW_STRUCT},
    {"super", TokenType::KW_SUPER},
    {"switch", TokenType::KW_SWITCH},
    {"this", TokenType::KW_THIS},
    {"throw", TokenType::KW_THROW},
    {"true", TokenType::KW_TRUE},
    {"try", TokenType::KW_TRY},
    {"typedef", TokenType::KW_TYPEDEF},
    {"union", TokenType::KW_UNION},
    {"unsigned", TokenType::KW_UNSIGNED},
    {"using", TokenType::KW_USING},
    {"void", TokenType::KW_VOID},
    {"while", TokenType::KW_WHILE},
    {"xor", TokenType::KW_XOR}
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        
        // Literals
        case TokenType::INTEGER_LITERAL: return "INTEGER";
        case TokenType::FLOAT_LITERAL: return "FLOAT";
        case TokenType::CHAR_LITERAL: return "CHAR";
        case TokenType::STRING_LITERAL: return "STRING";
        case TokenType::BOOL_LITERAL: return "BOOL";
        case TokenType::NULL_LITERAL: return "NULL";
        
        // Identifiers
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        
        // Keywords
        case TokenType::KW_OBJECT: return "KW_OBJECT";
        case TokenType::KW_ASM: return "KW_ASM";
        case TokenType::KW_AND: return "KW_AND";
        case TokenType::KW_ASSERT: return "KW_ASSERT";
        case TokenType::KW_BREAK: return "KW_BREAK";
        case TokenType::KW_CASE: return "KW_CASE";
        case TokenType::KW_CATCH: return "KW_CATCH";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_CLASS: return "KW_CLASS";
        case TokenType::KW_CONST: return "KW_CONST";
        case TokenType::KW_CONTINUE: return "KW_CONTINUE";
        case TokenType::KW_DEFAULT: return "KW_DEFAULT";
        case TokenType::KW_DELETE: return "KW_DELETE";
        case TokenType::KW_DO: return "KW_DO";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_ENUM: return "KW_ENUM";
        case TokenType::KW_FALSE: return "KW_FALSE";
        case TokenType::KW_FLOAT: return "KW_FLOAT";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_IMPORT: return "KW_IMPORT";
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_IS: return "KW_IS";
        case TokenType::KW_LAMBDA: return "KW_LAMBDA";
        case TokenType::KW_MEMALLOC: return "KW_MEMALLOC";
        case TokenType::KW_NAMESPACE: return "KW_NAMESPACE";
        case TokenType::KW_NEW: return "KW_NEW";
        case TokenType::KW_NOT: return "KW_NOT";
        case TokenType::KW_OPERATOR: return "KW_OPERATOR";
        case TokenType::KW_OR: return "KW_OR";
        case TokenType::KW_REQUIRE: return "KW_REQUIRE";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_SIGNED: return "KW_SIGNED";
        case TokenType::KW_SIZEOF: return "KW_SIZEOF";
        case TokenType::KW_STRUCT: return "KW_STRUCT";
        case TokenType::KW_SUPER: return "KW_SUPER";
        case TokenType::KW_SWITCH: return "KW_SWITCH";
        case TokenType::KW_THIS: return "KW_THIS";
        case TokenType::KW_THROW: return "KW_THROW";
        case TokenType::KW_TRUE: return "KW_TRUE";
        case TokenType::KW_TRY: return "KW_TRY";
        case TokenType::KW_TYPEDEF: return "KW_TYPEDEF";
        case TokenType::KW_UNION: return "KW_UNION";
        case TokenType::KW_UNSIGNED: return "KW_UNSIGNED";
        case TokenType::KW_USING: return "KW_USING";
        case TokenType::KW_VOID: return "KW_VOID";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_XOR: return "KW_XOR";
        
        // Operators and punctuation
        case TokenType::OP_PLUS: return "OP_PLUS";
        case TokenType::OP_MINUS: return "OP_MINUS";
        case TokenType::OP_STAR: return "OP_STAR";
        case TokenType::OP_SLASH: return "OP_SLASH";
        case TokenType::OP_PERCENT: return "OP_PERCENT";
        case TokenType::OP_CARET: return "OP_CARET";
        case TokenType::OP_AMPERSAND: return "OP_AMPERSAND";
        case TokenType::OP_PIPE: return "OP_PIPE";
        case TokenType::OP_TILDE: return "OP_TILDE";
        case TokenType::OP_EXCLAIM: return "OP_EXCLAIM";
        case TokenType::OP_EQUAL: return "OP_EQUAL";
        case TokenType::OP_LESS: return "OP_LESS";
        case TokenType::OP_GREATER: return "OP_GREATER";
        case TokenType::OP_DOT: return "OP_DOT";
        case TokenType::OP_ARROW: return "OP_ARROW";
        case TokenType::OP_PLUS_PLUS: return "OP_PLUS_PLUS";
        case TokenType::OP_MINUS_MINUS: return "OP_MINUS_MINUS";
        case TokenType::OP_PLUS_EQUAL: return "OP_PLUS_EQUAL";
        case TokenType::OP_MINUS_EQUAL: return "OP_MINUS_EQUAL";
        case TokenType::OP_STAR_EQUAL: return "OP_STAR_EQUAL";
        case TokenType::OP_SLASH_EQUAL: return "OP_SLASH_EQUAL";
        case TokenType::OP_PERCENT_EQUAL: return "OP_PERCENT_EQUAL";
        case TokenType::OP_AMPERSAND_EQUAL: return "OP_AMPERSAND_EQUAL";
        case TokenType::OP_PIPE_EQUAL: return "OP_PIPE_EQUAL";
        case TokenType::OP_CARET_EQUAL: return "OP_CARET_EQUAL";
        case TokenType::OP_LESS_LESS: return "OP_LESS_LESS";
        case TokenType::OP_GREATER_GREATER: return "OP_GREATER_GREATER";
        case TokenType::OP_LESS_LESS_EQUAL: return "OP_LESS_LESS_EQUAL";
        case TokenType::OP_GREATER_GREATER_EQUAL: return "OP_GREATER_GREATER_EQUAL";
        case TokenType::OP_EQUAL_EQUAL: return "OP_EQUAL_EQUAL";
        case TokenType::OP_EXCLAIM_EQUAL: return "OP_EXCLAIM_EQUAL";
        case TokenType::OP_LESS_EQUAL: return "OP_LESS_EQUAL";
        case TokenType::OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
        case TokenType::OP_AMPERSAND_AMPERSAND: return "OP_AMPERSAND_AMPERSAND";
        case TokenType::OP_PIPE_PIPE: return "OP_PIPE_PIPE";
        case TokenType::OP_DOUBLE_COLON: return "OP_DOUBLE_COLON";
        case TokenType::OP_AT: return "OP_AT";
        case TokenType::OP_DOUBLE_STAR: return "OP_DOUBLE_STAR";
        case TokenType::OP_QUESTION: return "OP_QUESTION";
	case TokenType::OP_QUESTION_QUESTION: return "OP_QUESTION_QUESTION";
	case TokenType::OP_QUESTION_DOT: return "OP_QUESTION_DOT";
        
        // Delimiters
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::COMMA: return "COMMA";
        
        // Special tokens
        case TokenType::INJECTABLE_STRING: return "INJECTABLE_STRING";
        case TokenType::BIT_WIDTH_SPECIFIER: return "BIT_WIDTH_SPECIFIER";
        
        default: return "UNKNOWN";
    }
}

std::string Token::getValueString() const {
    if (type == TokenType::INTEGER_LITERAL) {
        if (std::holds_alternative<int64_t>(value.value)) {
            return std::to_string(std::get<int64_t>(value.value));
        }
    } else if (type == TokenType::FLOAT_LITERAL) {
        if (std::holds_alternative<double>(value.value)) {
            return std::to_string(std::get<double>(value.value));
        }
    } else if (type == TokenType::BOOL_LITERAL) {
        if (std::holds_alternative<bool>(value.value)) {
            return std::get<bool>(value.value) ? "true" : "false";
        }
    } else if (type == TokenType::CHAR_LITERAL) {
        if (std::holds_alternative<char>(value.value)) {
            char c = std::get<char>(value.value);
            if (c == '\n') return "\\n";
            if (c == '\t') return "\\t";
            if (c == '\r') return "\\r";
            return std::string(1, c);
        }
    } else if (type == TokenType::STRING_LITERAL) {
        if (std::holds_alternative<std::string>(value.value)) {
            return std::get<std::string>(value.value);
        }
    } else if (type == TokenType::IDENTIFIER) {
        if (std::holds_alternative<std::string>(value.value)) {
            return std::get<std::string>(value.value);
        }
    } else if (type == TokenType::ERROR) {
        if (std::holds_alternative<std::string>(value.value)) {
            return std::get<std::string>(value.value);
        }
    }
    
    return "";
}

std::string Token::toString() const {
    std::string result = "Line " + std::to_string(line) + ", Column " + std::to_string(column)
                        + ": " + tokenTypeToString(type) + " '" + std::string(lexeme) + "'";
    
    std::string valueStr = getValueString();
    if (!valueStr.empty()) {
        result += " Value: " + valueStr;
    }
    
    return result;
}

void Lexer::printTokenTableHeader() {
    std::cout << "====================================================" << std::endl;
    std::cout << std::left << std::setw(6) << "Line" << std::setw(8) << "Column"
              << std::setw(25) << "Token Type" << std::setw(25) << "Lexeme" 
              << "Value" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;
}

void Lexer::printTokenTableRow(const Token& token) {
    // Format lexeme for display - escape special characters and limit length
    std::string displayLexeme = std::string(token.lexeme);
    if (displayLexeme.length() > 20) {
        displayLexeme = displayLexeme.substr(0, 17) + "...";
    }
    
    // Print the token information in a nicely formatted table
    std::cout << std::left << std::setw(6) << token.line
              << std::setw(8) << token.column
              << std::setw(25) << tokenTypeToString(token.type)
              << std::setw(25) << displayLexeme;
              
    // Print value if available
    std::string value = token.getValueString();
    if (!value.empty()) {
        std::cout << value;
    }
    
    std::cout << std::endl;
}

void Lexer::printTokenTableSeparator() {
    std::cout << "----------------------------------------------------" << std::endl;
}

// Helper method to check if a byte is a continuation byte in UTF-8
bool Lexer::isUtf8ContinuationByte(char c) const {
    return (c & 0xC0) == 0x80; // Check if the byte has format 10xxxxxx
}

int Lexer::utf8CharSize(char firstByte) const {
    if ((firstByte & 0x80) == 0) return 1;      // 0xxxxxxx (ASCII)
    if ((firstByte & 0xE0) == 0xC0) return 2;   // 110xxxxx (2-byte)
    if ((firstByte & 0xF0) == 0xE0) return 3;   // 1110xxxx (3-byte)
    if ((firstByte & 0xF8) == 0xF0) return 4;   // 11110xxx (4-byte)
    return 1; // Invalid UTF-8, treat as single byte
}

// Helper method to check if a byte is a valid UTF-8 start byte
bool Lexer::isValidUtf8StartByte(char c) const {
    // Valid start bytes are: 0xxxxxxx, 110xxxxx, 1110xxxx, or 11110xxx
    return (c & 0x80) == 0x00 ||  // 0xxxxxxx (ASCII)
           (c & 0xE0) == 0xC0 ||  // 110xxxxx (2-byte sequence)
           (c & 0xF0) == 0xE0 ||  // 1110xxxx (3-byte sequence)
           (c & 0xF8) == 0xF0;    // 11110xxx (4-byte sequence)
}

std::string_view Lexer::getCurrentChar() const {
    if (currentPos >= source.length()) {
        return "";
    }
    
    int charSize = utf8CharSize(source[currentPos]);
    return source.substr(currentPos, charSize);
}

Lexer::Lexer(std::string_view source, std::string_view filename)
    : source(source), filename(filename), currentPos(0), line(1), column(1) {}

Token Lexer::nextToken() {
    if (!tokenBuffer.empty()) {
        Token token = tokenBuffer.front();
        tokenBuffer.erase(tokenBuffer.begin());
        return token;
    }
    
    skipWhitespace();
    
    if (currentPos >= source.length()) {
        return Token(TokenType::END_OF_FILE, "", line, column, 0);
    }
    
    return scanToken();
}

Token Lexer::peekToken() {
    if (tokenBuffer.empty()) {
        tokenBuffer.push_back(nextToken());
    }
    return tokenBuffer.front();
}

char Lexer::advance() {
    if (currentPos >= source.length()) {
        return '\0';
    }
    
    char c = source[currentPos];
    
    // Handle multi-byte UTF-8 characters
    int charSize = utf8CharSize(c);
    currentPos += charSize;
    
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++; // Increment column once per character, not per byte
    }
    
    return c;
}

char Lexer::peek() const {
    if (currentPos >= source.length()) {
        return '\0';
    }
    return source[currentPos];
}

char Lexer::peekNext() const {
    if (currentPos >= source.length()) {
        return '\0';
    }
    
    // Get size of current character
    int charSize = utf8CharSize(source[currentPos]);
    size_t nextPos = currentPos + charSize;
    
    if (nextPos >= source.length()) {
        return '\0';
    }
    
    return source[nextPos];
}

bool Lexer::match(char expected) {
    if (peek() != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                advance();
                break;
            case '/':
                if (peekNext() == '/' || peekNext() == '*') {
                    handleComment();
                } else {
                    return;
                }
                break;
            default:
                if (!std::isprint(c) && !std::isspace(c) && c != '\0') {
                    // Skip invalid UTF-8 or non-printable characters
                    advance();
                } else {
                    return;
                }
        }
    }
}

Token Lexer::handleComment() {
    if (peekNext() == '/') { // Line comment
        // Consume the second '/'
        advance();
        advance();
        
        // Consume until end of line or end of file
        while (peek() != '\n' && peek() != '\0') {
            advance();
        }
    } else if (peekNext() == '*') { // Block comment
        // Consume the opening '/*'
        advance();
        advance();
        
        // Consume until closing '*/'
        while (!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
            advance();
        }
        
        // Consume the closing '*/'
        if (peek() != '\0') {
            advance(); // *
            advance(); // /
        }
    }
    
    skipWhitespace();
    return scanToken();
}

Token Lexer::scanToken() {
    if (currentPos >= source.length()) {
        return Token(TokenType::END_OF_FILE, "", line, column, 0);
    }

    char c = advance();
    size_t startColumn = column - 1;
    size_t startPos = currentPos - 1;
    
    // Skip non-printable or invalid characters
    if (!std::isprint(c) && !std::isspace(c)) {
        return scanToken(); // Skip and try again
    }
    
    switch (c) {
        // Single character tokens
        case '(': return Token(TokenType::LPAREN, source.substr(startPos, 1), line, startColumn, 1);
        case ')': return Token(TokenType::RPAREN, source.substr(startPos, 1), line, startColumn, 1);
        case '{': return Token(TokenType::LBRACE, source.substr(startPos, 1), line, startColumn, 1);
        case '}': return Token(TokenType::RBRACE, source.substr(startPos, 1), line, startColumn, 1);
        case '[': return Token(TokenType::LBRACKET, source.substr(startPos, 1), line, startColumn, 1);
        case ']': return Token(TokenType::RBRACKET, source.substr(startPos, 1), line, startColumn, 1);
        case ';': return Token(TokenType::SEMICOLON, source.substr(startPos, 1), line, startColumn, 1);
        case ',': return Token(TokenType::COMMA, source.substr(startPos, 1), line, startColumn, 1);
        case '@': return Token(TokenType::OP_AT, source.substr(startPos, 1), line, startColumn, 1);
        
        // Potentially multi-character tokens
        case '.':
            if (std::isdigit(peek())) {
                // This is a decimal number starting with a dot
                currentPos--; // Go back to the dot
                column--;
                return handleNumber();
            }
            return Token(TokenType::OP_DOT, source.substr(startPos, 1), line, startColumn, 1);
            
        case ':':
            if (match(':')) {
                return Token(TokenType::OP_DOUBLE_COLON, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::COLON, source.substr(startPos, 1), line, startColumn, 1);
            
        case '+':
            if (match('+')) {
                return Token(TokenType::OP_PLUS_PLUS, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_PLUS_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_PLUS, source.substr(startPos, 1), line, startColumn, 1);
            
        case '-':
            if (match('>')) {
                return Token(TokenType::OP_ARROW, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('-')) {
                return Token(TokenType::OP_MINUS_MINUS, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_MINUS_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_MINUS, source.substr(startPos, 1), line, startColumn, 1);
            
        case '*':
            if (match('*')) {
                return Token(TokenType::OP_DOUBLE_STAR, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_STAR_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_STAR, source.substr(startPos, 1), line, startColumn, 1);
            
        case '/':
            if (match('=')) {
                return Token(TokenType::OP_SLASH_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_SLASH, source.substr(startPos, 1), line, startColumn, 1);
            
        case '%':
            if (match('=')) {
                return Token(TokenType::OP_PERCENT_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_PERCENT, source.substr(startPos, 1), line, startColumn, 1);
            
        case '&':
            if (match('&')) {
                return Token(TokenType::OP_AMPERSAND_AMPERSAND, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_AMPERSAND_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_AMPERSAND, source.substr(startPos, 1), line, startColumn, 1);
            
        case '|':
            if (match('|')) {
                return Token(TokenType::OP_PIPE_PIPE, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_PIPE_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_PIPE, source.substr(startPos, 1), line, startColumn, 1);
            
        case '^':
            if (match('=')) {
                return Token(TokenType::OP_CARET_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_CARET, source.substr(startPos, 1), line, startColumn, 1);
            
        case '!':
            if (match('=')) {
                return Token(TokenType::OP_EXCLAIM_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_EXCLAIM, source.substr(startPos, 1), line, startColumn, 1);
            
        case '=':
            if (match('=')) {
                return Token(TokenType::OP_EQUAL_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_EQUAL, source.substr(startPos, 1), line, startColumn, 1);
            
        case '<':
            if (match('<')) {
                if (match('=')) {
                    return Token(TokenType::OP_LESS_LESS_EQUAL, source.substr(startPos, 3), line, startColumn, 3);
                }
                return Token(TokenType::OP_LESS_LESS, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_LESS_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_LESS, source.substr(startPos, 1), line, startColumn, 1);
            
        case '>':
            if (match('>')) {
                if (match('=')) {
                    return Token(TokenType::OP_GREATER_GREATER_EQUAL, source.substr(startPos, 3), line, startColumn, 3);
                }
                return Token(TokenType::OP_GREATER_GREATER, source.substr(startPos, 2), line, startColumn, 2);
            } else if (match('=')) {
                return Token(TokenType::OP_GREATER_EQUAL, source.substr(startPos, 2), line, startColumn, 2);
            }
            return Token(TokenType::OP_GREATER, source.substr(startPos, 1), line, startColumn, 1);
        case '?':
	    if (match('?')) {
		return Token(TokenType::OP_QUESTION_QUESTION, source.substr(startPos, 2), line, startColumn, 2);
	    } else if (match('.')) {
		return Token(TokenType::OP_QUESTION_DOT, source.substr(startPos, 2), line, startColumn, 2);
	    }
	    return Token(TokenType::OP_QUESTION, source.substr(startPos, 1), line, startColumn, 1);
        case '~':
            return Token(TokenType::OP_TILDE, source.substr(startPos, 1), line, startColumn, 1);
            
        case '"':
            // String literal
            currentPos--;
            column--;
            return handleString();
            
        case '\'':
            // Character literal
            currentPos--;
            column--;
            return handleChar();
            
        default:
            // Handle whitespace explicitly
            if (std::isspace(c)) {
                return scanToken(); // Skip whitespace and get the next token
            }
            // Handle identifiers, numbers, and other tokens
            else if (std::isalpha(c) || c == '_') {
                // For all identifiers (including those starting with 'i')
                currentPos--; // Go back to the first character
                column--;
                
                // Special check for injectable strings (i")
                if (c == 'i' && peek() == '"') {
                    return handleInjectableString();
                }
                
                // Normal identifier handling
                return handleIdentifier();
            } else if (std::isdigit(c)) {
                // Number
                currentPos--;
                column--;
                return handleNumber();
            }
            
            // Unknown character
            return errorToken("Unexpected character");
    }
}

Token Lexer::handleIdentifier() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Check if first character is valid for identifier start
    char c = peek();
    if (!(std::isalpha(c) || c == '_' || (c & 0x80))) {
        return errorToken("Invalid identifier start");
    }
    
    // Consume characters until we hit a non-identifier character
    do {
        advance();
        c = peek();
    } while (currentPos < source.length() && 
             (std::isalnum(c) || c == '_' || (c & 0x80)));
    
    // Get the full identifier
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    std::string lexemeStr(lexeme);
    
    // Check if it's a keyword
    auto it = keywords.find(lexemeStr);
    if (it != keywords.end()) {
        TokenType type = it->second;
        
        // Special handling for boolean literals and null
        if (type == TokenType::KW_TRUE) {
            return Token(TokenType::BOOL_LITERAL, TokenValue(true), lexeme, line, startColumn, lexeme.length());
        } else if (type == TokenType::KW_FALSE) {
            return Token(TokenType::BOOL_LITERAL, TokenValue(false), lexeme, line, startColumn, lexeme.length());
        } else if (type == TokenType::NULL_LITERAL) {
            return Token(TokenType::NULL_LITERAL, TokenValue(nullptr), lexeme, line, startColumn, lexeme.length());
        }
        
        return Token(type, lexeme, line, startColumn, lexeme.length());
    }
    
    // Regular identifier
    return Token(TokenType::IDENTIFIER, TokenValue(lexemeStr), lexeme, line, startColumn, lexeme.length());
}

Token Lexer::handleNumber() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    bool isFloat = false;
    
    // Handle integer and floating-point literals
    while (std::isdigit(peek())) {
        advance();
    }
    
    // Check for decimal point
    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); // Consume '.'
        
        // Consume all digits after decimal point
        while (std::isdigit(peek())) {
            advance();
        }
    }
    
    // Extract the lexeme
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    // Check for bit-width specifier
    if (peek() == '{') {
        return handleBitWidthSpecifier();
    }
    
    // Parse the number
    if (isFloat) {
        double value = std::stod(std::string(lexeme));
        return Token(TokenType::FLOAT_LITERAL, TokenValue(value), lexeme, line, startColumn, lexeme.length());
    } else {
        int64_t value = std::stoll(std::string(lexeme));
        return Token(TokenType::INTEGER_LITERAL, TokenValue(value), lexeme, line, startColumn, lexeme.length());
    }
}

Token Lexer::handleString() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Expect opening quote
    if (peek() != '"') {
        return errorToken("Expected '\"' at start of string");
    }
    advance(); // Consume opening quote
    
    std::string value;
    
    // Consume characters until closing quote
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            // Handle escape sequences
            advance(); // Consume '\'
            
            switch (peek()) {
                case '\"': value += '\"'; break;
                case '\\': value += '\\'; break;
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                // Add more escape sequences as needed
                default:
                    return errorToken("Invalid escape sequence");
            }
        } else {
            value += peek();
        }
        
        advance();
    }
    
    // Check for closing quote
    if (peek() != '"') {
        return errorToken("Unterminated string");
    }
    advance(); // Consume closing quote
    
    // Extract the full lexeme including quotes
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    return Token(TokenType::STRING_LITERAL, TokenValue(value), lexeme, line, startColumn, lexeme.length());
}

Token Lexer::handleChar() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Expect opening quote
    if (peek() != '\'') {
        return errorToken("Expected ''' at start of char literal");
    }
    advance(); // Consume opening quote
    
    char value;
    
    // Handle character value
    if (peek() == '\\') {
        // Escape sequence
        advance(); // Consume '\'
        
        switch (peek()) {
            case '\'': value = '\''; break;
            case '\\': value = '\\'; break;
            case 'n': value = '\n'; break;
            case 't': value = '\t'; break;
            case 'r': value = '\r'; break;
            // Add more escape sequences as needed
            default:
                return errorToken("Invalid escape sequence");
        }
        
        advance(); // Consume the escaped character
    } else if (peek() == '\'' || peek() == '\0') {
        return errorToken("Empty character literal");
    } else {
        value = peek();
        advance(); // Consume the character
    }
    
    // Check for closing quote
    if (peek() != '\'') {
        return errorToken("Unterminated character literal");
    }
    advance(); // Consume closing quote
    
    // Extract the full lexeme including quotes
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    return Token(TokenType::CHAR_LITERAL, TokenValue(value), lexeme, line, startColumn, lexeme.length());
}

Token Lexer::handleInjectableString() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Expect 'i'
    if (peek() != 'i') {
        return errorToken("Expected 'i' at start of injectable string");
    }
    advance(); // Consume 'i'
    
    // Expect opening quote
    if (peek() != '"') {
        return errorToken("Expected '\"' after 'i' in injectable string");
    }
    advance(); // Consume opening quote
    
    std::string format;
    
    // Consume characters until closing quote
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            // Handle escape sequences
            advance(); // Consume '\'
            
            switch (peek()) {
                case '\"': format += '\"'; break;
                case '\\': format += '\\'; break;
                case 'n': format += '\n'; break;
                case 't': format += '\t'; break;
                case 'r': format += '\r'; break;
                // Add more escape sequences as needed
                default:
                    return errorToken("Invalid escape sequence");
            }
        } else {
            format += peek();
        }
        
        advance();
    }
    
    // Check for closing quote
    if (peek() != '"') {
        return errorToken("Unterminated injectable string");
    }
    advance(); // Consume closing quote
    
    // Expect ':'
    if (peek() != ':') {
        return errorToken("Expected ':' after closing quote in injectable string");
    }
    advance(); // Consume ':'
    
    // Expect '{'
    if (peek() != '{') {
        return errorToken("Expected '{' after ':' in injectable string");
    }
    advance(); // Consume '{'
    
    // Skip parsing the argument list for now (would require parsing expressions)
    // In a real implementation, you would tokenize the arguments here
    
    // Consume all characters until closing '}'
    int braceDepth = 1;
    while (braceDepth > 0 && peek() != '\0') {
        if (peek() == '{') {
            braceDepth++;
        } else if (peek() == '}') {
            braceDepth--;
        }
        
        advance();
    }
    
    if (braceDepth > 0) {
        return errorToken("Unterminated injectable string argument list");
    }
    
    // Expect ';'
    if (peek() != ';') {
        return errorToken("Expected ';' after '}' in injectable string");
    }
    advance(); // Consume ';'
    
    // Extract the full lexeme
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    return Token(TokenType::INJECTABLE_STRING, TokenValue(format), lexeme, line, startColumn, lexeme.length());
}

Token Lexer::handleBitWidthSpecifier() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Parse the type part (int, float, etc.)
    while (std::isalpha(peek())) {
        advance();
    }
    
    // We should be at the opening '{' now
    if (peek() != '{') {
        return errorToken("Expected '{' in bit width specifier");
    }
    advance(); // Consume '{'
    
    // Parse the bit width
    size_t bitWidth = 0;
    while (std::isdigit(peek())) {
        bitWidth = bitWidth * 10 + (peek() - '0');
        advance();
    }
    
    // Check for closing '}'
    if (peek() != '}') {
        return errorToken("Expected '}' in bit width specifier");
    }
    advance(); // Consume '}'
    
    // Extract the full lexeme
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    // Create a token with the bit width information
    TokenValue value;
    value.bitWidth = bitWidth;
    
    return Token(TokenType::BIT_WIDTH_SPECIFIER, value, lexeme, line, startColumn, lexeme.length());
}

Token Lexer::handleCustomOperator() {
    size_t startPos = currentPos;
    size_t startColumn = column;
    
    // Consume all characters that could be part of a custom operator
    while (!std::isalnum(peek()) && !std::isspace(peek()) && peek() != '\0' &&
           peek() != '(' && peek() != ')' && peek() != '{' && peek() != '}' &&
           peek() != '[' && peek() != ']' && peek() != ';' && peek() != ',') {
        advance();
    }
    
    // Extract the lexeme
    std::string_view lexeme = source.substr(startPos, currentPos - startPos);
    
    // For now, treat custom operators as identifiers
    // The parser will need to check if these are registered operators
    return Token(TokenType::IDENTIFIER, TokenValue(std::string(lexeme)), lexeme, line, startColumn, lexeme.length());
}

Token Lexer::errorToken(const std::string& message) {
    return Token(TokenType::ERROR, TokenValue(message), "", line, column, 0);
}

} // namespace flux
