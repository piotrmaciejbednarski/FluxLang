/**
 * @file token.hpp
 * @brief Token definitions for Flux lexer
 */

#pragma once

#include <string>
#include <variant>

namespace flux {

/**
 * @brief Token types in Flux language
 */
enum class TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET, COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR, COLON, AT, TILDE,
    
    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    ARROW, // ->
    
    // Logical operators
    AND, OR, 
    
    // Bitwise operators
    BITAND, BITOR, XOR, SHIFT_LEFT, SHIFT_RIGHT,
    
    // Assignment operators
    PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, 
    PERCENT_EQUAL, BITAND_EQUAL, BITOR_EQUAL, XOR_EQUAL,
    
    // Literals
    IDENTIFIER, STRING, INTEGER, FLOAT, CHAR,
    
    // Keywords
    OBJECT, WHEN, ASM, ASYNC, AWAIT, BREAK, CASE, CATCH,
    CLASS, CONST, CONTINUE, DEFAULT, DELETE, DO, ELSE, ENUM,
    FALSE, FOR, FUNCTION, GOTO, IF, IMPORT, IN, IS, LOCK,
    DUNDER_LOCK, LOCK_DUNDER, // __lock and lock__
    NAMESPACE, NEW, NOT, NULL_LITERAL, OPERATOR, PRINT, INPUT,
    RETURN, SIZEOF, STRUCT, SWITCH, THIS, THROW, TRUE, TRY,
    TYPEDEF, UNION, USING, VOLATILE, WHILE,
    
    // Types
    INT, FLOAT_TYPE, CHAR_TYPE, BOOL, VOID,
    
    // Special tokens
    SCOPE_RESOLUTION, // ::
    INTERPOLATED_STRING_START, // i"
    PERCENT, // %
    
    END_OF_FILE
};

/**
 * @brief Token class representing lexical tokens
 */
class Token {
public:
    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}
    
    std::string toString() const {
        std::string typeStr;
        // Convert TokenType to string (implementation omitted for brevity)
        return typeStr + " " + lexeme;
    }
    
    TokenType type;
    std::string lexeme;
    int line;
};

} // namespace flux
