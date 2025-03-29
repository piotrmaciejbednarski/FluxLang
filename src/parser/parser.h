#pragma once

#include <memory>
#include <vector>
#include <string_view>
#include <functional>
#include <unordered_map>

#include "../common/source.h"
#include "../common/error.h"
#include "../lexer/token.h"
#include "../lexer/tokenizer.h"
#include "ast.h"

namespace flux {
namespace parser {

// Parser class for the Flux language
class Parser {
public:
    // Constructor with tokenizer
    Parser(lexer::Tokenizer& tokenizer);
    
    // Parse a complete program
    std::unique_ptr<Program> parseProgram();
    
    // Get any errors that occurred during parsing
    const common::ErrorCollector& errors() const { return errors_; }
    
    // Check if there were errors during parsing
    bool hasErrors() const { return errors_.hasErrors(); }

private:
    // Tokenizer for getting tokens
    lexer::Tokenizer& tokenizer_;
    
    // Current token
    lexer::Token current_;
    
    // Previous token
    lexer::Token previous_;
    
    // Error collector
    common::ErrorCollector errors_;
    
    // Panic mode flag (for error recovery)
    bool panicMode_;

    // Flag for handling anonymous blocks
    bool inFunctionBody_ = false;

    // Logical control flag
    bool inControlStructure_ = false;

    // Flag cause we're fucking stupid
    bool inObjectBody_ = false;
    
    // Synchronization points for error recovery
    std::unordered_map<lexer::TokenType, bool> syncPoints_;
    
    // Token handling
    lexer::Token advance();
    void advanceWithGuard(const char* context);
    bool check(lexer::TokenType type) const;
    bool match(lexer::TokenType type);
    bool match(std::initializer_list<lexer::TokenType> types);
    lexer::Token consume(lexer::TokenType type, std::string_view message);
    void dumpNextToken();
    void resetParsingState();
    void synchronize();
    
    // Error reporting
    void error(std::string_view message);
    void error(const lexer::Token& token, std::string_view message);
    lexer::Token errorToken(std::string_view message);
    
    // Parsing declarations
    std::unique_ptr<Decl> declaration();
    std::unique_ptr<Decl> namespaceDeclaration();
    std::unique_ptr<Decl> classDeclaration();
    std::unique_ptr<Decl> objectDeclaration();
    std::unique_ptr<Decl> structDeclaration();
    std::unique_ptr<Decl> functionDeclaration();
    std::unique_ptr<Decl> variableDeclaration(bool isConst);
    std::unique_ptr<Decl> importDeclaration();
    std::unique_ptr<Decl> usingDeclaration();
    std::unique_ptr<Decl> operatorDeclaration();
    std::unique_ptr<Decl> templateDeclaration();
    std::unique_ptr<Decl> enumDeclaration();
    std::unique_ptr<Decl> typeDeclaration();
    std::unique_ptr<Decl> dataDeclaration();
    std::unique_ptr<Decl> asmDeclaration();
    
    // Parsing statements
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> dataTypeStatement();
    std::unique_ptr<Stmt> expressionStatement();
    std::unique_ptr<Stmt> anonymousBlockStatement();
    std::unique_ptr<Stmt> blockStatement();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> doWhileStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> forStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> breakStatement();
    std::unique_ptr<Stmt> continueStatement();
    std::unique_ptr<Stmt> throwStatement();
    std::unique_ptr<Stmt> tryStatement();
    std::unique_ptr<Stmt> switchStatement();
    std::unique_ptr<Stmt> variableStatement();
    
    // Parsing expressions
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> ternary();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> bitwiseOr();
    std::unique_ptr<Expr> bitwiseXor();
    std::unique_ptr<Expr> bitwiseAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> bitShift();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> exponentiation();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> postfix();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> qualifiedIdentifier();
    std::unique_ptr<Expr> parseSizeOfExpr();
    std::unique_ptr<Expr> parseTypeOfExpr();
    std::unique_ptr<Expr> parseOpExpr();
    
    // Parsing type expressions
    std::unique_ptr<TypeExpr> type();
    std::unique_ptr<TypeExpr> namedType();
    std::unique_ptr<TypeExpr> arrayType(std::unique_ptr<TypeExpr> elementType);
    std::unique_ptr<TypeExpr> pointerType(std::unique_ptr<TypeExpr> pointeeType);
    std::unique_ptr<TypeExpr> functionType();
    std::unique_ptr<TypeExpr> dataType();
    std::unique_ptr<TypeExpr> qualifiedType();
    
    // Helpers
    common::SourceRange makeRange(const lexer::Token& start, const lexer::Token& end) const;
    common::SourceRange makeRange(const lexer::Token& token) const;
    common::SourceRange makeRange(const common::SourcePosition& start, const common::SourcePosition& end) const;
    
    // Parse a delimited list of items
    template<typename T>
    std::vector<T> parseDelimitedList(
        lexer::TokenType delimiter,
        lexer::TokenType end,
        std::function<T()> parseItem);
};

} // namespace parser
} // namespace flux