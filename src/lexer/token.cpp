#include "token.h"
#include <unordered_map>

namespace flux {
namespace lexer {

std::string_view tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::BINARY_LITERAL: return "BINARY_LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        
        // Keywords
        case TokenType::AUTO: return "AUTO";
        case TokenType::AS: return "AS";
        case TokenType::ASM: return "ASM";
        case TokenType::ASSERT: return "ASSERT";
        case TokenType::AND: return "AND";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CASE: return "CASE";
        case TokenType::CATCH: return "CATCH";
        case TokenType::CONST: return "CONST";
        case TokenType::CONTINUE: return "CONTINUE";
        case TokenType::DATA: return "DATA";
        case TokenType::DEF: return "DEF";
        case TokenType::DEFAULT: return "DEFAULT";
        case TokenType::DO: return "DO";
        case TokenType::ELSE: return "ELSE";
        case TokenType::ENUM: return "ENUM";
        case TokenType::FOR: return "FOR";
        case TokenType::IF: return "IF";
        case TokenType::IMPORT: return "IMPORT";
        case TokenType::IN: return "IN";
        case TokenType::IS: return "IS";
        case TokenType::NAMESPACE: return "NAMESPACE";
        case TokenType::NOT: return "NOT";
        case TokenType::OBJECT: return "OBJECT";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::OR: return "OR";
        case TokenType::RETURN: return "RETURN";
        case TokenType::SIGNED: return "SIGNED";
        case TokenType::SIZEOF: return "SIZEOF";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::SUPER: return "SUPER";
        case TokenType::SWITCH: return "SWITCH";
        case TokenType::TEMPLATE: return "TEMPLATE";
        case TokenType::THIS: return "THIS";
        case TokenType::THROW: return "THROW";
        case TokenType::TRY: return "TRY";
        case TokenType::TYPEOF: return "TYPEOF";
        case TokenType::UNSIGNED: return "UNSIGNED";
        case TokenType::USING: return "USING";
        case TokenType::VOID: return "VOID";
        case TokenType::VOLATILE: return "VOLATILE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::XOR: return "XOR";
        
        // Operators
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::MODULO: return "MODULO";
        case TokenType::POWER: return "POWER";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case TokenType::DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case TokenType::MODULO_ASSIGN: return "MODULO_ASSIGN";
        case TokenType::POWER_ASSIGN: return "POWER_ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LOGICAL_AND: return "LOGICAL_AND";
        case TokenType::LOGICAL_OR: return "LOGICAL_OR";
        case TokenType::LOGICAL_NOT: return "LOGICAL_NOT";
        case TokenType::BITWISE_AND: return "BITWISE_AND";
        case TokenType::BITWISE_OR: return "BITWISE_OR";
        case TokenType::BITWISE_XOR: return "BITWISE_XOR";
        case TokenType::BITWISE_NOT: return "BITWISE_NOT";
        case TokenType::SHIFT_LEFT: return "SHIFT_LEFT";
        case TokenType::SHIFT_RIGHT: return "SHIFT_RIGHT";
        case TokenType::SHIFT_LEFT_ASSIGN: return "SHIFT_LEFT_ASSIGN";
        case TokenType::SHIFT_RIGHT_ASSIGN: return "SHIFT_RIGHT_ASSIGN";
        case TokenType::BITWISE_AND_ASSIGN: return "BITWISE_AND_ASSIGN";
        case TokenType::BITWISE_OR_ASSIGN: return "BITWISE_OR_ASSIGN";
        case TokenType::BITWISE_XOR_ASSIGN: return "BITWISE_XOR_ASSIGN";
        case TokenType::INCREMENT: return "INCREMENT";
        case TokenType::DECREMENT: return "DECREMENT";
        
        // Punctuation
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        case TokenType::SCOPE_RESOLUTION: return "SCOPE_RESOLUTION";
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::COLON: return "COLON";
        case TokenType::ARROW: return "ARROW";
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE: return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenType::ADDRESS_OF: return "ADDRESS_OF";
        case TokenType::DEREFERENCE: return "DEREFERENCE";
        
        // I-string tokens
        case TokenType::I_STRING_START: return "I_STRING_START";
        case TokenType::I_STRING_TEXT: return "I_STRING_TEXT";
        case TokenType::I_STRING_EXPR_START: return "I_STRING_EXPR_START";
        case TokenType::I_STRING_EXPR_END: return "I_STRING_EXPR_END";
        case TokenType::I_STRING_END: return "I_STRING_END";
        
        // Other
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::WHITESPACE: return "WHITESPACE";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::ERROR: return "ERROR";
        
        default: return "UNKNOWN";
    }
}

bool isKeyword(TokenType type) {
    return type >= TokenType::AUTO && type <= TokenType::XOR;
}

bool isOperator(TokenType type) {
    return (type >= TokenType::PLUS && type <= TokenType::DECREMENT) ||
           type == TokenType::ADDRESS_OF || type == TokenType::DEREFERENCE;
}

bool isLiteral(TokenType type) {
    return type >= TokenType::INTEGER_LITERAL && type <= TokenType::BINARY_LITERAL;
}

KeywordTable::KeywordTable() {
    // Initialize the keyword map based on the specification
    keywords_["auto"] = TokenType::AUTO;
    keywords_["as"] = TokenType::AS;
    keywords_["asm"] = TokenType::ASM;
    keywords_["assert"] = TokenType::ASSERT;
    keywords_["and"] = TokenType::AND;
    keywords_["break"] = TokenType::BREAK;
    keywords_["case"] = TokenType::CASE;
    keywords_["catch"] = TokenType::CATCH;
    keywords_["const"] = TokenType::CONST;
    keywords_["continue"] = TokenType::CONTINUE;
    keywords_["data"] = TokenType::DATA;
    keywords_["def"] = TokenType::DEF;
    keywords_["default"] = TokenType::DEFAULT;
    keywords_["do"] = TokenType::DO;
    keywords_["else"] = TokenType::ELSE;
    keywords_["enum"] = TokenType::ENUM;
    keywords_["for"] = TokenType::FOR;
    keywords_["if"] = TokenType::IF;
    keywords_["import"] = TokenType::IMPORT;
    keywords_["in"] = TokenType::IN;
    keywords_["is"] = TokenType::IS;
    keywords_["namespace"] = TokenType::NAMESPACE;
    keywords_["not"] = TokenType::NOT;
    keywords_["object"] = TokenType::OBJECT;
    keywords_["operator"] = TokenType::OPERATOR;
    keywords_["or"] = TokenType::OR;
    keywords_["return"] = TokenType::RETURN;
    keywords_["signed"] = TokenType::SIGNED;
    keywords_["sizeof"] = TokenType::SIZEOF;
    keywords_["struct"] = TokenType::STRUCT;
    keywords_["super"] = TokenType::SUPER;
    keywords_["switch"] = TokenType::SWITCH;
    keywords_["template"] = TokenType::TEMPLATE;
    keywords_["this"] = TokenType::THIS;
    keywords_["throw"] = TokenType::THROW;
    keywords_["try"] = TokenType::TRY;
    keywords_["typeof"] = TokenType::TYPEOF;
    keywords_["unsigned"] = TokenType::UNSIGNED;
    keywords_["using"] = TokenType::USING;
    keywords_["void"] = TokenType::VOID;
    keywords_["volatile"] = TokenType::VOLATILE;
    keywords_["while"] = TokenType::WHILE;
    keywords_["xor"] = TokenType::XOR;
}

TokenType KeywordTable::lookup(std::string_view text) const {
    auto it = keywords_.find(text);
    return (it != keywords_.end()) ? it->second : TokenType::IDENTIFIER;
}

bool KeywordTable::isKeyword(std::string_view text) const {
    return keywords_.find(text) != keywords_.end();
}

// Global keyword table instance
const KeywordTable keywordTable;

} // namespace lexer
} // namespace flux