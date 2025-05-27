#include "parser2.h"
#include <algorithm>
#include <iostream>

namespace flux {
namespace parser {

Parser::Parser(std::vector<lexer::Token> tokens, std::shared_ptr<common::Source> source, common::Arena& arena)
    : tokens_(std::move(tokens)), source_(source), arena_(arena), factory_(arena), current_(0) {
}

Program* Parser::parseProgram() {
    std::vector<Declaration*> declarations;
    common::SourcePosition start = tokens_.empty() ? common::SourcePosition() : tokens_[0].range.start;
    
    while (!isAtEnd()) {
        try {
            Declaration* decl = parseDeclaration();
            if (decl) {
                declarations.push_back(decl);
            }
        } catch (const common::Error& e) {
            errorCollector_.addError(e);
            synchronize();
        }
    }
    
    common::SourcePosition end = tokens_.empty() ? common::SourcePosition() : previous().range.end;
    return factory_.createProgram(std::move(declarations), common::SourceRange(start, end));
}

const lexer::Token& Parser::peek() const {
    if (isAtEnd()) {
        static lexer::Token eofToken(lexer::TokenType::END_OF_FILE, "", common::SourceRange());
        return eofToken;
    }
    return tokens_[current_];
}

const lexer::Token& Parser::peekNext() const {
    if (current_ + 1 >= tokens_.size()) {
        static lexer::Token eofToken(lexer::TokenType::END_OF_FILE, "", common::SourceRange());
        return eofToken;
    }
    return tokens_[current_ + 1];
}

const lexer::Token& Parser::previous() const {
    if (current_ == 0) {
        return tokens_[0];
    }
    return tokens_[current_ - 1];
}

lexer::Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::match(lexer::TokenType type) {
    return check(type);
}

bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            return true;
        }
    }
    return false;
}

lexer::Token Parser::consume(lexer::TokenType type) {
    if (check(type)) {
        return advance();
    }
    return peek();
}

bool Parser::check(lexer::TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || peek().type == lexer::TokenType::END_OF_FILE;
}

void Parser::reportError(common::ErrorCode code, std::string_view message) {
    common::SourceRange range = isAtEnd() ? previous().range : peek().range;
    reportError(code, message, range);
}

void Parser::reportError(common::ErrorCode code, std::string_view message, const common::SourceRange& range) {
    output::SourceLocation location(
        source_->filename(),
        static_cast<int>(range.start.line),
        static_cast<int>(range.start.column)
    );
    errorCollector_.addError(code, message, location);
}

lexer::Token Parser::consumeOrError(lexer::TokenType type, std::string_view errorMessage) {
    if (check(type)) {
        return advance();
    }
    
    reportError(common::ErrorCode::UNEXPECTED_TOKEN, errorMessage);
    return peek();
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == lexer::TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case lexer::TokenType::OBJECT:
            case lexer::TokenType::STRUCT:
            case lexer::TokenType::DEF:
            case lexer::TokenType::NAMESPACE:
            case lexer::TokenType::IMPORT:
            case lexer::TokenType::USING:
            case lexer::TokenType::IF:
            case lexer::TokenType::WHILE:
            case lexer::TokenType::FOR:
            case lexer::TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

common::SourceRange Parser::makeRange(const common::SourcePosition& start) const {
    return common::SourceRange(start, previous().range.end);
}

common::SourceRange Parser::makeRange(const common::SourceRange& start, const common::SourceRange& end) const {
    return common::SourceRange(start.start, end.end);
}

Declaration* Parser::parseDeclaration() {
    if (match(lexer::TokenType::IMPORT)) {
        return parseImportDeclaration();
    }
    if (match(lexer::TokenType::USING)) {
        return parseUsingDeclaration();
    }
    if (match(lexer::TokenType::NAMESPACE)) {
        return parseNamespaceDeclaration();
    }
    if (match(lexer::TokenType::TEMPLATE)) {
        if (peekNext().type == lexer::TokenType::OBJECT) {
            return parseObjectDeclaration();
        } else if (peekNext().type == lexer::TokenType::DEF) {
            return parseFunctionDeclaration();
        }
    }
    if (match(lexer::TokenType::OBJECT)) {
        return parseObjectDeclaration();
    }
    if (match(lexer::TokenType::STRUCT)) {
        return parseStructDeclaration();
    }
    if (match(lexer::TokenType::DEF)) {
        return parseFunctionDeclaration();
    }
    if (match(lexer::TokenType::OPERATOR)) {
        return parseOperatorDeclaration();
    }
    
    reportError(common::ErrorCode::EXPECTED_DECLARATION, "Expected declaration");
    return nullptr;
}

Declaration* Parser::parseImportDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::IMPORT);
    
    if (!check(lexer::TokenType::STRING_LITERAL)) {
        reportError(common::ErrorCode::EXPECTED_EXPRESSION, "Expected module name");
        return nullptr;
    }
    
    std::string_view module = consume(lexer::TokenType::STRING_LITERAL).text;
    std::string_view alias = "";
    
    if (match(lexer::TokenType::AS)) {
        consume(lexer::TokenType::AS);
        if (!check(lexer::TokenType::IDENTIFIER)) {
            reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected alias identifier");
            return nullptr;
        }
        alias = consume(lexer::TokenType::IDENTIFIER).text;
    }
    
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after import declaration");
    
    return factory_.createImportDeclaration(module, alias, makeRange(start));
}

Declaration* Parser::parseUsingDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::USING);
    std::vector<std::string_view> names;
    
    do {
        if (!check(lexer::TokenType::IDENTIFIER)) {
            reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected identifier");
            return nullptr;
        }
        names.push_back(consume(lexer::TokenType::IDENTIFIER).text);
        
        if (match(lexer::TokenType::SCOPE_RESOLUTION)) {
            consume(lexer::TokenType::SCOPE_RESOLUTION);
            if (!check(lexer::TokenType::IDENTIFIER)) {
                reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected identifier after '::'");
                return nullptr;
            }
            std::string combined = std::string(names.back()) + "::" + std::string(consume(lexer::TokenType::IDENTIFIER).text);
            names.back() = arena_.alloc<std::string>(std::move(combined))->c_str();
        }
    } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after using declaration");
    
    return factory_.createUsingDeclaration(std::move(names), makeRange(start));
}

Declaration* Parser::parseNamespaceDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::NAMESPACE);
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected namespace name");
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    
    consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after namespace name");
    
    std::vector<Declaration*> declarations;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        Declaration* decl = parseDeclaration();
        if (decl) {
            declarations.push_back(decl);
        }
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after namespace body");
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after namespace declaration");
    
    return factory_.createNamespaceDeclaration(name, std::move(declarations), makeRange(start));
}

Declaration* Parser::parseObjectDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::OBJECT);
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected object name");
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    std::vector<std::string_view> inheritance;
    
    if (match(lexer::TokenType::LESS_THAN)) {
        consume(lexer::TokenType::LESS_THAN);
        do {
            if (!check(lexer::TokenType::IDENTIFIER)) {
                reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected base class name");
                return nullptr;
            }
            inheritance.push_back(consume(lexer::TokenType::IDENTIFIER).text);
        } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
        
        consumeOrError(lexer::TokenType::GREATER_THAN, "Expected '>' after inheritance list");
    }
    
    consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after object declaration");
    
    std::vector<Declaration*> members;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        if (match(lexer::TokenType::DEF)) {
            members.push_back(parseFunctionDeclaration());
        } else if (isTypeToken(peek().type)) {
            Statement* varDecl = parseVariableDeclaration();
            if (varDecl && varDecl->kind() == Statement::Kind::VARIABLE_DECLARATION) {
                VariableDeclaration* var = static_cast<VariableDeclaration*>(varDecl);
                FunctionDeclaration* getter = factory_.createFunctionDeclaration(
                    var->name(), {}, var->type(), nullptr, var->range()
                );
                members.push_back(getter);
            }
        } else {
            advance();
        }
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after object body");
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after object declaration");
    
    return factory_.createObjectDeclaration(name, std::move(members), std::move(inheritance), makeRange(start));
}

Declaration* Parser::parseStructDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::STRUCT);
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected struct name");
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    
    consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after struct name");
    
    std::vector<StructMember> members;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        Type* type = parseType();
        if (!type) {
            advance();
            continue;
        }
        
        if (!check(lexer::TokenType::IDENTIFIER)) {
            reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected member name");
            advance();
            continue;
        }
        
        std::string_view memberName = consume(lexer::TokenType::IDENTIFIER).text;
        members.emplace_back(type, memberName);
        
        consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after struct member");
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after struct body");
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after struct declaration");
    
    return factory_.createStructDeclaration(name, std::move(members), makeRange(start));
}

Declaration* Parser::parseFunctionDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::DEF);
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected function name");
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::vector<Parameter> parameters = parseParameterList();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    consumeOrError(lexer::TokenType::ARROW, "Expected '->' before return type");
    Type* returnType = parseType();
    
    Statement* body = nullptr;
    if (match(lexer::TokenType::LEFT_BRACE)) {
        body = parseBlockStatement();
    } else {
        consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after function declaration");
    }
    
    return factory_.createFunctionDeclaration(name, std::move(parameters), returnType, body, makeRange(start));
}

Declaration* Parser::parseOperatorDeclaration() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::OPERATOR);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'operator'");
    
    std::vector<Parameter> parameters = parseParameterList();
    
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    consumeOrError(lexer::TokenType::LEFT_BRACKET, "Expected '[' before operator symbol");
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected operator symbol");
        return nullptr;
    }
    
    std::string_view symbol = consume(lexer::TokenType::IDENTIFIER).text;
    
    consumeOrError(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after operator symbol");
    consumeOrError(lexer::TokenType::ARROW, "Expected '->' before return type");
    
    Type* returnType = parseType();
    Statement* body = parseBlockStatement();
    
    return factory_.createOperatorDeclaration(symbol, std::move(parameters), returnType, body, makeRange(start));
}

Statement* Parser::parseStatement() {
    if (isTypeToken(peek().type)) {
        return parseVariableDeclaration();
    }
    if (match(lexer::TokenType::LEFT_BRACE)) {
        return parseBlockStatement();
    }
    if (match(lexer::TokenType::IF)) {
        return parseIfStatement();
    }
    if (match(lexer::TokenType::WHILE)) {
        return parseWhileStatement();
    }
    if (match(lexer::TokenType::FOR)) {
        return parseForStatement();
    }
    if (match(lexer::TokenType::SWITCH)) {
        return parseSwitchStatement();
    }
    if (match(lexer::TokenType::RETURN)) {
        return parseReturnStatement();
    }
    if (match(lexer::TokenType::BREAK)) {
        return parseBreakStatement();
    }
    if (match(lexer::TokenType::CONTINUE)) {
        return parseContinueStatement();
    }
    if (match(lexer::TokenType::THROW)) {
        return parseThrowStatement();
    }
    if (match(lexer::TokenType::TRY)) {
        return parseTryCatchStatement();
    }
    if (match(lexer::TokenType::ASM)) {
        return parseAssemblyStatement();
    }
    
    return parseExpressionStatement();
}

Statement* Parser::parseBlockStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::LEFT_BRACE);
    std::vector<Statement*> statements;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        Statement* stmt = parseStatement();
        if (stmt) {
            statements.push_back(stmt);
        }
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after block");
    
    return factory_.createBlockStatement(std::move(statements), makeRange(start));
}

Statement* Parser::parseExpressionStatement() {
    common::SourcePosition start = peek().range.start;
    Expression* expr = parseExpression();
    
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after expression");
    
    return factory_.createExpressionStatement(expr, makeRange(start));
}

Statement* Parser::parseVariableDeclaration() {
    common::SourcePosition start = peek().range.start;
    bool isConst = match(lexer::TokenType::CONST);
    if (isConst) consume(lexer::TokenType::CONST);
    
    Type* type = parseType();
    if (!type) {
        reportError(common::ErrorCode::EXPECTED_TYPE, "Expected type in variable declaration");
        return nullptr;
    }
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected variable name");
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    Expression* initializer = nullptr;
    
    if (match(lexer::TokenType::ASSIGN)) {
        consume(lexer::TokenType::ASSIGN);
        initializer = parseExpression();
    }
    
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return factory_.createVariableDeclaration(type, name, initializer, isConst, makeRange(start));
}

Statement* Parser::parseIfStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::IF);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'if'");
    Expression* condition = parseExpression();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after if condition");
    
    Statement* thenStmt = parseStatement();
    Statement* elseStmt = nullptr;
    
    if (match(lexer::TokenType::ELSE)) {
        consume(lexer::TokenType::ELSE);
        elseStmt = parseStatement();
    }
    
    return factory_.createIfStatement(condition, thenStmt, elseStmt, makeRange(start));
}

Statement* Parser::parseWhileStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::WHILE);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    Expression* condition = parseExpression();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after while condition");
    
    Statement* body = parseStatement();
    
    return factory_.createWhileStatement(condition, body, makeRange(start));
}

Statement* Parser::parseForStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::FOR);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'for'");
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected variable name in for loop");
        return nullptr;
    }
    
    std::string_view variable = consume(lexer::TokenType::IDENTIFIER).text;
    
    consumeOrError(lexer::TokenType::IN, "Expected 'in' after for variable");
    Expression* iterable = parseExpression();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after for iterable");
    
    Statement* body = parseStatement();
    
    return factory_.createForStatement(variable, iterable, body, makeRange(start));
}

Statement* Parser::parseSwitchStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::SWITCH);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'switch'");
    Expression* expression = parseExpression();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after switch expression");
    
    consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after switch expression");
    
    std::vector<SwitchStatement::CaseClause> cases;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        if (match(lexer::TokenType::CASE)) {
            consume(lexer::TokenType::CASE);
            consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'case'");
            
            Expression* value = nullptr;
            if (!match(lexer::TokenType::DEFAULT)) {
                value = parseExpression();
            } else {
                consume(lexer::TokenType::DEFAULT);
            }
            
            consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after case value");
            consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after case");
            
            Statement* body = parseBlockStatement();
            cases.emplace_back(value, body);
        } else {
            advance();
        }
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after switch body");
    
    return factory_.createSwitchStatement(expression, std::move(cases), makeRange(start));
}

Statement* Parser::parseReturnStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::RETURN);
    
    Expression* value = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after return statement");
    
    return factory_.createReturnStatement(value, makeRange(start));
}

Statement* Parser::parseBreakStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::BREAK);
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after 'break'");
    return factory_.createBreakStatement(makeRange(start));
}

Statement* Parser::parseContinueStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::CONTINUE);
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after 'continue'");
    return factory_.createContinueStatement(makeRange(start));
}

Statement* Parser::parseThrowStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::THROW);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'throw'");
    Expression* value = parseExpression();
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after throw expression");
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after throw statement");
    
    return factory_.createThrowStatement(value, makeRange(start));
}

Statement* Parser::parseTryCatchStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::TRY);
    
    Statement* tryBlock = parseBlockStatement();
    
    consumeOrError(lexer::TokenType::CATCH, "Expected 'catch' after try block");
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'catch'");
    
    std::string_view catchVar = "";
    if (check(lexer::TokenType::IDENTIFIER)) {
        catchVar = consume(lexer::TokenType::IDENTIFIER).text;
    } else if (match(lexer::TokenType::AUTO)) {
        consume(lexer::TokenType::AUTO);
        if (check(lexer::TokenType::IDENTIFIER)) {
            catchVar = consume(lexer::TokenType::IDENTIFIER).text;
        }
    }
    
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after catch variable");
    Statement* catchBlock = parseBlockStatement();
    
    return factory_.createTryCatchStatement(tryBlock, catchVar, catchBlock, makeRange(start));
}

Statement* Parser::parseAssemblyStatement() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::ASM);
    
    consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'asm'");
    
    std::string code = "";
    while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        code += std::string(advance().text) + " ";
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after assembly code");
    consumeOrError(lexer::TokenType::SEMICOLON, "Expected ';' after assembly statement");
    
    std::string_view codeView = arena_.alloc<std::string>(std::move(code))->c_str();
    return factory_.createAssemblyStatement(codeView, makeRange(start));
}

Expression* Parser::parseExpression() {
    return parseAssignmentExpression();
}

Expression* Parser::parseAssignmentExpression() {
    Expression* expr = parseTernaryExpression();
    
    if (isAssignmentOperator(peek().type)) {
        lexer::TokenType op = consume(peek().type).type;
        Expression* right = parseAssignmentExpression();
        return factory_.createAssignmentExpression(
            tokenToAssignmentOperator(op), expr, right, expr->range()
        );
    }
    
    return expr;
}

Expression* Parser::parseTernaryExpression() {
    Expression* expr = parseBinaryExpression();
    
    if (match(lexer::TokenType::QUESTION)) {
        consume(lexer::TokenType::QUESTION);
        Expression* trueExpr = parseExpression();
        consumeOrError(lexer::TokenType::COLON, "Expected ':' after ternary true expression");
        Expression* falseExpr = parseExpression();
        return factory_.createTernaryExpression(expr, trueExpr, falseExpr, expr->range());
    }
    
    return expr;
}

Expression* Parser::parseBinaryExpression(int minPrecedence) {
    Expression* left = parseUnaryExpression();
    
    while (isBinaryOperator(peek().type)) {
        int precedence = getOperatorPrecedence(peek().type);
        if (precedence < minPrecedence) break;
        
        lexer::TokenType op = consume(peek().type).type;
        int nextMinPrec = isRightAssociative(op) ? precedence : precedence + 1;
        Expression* right = parseBinaryExpression(nextMinPrec);
        
        left = factory_.createBinaryExpression(
            tokenToBinaryOperator(op), left, right, left->range()
        );
    }
    
    return left;
}

Expression* Parser::parseUnaryExpression() {
    if (isUnaryOperator(peek().type)) {
        lexer::TokenType op = consume(peek().type).type;
        Expression* operand = parseUnaryExpression();
        return factory_.createUnaryExpression(
            tokenToUnaryOperator(op), operand, true, operand->range()
        );
    }
    
    return parsePostfixExpression();
}

Expression* Parser::parsePostfixExpression() {
    Expression* expr = parsePrimaryExpression();
    
    while (true) {
        if (match(lexer::TokenType::DOT)) {
            consume(lexer::TokenType::DOT);
            if (!check(lexer::TokenType::IDENTIFIER)) {
                reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected member name after '.'");
                break;
            }
            std::string_view member = consume(lexer::TokenType::IDENTIFIER).text;
            expr = factory_.createMemberAccessExpression(
                expr, member, MemberAccessExpression::AccessType::DOT, expr->range()
            );
        } else if (match(lexer::TokenType::SCOPE_RESOLUTION)) {
            consume(lexer::TokenType::SCOPE_RESOLUTION);
            if (!check(lexer::TokenType::IDENTIFIER)) {
                reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected member name after '::'");
                break;
            }
            std::string_view member = consume(lexer::TokenType::IDENTIFIER).text;
            expr = factory_.createMemberAccessExpression(
                expr, member, MemberAccessExpression::AccessType::SCOPE, expr->range()
            );
        } else if (match(lexer::TokenType::LEFT_BRACKET)) {
            consume(lexer::TokenType::LEFT_BRACKET);
            Expression* index = parseExpression();
            consumeOrError(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array index");
            expr = factory_.createArrayAccessExpression(expr, index, expr->range());
        } else if (match(lexer::TokenType::LEFT_PAREN)) {
            consume(lexer::TokenType::LEFT_PAREN);
            std::vector<Expression*> args = parseExpressionList();
            consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after function arguments");
            expr = factory_.createCallExpression(expr, std::move(args), expr->range());
        } else if (match({lexer::TokenType::INCREMENT, lexer::TokenType::DECREMENT})) {
            lexer::TokenType op = consume(peek().type).type;
            UnaryExpression::Operator unaryOp = (op == lexer::TokenType::INCREMENT) ?
                UnaryExpression::Operator::POST_INCREMENT : UnaryExpression::Operator::POST_DECREMENT;
            expr = factory_.createUnaryExpression(unaryOp, expr, false, expr->range());
        } else {
            break;
        }
    }
    
    return expr;
}

Expression* Parser::parsePrimaryExpression() {
    common::SourcePosition start = peek().range.start;
    
    if (match({lexer::TokenType::INTEGER_LITERAL, lexer::TokenType::FLOAT_LITERAL, 
               lexer::TokenType::STRING_LITERAL, lexer::TokenType::BINARY_LITERAL})) {
        lexer::Token token = consume(peek().type);
        return factory_.createLiteralExpression(
            tokenToLiteralKind(token.type), token.text, makeRange(start)
        );
    }
    
    if (match(lexer::TokenType::IDENTIFIER)) {
        std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
        return factory_.createIdentifierExpression(name, makeRange(start));
    }
    
    if (match(lexer::TokenType::LEFT_PAREN)) {
        consume(lexer::TokenType::LEFT_PAREN);
        if (check(lexer::TokenType::IDENTIFIER)) {
            Type* type = parseType();
            consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after cast type");
            Expression* expr = parseUnaryExpression();
            return factory_.createCastExpression(type, expr, makeRange(start));
        } else {
            Expression* expr = parseExpression();
            consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after expression");
            return expr;
        }
    }
    
    if (match(lexer::TokenType::LEFT_BRACKET)) {
        consume(lexer::TokenType::LEFT_BRACKET);
        std::vector<Expression*> elements = parseExpressionList();
        consumeOrError(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array elements");
        return factory_.createArrayLiteralExpression(std::move(elements), makeRange(start));
    }
    
    if (match(lexer::TokenType::LEFT_BRACE)) {
        consume(lexer::TokenType::LEFT_BRACE);
        std::vector<DictLiteralExpression::KeyValuePair> pairs;
        
        if (!check(lexer::TokenType::RIGHT_BRACE)) {
            do {
                Expression* key = parseExpression();
                consumeOrError(lexer::TokenType::COLON, "Expected ':' after dictionary key");
                Expression* value = parseExpression();
                pairs.emplace_back(key, value);
            } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
        }
        
        consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after dictionary");
        return factory_.createDictLiteralExpression(std::move(pairs), makeRange(start));
    }
    
    if (match(lexer::TokenType::SIZEOF)) {
        consume(lexer::TokenType::SIZEOF);
        consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'sizeof'");
        Type* type = parseType();
        consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after sizeof type");
        return factory_.createSizeofExpression(type, makeRange(start));
    }
    
    if (match(lexer::TokenType::TYPEOF)) {
        consume(lexer::TokenType::TYPEOF);
        consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'typeof'");
        Expression* expr = parseExpression();
        consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after typeof expression");
        return factory_.createTypeofExpression(expr, makeRange(start));
    }
    
    if (match(lexer::TokenType::ASM)) {
        consume(lexer::TokenType::ASM);
        consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'asm'");
        
        std::string code = "";
        while (!check(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
            code += std::string(advance().text) + " ";
        }
        
        consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after assembly code");
        
        std::string_view codeView = arena_.alloc<std::string>(std::move(code))->c_str();
        return factory_.createAssemblyExpression(codeView, makeRange(start));
    }
    
    reportError(common::ErrorCode::EXPECTED_EXPRESSION, "Expected expression");
    return nullptr;
}

Type* Parser::parseType() {
    common::SourcePosition start = peek().range.start;
    
    Type* type = parsePrimitiveType();
    if (!type) return nullptr;
    
    while (true) {
        if (match(lexer::TokenType::LEFT_BRACKET)) {
            consume(lexer::TokenType::LEFT_BRACKET);
            Expression* size = nullptr;
            if (!check(lexer::TokenType::RIGHT_BRACKET)) {
                size = parseExpression();
            }
            consumeOrError(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array size");
            type = factory_.createArrayType(type, size, makeRange(start));
        } else if (match(lexer::TokenType::MULTIPLY)) {
            consume(lexer::TokenType::MULTIPLY);
            type = factory_.createPointerType(type, makeRange(start));
        } else {
            break;
        }
    }
    
    return type;
}

Type* Parser::parsePrimitiveType() {
    common::SourcePosition start = peek().range.start;
    
    if (match(lexer::TokenType::VOID)) {
        consume(lexer::TokenType::VOID);
        return factory_.createPrimitiveType("void", makeRange(start));
    }
    
    if (match(lexer::TokenType::AUTO)) {
        consume(lexer::TokenType::AUTO);
        return factory_.createPrimitiveType("auto", makeRange(start));
    }
    
    bool isSigned = true;
    if (match(lexer::TokenType::SIGNED)) {
        consume(lexer::TokenType::SIGNED);
        isSigned = true;
    } else if (match(lexer::TokenType::UNSIGNED)) {
        consume(lexer::TokenType::UNSIGNED);
        isSigned = false;
    }
    
    if (match(lexer::TokenType::DATA)) {
        consume(lexer::TokenType::DATA);
        consumeOrError(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
        Expression* size = parseExpression();
        consumeOrError(lexer::TokenType::RIGHT_BRACE, "Expected '}' after data size");
        return factory_.createDataType(size, isSigned, makeRange(start));
    }
    
    if (match(lexer::TokenType::DEF)) {
        return parseFunctionType();
    }
    
    if (check(lexer::TokenType::IDENTIFIER)) {
        std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
        
        if (match(lexer::TokenType::LESS_THAN)) {
            consume(lexer::TokenType::LESS_THAN);
            std::vector<Type*> templateArgs = parseTypeList();
            consumeOrError(lexer::TokenType::GREATER_THAN, "Expected '>' after template arguments");
            return factory_.createObjectType(name, std::move(templateArgs), makeRange(start));
        }
        
        return factory_.createPrimitiveType(name, makeRange(start));
    }
    
    return nullptr;
}

Type* Parser::parseArrayType(Type* elementType) {
    common::SourcePosition start = previous().range.start;
    
    Expression* size = nullptr;
    if (!check(lexer::TokenType::RIGHT_BRACKET)) {
        size = parseExpression();
    }
    
    consumeOrError(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array size");
    
    return factory_.createArrayType(elementType, size, makeRange(start));
}

Type* Parser::parsePointerType(Type* pointeeType) {
    common::SourcePosition start = previous().range.start;
    return factory_.createPointerType(pointeeType, makeRange(start));
}

Type* Parser::parseFunctionType() {
    common::SourcePosition start = peek().range.start;
    consume(lexer::TokenType::DEF);
    
    consumeOrError(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'def'");
    
    std::vector<Type*> paramTypes;
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            Type* paramType = parseType();
            if (paramType) {
                paramTypes.push_back(paramType);
            }
        } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    }
    
    consumeOrError(lexer::TokenType::RIGHT_PAREN, "Expected ')' after function parameters");
    consumeOrError(lexer::TokenType::ARROW, "Expected '->' after function parameters");
    
    Type* returnType = parseType();
    
    return factory_.createFunctionType(std::move(paramTypes), returnType, makeRange(start));
}

Type* Parser::parseObjectType() {
    common::SourcePosition start = peek().range.start;
    
    if (!check(lexer::TokenType::IDENTIFIER)) {
        return nullptr;
    }
    
    std::string_view name = consume(lexer::TokenType::IDENTIFIER).text;
    std::vector<Type*> templateArgs;
    
    if (match(lexer::TokenType::LESS_THAN)) {
        consume(lexer::TokenType::LESS_THAN);
        templateArgs = parseTypeList();
        consumeOrError(lexer::TokenType::GREATER_THAN, "Expected '>' after template arguments");
    }
    
    return factory_.createObjectType(name, std::move(templateArgs), makeRange(start));
}

std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> parameters;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            Type* type = parseType();
            if (!type) {
                reportError(common::ErrorCode::EXPECTED_TYPE, "Expected parameter type");
                continue;
            }
            
            std::string_view name = "";
            if (check(lexer::TokenType::IDENTIFIER)) {
                name = consume(lexer::TokenType::IDENTIFIER).text;
            }
            
            Expression* defaultValue = nullptr;
            if (match(lexer::TokenType::ASSIGN)) {
                consume(lexer::TokenType::ASSIGN);
                defaultValue = parseExpression();
            }
            
            parameters.emplace_back(type, name, defaultValue);
        } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    }
    
    return parameters;
}

std::vector<Expression*> Parser::parseExpressionList() {
    std::vector<Expression*> expressions;
    
    if (!check(lexer::TokenType::RIGHT_PAREN) && !check(lexer::TokenType::RIGHT_BRACKET)) {
        do {
            Expression* expr = parseExpression();
            if (expr) {
                expressions.push_back(expr);
            }
        } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    }
    
    return expressions;
}

std::vector<Type*> Parser::parseTypeList() {
    std::vector<Type*> types;
    
    if (!check(lexer::TokenType::GREATER_THAN)) {
        do {
            Type* type = parseType();
            if (type) {
                types.push_back(type);
            }
        } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    }
    
    return types;
}

std::vector<std::string_view> Parser::parseIdentifierList() {
    std::vector<std::string_view> identifiers;
    
    do {
        if (check(lexer::TokenType::IDENTIFIER)) {
            identifiers.push_back(consume(lexer::TokenType::IDENTIFIER).text);
        } else {
            reportError(common::ErrorCode::EXPECTED_IDENTIFIER, "Expected identifier");
        }
    } while (match(lexer::TokenType::COMMA) && consume(lexer::TokenType::COMMA).type != lexer::TokenType::END_OF_FILE);
    
    return identifiers;
}

bool Parser::isTypeToken(lexer::TokenType token) const {
    switch (token) {
        case lexer::TokenType::VOID:
        case lexer::TokenType::AUTO:
        case lexer::TokenType::SIGNED:
        case lexer::TokenType::UNSIGNED:
        case lexer::TokenType::DATA:
        case lexer::TokenType::DEF:
        case lexer::TokenType::IDENTIFIER:
            return true;
        default:
            return false;
    }
}

bool Parser::canStartExpression(lexer::TokenType token) const {
    switch (token) {
        case lexer::TokenType::INTEGER_LITERAL:
        case lexer::TokenType::FLOAT_LITERAL:
        case lexer::TokenType::STRING_LITERAL:
        case lexer::TokenType::BINARY_LITERAL:
        case lexer::TokenType::IDENTIFIER:
        case lexer::TokenType::LEFT_PAREN:
        case lexer::TokenType::LEFT_BRACKET:
        case lexer::TokenType::LEFT_BRACE:
        case lexer::TokenType::SIZEOF:
        case lexer::TokenType::TYPEOF:
        case lexer::TokenType::ASM:
        case lexer::TokenType::PLUS:
        case lexer::TokenType::MINUS:
        case lexer::TokenType::LOGICAL_NOT:
        case lexer::TokenType::NOT:
        case lexer::TokenType::BITWISE_NOT:
        case lexer::TokenType::INCREMENT:
        case lexer::TokenType::DECREMENT:
        case lexer::TokenType::ADDRESS_OF:
        case lexer::TokenType::MULTIPLY:
            return true;
        default:
            return false;
    }
}

bool Parser::canStartStatement(lexer::TokenType token) const {
    if (canStartExpression(token) || isTypeToken(token)) {
        return true;
    }
    
    switch (token) {
        case lexer::TokenType::LEFT_BRACE:
        case lexer::TokenType::IF:
        case lexer::TokenType::WHILE:
        case lexer::TokenType::FOR:
        case lexer::TokenType::SWITCH:
        case lexer::TokenType::RETURN:
        case lexer::TokenType::BREAK:
        case lexer::TokenType::CONTINUE:
        case lexer::TokenType::THROW:
        case lexer::TokenType::TRY:
        case lexer::TokenType::ASM:
        case lexer::TokenType::CONST:
            return true;
        default:
            return false;
    }
}

bool Parser::canStartDeclaration(lexer::TokenType token) const {
    switch (token) {
        case lexer::TokenType::IMPORT:
        case lexer::TokenType::USING:
        case lexer::TokenType::NAMESPACE:
        case lexer::TokenType::OBJECT:
        case lexer::TokenType::STRUCT:
        case lexer::TokenType::DEF:
        case lexer::TokenType::OPERATOR:
        case lexer::TokenType::TEMPLATE:
            return true;
        default:
            return false;
    }
}

int Parser::getOperatorPrecedence(lexer::TokenType token) const {
    return ast_utils::getOperatorPrecedence(token);
}

bool Parser::isRightAssociative(lexer::TokenType token) const {
    return ast_utils::isRightAssociative(token);
}

bool Parser::isBinaryOperator(lexer::TokenType token) const {
    return ast_utils::isBinaryOperator(token);
}

bool Parser::isUnaryOperator(lexer::TokenType token) const {
    return ast_utils::isUnaryOperator(token);
}

bool Parser::isAssignmentOperator(lexer::TokenType token) const {
    return ast_utils::isAssignmentOperator(token);
}

BinaryExpression::Operator Parser::tokenToBinaryOperator(lexer::TokenType token) const {
    return ast_utils::tokenToBinaryOperator(token);
}

UnaryExpression::Operator Parser::tokenToUnaryOperator(lexer::TokenType token) const {
    return ast_utils::tokenToUnaryOperator(token);
}

AssignmentExpression::Operator Parser::tokenToAssignmentOperator(lexer::TokenType token) const {
    return ast_utils::tokenToAssignmentOperator(token);
}

LiteralExpression::LiteralKind Parser::tokenToLiteralKind(lexer::TokenType token) const {
    return ast_utils::tokenToLiteralKind(token);
}

std::unique_ptr<Parser> createParser(std::vector<lexer::Token> tokens, 
                                   std::shared_ptr<common::Source> source, 
                                   common::Arena& arena) {
    return std::make_unique<Parser>(std::move(tokens), source, arena);
}

Program* parseSource(std::shared_ptr<common::Source> source, common::Arena& arena, common::ErrorCollector& errors) {
    lexer::Tokenizer tokenizer(source, arena);
    std::vector<lexer::Token> tokens = tokenizer.tokenize();
    
    if (tokenizer.errorCollector().hasErrors()) {
        for (const auto& error : tokenizer.errorCollector().errors()) {
            errors.addError(error);
        }
        return nullptr;
    }
    
    Parser parser(std::move(tokens), source, arena);
    Program* program = parser.parseProgram();
    
    if (parser.hasErrors()) {
        for (const auto& error : parser.errorCollector().errors()) {
            errors.addError(error);
        }
    }
    
    return program;
}

Program* parseFile(const std::string& filename, common::Arena& arena, common::ErrorCollector& errors) {
    try {
        auto source = common::Source::fromFile(filename, arena);
        return parseSource(source, arena, errors);
    } catch (const common::Error& e) {
        errors.addError(e);
        return nullptr;
    }
}

} // namespace parser
} // namespace flux