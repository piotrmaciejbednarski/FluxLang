#pragma once

#include "ast.h"
#include "../lexer/tokenizer.h"
#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include <vector>
#include <memory>

namespace flux {
namespace parser {

class Parser {
public:
    Parser(std::vector<lexer::Token> tokens, std::shared_ptr<common::Source> source, common::Arena& arena);
    ~Parser() = default;
    
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;
    
    Program* parseProgram();
    
    common::ErrorCollector& errorCollector() { return errorCollector_; }
    bool hasErrors() const { return errorCollector_.hasErrors(); }

private:
    std::vector<lexer::Token> tokens_;
    std::shared_ptr<common::Source> source_;
    common::Arena& arena_;
    ASTFactory factory_;
    common::ErrorCollector errorCollector_;
    size_t current_;
    
    // Token operations
    const lexer::Token& peek() const;
    const lexer::Token& peekNext() const;
    const lexer::Token& previous() const;
    lexer::Token advance();
    bool match(lexer::TokenType type);
    lexer::Token consume(lexer::TokenType type);
    bool isAtEnd() const;
    
    // Error handling
    void reportError(common::ErrorCode code, std::string_view message);
    lexer::Token consumeOrError(lexer::TokenType type, std::string_view errorMessage);
    void synchronize();
    
    // Parsing methods
    Declaration* parseDeclaration();
    Declaration* parseImportDeclaration();
    Declaration* parseUsingDeclaration();
    Declaration* parseNamespaceDeclaration();
    Declaration* parseObjectDeclaration();
    Declaration* parseStructDeclaration();
    Declaration* parseFunctionDeclaration();
    Declaration* parseOperatorDeclaration();
    
    Statement* parseStatement();
    Statement* parseBlockStatement();
    Statement* parseExpressionStatement();
    Statement* parseVariableDeclaration();
    Statement* parseIfStatement();
    Statement* parseWhileStatement();
    Statement* parseForStatement();
    Statement* parseSwitchStatement();
    Statement* parseReturnStatement();
    Statement* parseBreakStatement();
    Statement* parseContinueStatement();
    Statement* parseThrowStatement();
    Statement* parseTryCatchStatement();
    Statement* parseAssemblyStatement();
    
    Expression* parseExpression();
    Expression* parseAssignmentExpression();
    Expression* parseTernaryExpression();
    Expression* parseBinaryExpression(int minPrecedence = 0);
    Expression* parseUnaryExpression();
    Expression* parsePostfixExpression();
    Expression* parsePrimaryExpression();
    
    Type* parseType();
    Type* parsePrimitiveType();
    Type* parseFunctionType();
    
    std::vector<Parameter> parseParameterList();
    std::vector<Expression*> parseExpressionList();
    std::vector<Type*> parseTypeList();
};

} // namespace parser
} // namespace flux