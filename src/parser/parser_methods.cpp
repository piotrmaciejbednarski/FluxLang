/**
 * @file parser_methods.cpp
 * @brief Implementations for Parser class methods
 */

#include "parser/parser.hpp"
#include <stdexcept>

namespace flux {

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), current(0) {}

bool Parser::isAtEnd() const {
    return current >= static_cast<int>(tokens.size()) || 
           tokens[current].type == TokenType::END_OF_FILE;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::peek() const {
    return tokens[current];
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return tokens[current].type == type;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();

    throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    // For now, we'll just throw a runtime error 
    // In a real implementation, you'd want more robust error handling
    throw ParseError(message + " at line " + std::to_string(token.line));
}

void Parser::synchronize() {
    // Move past the current token
    advance();

    // Skip tokens until we find a likely statement boundary
    while (!isAtEnd()) {
        // If we just consumed a semicolon, we're likely at a statement boundary
        if (previous().type == TokenType::SEMICOLON) return;

        // These tokens often indicate the start of a new statement or declaration
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::FUNCTION:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}

std::vector<FunctionDeclarationStmt::Parameter> Parser::parseParameters() {
    std::vector<FunctionDeclarationStmt::Parameter> parameters;

    // No parameters case
    if (check(TokenType::RIGHT_PAREN)) {
        return parameters;
    }

    do {
        // Parse parameter type
        std::shared_ptr<Type> paramType = parseType();

        // Parse parameter name
        Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name.");

        parameters.push_back({paramType, paramName});
    } while (match({TokenType::COMMA}));

    return parameters;
}

std::vector<ExprPtr> Parser::parseArguments() {
    std::vector<ExprPtr> arguments;

    // No arguments case
    if (check(TokenType::RIGHT_PAREN)) {
        return arguments;
    }

    do {
        arguments.push_back(expression());
    } while (match({TokenType::COMMA}));

    return arguments;
}

std::vector<StmtPtr> Parser::parseClassMembers() {
    std::vector<StmtPtr> members;

    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        // Allow various types of members: methods, variables, etc.
        if (check(TokenType::FUNCTION)) {
            members.push_back(functionDeclaration());
        } else if (check(TokenType::INT) || check(TokenType::FLOAT) || 
                   check(TokenType::CHAR) || check(TokenType::BOOL) ||
                   check(TokenType::IDENTIFIER)) {
            members.push_back(varDeclaration());
        } else {
            throw error(peek(), "Expected method or variable declaration in class/object.");
        }
    }

    return members;
}

} // namespace flux
