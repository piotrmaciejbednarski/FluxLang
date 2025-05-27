#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include "../common/source.h"

namespace flux {
namespace lexer {

// Token types for the Flux language
enum class TokenType {
    // End of file
    END_OF_FILE,
    
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHARACTER_LITERAL,
    DATA_LITERAL,
    I_STRING_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords
    KEYWORD_AUTO,
    KEYWORD_AS,
    KEYWORD_ASM,
    KEYWORD_ASSERT,
    KEYWORD_AND,
    KEYWORD_BREAK,
    KEYWORD_CASE,
    KEYWORD_CATCH,
    KEYWORD_CONST,
    KEYWORD_CONTINUE,
    KEYWORD_DATA,
    KEYWORD_DEF,
    KEYWORD_DEFAULT,
    KEYWORD_DO,
    KEYWORD_ELSE,
    KEYWORD_ENUM,
    KEYWORD_FOR,
    KEYWORD_IF,
    KEYWORD_IMPORT,
    KEYWORD_IN,
    KEYWORD_IS,
    KEYWORD_NAMESPACE,
    KEYWORD_NOT,
    KEYWORD_OBJECT,
    KEYWORD_OPERATOR,
    KEYWORD_OR,
    KEYWORD_RETURN,
    KEYWORD_SIGNED,
    KEYWORD_SIZEOF,
    KEYWORD_STRUCT,
    KEYWORD_SUPER,
    KEYWORD_SWITCH,
    KEYWORD_TEMPLATE,
    KEYWORD_THIS,
    KEYWORD_THROW,
    KEYWORD_TRY,
    KEYWORD_TYPEOF,
    KEYWORD_UNSIGNED,
    KEYWORD_USING,
    KEYWORD_VOID,
    KEYWORD_VOLATILE,
    KEYWORD_WHILE,
    KEYWORD_XOR,
    
    // Operators
    // Assignment operators
    ASSIGN,                 // =
    ASSIGN_ADD,             // +=
    ASSIGN_SUB,             // -=
    ASSIGN_MUL,             // *=
    ASSIGN_DIV,             // /=
    ASSIGN_MOD,             // %=
    ASSIGN_AND,             // &=
    ASSIGN_OR,              // |=
    ASSIGN_XOR,             // ^^=
    ASSIGN_SHL,             // <<=
    ASSIGN_SHR,             // >>=
    ASSIGN_POW,             // **=
    ASSIGN_EXP,             // ^=
    
    // Arithmetic operators
    PLUS,                   // +
    MINUS,                  // -
    MULTIPLY,               // *
    DIVIDE,                 // /
    MODULO,                 // %
    POWER,                  // **
    EXPONENT,               // ^
    
    // Increment/decrement
    INCREMENT,              // ++
    DECREMENT,              // --
    
    // Comparison operators
    EQUAL,                  // ==
    NOT_EQUAL,              // !=
    LESS_THAN,              // <
    LESS_EQUAL,             // <=
    GREATER_THAN,           // >
    GREATER_EQUAL,          // >=
    
    // Logical operators
    LOGICAL_AND,            // &&
    LOGICAL_OR,             // ||
    LOGICAL_NOT,            // !
    
    // Bitwise operators
    BITWISE_AND,            // &
    BITWISE_OR,             // |
    BITWISE_XOR,            // ^^
    BITWISE_NOT,            // ~
    SHIFT_LEFT,             // <<
    SHIFT_RIGHT,            // >>
    
    // Unary operators
    ADDRESS_OF,             // @
    DEREFERENCE,            // * (context-dependent)
    
    // Other operators
    CONDITIONAL,            // ?
    SCOPE_RESOLUTION,       // ::
    MEMBER_ACCESS,          // .
    
    // Punctuation
    SEMICOLON,              // ;
    COLON,                  // :
    COMMA,                  // ,
    
    // Brackets
    PAREN_OPEN,             // (
    PAREN_CLOSE,            // )
    BRACE_OPEN,             // {
    BRACE_CLOSE,            // }
    BRACKET_OPEN,           // [
    BRACKET_CLOSE,          // ]
    
    // Special
    ARROW,                  // ->
    RANGE,                  // ..
    
    // Comments (usually filtered out)
    LINE_COMMENT,
    BLOCK_COMMENT,
    
    // Error token
    ERROR,
    
    // Whitespace (usually filtered out)
    WHITESPACE,
    NEWLINE
};

// Token structure
struct Token {
    TokenType type;
    std::string_view text;
    common::SourcePosition position;
    
    // For numeric literals
    union {
        long long integer_value;
        double float_value;
    };
    
    // For string literals (processed text without quotes/escapes)
    std::string processed_text;
    
    Token() : type(TokenType::ERROR), integer_value(0) {}
    
    Token(TokenType t, std::string_view txt, const common::SourcePosition& pos)
        : type(t), text(txt), position(pos), integer_value(0) {}
    
    // Check if token is a keyword
    bool isKeyword() const;
    
    // Check if token is an operator
    bool isOperator() const;
    
    // Check if token is a literal
    bool isLiteral() const;
    
    // Check if token should be filtered out (whitespace, comments)
    bool shouldFilter() const;
    
    // Get string representation of token type
    std::string_view typeToString() const;
    
    // Get precedence for operators (0 = lowest, higher = higher precedence)
    int getOperatorPrecedence() const;
    
    // Check if operator is right associative
    bool isRightAssociative() const;
};

// Convert token type to string
std::string_view tokenTypeToString(TokenType type);

// Check if a string is a keyword and return the corresponding token type
TokenType getKeywordType(std::string_view text);

// Check if character is valid for identifier start
bool isIdentifierStart(char c);

// Check if character is valid for identifier continuation
bool isIdentifierContinue(char c);

// Check if character is a digit
bool isDigit(char c);

// Check if character is a hex digit
bool isHexDigit(char c);

// Check if character is a binary digit
bool isBinaryDigit(char c);

// Check if character is an octal digit
bool isOctalDigit(char c);

// Check if character is whitespace
bool isWhitespace(char c);

// Escape sequence processing
std::string processEscapeSequences(std::string_view text);

// Parse integer literal
long long parseIntegerLiteral(std::string_view text, int base = 10);

// Parse float literal
double parseFloatLiteral(std::string_view text);

} // namespace lexer
} // namespace flux