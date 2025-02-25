#ifndef FLUX_LEXER_H
#define FLUX_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include "error.h"

namespace flux {

// Token types for Flux language
enum class TokenType {
    // Special tokens
    END_OF_FILE,
    ERROR,
    
    // Literals
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    
    // Keywords
    OBJECT,
    ASM,
    AND,
    ASSERT,
    BREAK,
    BOOL,
    CASE,
    CATCH,
    STRING,
    CLASS,
    CONST,
    CONTINUE,
    DEFAULT,
    DELETE,
    DO,
    ELSE,
    ENUM,
    FALSE,
    FLOAT,
    FOR,
    FUNCTION,
    IF,
    IMPORT,
    INT,
    IS,
    LAMBDA,
    MEMALLOC,
    NAMESPACE,
    NEW,
    NOT,
    NULLPTR,
    OPERATOR,
    OR,
    PRINT,
    REQUIRE,
    RETURN,
    SIGNED,
    SIZEOF,
    STRUCT,
    SUPER,
    SWITCH,
    THIS,
    THROW,
    TRUE,
    TRY,
    TYPEDEF,
    UNION,
    UNSIGNED,
    USING,
    VOID,
    WHILE,
    XOR,
    
    // Punctuation and operators
    LEFT_PAREN,       // (
    RIGHT_PAREN,      // )
    LEFT_BRACE,       // {
    RIGHT_BRACE,      // }
    LEFT_BRACKET,     // [
    RIGHT_BRACKET,    // ]
    COMMA,            // ,
    DOT,              // .
    MINUS,            // -
    PLUS,             // +
    SEMICOLON,        // ;
    SLASH,            // /
    STAR,             // *
    MODULO,           // %
    
    // Bitwise operators
    BIT_AND,          // &
    BIT_OR,           // |
    BIT_XOR,          // ^
    BIT_NOT,          // ~
    LEFT_SHIFT,       // <<
    RIGHT_SHIFT,      // >>
    
    // Comparison operators
    BANG,             // !
    BANG_EQUAL,       // !=
    EQUAL,            // =
    EQUAL_EQUAL,      // ==
    GREATER,          // >
    GREATER_EQUAL,    // >=
    LESS,             // <
    LESS_EQUAL,       // <=
    
    // Compound assignment operators
    PLUS_EQUAL,       // +=
    MINUS_EQUAL,      // -=
    STAR_EQUAL,       // *=
    SLASH_EQUAL,      // /=
    MODULO_EQUAL,     // %=
    AND_EQUAL,        // &=
    OR_EQUAL,         // |=
    XOR_EQUAL,        // ^=
    
    // Logical operators
    LOGICAL_AND,      // &&
    LOGICAL_OR,       // ||
    
    // Other operators
    INCREMENT,        // ++
    DECREMENT,        // --
    ARROW,            // ->
    DOUBLE_COLON,     // ::
    ADDRESS_OF,       // @
    EXPONENT,         // **
    
    // String interpolation
    INTERP_START,     // i"
    INTERP_END,       // ":{
    INTERP_CLOSE,     // ;}
    
    // Other tokens
    COLON,            // :
    QUESTION,         // ?
};

// Structure to represent a token
struct Token {
    TokenType type;
    std::string_view lexeme;  // Using string_view for memory efficiency
    int line;
    int column;
    
    // For literals that need conversion/processing
    union {
        long long intValue;
        double floatValue;
    };
    
    Token(TokenType type, std::string_view lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
    
    // Utility methods
    bool isLiteral() const {
        return type == TokenType::INT_LITERAL || 
               type == TokenType::FLOAT_LITERAL || 
               type == TokenType::STRING_LITERAL || 
               type == TokenType::CHAR_LITERAL;
    }
    
    bool isKeyword() const {
        return type >= TokenType::OBJECT && type <= TokenType::XOR;
    }
    
    bool isOperator() const {
        return type >= TokenType::LEFT_PAREN && type <= TokenType::QUESTION;
    }
    
    SourceLocation getLocation() const {
        return SourceLocation("<source>", line, column);
    }
};

// Lexer class for tokenizing Flux source code
class Lexer {
private:
    std::string_view source;            // Using string_view for source text
    std::vector<Token> tokens;          // All tokens from the source
    std::string_view::iterator current;  // Current position in source
    std::string_view::iterator start;    // Start of current token
    int line;                           // Current line
    int column;                         // Current column
    
    // Keywords map (static to be initialized once)
    static const std::unordered_map<std::string_view, TokenType> keywords;
    
    // Helper methods
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    bool isAtEnd() const;
    void skipWhitespace();
    
    // Token scanners
    void scanToken();
    void scanNumber();
    void scanIdentifier();
    void scanString();
    void scanCharacter();
    void scanStringInterpolation();
    void scanComment();
    
    // Error handling
    void addError(const std::string& message);
    
    // Add a token to the token list
    void addToken(TokenType type);
    
public:
    Lexer(std::string_view source);
    
    // Get all tokens from the source
    std::vector<Token> scanTokens();
    
    // Reset to scan a new source
    void reset(std::string_view newSource);
};

} // namespace flux

#endif // FLUX_LEXER_H
