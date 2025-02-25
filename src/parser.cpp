#include <stdexcept>
#include <sstream>
#include "include/parser.h"
#include "include/ast.h"
#include "include/error.h"

namespace flux {

ParseError::ParseError(const std::string& message, const Token& token)
    : std::runtime_error(message), token(token) {}

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::pair<std::shared_ptr<Program>, size_t> Parser::parse() {
    std::vector<std::shared_ptr<Declaration>> declarations;
    
    while (!isAtEnd()) {
        try {
            declarations.push_back(parseDeclaration());
        } catch (const ParseError& err) {
            synchronize();
        }
    }
    
    // Create source location for the program
    SourceLocation loc;
    loc.filename = tokens.empty() ? "" : std::string(tokens[0].lexeme);
    loc.line = static_cast<int>(tokens.empty() ? 0 : tokens[0].line);
    loc.column = static_cast<int>(tokens.empty() ? 0 : tokens[0].column);
    loc.length = static_cast<int>(tokens.empty() ? 0 : tokens.back().line);
    
    std::shared_ptr<Program> program = std::make_shared<Program>(loc, declarations);
    size_t declarationCount = declarations.size();
    
    return std::make_pair(program, declarationCount);
}

std::shared_ptr<Expression> Parser::parseExpression() {
    return parseAssignmentExpression();
}

std::shared_ptr<Expression> Parser::parseAssignmentExpression() {
    std::shared_ptr<Expression> expr = parseLogicalOrExpression();

    // Handle assignment and compound assignments
    if (match({TokenType::OP_EQUAL, TokenType::OP_PLUS_EQUAL, 
               TokenType::OP_MINUS_EQUAL, TokenType::OP_STAR_EQUAL, 
               TokenType::OP_SLASH_EQUAL, TokenType::OP_PERCENT_EQUAL})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseAssignmentExpression();

        // Determine assignment operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_EQUAL:       op = BinaryExpression::Operator::ASSIGN; break;
            case TokenType::OP_PLUS_EQUAL:  op = BinaryExpression::Operator::ADD_ASSIGN; break;
            case TokenType::OP_MINUS_EQUAL: op = BinaryExpression::Operator::SUB_ASSIGN; break;
            case TokenType::OP_STAR_EQUAL:  op = BinaryExpression::Operator::MUL_ASSIGN; break;
            case TokenType::OP_SLASH_EQUAL: op = BinaryExpression::Operator::DIV_ASSIGN; break;
            case TokenType::OP_PERCENT_EQUAL: op = BinaryExpression::Operator::MOD_ASSIGN; break;
            default: throw error(operatorToken, "Unexpected assignment operator");
        }

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseLogicalOrExpression() {
    std::shared_ptr<Expression> expr = parseLogicalAndExpression();

    while (match({TokenType::OP_PIPE_PIPE, TokenType::KW_OR})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseLogicalAndExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        expr = std::make_shared<BinaryExpression>(
            loc, 
            BinaryExpression::Operator::OR, 
            expr, 
            right
        );
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseLogicalAndExpression() {
    std::shared_ptr<Expression> expr = parseBitwiseExpression();

    while (match({TokenType::OP_AMPERSAND_AMPERSAND, TokenType::KW_AND})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseBitwiseExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        expr = std::make_shared<BinaryExpression>(
            loc, 
            BinaryExpression::Operator::AND, 
            expr, 
            right
        );
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseBitwiseExpression() {
    std::shared_ptr<Expression> expr = parseEqualityExpression();

    while (match({TokenType::OP_AMPERSAND, TokenType::OP_PIPE, TokenType::OP_CARET})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseEqualityExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine bitwise operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_AMPERSAND: op = BinaryExpression::Operator::BIT_AND; break;
            case TokenType::OP_PIPE:      op = BinaryExpression::Operator::BIT_OR; break;
            case TokenType::OP_CARET:     op = BinaryExpression::Operator::BIT_XOR; break;
            default: throw error(operatorToken, "Unexpected bitwise operator");
        }

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseEqualityExpression() {
    std::shared_ptr<Expression> expr = parseComparisonExpression();

    while (match({TokenType::OP_EQUAL_EQUAL, TokenType::OP_EXCLAIM_EQUAL, 
                  TokenType::KW_IS, TokenType::KW_NOT})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseComparisonExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine equality operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_EQUAL_EQUAL: op = BinaryExpression::Operator::EQ; break;
            case TokenType::OP_EXCLAIM_EQUAL: op = BinaryExpression::Operator::NE; break;
            case TokenType::KW_IS:  // Similar to ==
                op = BinaryExpression::Operator::EQ; 
                break;
            case TokenType::KW_NOT:  // Similar to !=
                op = BinaryExpression::Operator::NE; 
                break;
            default: throw error(operatorToken, "Unexpected equality operator");
        }

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseComparisonExpression() {
    std::shared_ptr<Expression> expr = parseAdditiveExpression();

    while (match({TokenType::OP_LESS, TokenType::OP_LESS_EQUAL, 
                  TokenType::OP_GREATER, TokenType::OP_GREATER_EQUAL})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseAdditiveExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine comparison operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_LESS:          op = BinaryExpression::Operator::LT; break;
            case TokenType::OP_LESS_EQUAL:    op = BinaryExpression::Operator::LE; break;
            case TokenType::OP_GREATER:       op = BinaryExpression::Operator::GT; break;
            case TokenType::OP_GREATER_EQUAL: op = BinaryExpression::Operator::GE; break;
            default: throw error(operatorToken, "Unexpected comparison operator");
        }

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseAdditiveExpression() {
    std::shared_ptr<Expression> expr = parseMultiplicativeExpression();

    while (match({TokenType::OP_PLUS, TokenType::OP_MINUS})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseMultiplicativeExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine additive operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_PLUS:  op = BinaryExpression::Operator::ADD; break;
            case TokenType::OP_MINUS: op = BinaryExpression::Operator::SUB; break;
            default: throw error(operatorToken, "Unexpected additive operator");
        }

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseMultiplicativeExpression() {
    std::shared_ptr<Expression> expr = parseUnaryExpression();

    while (match({TokenType::OP_STAR, TokenType::OP_SLASH, TokenType::OP_PERCENT})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> right = parseUnaryExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine multiplicative operator
        BinaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_STAR:    op = BinaryExpression::Operator::MUL; break;
            case TokenType::OP_SLASH:   op = BinaryExpression::Operator::DIV; break;
            case TokenType::OP_PERCENT: op = BinaryExpression::Operator::MOD; break;
            default: throw error(operatorToken, "Unexpected multiplicative operator");
        }

        expr = std::make_shared<BinaryExpression>(loc, op, expr, right);
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parseUnaryExpression() {
    // Handle unary operators
    if (match({TokenType::OP_MINUS, TokenType::OP_EXCLAIM, TokenType::OP_TILDE, 
               TokenType::OP_STAR, TokenType::OP_AMPERSAND, 
               TokenType::OP_PLUS_PLUS, TokenType::OP_MINUS_MINUS})) {
        Token operatorToken = previous();
        std::shared_ptr<Expression> operand = parseUnaryExpression();

        // Create source location
        SourceLocation loc;
        loc.filename = std::string(operatorToken.lexeme);
        loc.line = static_cast<int>(operatorToken.line);
        loc.column = static_cast<int>(operatorToken.column);
        loc.length = static_cast<int>(operatorToken.length);

        // Determine unary operator
        UnaryExpression::Operator op;
        switch (operatorToken.type) {
            case TokenType::OP_MINUS:        op = UnaryExpression::Operator::NEG; break;
            case TokenType::OP_EXCLAIM:      op = UnaryExpression::Operator::NOT; break;
            case TokenType::OP_TILDE:        op = UnaryExpression::Operator::BIT_NOT; break;
            case TokenType::OP_STAR:         op = UnaryExpression::Operator::DEREFERENCE; break;
            case TokenType::OP_AMPERSAND:    op = UnaryExpression::Operator::ADDRESS_OF; break;
            case TokenType::OP_PLUS_PLUS:    op = UnaryExpression::Operator::PRE_INC; break;
            case TokenType::OP_MINUS_MINUS:  op = UnaryExpression::Operator::PRE_DEC; break;
            default: throw error(operatorToken, "Unexpected unary operator");
        }

        return std::make_shared<UnaryExpression>(loc, op, operand);
    }

    return parsePostfixExpression();
}

std::shared_ptr<Expression> Parser::parsePostfixExpression() {
    std::shared_ptr<Expression> expr = parsePrimaryExpression();

    while (true) {
        if (match({TokenType::LPAREN})) {
            // Function call
            std::vector<std::shared_ptr<Expression>> args;
            
            // Parse arguments if not immediately closing parenthesis
            if (!check(TokenType::RPAREN)) {
                do {
                    if (args.size() >= 255) {
                        throw error(peek(), "Cannot have more than 255 arguments");
                    }
                    args.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }

            Token parenToken = consume(TokenType::RPAREN, "Expect ')' after arguments");

            // Create source location
            SourceLocation loc;
            loc.filename = std::string(parenToken.lexeme);
            loc.line = static_cast<int>(parenToken.line);
            loc.column = static_cast<int>(parenToken.column);
            loc.length = static_cast<int>(parenToken.length);

            expr = std::make_shared<CallExpression>(loc, expr, args);
        }
        else if (match({TokenType::OP_DOT, TokenType::OP_ARROW})) {
            // Member access
            Token operatorToken = previous();
            Token memberToken = consume(TokenType::IDENTIFIER, "Expect member name after '.' or '->'");

            // Create source location
            SourceLocation loc;
            loc.filename = std::string(operatorToken.lexeme);
            loc.line = static_cast<int>(operatorToken.line);
            loc.column = static_cast<int>(operatorToken.column);
            loc.length = static_cast<int>(operatorToken.length);

            expr = std::make_shared<MemberAccessExpression>(
                loc, 
                expr, 
                std::string(memberToken.getValueString()), 
                operatorToken.type == TokenType::OP_ARROW
            );
        }
        else if (match({TokenType::LBRACKET})) {
            // Parse index expression
            std::shared_ptr<Expression> index = parseExpression();
            
            Token bracketToken = consume(TokenType::RBRACKET, "Expect ']' after index");

            // Create source location
            SourceLocation loc;
            loc.filename = std::string(bracketToken.lexeme);
            loc.line = static_cast<int>(bracketToken.line);
            loc.column = static_cast<int>(bracketToken.column);
            loc.length = static_cast<int>(bracketToken.length);

            expr = std::make_shared<IndexExpression>(loc, expr, index);
        }
        else if (match({TokenType::OP_PLUS_PLUS, TokenType::OP_MINUS_MINUS})) {
            // Postfix increment/decrement
            Token operatorToken = previous();

            // Create source location
            SourceLocation loc;
            loc.filename = std::string(operatorToken.lexeme);
            loc.line = static_cast<int>(operatorToken.line);
            loc.column = static_cast<int>(operatorToken.column);
            loc.length = static_cast<int>(operatorToken.length);

            // Determine postfix operator
            UnaryExpression::Operator op;
            switch (operatorToken.type) {
                case TokenType::OP_PLUS_PLUS:  op = UnaryExpression::Operator::POST_INC; break;
                case TokenType::OP_MINUS_MINUS: op = UnaryExpression::Operator::POST_DEC; break;
                default: throw error(operatorToken, "Unexpected postfix operator");
            }

            expr = std::make_shared<UnaryExpression>(loc, op, expr);
        }
        else {
            break;
        }
    }

    return expr;
}

std::shared_ptr<Expression> Parser::parsePrimaryExpression() {
    // Literal handling
    if (match({TokenType::INTEGER_LITERAL})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create appropriate type for integer literal
        auto intType = std::make_shared<PrimitiveType>(
            loc, 
            PrimitiveTypeKind::INT, 
            BitWidth::_32, 
            false
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            std::get<int64_t>(literalToken.value.value), 
            intType
        );
    }

    if (match({TokenType::FLOAT_LITERAL})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create appropriate type for float literal
        auto floatType = std::make_shared<PrimitiveType>(
            loc, 
            PrimitiveTypeKind::FLOAT, 
            BitWidth::_64, 
            false
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            std::get<double>(literalToken.value.value), 
            floatType
        );
    }

    if (match({TokenType::STRING_LITERAL})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create type for string literal (might need a specific string type)
        auto stringType = std::make_shared<UserDefinedType>(
            loc, 
            "string"
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            std::get<std::string>(literalToken.value.value), 
            stringType
        );
    }

    if (match({TokenType::CHAR_LITERAL})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create appropriate type for char literal
        auto charType = std::make_shared<PrimitiveType>(
            loc, 
            PrimitiveTypeKind::CHAR, 
            BitWidth::DEFAULT, 
            false
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            std::get<char>(literalToken.value.value), 
            charType
        );
    }

    if (match({TokenType::KW_TRUE, TokenType::KW_FALSE})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create appropriate type for boolean literal
        auto boolType = std::make_shared<PrimitiveType>(
            loc, 
            PrimitiveTypeKind::BOOL, 
            BitWidth::DEFAULT, 
            false
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            literalToken.type == TokenType::KW_TRUE, 
            boolType
        );
    }

    if (match({TokenType::NULL_LITERAL})) {
        Token literalToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create a generic pointer type for null
        auto nullType = std::make_shared<PointerType>(
            loc, 
            std::make_shared<PrimitiveType>(
                loc, 
                PrimitiveTypeKind::VOID, 
                BitWidth::DEFAULT, 
                false
            )
        );

        return std::make_shared<LiteralExpression>(
            loc, 
            nullptr, 
            nullType
        );
    }

    // Identifier or parenthesized expression
    if (match({TokenType::IDENTIFIER})) {
        Token identToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(identToken.lexeme);
        loc.line = static_cast<int>(identToken.line);
        loc.column = static_cast<int>(identToken.column);
        loc.length = static_cast<int>(identToken.length);

        return std::make_shared<IdentifierExpression>(
            loc, 
            std::string(identToken.getValueString())
        );
    }

    // Parenthesized expression
    if (match({TokenType::LPAREN})) {
        std::shared_ptr<Expression> expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return expr;
    }

    // Injectable string
    if (match({TokenType::INJECTABLE_STRING})) {
        Token stringToken = previous();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(stringToken.lexeme);
        loc.line = static_cast<int>(stringToken.line);
        loc.column = static_cast<int>(stringToken.column);
        loc.length = static_cast<int>(stringToken.length);

        // TODO: Implement full injectable string parsing
        // For now, just create an injectable string expression with the format
        return std::make_shared<InjectableStringExpression>(
            loc, 
            std::string(stringToken.getValueString()), 
            std::vector<std::shared_ptr<Expression>>{}
        );
    }

    // If no primary expression is found, throw an error
    throw error(peek(), "Expected expression");
}

std::shared_ptr<Declaration> Parser::parseDeclaration() {
    if (match({TokenType::KW_NAMESPACE})) return parseNamespaceDeclaration();
    if (match({TokenType::KW_CLASS})) return parseClassDeclaration();
    if (match({TokenType::KW_STRUCT})) return parseStructDeclaration();
    if (match({TokenType::KW_OBJECT})) return parseObjectDeclaration();
    if (match({TokenType::KW_OPERATOR})) return parseOperatorDeclaration();
    
    // Check for function or variable declaration
    Token typeToken = peek();
    std::shared_ptr<Type> type = parseType();
    
    // Function declaration
    if (check(TokenType::IDENTIFIER) && peek().type == TokenType::LPAREN) {
        return parseFunctionDeclaration();
    }
    
    // Variable declaration
    return parseVariableDeclaration();
}

std::shared_ptr<NamespaceDeclaration> Parser::parseNamespaceDeclaration() {
    Token namespaceToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect namespace name");
    
    consume(TokenType::LBRACE, "Expect '{' after namespace name");
    
    std::vector<std::shared_ptr<ClassDeclaration>> classes;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match({TokenType::KW_CLASS})) {
            classes.push_back(parseClassDeclaration());
        } else {
            throw error(peek(), "Namespaces can only contain classes");
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after namespace body");
    
    SourceLocation loc;
    loc.filename = std::string(namespaceToken.lexeme);
    loc.line = static_cast<int>(namespaceToken.line);
    loc.column = static_cast<int>(namespaceToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    return std::make_shared<NamespaceDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        classes
    );
}

// Error handling methods
bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || peek().type == TokenType::END_OF_FILE;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    // Create detailed error message
    std::stringstream ss;
    ss << "Parse Error at line " << token.line 
       << ", column " << token.column 
       << ": " << message;
    
    return ParseError(ss.str(), token);
}

void Parser::synchronize() {
    // Skip tokens until we find a statement boundary
    advance();
    
    while (!isAtEnd()) {
        // Look for tokens that typically start a new statement or declaration
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::KW_CLASS:
            case TokenType::KW_STRUCT:
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_FOR:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::shared_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    Token classToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect class name");
    
    consume(TokenType::LBRACE, "Expect '{' after class name");
    
    std::vector<std::shared_ptr<Declaration>> members;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match({TokenType::KW_OBJECT})) {
            members.push_back(parseObjectDeclaration());
        } else if (match({TokenType::KW_STRUCT})) {
            members.push_back(parseStructDeclaration());
        } else {
            throw error(peek(), "Classes can only contain objects and structs");
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after class body");
    
    SourceLocation loc;
    loc.filename = std::string(classToken.lexeme);
    loc.line = static_cast<int>(classToken.line);
    loc.column = static_cast<int>(classToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    return std::make_shared<ClassDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        members
    );
}

std::shared_ptr<StructDeclaration> Parser::parseStructDeclaration() {
    Token structToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect struct name");
    
    consume(TokenType::LBRACE, "Expect '{' after struct name");
    
    std::vector<VariableDeclaration> fields;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        std::shared_ptr<Type> fieldType = parseType();
        
        Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expect field name");
        
        SourceLocation loc;
        loc.filename = std::string(structToken.lexeme);
        loc.line = static_cast<int>(structToken.line);
        loc.column = static_cast<int>(structToken.column);
        loc.length = static_cast<int>(fieldNameToken.length);
        
        fields.emplace_back(
            loc,
            std::string(fieldNameToken.getValueString()),
            fieldType,
            nullptr
        );
        
        consume(TokenType::SEMICOLON, "Expect ';' after struct field");
    }
    
    consume(TokenType::RBRACE, "Expect '}' after struct body");
    
    SourceLocation loc;
    loc.filename = std::string(structToken.lexeme);
    loc.line = static_cast<int>(structToken.line);
    loc.column = static_cast<int>(structToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    return std::make_shared<StructDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        fields
    );
}

std::shared_ptr<ObjectDeclaration> Parser::parseObjectDeclaration() {
    Token objectToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect object name");
    
    consume(TokenType::LBRACE, "Expect '{' after object name");
    
    std::vector<std::shared_ptr<Declaration>> members;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match({TokenType::KW_STRUCT})) {
            members.push_back(parseStructDeclaration());
        } else if (match({TokenType::KW_OBJECT})) {
            members.push_back(parseObjectDeclaration());
        } else if (match({TokenType::KW_OPERATOR})) {
            members.push_back(parseOperatorDeclaration());
        } else {
            // Attempt to parse function or variable declaration
            Token typeToken = peek();
            std::shared_ptr<Type> type = parseType();
            
            // Check if this could be a function declaration
            if (check(TokenType::IDENTIFIER) && peek().type == TokenType::LPAREN) {
                members.push_back(parseFunctionDeclaration());
            } else {
                // If not a function, treat as variable declaration
                Token nameToken = consume(TokenType::IDENTIFIER, "Expect variable name");
                
                SourceLocation loc;
                loc.filename = std::string(typeToken.lexeme);
                loc.line = static_cast<int>(typeToken.line);
                loc.column = static_cast<int>(typeToken.column);
                loc.length = static_cast<int>(nameToken.length);
                
                std::shared_ptr<Expression> initializer = nullptr;
                if (match({TokenType::OP_EQUAL})) {
                    initializer = parseExpression();
                }
                
                consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
                
                members.push_back(std::make_shared<VariableDeclaration>(
                    loc, 
                    std::string(nameToken.getValueString()), 
                    type, 
                    initializer
                ));
            }
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after object body");
    
    SourceLocation loc;
    loc.filename = std::string(objectToken.lexeme);
    loc.line = static_cast<int>(objectToken.line);
    loc.column = static_cast<int>(objectToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    return std::make_shared<ObjectDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        members
    );
}

std::shared_ptr<OperatorDeclaration> Parser::parseOperatorDeclaration() {
    Token operatorToken = previous();
    
    consume(TokenType::LPAREN, "Expect '(' after 'operator' keyword");
    
    std::vector<Parameter> params;
    
    // Parse parameter list
    if (!check(TokenType::RPAREN)) {
        do {
            std::shared_ptr<Type> paramType = parseType();
            consume(TokenType::IDENTIFIER, "Expect parameter name");
            Token paramNameToken = previous();
            
            params.emplace_back(
                std::string(paramNameToken.getValueString()),
                paramType,
                false  // Assuming parameters are not pointers by default
            );
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expect ')' after parameter list");
    
    // Parse operator symbol
    Token opSymbolToken = advance();
    //if (!isValidOperatorSymbol(opSymbolToken)) {
    //    throw error(opSymbolToken, "Invalid operator symbol");
    //}
    
    std::string opSymbol(opSymbolToken.lexeme);
    
    // Parse function body
    std::shared_ptr<BlockStatement> body = parseBlock();
    
    // Create the operator declaration AST node
    SourceLocation loc;
    loc.filename = std::string(operatorToken.lexeme);
    loc.line = static_cast<int>(operatorToken.line);
    loc.column = static_cast<int>(operatorToken.column);
    loc.length = static_cast<int>(opSymbolToken.lexeme.length());
    
    // Assuming the return type is the same as the first parameter type
    std::shared_ptr<Type> returnType = params.empty() ? nullptr : params[0].getType();
    
    return std::make_shared<OperatorDeclaration>(
        loc,
        opSymbol,
        params,
        returnType,
        body
    );
}

std::shared_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    Token typeToken = previous();
    
    std::shared_ptr<Type> type = parseType();
    
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect variable name after type");
    
    SourceLocation loc;
    loc.filename = std::string(typeToken.lexeme);
    loc.line = static_cast<int>(typeToken.line);
    loc.column = static_cast<int>(typeToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    std::shared_ptr<Expression> initializer = nullptr;
    if (match({TokenType::OP_EQUAL})) {
        initializer = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
    
    return std::make_shared<VariableDeclaration>(
        loc, 
        std::string(nameToken.getValueString()), 
        type, 
        initializer
    );
}

std::shared_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    Token funcToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect function name");
    
    consume(TokenType::LPAREN, "Expect '(' after function name");
    
    std::vector<Parameter> params;
    
    if (!check(TokenType::RPAREN)) {
        do {
            if (params.size() >= 255) {
                throw error(peek(), "Cannot have more than 255 parameters");
            }
            
            std::shared_ptr<Type> paramType = parseType();
            
            bool isPointer = false;
            if (match({TokenType::OP_STAR})) {
                isPointer = true;
            }
            
            Token paramNameToken = consume(TokenType::IDENTIFIER, "Expect parameter name");
            
            params.emplace_back(
                std::string(paramNameToken.getValueString()),
                paramType,
                isPointer
            );
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expect ')' after parameters");
    
    std::shared_ptr<Type> returnType = parseType();
    
    std::shared_ptr<BlockStatement> body = nullptr;
    if (check(TokenType::LBRACE)) {
        body = parseBlock();
    }
    
    SourceLocation loc;
    loc.filename = std::string(funcToken.lexeme);
    loc.line = static_cast<int>(funcToken.line);
    loc.column = static_cast<int>(funcToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    return std::make_shared<FunctionDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        returnType,
        params,
        body
    );
}

std::shared_ptr<BlockStatement> Parser::parseBlock() {
    Token braceToken = consume(TokenType::LBRACE, "Expect '{' at start of block");
    
    std::vector<std::shared_ptr<Statement>> statements;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        // Try to convert Declaration to Statement
        std::shared_ptr<Declaration> decl = parseDeclaration();
        statements.push_back(std::dynamic_pointer_cast<Statement>(decl));
    }
    
    consume(TokenType::RBRACE, "Expect '}' at end of block");
    
    SourceLocation loc;
    loc.filename = std::string(braceToken.lexeme);
    loc.line = static_cast<int>(braceToken.line);
    loc.column = static_cast<int>(braceToken.column);
    loc.length = 1;
    
    return std::make_shared<BlockStatement>(loc, statements);
}

std::shared_ptr<Type> Parser::parseType() {
    // Track the starting token for source location
    Token typeToken = peek();
    
    // Updated to use correct token types from TokenType enum
    if (match({TokenType::KW_INT, TokenType::KW_FLOAT, 
               TokenType::KW_CHAR, TokenType::KW_VOID})) {
        PrimitiveTypeKind kind;
        BitWidth width = BitWidth::DEFAULT;
        bool isUnsigned = false;
        
        // Determine primitive type kind
        switch (previous().type) {
            case TokenType::KW_INT: kind = PrimitiveTypeKind::INT; break;
            case TokenType::KW_FLOAT: kind = PrimitiveTypeKind::FLOAT; break;
            case TokenType::KW_CHAR: kind = PrimitiveTypeKind::CHAR; break;
            case TokenType::KW_VOID: kind = PrimitiveTypeKind::VOID; break;
            default: throw error(typeToken, "Invalid primitive type");
        }
        
        // Check for bit width specifier
        if (match({TokenType::BIT_WIDTH_SPECIFIER})) {
            Token bitWidthToken = previous();
            
            // Extract bit width from the token
            if (bitWidthToken.value.bitWidth) {
                switch (*bitWidthToken.value.bitWidth) {
                    case 8: width = BitWidth::_8; break;
                    case 16: width = BitWidth::_16; break;
                    case 32: width = BitWidth::_32; break;
                    case 64: width = BitWidth::_64; break;
                    case 128: width = BitWidth::_128; break;
                    default: 
                        throw error(bitWidthToken, 
                            "Unsupported bit width: " + 
                            std::to_string(*bitWidthToken.value.bitWidth));
                }
            }
        }
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(typeToken.lexeme);
        loc.line = static_cast<int>(typeToken.line);
        loc.column = static_cast<int>(typeToken.column);
        loc.length = static_cast<int>(typeToken.length);
        
        // Create primitive type
        auto primitiveType = std::make_shared<PrimitiveType>(loc, kind, width, isUnsigned);
        
        // Check for array or pointer type modifiers
        return parseTypeModifiers(primitiveType);
    }
    
    // User-defined type handling
    if (check(TokenType::IDENTIFIER)) {
        Token nameToken = advance();
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(nameToken.lexeme);
        loc.line = static_cast<int>(nameToken.line);
        loc.column = static_cast<int>(nameToken.column);
        loc.length = static_cast<int>(nameToken.length);
        
        auto userType = std::make_shared<UserDefinedType>(loc, 
            std::string(nameToken.getValueString()));
        
        // Check for array or pointer type modifiers
        return parseTypeModifiers(userType);
    }
    
    // If no type is found, throw an error
    throw error(typeToken, "Expected a type");
}

std::shared_ptr<Type> Parser::parseTypeModifiers(std::shared_ptr<Type> baseType) {
    // Check for pointer type
    while (match({TokenType::OP_STAR})) {
        // Create source location
        SourceLocation loc;
        loc.filename = baseType->getLocation().filename;
        loc.line = baseType->getLocation().line;
        loc.column = baseType->getLocation().column;
        loc.length = baseType->getLocation().length;
        
        baseType = std::make_shared<PointerType>(loc, baseType);
    }
    
    // Check for array type
    while (match({TokenType::LBRACKET})) {
        // Optional size specification
        std::optional<size_t> size;
        
        // If next token is a number, parse array size
        if (check(TokenType::INTEGER_LITERAL)) {
            Token sizeToken = advance();
            if (std::holds_alternative<int64_t>(sizeToken.value.value)) {
                size = static_cast<size_t>(std::get<int64_t>(sizeToken.value.value));
            }
        }
        
        // Expect closing bracket
        consume(TokenType::RBRACKET, "Expect ']' after array size");
        
        // Create source location
        SourceLocation loc;
        loc.filename = baseType->getLocation().filename;
        loc.line = baseType->getLocation().line;
        loc.column = baseType->getLocation().column;
        loc.length = baseType->getLocation().length;
        
        baseType = std::make_shared<ArrayType>(loc, baseType, size);
    }
    
    return baseType;
}

} // namespace flux
