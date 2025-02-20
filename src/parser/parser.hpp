/**
 * @file parser.hpp
 * @brief Parser for Flux language
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

#include "../lexer/token.hpp"
#include "statements.hpp"
#include "expressions.hpp"
#include "ast.hpp"

namespace flux {

/**
 * @brief Parser exception
 */
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Parser for Flux language
 */
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    
    /**
     * @brief Parse a complete program
     * @return Parsed AST
     */
    std::unique_ptr<Program> parse();

private:
    // Statement parsing methods
    StmtPtr declaration();
    StmtPtr classDeclaration();
    StmtPtr objectDeclaration();
    StmtPtr namespaceDeclaration();
    StmtPtr functionDeclaration();
    StmtPtr operatorDeclaration();
    StmtPtr structDeclaration();
    StmtPtr varDeclaration();
    StmtPtr statement();
    StmtPtr expressionStatement();
    StmtPtr blockStatement(bool checkVolatile = true);
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr forStatement();
    StmtPtr whenStatement();
    StmtPtr asmStatement();
    StmtPtr returnStatement();
    StmtPtr breakStatement();
    StmtPtr continueStatement();
    StmtPtr lockStatement();
    
    // Expression parsing methods (with precedence climbing)
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();
    
    // Type parsing methods
    std::shared_ptr<Type> parseType();
    std::shared_ptr<PrimitiveType> parsePrimitiveType();
    int parseBitWidth();
    
    // Utility methods
    bool isAtEnd() const;
    Token advance();
    Token previous() const;
    Token peek() const;
    bool check(flux::TokenType type) const;
    bool match(const std::vector<flux::TokenType>& types);
    Token consume(flux::TokenType type, const std::string& message);
    ParseError error(const Token& token, const std::string& message);
    void synchronize();
    
    // Helper methods for parsing specific constructs
    std::vector<FunctionDeclarationStmt::Parameter> parseParameters();
    std::vector<ExprPtr> parseArguments();
    std::vector<StmtPtr> parseClassMembers();
    ExprPtr finishCall(ExprPtr callee);
    
    // State
    std::vector<Token> tokens;
    int current = 0;
};

} // namespace flux
