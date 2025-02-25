#ifndef FLUX_PARSER_H
#define FLUX_PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include "lexer.h"
#include "ast.h"

namespace flux {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, const Token& token);
    
    const Token& getToken() const { return token; }
    
private:
    Token token;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    
    // Entry point for parsing
    std::pair<std::shared_ptr<Program>, size_t> parse();

private:
    // Parsing methods for different language constructs
    std::shared_ptr<Declaration> parseDeclaration();
    std::shared_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::shared_ptr<VariableDeclaration> parseVariableDeclaration();
    std::shared_ptr<StructDeclaration> parseStructDeclaration();
    std::shared_ptr<ClassDeclaration> parseClassDeclaration();
    std::shared_ptr<NamespaceDeclaration> parseNamespaceDeclaration();
    std::shared_ptr<OperatorDeclaration> parseOperatorDeclaration();
    std::shared_ptr<ObjectDeclaration> parseObjectDeclaration();
    
    // Statement parsing
    std::shared_ptr<Statement> parseStatement();
    std::shared_ptr<BlockStatement> parseBlock();
    std::shared_ptr<IfStatement> parseIfStatement();
    std::shared_ptr<WhileStatement> parseWhileStatement();
    std::shared_ptr<ForStatement> parseForStatement();
    std::shared_ptr<ReturnStatement> parseReturnStatement();
    std::shared_ptr<BreakStatement> parseBreakStatement();
    std::shared_ptr<ContinueStatement> parseContinueStatement();
    std::shared_ptr<ExpressionStatement> parseExpressionStatement();
    
    // Expression parsing
    std::shared_ptr<Expression> parseExpression();
    std::shared_ptr<Expression> parseAssignmentExpression();
    std::shared_ptr<Expression> parseLogicalOrExpression();
    std::shared_ptr<Expression> parseLogicalAndExpression();
    std::shared_ptr<Expression> parseBitwiseExpression();
    std::shared_ptr<Expression> parseEqualityExpression();
    std::shared_ptr<Expression> parseComparisonExpression();
    std::shared_ptr<Expression> parseAdditiveExpression();
    std::shared_ptr<Expression> parseMultiplicativeExpression();
    std::shared_ptr<Expression> parseUnaryExpression();
    std::shared_ptr<Expression> parsePostfixExpression();
    std::shared_ptr<Expression> parsePrimaryExpression();
    
    // Type parsing
    std::shared_ptr<Type> parseType();
    std::shared_ptr<Type> parseTypeModifiers(std::shared_ptr<Type> baseType);
    
    // Token consumption and error handling
    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    bool isAtEnd() const;
    
    Token advance();
    Token peek() const;
    Token previous() const;
    
    Token consume(TokenType type, const std::string& message);
    
    ParseError error(const Token& token, const std::string& message);
    void synchronize();
    
    // Token stream state
    std::vector<Token> tokens;
    size_t current = 0;
};

} // namespace flux

#endif // FLUX_PARSER_H
