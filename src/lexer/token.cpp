#include "token.h"
#include <sstream>

namespace flux {
namespace lexer {

// Initialize the keyword map
const std::unordered_map<std::string_view, TokenType> Token::keywords_ = {
    {"asm", TokenType::KEYWORD_ASM},
    {"and", TokenType::KEYWORD_AND},
    {"break", TokenType::KEYWORD_BREAK},
    {"case", TokenType::KEYWORD_CASE},
    {"catch", TokenType::KEYWORD_CATCH},
    {"class", TokenType::KEYWORD_CLASS},
    {"const", TokenType::KEYWORD_CONST},
    {"continue", TokenType::KEYWORD_CONTINUE},
    {"data", TokenType::KEYWORD_DATA},
    {"def", TokenType::KEYWORD_DEF},
    {"default", TokenType::KEYWORD_DEFAULT},
    {"do", TokenType::KEYWORD_DO},
    {"else", TokenType::KEYWORD_ELSE},
    {"enum", TokenType::KEYWORD_ENUM},
    {"false", TokenType::KEYWORD_FALSE},
    {"for", TokenType::KEYWORD_FOR},
    {"if", TokenType::KEYWORD_IF},
    {"import", TokenType::KEYWORD_IMPORT},
    {"is", TokenType::KEYWORD_IS},
    {"namespace", TokenType::KEYWORD_NAMESPACE},
    {"not", TokenType::KEYWORD_NOT},
    {"object", TokenType::KEYWORD_OBJECT},
    {"op", TokenType::KEYWORD_OP},
    {"operator", TokenType::KEYWORD_OPERATOR},
    {"or", TokenType::KEYWORD_OR},
    {"return", TokenType::KEYWORD_RETURN},
    {"signed", TokenType::KEYWORD_SIGNED},
    {"sizeof", TokenType::KEYWORD_SIZEOF},
    {"struct", TokenType::KEYWORD_STRUCT},
    {"super", TokenType::KEYWORD_SUPER},
    {"switch", TokenType::KEYWORD_SWITCH},
    {"template", TokenType::KEYWORD_TEMPLATE},
    {"this", TokenType::KEYWORD_THIS},
    {"throw", TokenType::KEYWORD_THROW},
    {"true", TokenType::KEYWORD_TRUE},
    {"try", TokenType::KEYWORD_TRY},
    {"type", TokenType::KEYWORD_TYPE},
    {"typeof", TokenType::KEYWORD_TYPEOF},
    {"unsigned", TokenType::KEYWORD_UNSIGNED},
    {"using", TokenType::KEYWORD_USING},
    {"void", TokenType::KEYWORD_VOID},
    {"while", TokenType::KEYWORD_WHILE},
    {"xor", TokenType::KEYWORD_XOR}
};

// Constructor
Token::Token(TokenType type, std::string_view lexeme, 
             const common::SourcePosition& start, const common::SourcePosition& end)
    : type_(type), lexeme_(lexeme), start_(start), end_(end), int_value_(0) {
    // Initialize literal values based on token type
    if (type == TokenType::INTEGER_LITERAL) {
        try {
            // Handle hexadecimal numbers (0x format)
            if (lexeme.size() > 2 && lexeme[0] == '0' && (lexeme[1] == 'x')) {
                std::string hex_str(lexeme.substr(2));
                int_value_ = std::stoll(hex_str, nullptr, 16);
            }
            // Handle binary numbers (with 'b' suffix)
            else if (lexeme.size() > 1 && (lexeme.back() == 'b')) {
                std::string bin_str(lexeme.substr(0, lexeme.size() - 1));
                // Check if all digits are valid binary digits (0 or 1)
                for (char c : bin_str) {
                    if (c != '0' && c != '1') {
                        throw std::invalid_argument("Invalid binary digit");
                    }
                }
                int_value_ = std::stoll(bin_str, nullptr, 2);
            }
            // Handle octal numbers (with 'o' suffix)
            else if (lexeme.size() > 1 && (lexeme.back() == 'o')) {
                std::string oct_str(lexeme.substr(0, lexeme.size() - 1));
                // Check if all digits are valid octal digits (0-7)
                for (char c : oct_str) {
                    if (c < '0' || c > '7') {
                        throw std::invalid_argument("Invalid octal digit");
                    }
                }
                int_value_ = std::stoll(oct_str, nullptr, 8);
            }
            // Handle explicit decimal numbers (with 'd' suffix) 
            else if (lexeme.size() > 1 && (lexeme.back() == 'd')) {
                std::string dec_str(lexeme.substr(0, lexeme.size() - 1));
                int_value_ = std::stoll(dec_str, nullptr, 10);
            }
            // Handle hexadecimal numbers (with 'h' suffix)
            else if (lexeme.size() > 1 && (lexeme.back() == 'h')) {
                std::string hex_str(lexeme.substr(0, lexeme.size() - 1));
                int_value_ = std::stoll(hex_str, nullptr, 16);
            }
            // Regular decimal numbers (default)
            else {
                int_value_ = std::stoll(std::string(lexeme));
            }
        } catch (const std::exception& e) {
            // Handle invalid numbers, but don't throw - create a valid token with value 0
            int_value_ = 0;
            // In a real implementation, you might want to log this error or handle it differently
        }
    } else if (type == TokenType::FLOAT_LITERAL) {
        try {
            float_value_ = std::stod(std::string(lexeme));
        } catch (const std::exception& e) {
            // Handle invalid floats
            float_value_ = 0.0;
        }
    } else if (type == TokenType::BOOLEAN_LITERAL) {
        bool_value_ = (lexeme == "true");
    } else if (type == TokenType::CHAR_LITERAL) {
        // In Flux, char is the equivalent of Python's string
        // Remove the quotes
        if (lexeme.size() >= 2) {
            // Handle empty strings properly
            if (lexeme.size() == 2 && lexeme[0] == '"' && lexeme[1] == '"') {
                string_value_ = "";
            } else {
                string_value_ = std::string(lexeme.substr(1, lexeme.size() - 2));
            }
        } else {
            string_value_ = "";
        }
    }
}

// Template specializations for the constructor with literal values
template<>
Token::Token(TokenType type, std::string_view lexeme, int64_t literal_value,
             const common::SourcePosition& start, const common::SourcePosition& end)
    : type_(type), lexeme_(lexeme), start_(start), end_(end), int_value_(literal_value) {
}

template<>
Token::Token(TokenType type, std::string_view lexeme, double literal_value,
             const common::SourcePosition& start, const common::SourcePosition& end)
    : type_(type), lexeme_(lexeme), start_(start), end_(end), float_value_(literal_value) {
}

template<>
Token::Token(TokenType type, std::string_view lexeme, bool literal_value,
             const common::SourcePosition& start, const common::SourcePosition& end)
    : type_(type), lexeme_(lexeme), start_(start), end_(end), bool_value_(literal_value) {
}

template<>
Token::Token(TokenType type, std::string_view lexeme, std::string literal_value,
             const common::SourcePosition& start, const common::SourcePosition& end)
    : type_(type), lexeme_(lexeme), start_(start), end_(end), int_value_(0), string_value_(std::move(literal_value)) {
}

// Get literal values
int64_t Token::intValue() const {
    if (type_ != TokenType::INTEGER_LITERAL) {
        return 0;
    }
    return int_value_;
}

double Token::floatValue() const {
    if (type_ != TokenType::FLOAT_LITERAL) {
        return 0.0;
    }
    return float_value_;
}

std::string_view Token::stringValue() const {
    if (type_ != TokenType::CHAR_LITERAL) {
        return "";
    }
    // If the token is just two quotes, return an empty string
    if (lexeme_.size() == 2 && lexeme_[0] == '"' && lexeme_[1] == '"') {
        return "";
    }
    return string_value_;
}

bool Token::boolValue() const {
    if (type_ != TokenType::BOOLEAN_LITERAL) {
        return false;
    }
    return bool_value_;
}

// Check if token is one of the specified types
bool Token::isOneOf(std::initializer_list<TokenType> types) const {
    for (auto t : types) {
        if (type_ == t) {
            return true;
        }
    }
    return false;
}

// Check if token is a keyword
bool Token::isKeyword() const {
    return type_ >= TokenType::KEYWORD_ASM && type_ <= TokenType::KEYWORD_XOR;
}

// Check if token is an operator
bool Token::isOperator() const {
    return (type_ >= TokenType::PLUS && type_ <= TokenType::DOUBLE_ASTERISK_EQUAL) ||
           type_ == TokenType::DOT || type_ == TokenType::DOUBLE_COLON ||
           type_ == TokenType::QUESTION || type_ == TokenType::COLON ||
           type_ == TokenType::SEMICOLON || type_ == TokenType::COMMA ||
           type_ == TokenType::ASTERISK_PTR || type_ == TokenType::AT_REF;
}

// Convert token to string for debugging
std::string Token::toString() const {
    std::stringstream ss;
    
    ss << "Token(" << tokenTypeToString(type_) << ", '" << lexeme_ << "', ";
    
    // Add source position
    ss << "line " << start_.line << ", col " << start_.column << ")";
    
    return ss.str();
}

// Get string representation of token type
std::string_view Token::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        
        // Literals
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        
        // Identifiers
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        
        // Keywords
        case TokenType::KEYWORD_ASM: return "KEYWORD_ASM";
        case TokenType::KEYWORD_AND: return "KEYWORD_AND";
        case TokenType::KEYWORD_BREAK: return "KEYWORD_BREAK";
        case TokenType::KEYWORD_CASE: return "KEYWORD_CASE";
        case TokenType::KEYWORD_CATCH: return "KEYWORD_CATCH";
        case TokenType::KEYWORD_CLASS: return "KEYWORD_CLASS";
        case TokenType::KEYWORD_CONST: return "KEYWORD_CONST";
        case TokenType::KEYWORD_CONTINUE: return "KEYWORD_CONTINUE";
        case TokenType::KEYWORD_DATA: return "KEYWORD_DATA";
        case TokenType::KEYWORD_DEF: return "KEYWORD_DEF";
        case TokenType::KEYWORD_DEFAULT: return "KEYWORD_DEFAULT";
        case TokenType::KEYWORD_DO: return "KEYWORD_DO";
        case TokenType::KEYWORD_ELSE: return "KEYWORD_ELSE";
        case TokenType::KEYWORD_ENUM: return "KEYWORD_ENUM";
        case TokenType::KEYWORD_FALSE: return "KEYWORD_FALSE";
        case TokenType::KEYWORD_FOR: return "KEYWORD_FOR";
        case TokenType::KEYWORD_IF: return "KEYWORD_IF";
        case TokenType::KEYWORD_IMPORT: return "KEYWORD_IMPORT";
        case TokenType::KEYWORD_IS: return "KEYWORD_IS";
        case TokenType::KEYWORD_NAMESPACE: return "KEYWORD_NAMESPACE";
        case TokenType::KEYWORD_NOT: return "KEYWORD_NOT";
        case TokenType::KEYWORD_OBJECT: return "KEYWORD_OBJECT";
        case TokenType::KEYWORD_OP: return "KEYWORD_OP";
        case TokenType::KEYWORD_OPERATOR: return "KEYWORD_OPERATOR";
        case TokenType::KEYWORD_OR: return "KEYWORD_OR";
        case TokenType::KEYWORD_RETURN: return "KEYWORD_RETURN";
        case TokenType::KEYWORD_SIGNED: return "KEYWORD_SIGNED";
        case TokenType::KEYWORD_SIZEOF: return "KEYWORD_SIZEOF";
        case TokenType::KEYWORD_STRUCT: return "KEYWORD_STRUCT";
        case TokenType::KEYWORD_SUPER: return "KEYWORD_SUPER";
        case TokenType::KEYWORD_SWITCH: return "KEYWORD_SWITCH";
        case TokenType::KEYWORD_TEMPLATE: return "KEYWORD_TEMPLATE";
        case TokenType::KEYWORD_THIS: return "KEYWORD_THIS";
        case TokenType::KEYWORD_THROW: return "KEYWORD_THROW";
        case TokenType::KEYWORD_TRUE: return "KEYWORD_TRUE";
        case TokenType::KEYWORD_TRY: return "KEYWORD_TRY";
        case TokenType::KEYWORD_TYPE: return "KEYWORD_TYPE";
        case TokenType::KEYWORD_TYPEOF: return "KEYWORD_TYPEOF";
        case TokenType::KEYWORD_UNSIGNED: return "KEYWORD_UNSIGNED";
        case TokenType::KEYWORD_USING: return "KEYWORD_USING";
        case TokenType::KEYWORD_VOID: return "KEYWORD_VOID";
        case TokenType::KEYWORD_WHILE: return "KEYWORD_WHILE";
        case TokenType::KEYWORD_XOR: return "KEYWORD_XOR";
        
        // Operators
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::ASTERISK: return "ASTERISK";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::DOUBLE_ASTERISK: return "DOUBLE_ASTERISK";
        case TokenType::PLUS_PLUS: return "PLUS_PLUS";
        case TokenType::MINUS_MINUS: return "MINUS_MINUS";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::AMPERSAND_AMPERSAND: return "AMPERSAND_AMPERSAND";
        case TokenType::PIPE_PIPE: return "PIPE_PIPE";
        case TokenType::EXCLAMATION: return "EXCLAMATION";
        case TokenType::AMPERSAND: return "AMPERSAND";
        case TokenType::PIPE: return "PIPE";
        case TokenType::CARET: return "CARET";
        case TokenType::TILDE: return "TILDE";
        case TokenType::LESS_LESS: return "LESS_LESS";
        case TokenType::GREATER_GREATER: return "GREATER_GREATER";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::PLUS_EQUAL: return "PLUS_EQUAL";
        case TokenType::MINUS_EQUAL: return "MINUS_EQUAL";
        case TokenType::ASTERISK_EQUAL: return "ASTERISK_EQUAL";
        case TokenType::SLASH_EQUAL: return "SLASH_EQUAL";
        case TokenType::PERCENT_EQUAL: return "PERCENT_EQUAL";
        case TokenType::AMPERSAND_EQUAL: return "AMPERSAND_EQUAL";
        case TokenType::PIPE_EQUAL: return "PIPE_EQUAL";
        case TokenType::CARET_EQUAL: return "CARET_EQUAL";
        case TokenType::LESS_LESS_EQUAL: return "LESS_LESS_EQUAL";
        case TokenType::GREATER_GREATER_EQUAL: return "GREATER_GREATER_EQUAL";
        case TokenType::DOUBLE_ASTERISK_EQUAL: return "DOUBLE_ASTERISK_EQUAL";
        case TokenType::DOT: return "DOT";
        case TokenType::DOUBLE_COLON: return "DOUBLE_COLON";
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::ASTERISK_PTR: return "ASTERISK_PTR";
        case TokenType::AT_REF: return "AT_REF";
        case TokenType::ARROW: return "ARROW";
        
        // Delimiters
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE: return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        
        // Special tokens
        case TokenType::ISTRING_START: return "ISTRING_START";
        case TokenType::ISTRING_MIDDLE: return "ISTRING_MIDDLE";
        case TokenType::ISTRING_EXPR_START: return "ISTRING_EXPR_START";
        case TokenType::ISTRING_EXPR_END: return "ISTRING_EXPR_END";
        case TokenType::ISTRING_END: return "ISTRING_END";
        case TokenType::BANG_VOID: return "BANG_VOID";
        
        // Error
        case TokenType::ERROR: return "ERROR";
        
        default: return "UNKNOWN";
    }
}

// Get keyword token type from string
TokenType Token::getKeywordType(std::string_view keyword) {
    auto it = keywords_.find(keyword);
    if (it != keywords_.end()) {
        return it->second;
    }
    
    // Special case for !void
    if (keyword == "!void") {
        return TokenType::BANG_VOID;
    }
    
    return TokenType::IDENTIFIER;
}

} // namespace lexer
} // namespace flux