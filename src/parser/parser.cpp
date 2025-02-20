/**
 * @file parser.cpp
 * @brief Implementation of Flux parser
 */

#include "parser.hpp"
#include <iostream>
#include <sstream>

namespace flux {

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), current(0) {}

std::unique_ptr<Program> Parser::parse() {
    std::vector<StmtPtr> statements;
    
    try {
        while (!isAtEnd()) {
            statements.push_back(declaration());
        }
    } catch (const ParseError& error) {
        std::cerr << "Parse error: " << error.what() << std::endl;
        synchronize();
    }
    
    return std::make_unique<Program>(std::move(statements));
}

// Statement parsing methods

StmtPtr Parser::declaration() {
    try {
        if (match({TokenType::CLASS})) {
            return classDeclaration();
        }
        if (match({TokenType::OBJECT})) {
            return objectDeclaration();
        }
        if (match({TokenType::NAMESPACE})) {
            return namespaceDeclaration();
        }
        if (match({TokenType::STRUCT})) {
            return structDeclaration();
        }
        if (match({TokenType::OPERATOR})) {
            return operatorDeclaration();
        }
        
        // Check for function declaration
        // This is a bit complex as we need to look ahead for a function signature pattern
        if (check(TokenType::ASYNC) || check(TokenType::IDENTIFIER) || 
            check(TokenType::INT) || check(TokenType::FLOAT) || 
            check(TokenType::CHAR) || check(TokenType::VOID) || 
            check(TokenType::BOOL)) {
            
            // Save current position
            int startPos = current;
            
            // Try to parse as function declaration
            try {
                bool isAsync = match({TokenType::ASYNC});
                std::shared_ptr<Type> returnType = parseType();
                Token name = consume(TokenType::IDENTIFIER, "Expected function name.");
                
                if (check(TokenType::LEFT_PAREN)) {
                    // Rewind and parse as function declaration
                    current = startPos;
                    return functionDeclaration();
                }
                
                // Rewind for variable declaration
                current = startPos;
            } catch (const ParseError&) {
                // Rewind for fallback to other declaration types
                current = startPos;
            }
        }
        
        // If not a special declaration, it could be a variable declaration
        // or an expression statement
        return statement();
    } catch (const ParseError& error) {
        synchronize();
        return nullptr;
    }
}

StmtPtr Parser::classDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected class name.");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before class body.");
    
    std::vector<StmtPtr> members = parseClassMembers();
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after class body.");
    consume(TokenType::SEMICOLON, "Expected ';' after class declaration.");
    
    return std::make_shared<ClassDeclarationStmt>(name, std::move(members));
}

StmtPtr Parser::objectDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected object name.");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before object body.");
    
    std::vector<StmtPtr> members = parseClassMembers();
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after object body.");
    consume(TokenType::SEMICOLON, "Expected ';' after object declaration.");
    
    return std::make_shared<ObjectDeclarationStmt>(name, std::move(members));
}

StmtPtr Parser::namespaceDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected namespace name.");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before namespace body.");
    
    std::vector<StmtPtr> declarations;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        declarations.push_back(declaration());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after namespace body.");
    consume(TokenType::SEMICOLON, "Expected ';' after namespace declaration.");
    
    return std::make_shared<NamespaceDeclarationStmt>(name, std::move(declarations));
}

StmtPtr Parser::structDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected struct name.");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before struct body.");
    
    std::vector<StructDeclarationStmt::Field> fields;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        std::shared_ptr<Type> fieldType = parseType();
        Token fieldName = consume(TokenType::IDENTIFIER, "Expected field name.");
        consume(TokenType::SEMICOLON, "Expected ';' after field declaration.");
        
        fields.push_back({fieldType, fieldName});
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after struct body.");
    consume(TokenType::SEMICOLON, "Expected ';' after struct declaration.");
    
    return std::make_shared<StructDeclarationStmt>(name, std::move(fields));
}

StmtPtr Parser::functionDeclaration() {
    bool isAsync = match({TokenType::ASYNC});
    std::shared_ptr<Type> returnType = parseType();
    Token name = consume(TokenType::IDENTIFIER, "Expected function name.");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");
    std::vector<FunctionDeclarationStmt::Parameter> parameters = parseParameters();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
    
    bool isVolatile = false;
    if (match({TokenType::VOLATILE})) {
        isVolatile = true;
    }
    
    // Parse function body as a block
    consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
    std::vector<StmtPtr> bodyStatements;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        bodyStatements.push_back(statement());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after function body.");
    consume(TokenType::SEMICOLON, "Expected ';' after function declaration.");
    
    auto body = std::make_shared<BlockStmt>(std::move(bodyStatements), isVolatile);
    
    return std::make_shared<FunctionDeclarationStmt>(
        returnType, name, std::move(parameters), body, isVolatile, isAsync
    );
}

StmtPtr Parser::operatorDeclaration() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'operator'.");
    std::shared_ptr<Type> leftType = parseType();
    consume(TokenType::COMMA, "Expected ',' between operator parameter types.");
    std::shared_ptr<Type> rightType = parseType();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after operator parameter types.");
    
    consume(TokenType::LEFT_BRACKET, "Expected '[' before operator symbol.");
    Token op = advance(); // Get the operator token
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after operator symbol.");
    
    consume(TokenType::LEFT_BRACE, "Expected '{' before operator implementation.");
    std::vector<StmtPtr> bodyStatements;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        bodyStatements.push_back(statement());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after operator implementation.");
    consume(TokenType::SEMICOLON, "Expected ';' after operator declaration.");
    
    auto body = std::make_shared<BlockStmt>(std::move(bodyStatements));
    
    return std::make_shared<OperatorDeclarationStmt>(leftType, rightType, op, body);
}

StmtPtr Parser::varDeclaration() {
    bool isVolatile = false;
    if (match({TokenType::VOLATILE})) {
        isVolatile = true;
    }
    
    std::shared_ptr<Type> type = parseType();
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");
    
    ExprPtr initializer = nullptr;
    if (match({TokenType::EQUAL})) {
        initializer = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    return std::make_shared<VarDeclarationStmt>(type, name, initializer, isVolatile);
}

StmtPtr Parser::statement() {
    if (match({TokenType::IF})) {
        return ifStatement();
    }
    if (match({TokenType::WHILE})) {
        return whileStatement();
    }
    if (match({TokenType::FOR})) {
        return forStatement();
    }
    if (match({TokenType::WHEN})) {
        return whenStatement();
    }
    if (match({TokenType::ASM})) {
        return asmStatement();
    }
    if (match({TokenType::RETURN})) {
        return returnStatement();
    }
    if (match({TokenType::BREAK})) {
        return breakStatement();
    }
    if (match({TokenType::CONTINUE})) {
        return continueStatement();
    }
    if (match({TokenType::LEFT_BRACE})) {
        current--; // Go back to the '{' token
        return blockStatement();
    }
    if (match({TokenType::LOCK})) {
        if (check(TokenType::LEFT_PAREN)) {
            // This is a lock() call in an async function
            current--; // Go back to the 'lock' token
            return expressionStatement();
        } else {
            // This is a lock behavior definition
            return lockStatement();
        }
    }
    if (match({TokenType::DUNDER_LOCK})) {
        // This is a pre-lock behavior definition
        current--; // Go back to the '__lock' token
        return lockStatement();
    }
    if (match({TokenType::LOCK_DUNDER})) {
        // This is a post-lock behavior definition
        current--; // Go back to the 'lock__' token
        return lockStatement();
    }
    
    // Check for a type to determine if this is a variable declaration
    if (match({TokenType::VOLATILE})) {
        current--; // Go back to the 'volatile' token
        return varDeclaration();
    }
    
    if (check(TokenType::INT) || check(TokenType::FLOAT) || 
        check(TokenType::CHAR) || check(TokenType::BOOL) || 
        check(TokenType::VOID) || check(TokenType::IDENTIFIER)) {
        
        // Save current position
        int startPos = current;
        
        try {
            std::shared_ptr<Type> type = parseType();
            // If we successfully parsed a type and the next token is an identifier,
            // this is likely a variable declaration
            if (check(TokenType::IDENTIFIER)) {
                current = startPos; // Rewind
                return varDeclaration();
            }
        } catch (const ParseError&) {
            // Not a type, continue with expression statement
        }
        
        // Rewind for expression statement
        current = startPos;
    }
    
    return expressionStatement();
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_shared<ExpressionStmt>(std::move(expr));
}

StmtPtr Parser::blockStatement(bool checkVolatile) {
    consume(TokenType::LEFT_BRACE, "Expected '{' before block.");
    
    std::vector<StmtPtr> statements;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(statement());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    
    bool isVolatile = false;
    if (checkVolatile && match({TokenType::VOLATILE})) {
        isVolatile = true;
    }
    
    if (checkVolatile) {
        consume(TokenType::SEMICOLON, "Expected ';' after block.");
    }
    
    return std::make_shared<BlockStmt>(std::move(statements), isVolatile);
}

StmtPtr Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    ExprPtr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
    
    StmtPtr thenBranch = statement();
    StmtPtr elseBranch = nullptr;
    
    if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }
    
    return std::make_shared<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::whileStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'.");
    ExprPtr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");
    
    StmtPtr body = statement();
    
    return std::make_shared<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'.");
    
    // Initialize part
    StmtPtr initializer;
    if (match({TokenType::SEMICOLON})) {
        initializer = nullptr;
    } else if (check(TokenType::INT) || check(TokenType::FLOAT) || 
               check(TokenType::CHAR) || check(TokenType::BOOL) || 
               check(TokenType::VOID) || check(TokenType::IDENTIFIER)) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }
    
    // Condition part
    ExprPtr condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for condition.");
    
    // Increment part
    ExprPtr increment = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");
    
    // Body part
    StmtPtr body = statement();
    
    return std::make_shared<ForStmt>(std::move(initializer), std::move(condition), 
                                    std::move(increment), std::move(body));
}

StmtPtr Parser::whenStatement() {
    bool isAsync = false;
    if (match({TokenType::ASYNC})) {
        isAsync = true;
    }
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'when'.");
    ExprPtr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after when condition.");
    
    bool isVolatile = false;
    if (match({TokenType::VOLATILE})) {
        isVolatile = true;
    }
    
    StmtPtr body;
    if (check(TokenType::LEFT_BRACE)) {
        body = blockStatement(false);
    } else {
        body = std::make_shared<BlockStmt>(std::vector<StmtPtr>{statement()}, false);
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after when statement.");
    
    return std::make_shared<WhenStmt>(std::move(condition), std::move(body), isVolatile, isAsync);
}

StmtPtr Parser::asmStatement() {
    consume(TokenType::LEFT_BRACE, "Expected '{' after 'asm'.");
    
    // Collect all tokens until the closing brace
    std::stringstream asmCode;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        Token token = advance();
        asmCode << token.lexeme << " ";
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after assembly code.");
    consume(TokenType::SEMICOLON, "Expected ';' after assembly block.");
    
    return std::make_shared<AsmStmt>(asmCode.str());
}

StmtPtr Parser::returnStatement() {
    Token keyword = previous();
    
    ExprPtr value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    
    return std::make_shared<ReturnStmt>(std::move(value));
}

StmtPtr Parser::breakStatement() {
    consume(TokenType::SEMICOLON, "Expected ';' after 'break'.");
    return std::make_shared<BreakStmt>();
}

StmtPtr Parser::continueStatement() {
    consume(TokenType::SEMICOLON, "Expected ';' after 'continue'.");
    return std::make_shared<ContinueStmt>();
}

StmtPtr Parser::lockStatement() {
    LockStmt::LockType lockType;
    
    if (match({TokenType::LOCK})) {
        lockType = LockStmt::LockType::LOCK;
    } else if (match({TokenType::DUNDER_LOCK})) {
        lockType = LockStmt::LockType::PRE_LOCK;
    } else if (match({TokenType::LOCK_DUNDER})) {
        lockType = LockStmt::LockType::POST_LOCK;
    } else {
        throw error(peek(), "Expected 'lock', '__lock', or 'lock__'.");
    }
    
    Token functionName = consume(TokenType::IDENTIFIER, "Expected function name after lock type.");
    
    std::vector<Token> scopes;
    while (match({TokenType::SCOPE_RESOLUTION})) {
        Token scope = consume(TokenType::IDENTIFIER, "Expected scope name after '::'.");
        scopes.push_back(scope);
    }
    
    StmtPtr body = nullptr;
    if (check(TokenType::LEFT_BRACE)) {
        body = blockStatement(false);
    } else {
        body = std::make_shared<BlockStmt>(std::vector<StmtPtr>{}, false);
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after lock statement.");
    
    return std::make_shared<LockStmt>(lockType, std::move(scopes), std::move(body));
}

// Expression parsing methods

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = logicalOr();
    
    if (match({TokenType::EQUAL, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL, 
              TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL, TokenType::PERCENT_EQUAL})) {
        Token op = previous();
        ExprPtr value = assignment();
        
        // Check that left side is a valid assignment target
        if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_shared<AssignExpr>(varExpr->name, std::move(value));
        } else if (auto* memberExpr = dynamic_cast<MemberAccessExpr*>(expr.get())) {
            // Handle member assignment
            // For simplicity, we'll just use the standard AssignExpr
            return std::make_shared<AssignExpr>(memberExpr->member, std::move(value));
        } else if (auto* arrayExpr = dynamic_cast<ArrayAccessExpr*>(expr.get())) {
            // Handle array assignment - this is more complex
            // For simplicity, we'll just use a BinaryExpr
            return std::make_shared<BinaryExpr>(std::move(expr), op, std::move(value));
        }
        
        throw error(op, "Invalid assignment target.");
    }
    
    return expr;
}

ExprPtr Parser::logicalOr() {
    ExprPtr expr = logicalAnd();
    
    while (match({TokenType::OR})) {
        Token op = previous();
        ExprPtr right = logicalAnd();
        expr = std::make_shared<LogicalExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::logicalAnd() {
    ExprPtr expr = equality();
    
    while (match({TokenType::AND})) {
        Token op = previous();
        ExprPtr right = equality();
        expr = std::make_shared<LogicalExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    
    while (match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL, TokenType::IS})) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();
    
    while (match({TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL})) {
        Token op = previous();
        ExprPtr right = term();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        ExprPtr right = factor();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        Token op = previous();
        ExprPtr right = unary();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS, TokenType::TILDE, TokenType::STAR, TokenType::AT})) {
        Token op = previous();
        ExprPtr right = unary();
        
        if (op.type == TokenType::AT) {
            return std::make_shared<AddressOfExpr>(std::move(right));
        } else if (op.type == TokenType::STAR) {
            return std::make_shared<DereferenceExpr>(std::move(right));
        } else {
            return std::make_shared<UnaryExpr>(op, std::move(right));
        }
    }
    
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    
    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            expr = finishCall(std::move(expr));
        } else if (match({TokenType::LEFT_BRACKET})) {
            ExprPtr index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index.");
            expr = std::make_shared<ArrayAccessExpr>(std::move(expr), std::move(index));
        } else if (match({TokenType::DOT, TokenType::ARROW})) {
            Token op = previous();
            Token member = consume(TokenType::IDENTIFIER, "Expected property name after '.' or '->'.");
            expr = std::make_shared<MemberAccessExpr>(std::move(expr), op, member);
        } else {
            break;
        }
    }
    
    return expr;
}

ExprPtr Parser::primary() {
    // Literals
    if (match({TokenType::FALSE})) {
        return std::make_shared<BooleanLiteral>(false);
    }
    if (match({TokenType::TRUE})) {
        return std::make_shared<BooleanLiteral>(true);
    }
    if (match({TokenType::NULL_LITERAL})) {
        return std::make_shared<NullLiteral>();
    }
    if (match({TokenType::INTEGER})) {
        int64_t value = std::stoll(previous().lexeme);
        return std::make_shared<IntegerLiteral>(value);
    }
    if (match({TokenType::FLOAT})) {
        double value = std::stod(previous().lexeme);
        return std::make_shared<FloatLiteral>(value);
    }
    if (match({TokenType::STRING})) {
        std::string value = previous().lexeme;
        // Remove the quotes
        value = value.substr(1, value.size() - 2);
        return std::make_shared<StringLiteral>(value);
    }
    if (match({TokenType::CHAR})) {
        std::string value = previous().lexeme;
        // Remove the quotes and handle escape sequences
        value = value.substr(1, value.size() - 2);
        char c = value[0];
        // Handle escape sequences
        if (c == '\\' && value.size() > 1) {
            switch (value[1]) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case '0': c = '\0'; break;
                case '\\': c = '\\'; break;
                case '\'': c = '\''; break;
                case '\"': c = '\"'; break;
                default: break;
            }
        }
        return std::make_shared<CharLiteral>(c);
    }
    
    // Array literals
    if (match({TokenType::LEFT_BRACKET})) {
        std::vector<ExprPtr> elements;
        
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::COMMA}));
        }
        
        consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements.");
        
        // Check if this is a character array literal
        bool isCharArray = true;
        for (const auto& element : elements) {
            if (dynamic_cast<CharLiteral*>(element.get()) == nullptr) {
                isCharArray = false;
                break;
            }
        }
        
        if (isCharArray) {
            std::vector<char> chars;
            for (const auto& element : elements) {
                chars.push_back(dynamic_cast<CharLiteral*>(element.get())->value);
            }
            return std::make_shared<CharArrayLiteral>(std::move(chars));
        } else {
            return std::make_shared<ArrayLiteral>(std::move(elements));
        }
    }
    
    // Interpolated strings
    if (match({TokenType::INTERPOLATED_STRING_START})) {
        std::string format = previous().lexeme;
        // Remove the prefix and quotes
        format = format.substr(2, format.size() - 3);
        
        consume(TokenType::COLON, "Expected ':' after interpolated string format.");
        consume(TokenType::LEFT_BRACE, "Expected '{' after ':'.");
        
        std::vector<ExprPtr> expressions;
        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                expressions.push_back(expression());
            } while (match({TokenType::SEMICOLON}));
        }
        
        consume(TokenType::RIGHT_BRACE, "Expected '}' after interpolated string expressions.");
        consume(TokenType::SEMICOLON, "Expected ';' after interpolated string.");
        
        return std::make_shared<InterpolatedStringExpr>(format, std::move(expressions));
    }
    
    // Grouping expressions
    if (match({TokenType::LEFT_PAREN})) {
        ExprPtr expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_shared<GroupingExpr>(std::move(expr));
    }
    
    // Type casting expressions
    if (check(TokenType::INT) || check(TokenType::FLOAT) || 
        check(TokenType::CHAR) || check(TokenType::BOOL) || 
        check(TokenType::VOID) || check(TokenType::IDENTIFIER)) {
        
        // Save current position
        int startPos = current;
        
        try {
            std::shared_ptr<Type> type = parseType();
            
            if (match({TokenType::COLON})) {
                ExprPtr expr = expression();
                return std::make_shared<TypeCastExpr>(type, std::move(expr));
            }
        } catch (const ParseError&) {
            // Not a type cast, continue with identifier
        }
        
        // Rewind
        current = startPos;
    }
    
    // Variable references
    if (match({TokenType::IDENTIFIER})) {
        return std::make_shared<VariableExpr>(previous());
    }
    
    throw error(peek(), "Expected expression.");
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> arguments = parseArguments();
    Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
    
    return std::make_shared<CallExpr>(std::move(callee), paren, std::move(arguments));
}

std::vector<ExprPtr> Parser::parseArguments() {
    std::vector<ExprPtr> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    
    return arguments;
}

// Type parsing methods

std::shared_ptr<PrimitiveType> Parser::parsePrimitiveType() {
    Token typeToken = previous();
    
    PrimitiveType::PrimitiveKind kind;
    if (typeToken.type == TokenType::INT) {
        kind = PrimitiveType::PrimitiveKind::INT;
    } else if (typeToken.type == TokenType::FLOAT) {
        kind = PrimitiveType::PrimitiveKind::FLOAT;
    } else if (typeToken.type == TokenType::CHAR) {
        kind = PrimitiveType::PrimitiveKind::CHAR;
    } else if (typeToken.type == TokenType::BOOL) {
        kind = PrimitiveType::PrimitiveKind::BOOL;
    } else if (typeToken.type == TokenType::VOID) {
        kind = PrimitiveType::PrimitiveKind::VOID;
    } else {
        throw error(typeToken, "Expected primitive type.");
    }
    
    // Check for bit width specification
    int bitWidth = 0;
    if (match({TokenType::LEFT_BRACE})) {
        bitWidth = parseBitWidth();
    }
    
    return std::make_shared<PrimitiveType>(kind, bitWidth);
}

int Parser::parseBitWidth() {
    if (match({TokenType::INTEGER})) {
        int width = std::stoi(previous().lexeme);
        consume(TokenType::RIGHT_BRACE, "Expected '}' after bit width.");
        return width;
    }
    
    throw error(peek(), "Expected integer bit width.");
}

std::shared_ptr<Type> Parser::parseType() {
    if (match({TokenType::INT, TokenType::FLOAT, TokenType::CHAR, TokenType::BOOL, TokenType::VOID})) {
        auto primType = parsePrimitiveType();
        
        // Check for array type
        if (match({TokenType::LEFT_BRACKET})) {
            std::optional<size_t> size;
            
            if (!check(TokenType::RIGHT_BRACKET)) {
                // Check if there's a size specified
                if (match({TokenType::INTEGER})) {
                    size = std::stoul(previous().lexeme);
                }
            }
            
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array size.");
            
            return std::make_shared<ArrayType>(primType, size);
        }
        
        // Check for pointer type
        if (match({TokenType::STAR})) {
            return std::make_shared<PointerType>(primType);
        }
        
        return primType;
    } else if (match({TokenType::IDENTIFIER})) {
        Token typeName = previous();
        
        // Check for bit width specification
        int bitWidth = 0;
        if (match({TokenType::LEFT_BRACE})) {
            bitWidth = parseBitWidth();
        }
        
        // If it's a basic type like "string", "int", etc. with a custom bit width
        if (typeName.lexeme == "string") {
            auto stringType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::STRING, bitWidth);
            
            // Check for array type
            if (match({TokenType::LEFT_BRACKET})) {
                std::optional<size_t> size;
                
                if (!check(TokenType::RIGHT_BRACKET)) {
                    // Check if there's a size specified
                    if (match({TokenType::INTEGER})) {
                        size = std::stoul(previous().lexeme);
                    }
                }
                
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after array size.");
                
                return std::make_shared<ArrayType>(stringType, size);
            }
            
            // Check for pointer type
            if (match({TokenType::STAR})) {
                return std::make_shared<PointerType>(stringType);
            }
            
            return stringType;
        } else {
            // Custom type (class, object, struct)
            std::shared_ptr<Type> customType;
            
            // Determine the type based on previous declarations (not implemented here)
            // For simplicity, we'll just create a ClassType
            customType = std::make_shared<ClassType>(typeName.lexeme);
            
            // Check for array type
            if (match({TokenType::LEFT_BRACKET})) {
                std::optional<size_t> size;
                
                if (!check(TokenType::RIGHT_BRACKET)) {
                    // Check if there's a size specified
                    if (match({TokenType::INTEGER})) {
                        size = std::stoul(previous().lexeme);
                    }
                }
                
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after array size.");
                
                return std::make_shared<ArrayType>(customType, size);
            }
            
            // Check for pointer type
            if (match({TokenType::STAR})) {
                return std::make_shared<PointerType>(customType);
            }
            
            return customType;
        }
    }
    
    throw error(peek(), "Expected type.");
}

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
            case TokenType::VOLATILE:
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

std::vector<StmtPtr> Parser::parseClassMembers() {
    std::vector<StmtPtr> members;

    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        // Allow various types of members: methods, variables, etc.
        if (match({TokenType::FUNCTION})) {
            members.push_back(functionDeclaration());
        } else if (check(TokenType::INT) || check(TokenType::FLOAT) || 
                   check(TokenType::CHAR) || check(TokenType::BOOL) ||
                   check(TokenType::VOID) || check(TokenType::IDENTIFIER)) {
            members.push_back(varDeclaration());
        } else {
            throw error(peek(), "Expected method or variable declaration in class/object.");
        }
    }

    return members;
}

} // namespace flux
