#ifndef FLUX_LEXER_H
#define FLUX_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <variant>
#include "error.h"

namespace flux {

// Forward declaration
class SourceManager;

// Token types in Flux
enum class TokenType {
    // Special tokens
    END_OF_FILE,
    ERROR,
    
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    NULL_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords
    KW_OBJECT,
    KW_ASM,
    KW_AND,
    KW_ASSERT,
    KW_BREAK,
    KW_CASE,
    KW_CATCH,
    KW_CHAR,
    KW_CLASS,
    KW_CONST,
    KW_CONTINUE,
    KW_DEFAULT,
    KW_DELETE,
    KW_DO,
    KW_ELSE,
    KW_ENUM,
    KW_FALSE,
    KW_FLOAT,
    KW_FOR,
    KW_IF,
    KW_IMPORT,
    KW_INT,
    KW_IS,
    KW_LAMBDA,
    KW_MEMALLOC,
    KW_NAMESPACE,
    KW_NEW,
    KW_NOT,
    KW_OPERATOR,
    KW_OR,
    KW_REQUIRE,
    KW_RETURN,
    KW_SIGNED,
    KW_SIZEOF,
    KW_STRUCT,
    KW_SUPER,
    KW_SWITCH,
    KW_THIS,
    KW_THROW,
    KW_TRUE,
    KW_TRY,
    KW_TYPEDEF,
    KW_UNION,
    KW_UNSIGNED,
    KW_USING,
    KW_VOID,
    KW_WHILE,
    KW_XOR,
    
    // Operators and punctuation
    OP_PLUS,             // +
    OP_MINUS,            // -
    OP_STAR,             // *
    OP_SLASH,            // /
    OP_PERCENT,          // %
    OP_CARET,            // ^
    OP_AMPERSAND,        // &
    OP_PIPE,             // |
    OP_TILDE,            // ~
    OP_EXCLAIM,          // !
    OP_EQUAL,            // =
    OP_LESS,             // <
    OP_GREATER,          // >
    OP_DOT,              // .
    OP_ARROW,            // ->
    OP_PLUS_PLUS,        // ++
    OP_MINUS_MINUS,      // --
    OP_PLUS_EQUAL,       // +=
    OP_MINUS_EQUAL,      // -=
    OP_STAR_EQUAL,       // *=
    OP_SLASH_EQUAL,      // /=
    OP_PERCENT_EQUAL,    // %=
    OP_AMPERSAND_EQUAL,  // &=
    OP_PIPE_EQUAL,       // |=
    OP_CARET_EQUAL,      // ^=
    OP_LESS_LESS,        // <<
    OP_GREATER_GREATER,  // >>
    OP_LESS_LESS_EQUAL,  // <<=
    OP_GREATER_GREATER_EQUAL, // >>=
    OP_EQUAL_EQUAL,      // ==
    OP_EXCLAIM_EQUAL,    // !=
    OP_LESS_EQUAL,       // <=
    OP_GREATER_EQUAL,    // >=
    OP_AMPERSAND_AMPERSAND, // &&
    OP_PIPE_PIPE,        // ||
    OP_DOUBLE_COLON,     // ::
    OP_AT,               // @  (for pointers in Flux)
    OP_DOUBLE_STAR,      // ** (exponentiation)
    OP_QUESTION,
    OP_QUESTION_QUESTION,
    OP_QUESTION_DOT,
    
    // Delimiters
    LPAREN,              // (
    RPAREN,              // )
    LBRACE,              // {
    RBRACE,              // }
    LBRACKET,            // [
    RBRACKET,            // ]
    SEMICOLON,           // ;
    COLON,               // :
    COMMA,               // ,
    
    // Special tokens for Flux-specific features
    INJECTABLE_STRING,   // i"...":{...;}
    BIT_WIDTH_SPECIFIER, // int{32}, float{64}, etc.
};

// Helper function to convert TokenType to string representation
std::string tokenTypeToString(TokenType type);

// Structure to hold the value of a token
struct TokenValue {
    using LiteralValue = std::variant<
        std::monostate,  // For tokens with no specific value
        int64_t,         // Integer literals
        double,          // Float literals
        bool,            // Bool literals
        char,            // Char literals
        std::string,     // String literals, identifiers
        std::nullptr_t   // nullptr literal
    >;
    
    LiteralValue value;
    std::optional<size_t> bitWidth;  // For types with bit width specifiers

    TokenValue() : value(std::monostate{}) {}
    
    template<typename T>
    TokenValue(T val) : value(val) {}
    
    template<typename T>
    TokenValue(T val, size_t width) : value(val), bitWidth(width) {}
};

// Token structure
struct Token {
    TokenType type;
    TokenValue value;
    std::string_view lexeme;  // The actual text of the token
    size_t line;
    size_t column;
    size_t length;
    
    Token(TokenType type, std::string_view lexeme, size_t line, size_t column, size_t length)
        : type(type), lexeme(lexeme), line(line), column(column), length(length) {}
        
    Token(TokenType type, TokenValue value, std::string_view lexeme, size_t line, size_t column, size_t length)
        : type(type), value(value), lexeme(lexeme), line(line), column(column), length(length) {}
        
    // Get a string representation of the token's value
    std::string getValueString() const;
    
    // Get a formatted representation of the token
    std::string toString() const;
};

// The Lexer class
class Lexer {
public:
    Lexer(std::string_view source, std::string_view filename);
    
    // Get the next token from the source
    Token nextToken();
    
    // Peek at the next token without consuming it
    Token peekToken();
    
    // Get the current position in the source
    size_t getCurrentPosition() const { return currentPos; }
    
    // Get the current line
    size_t getCurrentLine() const { return line; }
    
    // Get the current column
    size_t getCurrentColumn() const { return column; }
    
    // Print a formatted table header for token output
    static void printTokenTableHeader();
    
    // Print a formatted table row for a token
    static void printTokenTableRow(const Token& token);
    
    // Print a separator line for the token table
    static void printTokenTableSeparator();

private:
    std::string_view source;
    std::string_view filename;
    size_t currentPos;
    size_t line;
    size_t column;
    std::vector<Token> tokenBuffer;  // For peeking ahead
    
    // Static map of keywords to token types
    static const std::unordered_map<std::string, TokenType> keywords;
    
    // Helper methods for tokenization
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void skipWhitespace();
    
    // UTF-8 handling methods
    bool isUtf8ContinuationByte(char c) const;
    bool isValidUtf8StartByte(char c) const;
    std::string_view getCurrentChar() const;
    int utf8CharSize(char firstByte) const;
    
    // Token parsing methods
    Token scanToken();
    Token handleIdentifier();
    Token handleNumber();
    Token handleString();
    Token handleChar();
    Token handleOperator();
    Token handleComment();
    Token handleInjectableString();
    Token handleBitWidthSpecifier();
    Token handleCustomOperator();
    
    // Error handling
    Token errorToken(const std::string& message);
};

} // namespace flux

#endif // FLUX_LEXER_H
