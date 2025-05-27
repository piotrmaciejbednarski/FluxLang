#include "token.h"
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <sstream>

namespace flux {
namespace lexer {

// Keyword mapping
static const std::unordered_map<std::string_view, TokenType> keywordMap = {
    {"auto", TokenType::KEYWORD_AUTO},
    {"as", TokenType::KEYWORD_AS},
    {"asm", TokenType::KEYWORD_ASM},
    {"assert", TokenType::KEYWORD_ASSERT},
    {"and", TokenType::KEYWORD_AND},
    {"break", TokenType::KEYWORD_BREAK},
    {"case", TokenType::KEYWORD_CASE},
    {"catch", TokenType::KEYWORD_CATCH},
    {"const", TokenType::KEYWORD_CONST},
    {"continue", TokenType::KEYWORD_CONTINUE},
    {"data", TokenType::KEYWORD_DATA},
    {"def", TokenType::KEYWORD_DEF},
    {"default", TokenType::KEYWORD_DEFAULT},
    {"do", TokenType::KEYWORD_DO},
    {"else", TokenType::KEYWORD_ELSE},
    {"enum", TokenType::KEYWORD_ENUM},
    {"for", TokenType::KEYWORD_FOR},
    {"if", TokenType::KEYWORD_IF},
    {"import", TokenType::KEYWORD_IMPORT},
    {"in", TokenType::KEYWORD_IN},
    {"is", TokenType::KEYWORD_IS},
    {"namespace", TokenType::KEYWORD_NAMESPACE},
    {"not", TokenType::KEYWORD_NOT},
    {"object", TokenType::KEYWORD_OBJECT},
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
    {"try", TokenType::KEYWORD_TRY},
    {"typeof", TokenType::KEYWORD_TYPEOF},
    {"unsigned", TokenType::KEYWORD_UNSIGNED},
    {"using", TokenType::KEYWORD_USING},
    {"void", TokenType::KEYWORD_VOID},
    {"volatile", TokenType::KEYWORD_VOLATILE},
    {"while", TokenType::KEYWORD_WHILE},
    {"xor", TokenType::KEYWORD_XOR}
};

bool Token::isKeyword() const {
    return type >= TokenType::KEYWORD_AUTO && type <= TokenType::KEYWORD_XOR;
}

bool Token::isOperator() const {
    return type >= TokenType::ASSIGN && type <= TokenType::MEMBER_ACCESS;
}

bool Token::isLiteral() const {
    return type >= TokenType::INTEGER_LITERAL && type <= TokenType::I_STRING_LITERAL;
}

bool Token::shouldFilter() const {
    return type == TokenType::WHITESPACE || 
           type == TokenType::NEWLINE || 
           type == TokenType::LINE_COMMENT || 
           type == TokenType::BLOCK_COMMENT;
}

std::string_view Token::typeToString() const {
    return tokenTypeToString(type);
}

int Token::getOperatorPrecedence() const {
    switch (type) {
        // Comma operator (lowest precedence)
        case TokenType::COMMA: return 1;
        
        // Assignment operators
        case TokenType::ASSIGN:
        case TokenType::ASSIGN_ADD:
        case TokenType::ASSIGN_SUB:
        case TokenType::ASSIGN_MUL:
        case TokenType::ASSIGN_DIV:
        case TokenType::ASSIGN_MOD:
        case TokenType::ASSIGN_AND:
        case TokenType::ASSIGN_OR:
        case TokenType::ASSIGN_XOR:
        case TokenType::ASSIGN_SHL:
        case TokenType::ASSIGN_SHR:
        case TokenType::ASSIGN_POW:
        case TokenType::ASSIGN_EXP:
            return 2;
        
        // Conditional operator
        case TokenType::CONDITIONAL: return 3;
        
        // Logical OR
        case TokenType::LOGICAL_OR: return 4;
        
        // Logical AND
        case TokenType::LOGICAL_AND: return 5;
        
        // Bitwise OR
        case TokenType::BITWISE_OR: return 6;
        
        // Bitwise XOR
        case TokenType::BITWISE_XOR: return 7;
        
        // Bitwise AND
        case TokenType::BITWISE_AND: return 8;
        
        // Equality operators
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
            return 9;
        
        // Relational operators
        case TokenType::LESS_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_THAN:
        case TokenType::GREATER_EQUAL:
            return 10;
        
        // Shift operators
        case TokenType::SHIFT_LEFT:
        case TokenType::SHIFT_RIGHT:
            return 11;
        
        // Additive operators
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 12;
        
        // Multiplicative operators
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
            return 13;
        
        // Exponentiation (higher than multiplication)
        case TokenType::POWER:
        case TokenType::EXPONENT:
            return 14;
        
        // Unary operators (highest precedence for binary operators)
        case TokenType::LOGICAL_NOT:
        case TokenType::BITWISE_NOT:
        case TokenType::ADDRESS_OF:
        case TokenType::DEREFERENCE:
            return 15;
        
        // Member access and scope resolution
        case TokenType::MEMBER_ACCESS:
        case TokenType::SCOPE_RESOLUTION:
            return 16;
        
        default:
            return 0; // Not an operator or unknown
    }
}

bool Token::isRightAssociative() const {
    switch (type) {
        // Assignment operators are right associative
        case TokenType::ASSIGN:
        case TokenType::ASSIGN_ADD:
        case TokenType::ASSIGN_SUB:
        case TokenType::ASSIGN_MUL:
        case TokenType::ASSIGN_DIV:
        case TokenType::ASSIGN_MOD:
        case TokenType::ASSIGN_AND:
        case TokenType::ASSIGN_OR:
        case TokenType::ASSIGN_XOR:
        case TokenType::ASSIGN_SHL:
        case TokenType::ASSIGN_SHR:
        case TokenType::ASSIGN_POW:
        case TokenType::ASSIGN_EXP:
            return true;
        
        // Conditional operator is right associative
        case TokenType::CONDITIONAL:
            return true;
        
        // Exponentiation is right associative
        case TokenType::POWER:
        case TokenType::EXPONENT:
            return true;
        
        default:
            return false;
    }
}

std::string_view tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHARACTER_LITERAL: return "CHARACTER_LITERAL";
        case TokenType::DATA_LITERAL: return "DATA_LITERAL";
        case TokenType::I_STRING_LITERAL: return "I_STRING_LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        
        // Keywords
        case TokenType::KEYWORD_AUTO: return "auto";
        case TokenType::KEYWORD_AS: return "as";
        case TokenType::KEYWORD_ASM: return "asm";
        case TokenType::KEYWORD_ASSERT: return "assert";
        case TokenType::KEYWORD_AND: return "and";
        case TokenType::KEYWORD_BREAK: return "break";
        case TokenType::KEYWORD_CASE: return "case";
        case TokenType::KEYWORD_CATCH: return "catch";
        case TokenType::KEYWORD_CONST: return "const";
        case TokenType::KEYWORD_CONTINUE: return "continue";
        case TokenType::KEYWORD_DATA: return "data";
        case TokenType::KEYWORD_DEF: return "def";
        case TokenType::KEYWORD_DEFAULT: return "default";
        case TokenType::KEYWORD_DO: return "do";
        case TokenType::KEYWORD_ELSE: return "else";
        case TokenType::KEYWORD_ENUM: return "enum";
        case TokenType::KEYWORD_FOR: return "for";
        case TokenType::KEYWORD_IF: return "if";
        case TokenType::KEYWORD_IMPORT: return "import";
        case TokenType::KEYWORD_IN: return "in";
        case TokenType::KEYWORD_IS: return "is";
        case TokenType::KEYWORD_NAMESPACE: return "namespace";
        case TokenType::KEYWORD_NOT: return "not";
        case TokenType::KEYWORD_OBJECT: return "object";
        case TokenType::KEYWORD_OPERATOR: return "operator";
        case TokenType::KEYWORD_OR: return "or";
        case TokenType::KEYWORD_RETURN: return "return";
        case TokenType::KEYWORD_SIGNED: return "signed";
        case TokenType::KEYWORD_SIZEOF: return "sizeof";
        case TokenType::KEYWORD_STRUCT: return "struct";
        case TokenType::KEYWORD_SUPER: return "super";
        case TokenType::KEYWORD_SWITCH: return "switch";
        case TokenType::KEYWORD_TEMPLATE: return "template";
        case TokenType::KEYWORD_THIS: return "this";
        case TokenType::KEYWORD_THROW: return "throw";
        case TokenType::KEYWORD_TRY: return "try";
        case TokenType::KEYWORD_TYPEOF: return "typeof";
        case TokenType::KEYWORD_UNSIGNED: return "unsigned";
        case TokenType::KEYWORD_USING: return "using";
        case TokenType::KEYWORD_VOID: return "void";
        case TokenType::KEYWORD_VOLATILE: return "volatile";
        case TokenType::KEYWORD_WHILE: return "while";
        case TokenType::KEYWORD_XOR: return "xor";
        
        // Operators
        case TokenType::ASSIGN: return "=";
        case TokenType::ASSIGN_ADD: return "+=";
        case TokenType::ASSIGN_SUB: return "-=";
        case TokenType::ASSIGN_MUL: return "*=";
        case TokenType::ASSIGN_DIV: return "/=";
        case TokenType::ASSIGN_MOD: return "%=";
        case TokenType::ASSIGN_AND: return "&=";
        case TokenType::ASSIGN_OR: return "|=";
        case TokenType::ASSIGN_XOR: return "^^=";
        case TokenType::ASSIGN_SHL: return "<<=";
        case TokenType::ASSIGN_SHR: return ">>=";
        case TokenType::ASSIGN_POW: return "**=";
        case TokenType::ASSIGN_EXP: return "^=";
        
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::MULTIPLY: return "*";
        case TokenType::DIVIDE: return "/";
        case TokenType::MODULO: return "%";
        case TokenType::POWER: return "**";
        case TokenType::EXPONENT: return "^";
        
        case TokenType::INCREMENT: return "++";
        case TokenType::DECREMENT: return "--";
        
        case TokenType::EQUAL: return "==";
        case TokenType::NOT_EQUAL: return "!=";
        case TokenType::LESS_THAN: return "<";
        case TokenType::LESS_EQUAL: return "<=";
        case TokenType::GREATER_THAN: return ">";
        case TokenType::GREATER_EQUAL: return ">=";
        
        case TokenType::LOGICAL_AND: return "&&";
        case TokenType::LOGICAL_OR: return "||";
        case TokenType::LOGICAL_NOT: return "!";
        
        case TokenType::BITWISE_AND: return "&";
        case TokenType::BITWISE_OR: return "|";
        case TokenType::BITWISE_XOR: return "^^";
        case TokenType::BITWISE_NOT: return "~";
        case TokenType::SHIFT_LEFT: return "<<";
        case TokenType::SHIFT_RIGHT: return ">>";
        
        case TokenType::ADDRESS_OF: return "@";
        case TokenType::DEREFERENCE: return "*";
        
        case TokenType::CONDITIONAL: return "?";
        case TokenType::SCOPE_RESOLUTION: return "::";
        case TokenType::MEMBER_ACCESS: return ".";
        
        case TokenType::SEMICOLON: return ";";
        case TokenType::COLON: return ":";
        case TokenType::COMMA: return ",";
        
        case TokenType::PAREN_OPEN: return "(";
        case TokenType::PAREN_CLOSE: return ")";
        case TokenType::BRACE_OPEN: return "{";
        case TokenType::BRACE_CLOSE: return "}";
        case TokenType::BRACKET_OPEN: return "[";
        case TokenType::BRACKET_CLOSE: return "]";
        
        case TokenType::ARROW: return "->";
        case TokenType::RANGE: return "..";
        
        case TokenType::LINE_COMMENT: return "LINE_COMMENT";
        case TokenType::BLOCK_COMMENT: return "BLOCK_COMMENT";
        case TokenType::ERROR: return "ERROR";
        case TokenType::WHITESPACE: return "WHITESPACE";
        case TokenType::NEWLINE: return "NEWLINE";
        
        default: return "UNKNOWN";
    }
}

TokenType getKeywordType(std::string_view text) {
    auto it = keywordMap.find(text);
    if (it != keywordMap.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

bool isIdentifierStart(char c) {
    return std::isalpha(c) || c == '_';
}

bool isIdentifierContinue(char c) {
    return std::isalnum(c) || c == '_';
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

std::string processEscapeSequences(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            char next = text[i + 1];
            switch (next) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '\"': result += '\"'; break;
                case '\'': result += '\''; break;
                case '0': result += '\0'; break;
                case 'x': {
                    // Hex escape sequence \xHH
                    if (i + 3 < text.size() && isHexDigit(text[i + 2]) && isHexDigit(text[i + 3])) {
                        char hex_str[3] = {text[i + 2], text[i + 3], '\0'};
                        result += static_cast<char>(std::strtol(hex_str, nullptr, 16));
                        i += 2; // Skip extra characters
                    } else {
                        result += next; // Invalid hex escape, treat as literal
                    }
                    break;
                }
                case 'u': {
                    // Unicode escape sequence \uHHHH
                    if (i + 5 < text.size()) {
                        bool valid = true;
                        for (int j = 2; j <= 5; ++j) {
                            if (!isHexDigit(text[i + j])) {
                                valid = false;
                                break;
                            }
                        }
                        if (valid) {
                            char hex_str[5] = {text[i + 2], text[i + 3], text[i + 4], text[i + 5], '\0'};
                            int unicode_value = std::strtol(hex_str, nullptr, 16);
                            // Simple ASCII conversion (for now)
                            if (unicode_value < 128) {
                                result += static_cast<char>(unicode_value);
                            } else {
                                result += '?'; // Placeholder for non-ASCII
                            }
                            i += 4; // Skip extra characters
                        } else {
                            result += next; // Invalid unicode escape, treat as literal
                        }
                    } else {
                        result += next; // Invalid unicode escape, treat as literal
                    }
                    break;
                }
                default:
                    result += next; // Unknown escape sequence, treat as literal
                    break;
            }
            ++i; // Skip the escaped character
        } else {
            result += text[i];
        }
    }
    
    return result;
}

long long parseIntegerLiteral(std::string_view text, int base) {
    std::string temp(text);
    char* end;
    return std::strtoll(temp.c_str(), &end, base);
}

double parseFloatLiteral(std::string_view text) {
    std::string temp(text);
    return std::strtod(temp.c_str(), nullptr);
}

} // namespace lexer
} // namespace flux