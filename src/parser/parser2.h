#pragma once

#include "ast.h"
#include "../lexer/tokenizer.h"
#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace flux {
namespace parser {

class Parser {
public:
    explicit Parser(std::unique_ptr<lexer::Tokenizer> tokenizer, common::Arena& arena = common::Arena::defaultArena());
    
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<Decl> parseDeclaration();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<TypeExpr> parseTypeExpression();
    
    const common::ErrorCollector& errors() const { return errors_; }
    bool hasErrors() const { return errors_.hasErrors(); }
    const lexer::Token& current() const { return current_; }
    const lexer::Token& previous() const { return previous_; }

private:
    std::unique_ptr<lexer::Tokenizer> tokenizer_;
    lexer::Token current_;
    lexer::Token previous_;
    common::ErrorCollector errors_;
    common::Arena& arena_;
    
    void advance();
    bool check(lexer::TokenType type) const;
    bool match(lexer::TokenType type);
    bool match(std::initializer_list<lexer::TokenType> types);
    lexer::Token consume(lexer::TokenType type, std::string_view message);
    bool isAtEnd() const;
    
    void synchronize();
    void error(std::string_view message);
    void error(const lexer::Token& token, std::string_view message);
    
    std::unique_ptr<Decl> parseImportDecl();
    std::unique_ptr<Decl> parseUsingDecl();
    std::unique_ptr<Decl> parseDataDecl();
    std::unique_ptr<Decl> parseNamespaceDecl();
    std::unique_ptr<Decl> parseObjectDecl();
    std::unique_ptr<Decl> parseStructDecl();
    std::unique_ptr<Decl> parseEnumDecl();
    std::unique_ptr<Decl> parseFunctionDecl();
    std::unique_ptr<Decl> parseVarDecl();
    std::unique_ptr<Decl> parseOperatorDecl();
    std::unique_ptr<Decl> parseTemplateDecl();
    std::unique_ptr<Decl> parseAsmDecl();
    
    std::unique_ptr<Stmt> parseBlockStmt();
    std::unique_ptr<Stmt> parseExprStmt();
    std::unique_ptr<Stmt> parseVarStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseDoWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseBreakStmt();
    std::unique_ptr<Stmt> parseContinueStmt();
    std::unique_ptr<Stmt> parseThrowStmt();
    std::unique_ptr<Stmt> parseTryStmt();
    std::unique_ptr<Stmt> parseSwitchStmt();
    std::unique_ptr<Stmt> parseAssertStmt();
    
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseTernary();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseBitwiseOr();
    std::unique_ptr<Expr> parseBitwiseXor();
    std::unique_ptr<Expr> parseBitwiseAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseShift();
    std::unique_ptr<Expr> parseAddition();
    std::unique_ptr<Expr> parseMultiplication();
    std::unique_ptr<Expr> parseExponentiation();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    
    std::unique_ptr<Expr> parseArrayLiteral();
    std::unique_ptr<Expr> parseDictLiteral();
    std::unique_ptr<Expr> parseIString();
    std::unique_ptr<Expr> parseCast();
    std::unique_ptr<Expr> parseSizeOf();
    std::unique_ptr<Expr> parseTypeOf();
    std::unique_ptr<Expr> parseOp();
    
    std::unique_ptr<TypeExpr> parseNamedType();
    std::unique_ptr<TypeExpr> parseArrayType();
    std::unique_ptr<TypeExpr> parsePointerType();
    std::unique_ptr<TypeExpr> parseFunctionType();
    std::unique_ptr<TypeExpr> parseDataType();
    std::unique_ptr<TypeExpr> parseTemplateType();
    
    std::vector<FunctionDecl::Parameter> parseFunctionParameters();
    std::vector<std::string_view> parseTemplateParameters();
    std::vector<std::unique_ptr<Expr>> parseExpressionList();
    std::vector<std::unique_ptr<TypeExpr>> parseTypeList();
    
    common::SourceRange getCurrentRange() const;
    common::SourceRange getRangeFrom(const common::SourcePosition& start) const;
};

} // namespace parser
} // namespace flux