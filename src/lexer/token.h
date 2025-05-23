#pragma once

#include "../common/source.h"
#include "../common/arena.h"
#include <string>
#include <string_view>
#include <unordered_map>

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
    BINARY_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords (from the specification keyword list)
    AUTO,
    AS,
    ASM,
    ASSERT,
    AND,
    BREAK,
    CASE,
    CATCH,
    CONST,
    CONTINUE,
    DATA,
    DEF,
    DEFAULT,
    DO,
    ELSE,
    ENUM,
    FOR,
    IF,
    IMPORT,
    IN,
    IS,
    NAMESPACE,
    NOT,
    OBJECT,
    OPERATOR,
    OR,
    RETURN,
    SIGNED,
    SIZEOF,
    STRUCT,
    SUPER,
    SWITCH,
    TEMPLATE,
    THIS,
    THROW,
    TRY,
    TYPEOF,
    UNSIGNED,
    USING,
    VOID,
    VOLATILE,
    WHILE,
    XOR,
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    MULTIPLY,       // *
    DIVIDE,         // /
    MODULO,         // %
    POWER,          // **
    
    // Assignment operators
    ASSIGN,         // =
    PLUS_ASSIGN,    // +=
    MINUS_ASSIGN,   // -=
    MULTIPLY_ASSIGN, // *=
    DIVIDE_ASSIGN,  // /=
    MODULO_ASSIGN,  // %=
    POWER_ASSIGN,   // **=
    
    // Comparison operators
    EQUAL,          // ==
    NOT_EQUAL,      // !=
    LESS_THAN,      // <
    LESS_EQUAL,     // <=
    GREATER_THAN,   // >
    GREATER_EQUAL,  // >=
    
    // Logical operators
    LOGICAL_AND,    // &&
    LOGICAL_OR,     // ||
    LOGICAL_NOT,    // !
    
    // Bitwise operators
    BITWISE_AND,    // &
    BITWISE_OR,     // |
    BITWISE_XOR,    // ^
    BITWISE_NOT,    // ~
    SHIFT_LEFT,     // <<
    SHIFT_RIGHT,    // >>
    SHIFT_LEFT_ASSIGN,  // <<=
    SHIFT_RIGHT_ASSIGN, // >>=
    BITWISE_AND_ASSIGN, // &=
    BITWISE_OR_ASSIGN,  // |=
    BITWISE_XOR_ASSIGN, // ^=
    
    // Increment/decrement
    INCREMENT,      // ++
    DECREMENT,      // --
    
    // Punctuation
    SEMICOLON,      // ;
    COMMA,          // ,
    DOT,            // .
    SCOPE_RESOLUTION, // ::
    QUESTION,       // ?
    COLON,          // :
    ARROW,          // ->
    
    // Brackets and braces
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    
    // Pointer operators
    ADDRESS_OF,     // @
    DEREFERENCE,    // * (context-dependent with MULTIPLY)
    
    // Special tokens for i-strings (injectable strings)
    I_STRING_START, // i"
    I_STRING_TEXT,  // text part of i-string
    I_STRING_EXPR_START, // :{
    I_STRING_EXPR_END,   // };
    I_STRING_END,   // "
    
    // Comments
    COMMENT,
    
    // Whitespace (usually skipped)
    WHITESPACE,
    NEWLINE,
    
    // Error token
    ERROR
};

// Token structure
struct Token {
    TokenType type;
    std::string_view text;
    common::SourceRange range;
    
    // Additional data for specific token types
    union {
        int64_t intValue;
        double floatValue;
    };
    
    Token(TokenType t, std::string_view txt, const common::SourceRange& r)
        : type(t), text(txt), range(r), intValue(0) {}
    
    Token(TokenType t, std::string_view txt, const common::SourceRange& r, int64_t val)
        : type(t), text(txt), range(r), intValue(val) {}
    
    Token(TokenType t, std::string_view txt, const common::SourceRange& r, double val)
        : type(t), text(txt), range(r), floatValue(val) {}
};

// Convert token type to string for debugging
std::string_view tokenTypeToString(TokenType type);

// Check if a token type is a keyword
bool isKeyword(TokenType type);

// Check if a token type is an operator
bool isOperator(TokenType type);

// Check if a token type is a literal
bool isLiteral(TokenType type);

// Keyword lookup table
class KeywordTable {
public:
    KeywordTable();
    
    // Look up a keyword by text
    TokenType lookup(std::string_view text) const;
    
    // Check if text is a keyword
    bool isKeyword(std::string_view text) const;

private:
    std::unordered_map<std::string_view, TokenType> keywords_;
};

// Global keyword table instance
extern const KeywordTable keywordTable;

} // namespace lexer
} // namespace flux