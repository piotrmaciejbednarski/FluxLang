#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include "../common/source.h"

namespace flux {
namespace lexer {

// Token types representing all possible tokens in the Flux language
enum class TokenType {
    // End of file
    END_OF_FILE,
    
    // Literals
    INTEGER_LITERAL,      // 123, 10h (hex)
    FLOAT_LITERAL,        // 123.45, 1e+10
    CHAR_LITERAL,         // "string"
    BOOLEAN_LITERAL,      // true, false
    
    // Identifiers
    IDENTIFIER,           // variable and function names
    
    // Keywords
    KEYWORD_ASM,          // asm
    KEYWORD_AND,          // and
    KEYWORD_BREAK,        // break
    KEYWORD_CASE,         // case
    KEYWORD_CATCH,        // catch
    KEYWORD_CLASS,        // class
    KEYWORD_CONST,        // const
    KEYWORD_CONTINUE,     // continue
    KEYWORD_DATA,         // data
    KEYWORD_DEF,          // def
    KEYWORD_DEFAULT,      // default
    KEYWORD_DO,           // do
    KEYWORD_ELSE,         // else
    KEYWORD_ENUM,         // enum
    KEYWORD_FALSE,        // false
    KEYWORD_FOR,          // for
    KEYWORD_IF,           // if
    KEYWORD_IMPORT,       // import
    KEYWORD_IS,           // is
    KEYWORD_NAMESPACE,    // namespace
    KEYWORD_NOT,          // not
    KEYWORD_OBJECT,       // object
    KEYWORD_OP,           // op
    KEYWORD_OPERATOR,     // operator
    KEYWORD_OR,           // or
    KEYWORD_RETURN,       // return
    KEYWORD_SIGNED,       // signed
    KEYWORD_SIZEOF,       // sizeof
    KEYWORD_STRUCT,       // struct
    KEYWORD_SUPER,        // super
    KEYWORD_SWITCH,       // switch
    KEYWORD_TEMPLATE,     // template
    KEYWORD_THIS,         // this
    KEYWORD_THROW,        // throw
    KEYWORD_TRUE,         // true
    KEYWORD_TRY,          // try
    KEYWORD_TYPE,         // type
    KEYWORD_TYPEOF,       // typeof
    KEYWORD_UNSIGNED,     // unsigned
    KEYWORD_USING,        // using
    KEYWORD_VOID,         // void
    KEYWORD_WHILE,        // while
    KEYWORD_XOR,          // xor
    
    // Operators
    // Arithmetic operators
    PLUS,                 // +
    MINUS,                // -
    ASTERISK,             // *
    SLASH,                // /
    PERCENT,              // %
    DOUBLE_ASTERISK,      // **
    
    // Increment/decrement
    PLUS_PLUS,            // ++
    MINUS_MINUS,          // --
    
    // Comparison operators
    EQUAL_EQUAL,          // ==
    NOT_EQUAL,            // !=
    LESS,                 // <
    LESS_EQUAL,           // <=
    GREATER,              // >
    GREATER_EQUAL,        // >=
    
    // Logical operators
    AMPERSAND_AMPERSAND,  // &&
    PIPE_PIPE,            // ||
    EXCLAMATION,          // !
    
    // Bitwise operators
    AMPERSAND,            // &
    PIPE,                 // |
    CARET,                // ^
    TILDE,                // ~
    LESS_LESS,            // <<
    GREATER_GREATER,      // >>
    
    // Assignment operators
    EQUAL,                // =
    PLUS_EQUAL,           // +=
    MINUS_EQUAL,          // -=
    ASTERISK_EQUAL,       // *=
    SLASH_EQUAL,          // /=
    PERCENT_EQUAL,        // %=
    AMPERSAND_EQUAL,      // &=
    PIPE_EQUAL,           // |=
    CARET_EQUAL,          // ^=
    LESS_LESS_EQUAL,      // <<=
    GREATER_GREATER_EQUAL, // >>=
    DOUBLE_ASTERISK_EQUAL, // **=
    
    // Other operators
    DOT,                  // .
    DOUBLE_COLON,         // ::
    QUESTION,             // ?
    COLON,                // :
    SEMICOLON,            // ;
    COMMA,                // ,
    ARROW,                // ->
    
    // Pointer operators
    ASTERISK_PTR,         // * (pointer dereference)
    AMPERSAND_REF,        // & (address-of)
    
    // Delimiters
    LEFT_PAREN,           // (
    RIGHT_PAREN,          // )
    LEFT_BRACE,           // {
    RIGHT_BRACE,          // }
    LEFT_BRACKET,         // [
    RIGHT_BRACKET,        // ]
    
    // Special tokens
    ISTRING_START,        // i"
    ISTRING_MIDDLE,       // text between {} in i-string
    ISTRING_EXPR_START,   // { in i-string
    ISTRING_EXPR_END,     // } in i-string
    ISTRING_END,          // " at end of i-string
    
    // Special
    BANG_VOID,            // !void (for main return type)
    
    // Error token
    ERROR
};

// Token class to represent a single token in the source code
class Token {
public:
    // Default constructor (adds an empty token for initialization)
    Token() 
        : type_(TokenType::ERROR), 
          lexeme_(""), 
          start_({0, 0}), 
          end_({0, 0}), 
          int_value_(0) {}

    // Constructor
    Token(TokenType type, std::string_view lexeme, const common::SourcePosition& start, 
          const common::SourcePosition& end);
    
    // Constructor with value (for literals)
    template<typename T>
    Token(TokenType type, std::string_view lexeme, T literal_value, 
          const common::SourcePosition& start, const common::SourcePosition& end);
    
    // Getters
    TokenType type() const { return type_; }
    std::string_view lexeme() const { return lexeme_; }
    common::SourcePosition start() const { return start_; }
    common::SourcePosition end() const { return end_; }
    common::SourceRange range() const { return {start_, end_}; }
    
    // Get literal value for different token types
    int64_t intValue() const;
    double floatValue() const;
    std::string_view stringValue() const;
    bool boolValue() const;
    
    // Check if token is of a specific type
    bool is(TokenType type) const { return type_ == type; }
    
    // Check if token is one of the specified types
    bool isOneOf(std::initializer_list<TokenType> types) const;
    
    // Check if token is a keyword
    bool isKeyword() const;
    
    // Check if token is an operator
    bool isOperator() const;
    
    // Convert token to string for debugging
    std::string toString() const;
    
    // Get string representation of token type
    static std::string_view tokenTypeToString(TokenType type);
    
    // Get keyword token type from string
    static TokenType getKeywordType(std::string_view keyword);

private:
    TokenType type_;                  // The type of the token
    std::string_view lexeme_;         // The original string of the token
    common::SourcePosition start_;    // Starting position in source
    common::SourcePosition end_;      // Ending position in source
    
    // Union for storing literal values
    union {
        int64_t int_value_;          // For INTEGER_LITERAL
        double float_value_;         // For FLOAT_LITERAL
        bool bool_value_;            // For BOOLEAN_LITERAL
    };
    
    std::string string_value_;       // For CHAR_LITERAL (string in Flux)
    
    // Static map of keywords to token types
    static const std::unordered_map<std::string_view, TokenType> keywords_;
};

} // namespace lexer
} // namespace flux