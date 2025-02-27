#include <stdexcept>
#include <sstream>
#include <iostream>
#include "include/parser.h"
#include "include/ast.h"
#include "include/error.h"

namespace flux {

ParseError::ParseError(const std::string& message, const Token& token)
    : std::runtime_error(message), token(token) {}

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

int total_decs = 0;
int total_funcs = 0;
int total_vars = 0;
int total_structs = 0;
int total_classes = 0;
int total_objects = 0;
int total_namespaces = 0;

std::pair<std::shared_ptr<Program>, size_t> Parser::parse() {
    std::vector<std::shared_ptr<Declaration>> declarations;
    
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    
    while (!isAtEnd()) {
        // Skip any leading semicolons
        while (match({TokenType::SEMICOLON})) {
            std::cout << "Skipping extraneous semicolon" << std::endl;
        }
        
        if (isAtEnd()) break;
        
        std::cout << "Current token: " << tokenTypeToString(peek().type) 
                  << ", Lexeme: " << std::string(peek().lexeme) 
                  << std::endl;
        
        try {
            // Directly parse function declarations at the top level
            if ((check(TokenType::IDENTIFIER) || 
                 check(TokenType::KW_INT) || check(TokenType::KW_VOID) ||
                 check(TokenType::KW_FLOAT) || check(TokenType::KW_CHAR)) &&
                current + 1 < tokens.size() && 
                tokens[current + 1].type == TokenType::IDENTIFIER) {
                
                // Determine if this is a function declaration
                size_t lookAhead = current + 2;
                bool isFunctionDeclaration = false;
                
                // Look ahead to check for function parameter list
                if (lookAhead < tokens.size() && 
                    tokens[lookAhead].type == TokenType::LPAREN) {
                    
                    std::cout << "Detected potential function declaration" << std::endl;
                    
                    // Determine return type
                    std::shared_ptr<Type> returnType;
                    
                    // If first token is a type keyword, use it for return type
                    if (check(TokenType::KW_INT) || check(TokenType::KW_VOID) ||
                        check(TokenType::KW_FLOAT) || check(TokenType::KW_CHAR)) {
                        returnType = parseType();
                    } 
                    // Otherwise, it might be a user-defined type
                    else if (check(TokenType::IDENTIFIER)) {
                        Token typeToken = advance();
                        
                        SourceLocation loc;
                        loc.filename = std::string(typeToken.lexeme);
                        loc.line = static_cast<int>(typeToken.line);
                        loc.column = static_cast<int>(typeToken.column);
                        loc.length = static_cast<int>(typeToken.length);
                        
                        returnType = std::make_shared<UserDefinedType>(
                            loc, 
                            std::string(typeToken.getValueString())
                        );
                    }
                    
                    // Get function name
                    Token nameToken = consume(TokenType::IDENTIFIER, "Expect function name");
                    std::string functionName = std::string(nameToken.getValueString());
                    
                    // Consume left parenthesis
                    consume(TokenType::LPAREN, "Expect '(' after function name");
                    
                    // Parse parameters
                    std::vector<Parameter> params;
                    if (!check(TokenType::RPAREN)) {
                        do {
                            std::shared_ptr<Type> paramType = parseType();
                            Token paramNameToken = consume(TokenType::IDENTIFIER, "Expect parameter name");
                            params.emplace_back(
                                std::string(paramNameToken.getValueString()), 
                                paramType, 
                                false
                            );
                        } while (match({TokenType::COMMA}));
                    }
                    
                    // Consume right parenthesis
                    consume(TokenType::RPAREN, "Expect ')' after parameters");
                    
                    // Parse function body
                    std::shared_ptr<BlockStatement> body = nullptr;
                    if (match({TokenType::LBRACE})) {
                        std::vector<std::shared_ptr<Statement>> statements;
                        
                        // Parse statements within function body
                        while (!check(TokenType::RBRACE) && !isAtEnd()) {
                            try {
                                std::shared_ptr<Statement> stmt = parseStatement();
                                if (stmt) {
                                    statements.push_back(stmt);
                                }
                            } catch (const ParseError& err) {
                                std::cerr << "Error parsing statement: " << err.what() << std::endl;
                                synchronize();
                            }
                        }
                        
                        // Consume closing brace
                        consume(TokenType::RBRACE, "Expect '}' after function body");
                        
                        // Create block statement
                        SourceLocation loc;
                        loc.filename = std::string(nameToken.lexeme);
                        loc.line = static_cast<int>(nameToken.line);
                        loc.column = static_cast<int>(nameToken.column);
                        loc.length = static_cast<int>(nameToken.length);
                        
                        body = std::make_shared<BlockStatement>(loc, statements);
                    }
                    
                    // Optional semicolon after function
                    if (check(TokenType::SEMICOLON)) {
                        advance();
                    }
                    
                    // Create source location
                    SourceLocation loc;
                    loc.filename = std::string(nameToken.lexeme);
                    loc.line = static_cast<int>(nameToken.line);
                    loc.column = static_cast<int>(nameToken.column);
                    loc.length = static_cast<int>(nameToken.length);
                    
                    // Create function declaration
                    auto funcDecl = std::make_shared<FunctionDeclaration>(
                        loc,
                        functionName,
                        returnType,
                        params,
                        body
                    );
                    
                    declarations.push_back(funcDecl);
                    
                    std::cout << "Added function declaration: " << functionName << std::endl;
                    continue;
                }
            }
            
            // Fallback to standard declaration parsing if not a function
            std::shared_ptr<Declaration> decl = parseDeclaration();
            if (decl) {
                declarations.push_back(decl);
            }
        } catch (const ParseError& err) {
            std::cerr << "Parse error: " << err.what() << std::endl;
            synchronize();
        }
    }
    
    // Create program source location
    SourceLocation progLoc;
    progLoc.filename = tokens.empty() ? "" : std::string(tokens[0].lexeme);
    progLoc.line = static_cast<int>(tokens.empty() ? 0 : tokens[0].line);
    progLoc.column = static_cast<int>(tokens.empty() ? 0 : tokens[0].column);
    progLoc.length = static_cast<int>(tokens.empty() ? 0 : tokens.back().line);
    
    // Create the program
    std::shared_ptr<Program> program = std::make_shared<Program>(progLoc, declarations);
    
    // Debug output
    std::cout << "\nProgram Declarations count: " << program->getDeclarations().size() << std::endl;
    for (const auto& decl : program->getDeclarations()) {
        std::cout << "  Declaration: " << decl->getName() << std::endl;
    }
    
    return std::make_pair(program, declarations.size());
}

std::shared_ptr<Expression> Parser::parseExpression() {
    if (check(TokenType::IDENTIFIER) && peek().lexeme == "i" && 
        current + 1 < tokens.size() && tokens[current + 1].type == TokenType::STRING_LITERAL) {
        advance();
        Token stringToken = advance();
        return parseInjectableString();
    }
    
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
                    
                    // Special handling for injectable strings
                    if (check(TokenType::INJECTABLE_STRING)) {
                        advance(); // Consume the injectable string token
                        args.push_back(parseInjectableString());
                    } else {
                        args.push_back(parseExpression());
                    }
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
        else if (match({TokenType::OP_DOUBLE_COLON})) {
            // Scope resolution operator (::)
            Token operatorToken = previous();
            Token memberToken = consume(TokenType::IDENTIFIER, "Expect identifier after '::'");

            // Create source location
            SourceLocation loc;
            loc.filename = std::string(operatorToken.lexeme);
            loc.line = static_cast<int>(operatorToken.line);
            loc.column = static_cast<int>(operatorToken.column);
            loc.length = static_cast<int>(operatorToken.length);

            // Create a ScopeResolutionExpression
            expr = std::make_shared<ScopeResolutionExpression>(
                loc,
                expr,
                std::string(memberToken.getValueString())
            );
        }
        else if (match({TokenType::LBRACKET})) {
            expr = parseIndex(expr);
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

std::shared_ptr<Expression> Parser::parseQualifiedIdentifier() {
    Token startToken = peek();
    
    // First part must be an identifier
    Token identToken = consume(TokenType::IDENTIFIER, "Expect identifier");
    
    SourceLocation loc;
    loc.filename = std::string(identToken.lexeme);
    loc.line = static_cast<int>(identToken.line);
    loc.column = static_cast<int>(identToken.column);
    loc.length = static_cast<int>(identToken.length);
    
    std::shared_ptr<Expression> expr = std::make_shared<IdentifierExpression>(
        loc, 
        std::string(identToken.getValueString())
    );
    
    // Keep parsing scope resolution operators
    while (match({TokenType::OP_DOUBLE_COLON})) {
        Token scopeToken = previous();
        Token nextIdentToken = consume(TokenType::IDENTIFIER, "Expect identifier after '::'");
        
        SourceLocation scopeLoc;
        scopeLoc.filename = std::string(scopeToken.lexeme);
        scopeLoc.line = static_cast<int>(scopeToken.line);
        scopeLoc.column = static_cast<int>(scopeToken.column);
        scopeLoc.length = static_cast<int>(scopeToken.length);
        
        expr = std::make_shared<ScopeResolutionExpression>(
            scopeLoc,
            expr,
            std::string(nextIdentToken.getValueString())
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parsePrimaryExpression() {
    // Debug output
    std::cout << "Parsing primary expression. Current token: " 
              << tokenTypeToString(peek().type) 
              << ", Lexeme: " << std::string(peek().lexeme) 
              << std::endl;
    
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
        
        std::cout << "Parsing string literal: '" << std::get<std::string>(literalToken.value.value) << "'" << std::endl;
        
        // Create source location
        SourceLocation loc;
        loc.filename = std::string(literalToken.lexeme);
        loc.line = static_cast<int>(literalToken.line);
        loc.column = static_cast<int>(literalToken.column);
        loc.length = static_cast<int>(literalToken.length);

        // Create type for string literal
        auto stringType = std::make_shared<UserDefinedType>(
            loc, 
            "string"
        );

        auto literalExpr = std::make_shared<LiteralExpression>(
            loc, 
            std::get<std::string>(literalToken.value.value), 
            stringType
        );
        
        std::cout << "Created string literal expression" << std::endl;
        return literalExpr;
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
    if (check(TokenType::IDENTIFIER)) {
        // Check if we have a scope resolution ahead
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::OP_DOUBLE_COLON) {
            // Handle qualified identifier (namespace::identifier)
            return parseQualifiedIdentifier();
        } else {
            // Regular identifier
            Token identToken = advance();
            
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
    }

    // Parenthesized expression
    if (match({TokenType::LPAREN})) {
        std::shared_ptr<Expression> expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return expr;
    }

    // Injectable string
    if (check(TokenType::IDENTIFIER) && peek().lexeme == "i" && 
        current + 1 < tokens.size() && tokens[current + 1].type == TokenType::STRING_LITERAL) {
        
        // Consume the 'i'
        advance();
        
        // Parse the string literal as an injectable string
        Token stringToken = advance();
        
        // Return the injectable string expression
        return std::make_shared<InjectableStringExpression>(
            SourceLocation{
                std::string(stringToken.lexeme), 
                static_cast<int>(stringToken.line), 
                static_cast<int>(stringToken.column), 
                static_cast<int>(stringToken.length)
            }, 
            std::get<std::string>(stringToken.value.value), 
            std::vector<std::shared_ptr<Expression>>{}
        );
    }
    
    throw error(peek(), "Expected expression");
}

std::shared_ptr<Declaration> Parser::parseDeclaration() {
    total_decs++;

    // Skip any stray semicolons
    while (match({TokenType::SEMICOLON})) {
        std::cout << "Skipping extraneous semicolon" << std::endl;
    }

    // Handle simple function declarations: int main(), void test(), etc.
    if (check(TokenType::KW_INT) || check(TokenType::KW_VOID) || 
        check(TokenType::KW_FLOAT) || check(TokenType::KW_CHAR)) {
        
        auto returnType = parseType();
        
        if (check(TokenType::IDENTIFIER)) {
            Token nameToken = peek();
            std::string functionName = std::string(nameToken.getValueString());
            
            // Check if the next token is an opening parenthesis for a function
            if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LPAREN) {
                total_funcs++;
                return parseFunctionDeclaration();
            }
        }
    }
    
    // Original parsing logic
    if (match({TokenType::KW_NAMESPACE})) {
        total_namespaces++;
        return parseNamespaceDeclaration();
    }
    
    if (match({TokenType::KW_CLASS})) {
        total_classes++;
        return parseClassDeclaration();
    }
    
    if (match({TokenType::KW_STRUCT})) {
        total_structs++;
        return parseStructDeclaration();
    }
    
    if (match({TokenType::KW_OBJECT})) {
        total_objects++;
        return parseObjectDeclaration();
    }
    
    if (match({TokenType::KW_OPERATOR})) {
        return parseOperatorDeclaration();
    }
    
    if (match({TokenType::KW_IMPORT})) {
        return parseImportDeclaration();
    }
    
    // Try to parse a statement if it looks like a valid expression
    if (check(TokenType::IDENTIFIER)) {
        std::string identName = std::string(peek().getValueString());
        
        // Check for builtin function calls as statements (like print)
        if (identName == "print" || identName == "input" || 
            identName == "open" || identName == "socket") {
            
            Token identToken = advance();
            
            if (match({TokenType::LPAREN})) {
                // Parse arguments
                std::vector<std::shared_ptr<Expression>> args;
                if (!check(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match({TokenType::COMMA}));
                }
                
                consume(TokenType::RPAREN, "Expect ')' after arguments");
                consume(TokenType::SEMICOLON, "Expect ';' after statement");
                
                // Create source location
                SourceLocation loc;
                loc.filename = std::string(identToken.lexeme);
                loc.line = static_cast<int>(identToken.line);
                loc.column = static_cast<int>(identToken.column);
                loc.length = static_cast<int>(identToken.length);
                
                // Create expression statement with call expression
                auto expr = std::make_shared<BuiltinCallExpression>(
                    loc, 
                    identName == "print" ? BuiltinCallExpression::BuiltinType::PRINT :
                    identName == "input" ? BuiltinCallExpression::BuiltinType::INPUT :
                    identName == "open" ? BuiltinCallExpression::BuiltinType::OPEN :
                    BuiltinCallExpression::BuiltinType::SOCKET,
                    args
                );
                
                auto stmt = std::make_shared<ExpressionStatement>(loc, expr);
                
                // Create a wrapper declaration for the statement
                return std::make_shared<FunctionDeclaration>(
                    loc,
                    "statement_" + std::to_string(total_decs),
                    std::make_shared<PrimitiveType>(
                        loc, PrimitiveTypeKind::VOID, BitWidth::DEFAULT, false
                    ),
                    std::vector<Parameter>{},
                    std::make_shared<BlockStatement>(loc, std::vector<std::shared_ptr<Statement>>{stmt})
                );
            }
        }
    }
    
    // Try parsing a type followed by variable or function declaration
    size_t startPos = current;
    try {
        std::shared_ptr<Type> type = parseType();
        
        if (check(TokenType::IDENTIFIER)) {
            // Check if this is a function declaration
            if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LPAREN) {
                total_funcs++;
                return parseFunctionDeclaration();
            }
            
            // Otherwise it's a variable declaration
            total_vars++;
            return parseVariableDeclaration();
        }
    } catch (const ParseError& e) {
        current = startPos;
    }
    
    // Special case for function with inferred return type
    if (check(TokenType::IDENTIFIER) && 
        current + 1 < tokens.size() && 
        tokens[current + 1].type == TokenType::LPAREN) {
        
        Token nameToken = advance();
        
        SourceLocation loc;
        loc.filename = std::string(nameToken.lexeme);
        loc.line = static_cast<int>(nameToken.line);
        loc.column = static_cast<int>(nameToken.column);
        loc.length = static_cast<int>(nameToken.length);
        
        auto returnType = std::make_shared<PrimitiveType>(
            loc, 
            PrimitiveTypeKind::INT,
            BitWidth::_32,
            false
        );
        
        total_funcs++;
        
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
        
        std::shared_ptr<BlockStatement> body = nullptr;
        if (check(TokenType::LBRACE)) {
            body = parseBlock();
        }
        
        return std::make_shared<FunctionDeclaration>(
            loc,
            std::string(nameToken.getValueString()),
            returnType,
            params,
            body
        );
    }
    
    // Fallback error
    std::cout << "No valid declaration found. Current token: " 
              << tokenTypeToString(peek().type) 
              << ", Lexeme: " << std::string(peek().lexeme) 
              << std::endl;
    throw error(peek(), "Expected declaration");
}

std::shared_ptr<NamespaceDeclaration> Parser::parseNamespaceDeclaration() {
    Token namespaceToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect namespace name");
    
    consume(TokenType::LBRACE, "Expect '{' after namespace name");
    
    std::vector<std::shared_ptr<ClassDeclaration>> classes;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match({TokenType::KW_CLASS})) {
            total_classes++;
            classes.push_back(parseClassDeclaration());
        } else {
            throw error(peek(), "Namespaces can only contain classes");
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after namespace body");
    
    // Optional semicolon after namespace declaration
    match({TokenType::SEMICOLON});
    
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
    std::cout << "Synchronizing parser state..." << std::endl;
    
    // Skip the current token that caused the error
    advance();
    
    while (!isAtEnd()) {
        // Look for statement or declaration boundaries
        if (previous().type == TokenType::SEMICOLON) {
            std::cout << "Synchronized at semicolon" << std::endl;
            return;
        }
        
        if (previous().type == TokenType::RBRACE) {
            std::cout << "Synchronized at right brace" << std::endl;
            return;
        }
        
        // Look for function declarations
        if ((peek().type == TokenType::KW_INT || 
             peek().type == TokenType::KW_VOID ||
             peek().type == TokenType::KW_FLOAT ||
             peek().type == TokenType::KW_CHAR) && 
            current + 1 < tokens.size() && 
            tokens[current + 1].type == TokenType::IDENTIFIER &&
            current + 2 < tokens.size() && 
            tokens[current + 2].type == TokenType::LPAREN) {
                
            std::cout << "Synchronized at function declaration" << std::endl;
            return;
        }
        
        // Look for potential declaration starts
        switch (peek().type) {
            case TokenType::KW_CLASS:
            case TokenType::KW_STRUCT:
            case TokenType::KW_OBJECT:
            case TokenType::KW_NAMESPACE:
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_CHAR:
            case TokenType::KW_VOID:
                std::cout << "Synchronized at keyword: " << tokenTypeToString(peek().type) << std::endl;
                return;
                
            case TokenType::KW_FOR:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_RETURN:
            case TokenType::KW_IMPORT:
                // Only synchronize at these if we're inside a function body
                if (peek().type == TokenType::KW_RETURN) {
                    std::cout << "Synchronized at return statement" << std::endl;
                    return;
                }
                break;
                
            default:
                break;
        }
        
        // Check for function declarations by looking ahead
        if (peek().type == TokenType::IDENTIFIER && 
            current + 1 < tokens.size() && 
            tokens[current + 1].type == TokenType::LPAREN) {
            std::cout << "Synchronized at function call or declaration" << std::endl;
            return;
        }
        
        advance();
    }
    
    std::cout << "Synchronized at end of file" << std::endl;
}

std::shared_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    Token classToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect class name");
    
    consume(TokenType::LBRACE, "Expect '{' after class name");
    
    std::vector<std::shared_ptr<Declaration>> members;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            if (match({TokenType::KW_OBJECT})) {
                members.push_back(parseObjectDeclaration());
            } 
            else if (match({TokenType::KW_STRUCT})) {
                total_structs++;
                members.push_back(parseStructDeclaration());
            }
            else {
                throw error(peek(), "Classes can only contain objects and structs");
            }
        }
        catch (const ParseError& err) {
            std::cerr << "Error parsing class member: " << err.what() << std::endl;
            synchronize();
        }
    }
    
    consume(TokenType::RBRACE, "Expected: '}' after class body");
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
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
    std::string structName = "";
    Token nameToken = structToken; // Initialize with a valid token
    
    // Check if the struct has a name before the opening brace
    if (!check(TokenType::LBRACE)) {
        // This is a standard named struct declaration: struct MyStruct { ... }
        nameToken = consume(TokenType::IDENTIFIER, "Expect struct name");
        structName = std::string(nameToken.getValueString());
    }
    
    consume(TokenType::LBRACE, "Expect '{' after struct keyword or name");
    
    std::vector<VariableDeclaration> fields;
    
    // Parse all fields inside the struct
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        std::shared_ptr<Type> fieldType = parseType();
        Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expect field name");
        
        // Parse initializer if present
        std::shared_ptr<Expression> initializer = nullptr;
        if (match({TokenType::OP_EQUAL})) {
            initializer = parseExpression();
        }
        
        // Expect semicolon after field
        consume(TokenType::SEMICOLON, "Expect ';' after struct field");
        
        // Create field source location
        SourceLocation fieldLoc;
        fieldLoc.filename = std::string(fieldNameToken.lexeme);
        fieldLoc.line = static_cast<int>(fieldNameToken.line);
        fieldLoc.column = static_cast<int>(fieldNameToken.column);
        fieldLoc.length = static_cast<int>(fieldNameToken.length);
        
        // Add field to the struct
        fields.emplace_back(
            fieldLoc,
            std::string(fieldNameToken.getValueString()),
            fieldType,
            initializer
        );
    }
    
    consume(TokenType::RBRACE, "Expect '}' after struct body");
    
    // For anonymous structs, look for a variable name after the closing brace
    if (structName.empty()) {
        nameToken = consume(TokenType::IDENTIFIER, "Expect variable name after anonymous struct");
        structName = std::string(nameToken.getValueString());
    }
    
    // Consume optional semicolon
    match({TokenType::SEMICOLON});
    
    // Create source location for the struct
    SourceLocation loc;
    loc.filename = std::string(structToken.lexeme);
    loc.line = static_cast<int>(structToken.line);
    loc.column = static_cast<int>(structToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    // Return the struct declaration
    return std::make_shared<StructDeclaration>(
        loc,
        structName,
        fields
    );
}

std::shared_ptr<Declaration> Parser::parseObjectMemberDeclaration() {
    // Parse type for the field
    std::shared_ptr<Type> fieldType = parseType();
    
    // Get the field name
    Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expect member name");
    std::string fieldName = std::string(fieldNameToken.getValueString());
    
    // Create source location for the field
    SourceLocation fieldLoc;
    fieldLoc.filename = std::string(fieldNameToken.lexeme);
    fieldLoc.line = static_cast<int>(fieldNameToken.line);
    fieldLoc.column = static_cast<int>(fieldNameToken.column);
    fieldLoc.length = static_cast<int>(fieldNameToken.length);
    
    // Check for initializer
    std::shared_ptr<Expression> initializer = nullptr;
    if (match({TokenType::OP_EQUAL})) {
        initializer = parseExpression();
    }
    
    // Consume trailing semicolon
    consume(TokenType::SEMICOLON, "Expect ';' after member declaration");
    
    // Return a VariableDeclaration for the member
    return std::make_shared<VariableDeclaration>(
        fieldLoc,
        fieldName,
        fieldType,
        initializer
    );
}

std::shared_ptr<ObjectDeclaration> Parser::parseObjectDeclaration() {
    Token objectToken = previous();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect object name");
    
    consume(TokenType::LBRACE, "Expect '{' after object name");
    
    std::vector<std::shared_ptr<Declaration>> members;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            // Handle different types of members
            if (match({TokenType::KW_STRUCT})) {
                total_structs++;
                members.push_back(parseStructDeclaration());
            } 
            else if (match({TokenType::KW_OBJECT})) {
                members.push_back(parseObjectDeclaration());
            } 
            // Parse variable declarations (member variables)
            else if (check(TokenType::KW_CHAR) || check(TokenType::KW_INT) || 
                    check(TokenType::KW_FLOAT) || check(TokenType::KW_BOOL) || 
                    check(TokenType::KW_VOID)) {
                
                // Parse member variable
                members.push_back(parseObjectMemberDeclaration());
                std::cout << "Added member variable to object: " 
                          << members.back()->getName() << std::endl;
            } 
            else if (check(TokenType::IDENTIFIER)) {
                // Check if this is a user-defined type
                Token typeToken = advance();
                std::string typeName = std::string(typeToken.getValueString());
                
                if (check(TokenType::IDENTIFIER)) {
                    // This is a variable declaration with a user-defined type
                    Token nameToken = advance();
                    std::string name = std::string(nameToken.getValueString());
                    
                    // Create source location
                    SourceLocation loc;
                    loc.filename = std::string(typeToken.lexeme);
                    loc.line = static_cast<int>(typeToken.line);
                    loc.column = static_cast<int>(typeToken.column);
                    loc.length = static_cast<int>(nameToken.length);
                    
                    // Create a type for the variable
                    auto type = std::make_shared<UserDefinedType>(loc, typeName);
                    
                    // Check for initializer
                    std::shared_ptr<Expression> initializer = nullptr;
                    if (match({TokenType::OP_EQUAL})) {
                        initializer = parseExpression();
                    }
                    
                    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
                    
                    // Add variable declaration
                    members.push_back(std::make_shared<VariableDeclaration>(
                        loc, name, type, initializer
                    ));
                    
                    std::cout << "Added user-defined type member: " 
                              << typeName << " " << name << std::endl;
                } else {
                    throw error(peek(), "Expected identifier after type name");
                }
            } 
            else {
                throw error(peek(), "Expected member declaration in object");
            }
        } catch (const ParseError& err) {
            std::cerr << "Error parsing object member: " << err.what() << std::endl;
            synchronize();
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after object body");
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    // Optional semicolon
    match({TokenType::SEMICOLON});
    
    SourceLocation loc;
    loc.filename = std::string(objectToken.lexeme);
    loc.line = static_cast<int>(objectToken.line);
    loc.column = static_cast<int>(objectToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    std::cout << "Created object declaration: " << std::string(nameToken.getValueString()) 
              << " with " << members.size() << " members" << std::endl;
              
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
    // At this point we've already parsed the return type
    std::shared_ptr<Type> returnType = nullptr;
    
    // Save the current position in case we need to rewind
    size_t startPos = current;
    
    // If we're at a type token, parse the return type
    if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) || 
        check(TokenType::KW_CHAR) || check(TokenType::KW_VOID)) {
        returnType = parseType();
    }
    
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect function name");
    
    // Debug print
    std::cout << "Parsing Function Declaration: " << std::string(nameToken.getValueString()) << std::endl;
    
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
    
    SourceLocation loc;
    loc.filename = std::string(nameToken.lexeme);
    loc.line = static_cast<int>(nameToken.line);
    loc.column = static_cast<int>(nameToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    // If return type wasn't parsed earlier, use a default
    if (!returnType) {
        returnType = std::make_shared<PrimitiveType>(
            loc,
            nameToken.getValueString() == "main" ? PrimitiveTypeKind::INT : PrimitiveTypeKind::VOID,
            BitWidth::_32,
            false
        );
    }
    
    std::shared_ptr<BlockStatement> body = nullptr;
    if (match({TokenType::LBRACE})) {
        // Parse function body
        std::vector<std::shared_ptr<Statement>> statements;
        
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            try {
                if (check(TokenType::IDENTIFIER)) {
                    std::string identName = std::string(peek().getValueString());
                    size_t peekPos = current;
                    
                    // Handle known builtin functions or function calls
                    if ((identName == "print" || identName == "input" || 
                         identName == "open" || identName == "socket" || 
                         identName == "test") &&  // Add test function too
                        current + 1 < tokens.size() && 
                        tokens[current + 1].type == TokenType::LPAREN) {
                        
                        Token identToken = advance();
                        
                        if (match({TokenType::LPAREN})) {
                            // Parse function call arguments
                            std::vector<std::shared_ptr<Expression>> args;
                            
                            if (!check(TokenType::RPAREN)) {
                                do {
                                    args.push_back(parseExpression());
                                } while (match({TokenType::COMMA}));
                            }
                            
                            consume(TokenType::RPAREN, "Expect ')' after arguments");
                            consume(TokenType::SEMICOLON, "Expect ';' after statement");
                            
                            // Create source location
                            SourceLocation callLoc;
                            callLoc.filename = std::string(identToken.lexeme);
                            callLoc.line = static_cast<int>(identToken.line);
                            callLoc.column = static_cast<int>(identToken.column);
                            callLoc.length = static_cast<int>(identToken.length);
                            
                            // Create appropriate expression
                            std::shared_ptr<Expression> expr;
                            if (identName == "print" || identName == "input" || 
                                identName == "open" || identName == "socket") {
                                // Create builtin call expression
                                expr = std::make_shared<BuiltinCallExpression>(
                                    callLoc, 
                                    identName == "print" ? BuiltinCallExpression::BuiltinType::PRINT :
                                    identName == "input" ? BuiltinCallExpression::BuiltinType::INPUT :
                                    identName == "open" ? BuiltinCallExpression::BuiltinType::OPEN :
                                    BuiltinCallExpression::BuiltinType::SOCKET,
                                    args
                                );
                            } else {
                                // Create regular function call expression
                                auto identExpr = std::make_shared<IdentifierExpression>(callLoc, identName);
                                expr = std::make_shared<CallExpression>(callLoc, identExpr, args);
                            }
                            
                            // Add expression statement to the body
                            statements.push_back(std::make_shared<ExpressionStatement>(callLoc, expr));
                            std::cout << "Added function call statement for: " << identName << std::endl;
                            continue;
                        } else {
                            current = peekPos; // Rewind if not a function call
                        }
                    }
                }
                
                // Handle return statements
                if (match({TokenType::KW_RETURN})) {
                    Token returnToken = previous();
                    
                    std::shared_ptr<Expression> returnValue = nullptr;
                    if (!check(TokenType::SEMICOLON)) {
                        returnValue = parseExpression();
                    }
                    
                    consume(TokenType::SEMICOLON, "Expect ';' after return value");
                    
                    SourceLocation returnLoc;
                    returnLoc.filename = std::string(returnToken.lexeme);
                    returnLoc.line = static_cast<int>(returnToken.line);
                    returnLoc.column = static_cast<int>(returnToken.column);
                    returnLoc.length = static_cast<int>(returnToken.length);
                    
                    statements.push_back(std::make_shared<ReturnStatement>(returnLoc, returnValue));
                    std::cout << "Added return statement" << std::endl;
                    continue;
                }
                
                // Fall back to regular statement parsing
                statements.push_back(parseStatement());
            } catch (const ParseError& err) {
                std::cerr << "Error parsing statement in function body: " << err.what() << std::endl;
                synchronize();
            }
        }
        
        consume(TokenType::RBRACE, "Expect '}' after function body");
        
        // Create block statement for the function body
        body = std::make_shared<BlockStatement>(loc, statements);
    } else {
        consume(TokenType::SEMICOLON, "Expect ';' after function declaration");
    }
    
    // Consume the semicolon after the function declaration
    if (check(TokenType::SEMICOLON)) {
        std::cout << "Consuming trailing semicolon after function declaration" << std::endl;
        advance(); // Consume the semicolon
    }
    
    // Debug print
    std::cout << "Parsed Function: " 
              << std::string(nameToken.getValueString()) 
              << ", Return Type: " << returnType->toString() 
              << ", Params: " << params.size() 
              << ", Has Body: " << (body != nullptr) 
              << std::endl;
    
    return std::make_shared<FunctionDeclaration>(
        loc,
        std::string(nameToken.getValueString()),
        returnType,
        params,
        body
    );
}

std::shared_ptr<FunctionDeclaration> Parser::parseFunctionDeclarationDirect() {
    // At this point we expect to be at a type token for the return type
    std::shared_ptr<Type> returnType = parseType();
    
    // Get the function name
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect function name");
    std::string functionName = std::string(nameToken.getValueString());
    
    // Debug print
    std::cout << "Directly parsing Function Declaration: " << functionName << std::endl;
    
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
    
    SourceLocation loc;
    loc.filename = std::string(nameToken.lexeme);
    loc.line = static_cast<int>(nameToken.line);
    loc.column = static_cast<int>(nameToken.column);
    loc.length = static_cast<int>(nameToken.length);
    
    // Parse the function body
    std::shared_ptr<BlockStatement> body = nullptr;
    if (match({TokenType::LBRACE})) {
        // Parse function body statements
        std::vector<std::shared_ptr<Statement>> statements;
        
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            try {
                // Handle builtin function calls
                if (check(TokenType::IDENTIFIER)) {
                    std::string identName = std::string(peek().getValueString());
                    
                    if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LPAREN) {
                        Token identToken = advance();
                        
                        if (match({TokenType::LPAREN})) {
                            // Parse arguments
                            std::vector<std::shared_ptr<Expression>> args;
                            
                            if (!check(TokenType::RPAREN)) {
                                do {
                                    std::shared_ptr<Expression> arg = parseExpression();
                                    args.push_back(arg);
                                } while (match({TokenType::COMMA}));
                            }
                            
                            consume(TokenType::RPAREN, "Expect ')' after arguments");
                            consume(TokenType::SEMICOLON, "Expect ';' after function call");
                            
                            // Create source location
                            SourceLocation callLoc;
                            callLoc.filename = std::string(identToken.lexeme);
                            callLoc.line = static_cast<int>(identToken.line);
                            callLoc.column = static_cast<int>(identToken.column);
                            callLoc.length = static_cast<int>(identToken.length);
                            
                            // Create appropriate expression based on function type
                            std::shared_ptr<Expression> expr;
                            if (identName == "print" || identName == "input" || 
                                identName == "open" || identName == "socket") {
                                // Builtin function call
                                expr = std::make_shared<BuiltinCallExpression>(
                                    callLoc, 
                                    identName == "print" ? BuiltinCallExpression::BuiltinType::PRINT :
                                    identName == "input" ? BuiltinCallExpression::BuiltinType::INPUT :
                                    identName == "open" ? BuiltinCallExpression::BuiltinType::OPEN :
                                    BuiltinCallExpression::BuiltinType::SOCKET,
                                    args
                                );
                            } else {
                                // User-defined function call
                                auto identExpr = std::make_shared<IdentifierExpression>(callLoc, identName);
                                expr = std::make_shared<CallExpression>(callLoc, identExpr, args);
                            }
                            
                            statements.push_back(std::make_shared<ExpressionStatement>(callLoc, expr));
                            std::cout << "Added function call to " << identName << " in function body" << std::endl;
                            continue;
                        }
                    }
                }
                
                // Handle return statements
                if (match({TokenType::KW_RETURN})) {
                    Token returnToken = previous();
                    
                    std::shared_ptr<Expression> returnValue = nullptr;
                    if (!check(TokenType::SEMICOLON)) {
                        returnValue = parseExpression();
                    }
                    
                    consume(TokenType::SEMICOLON, "Expect ';' after return statement");
                    
                    SourceLocation returnLoc;
                    returnLoc.filename = std::string(returnToken.lexeme);
                    returnLoc.line = static_cast<int>(returnToken.line);
                    returnLoc.column = static_cast<int>(returnToken.column);
                    returnLoc.length = static_cast<int>(returnToken.length);
                    
                    statements.push_back(std::make_shared<ReturnStatement>(returnLoc, returnValue));
                    std::cout << "Added return statement to function body" << std::endl;
                    continue;
                }
                
                // Parse other statement types
                std::shared_ptr<Statement> stmt = parseStatement();
                if (stmt) {
                    statements.push_back(stmt);
                }
            } catch (const ParseError& err) {
                std::cerr << "Error parsing statement in function body: " << err.what() << std::endl;
                synchronize();
            }
        }
        
        consume(TokenType::RBRACE, "Expect '}' after function body");
        
        // Create the block statement
        body = std::make_shared<BlockStatement>(loc, statements);
    }
    
    // Consume optional semicolon after function
    if (check(TokenType::SEMICOLON)) {
        advance();
    }
    
    // Create and return the function declaration
    auto funcDecl = std::make_shared<FunctionDeclaration>(
        loc,
        functionName,
        returnType,
        params,
        body
    );
    
    std::cout << "Successfully parsed function: " << functionName << std::endl;
    total_funcs++;
    
    return funcDecl;
}

std::shared_ptr<BlockStatement> Parser::parseBlock() {
    Token braceToken = consume(TokenType::LBRACE, "Expect '{' at start of block");
    
    std::vector<std::shared_ptr<Statement>> statements;
    
    std::cout << "Parsing block starting at line " << braceToken.line << std::endl;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            // Check for builtin function calls or other function calls
            if (check(TokenType::IDENTIFIER)) {
                std::string identName = std::string(peek().getValueString());
                
                // Special handling for function calls (both builtin and user-defined)
                if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LPAREN) {
                    Token identToken = advance();
                    
                    if (match({TokenType::LPAREN})) {
                        // Parse arguments
                        std::vector<std::shared_ptr<Expression>> args;
                        
                        if (!check(TokenType::RPAREN)) {
                            do {
                                std::shared_ptr<Expression> arg = parseExpression();
                                if (arg) {
                                    args.push_back(arg);
                                } else {
                                    std::cerr << "Warning: Null expression in function arguments" << std::endl;
                                }
                            } while (match({TokenType::COMMA}));
                        }
                        
                        consume(TokenType::RPAREN, "Expect ')' after arguments");
                        consume(TokenType::SEMICOLON, "Expect ';' after statement");
                        
                        // Create source location
                        SourceLocation loc;
                        loc.filename = std::string(identToken.lexeme);
                        loc.line = static_cast<int>(identToken.line);
                        loc.column = static_cast<int>(identToken.column);
                        loc.length = static_cast<int>(identToken.length);
                        
                        // Create appropriate expression
                        std::shared_ptr<Expression> expr;
                        if (identName == "print" || identName == "input" || 
                            identName == "open" || identName == "socket") {
                            // Create builtin call expression
                            expr = std::make_shared<BuiltinCallExpression>(
                                loc, 
                                identName == "print" ? BuiltinCallExpression::BuiltinType::PRINT :
                                identName == "input" ? BuiltinCallExpression::BuiltinType::INPUT :
                                identName == "open" ? BuiltinCallExpression::BuiltinType::OPEN :
                                BuiltinCallExpression::BuiltinType::SOCKET,
                                args
                            );
                        } else {
                            // Create regular function call expression for user-defined functions
                            auto identExpr = std::make_shared<IdentifierExpression>(loc, identName);
                            expr = std::make_shared<CallExpression>(loc, identExpr, args);
                        }
                        
                        // Add expression statement to the body
                        auto exprStmt = std::make_shared<ExpressionStatement>(loc, expr);
                        statements.push_back(exprStmt);
                        
                        std::cout << "Added function call statement for: " << identName << std::endl;
                        continue;
                    }
                }
            }
            
            // Handle return statements
            if (match({TokenType::KW_RETURN})) {
                Token returnToken = previous();
                
                std::shared_ptr<Expression> returnValue = nullptr;
                if (!check(TokenType::SEMICOLON)) {
                    try {
                        returnValue = parseExpression();
                        std::cout << "Return statement has expression value" << std::endl;
                    } catch (const ParseError& e) {
                        std::cerr << "Error parsing return expression: " << e.what() << std::endl;
                        synchronize();
                    }
                } else {
                    std::cout << "Return statement has no expression (void return)" << std::endl;
                }
                
                consume(TokenType::SEMICOLON, "Expect ';' after return value");
                
                SourceLocation returnLoc;
                returnLoc.filename = std::string(returnToken.lexeme);
                returnLoc.line = static_cast<int>(returnToken.line);
                returnLoc.column = static_cast<int>(returnToken.column);
                returnLoc.length = static_cast<int>(returnToken.length);
                
                statements.push_back(std::make_shared<ReturnStatement>(returnLoc, returnValue));
                std::cout << "Added return statement" << std::endl;
                continue;
            }
            
            // Handle other statement types
            std::shared_ptr<Statement> stmt = parseStatement();
            if (stmt) {
                statements.push_back(stmt);
                std::cout << "Added generic statement to block" << std::endl;
            } else {
                std::cerr << "Warning: Failed to parse statement in block" << std::endl;
            }
        } catch (const ParseError& err) {
            std::cerr << "Error parsing statement in block: " << err.what() << std::endl;
            synchronize();
            
            // If we're synchronized at a token that suggests we're outside 
            // the current function, break out of the loop
            if ((peek().type == TokenType::KW_INT || 
                 peek().type == TokenType::KW_VOID || 
                 peek().type == TokenType::KW_FLOAT || 
                 peek().type == TokenType::KW_CHAR) &&
                current + 1 < tokens.size() && 
                tokens[current + 1].type == TokenType::IDENTIFIER &&
                current + 2 < tokens.size() && 
                tokens[current + 2].type == TokenType::LPAREN) {
                
                std::cout << "Detected start of a new function declaration, ending current block" << std::endl;
                break;
            }
        }
    }
    
    // Make sure we consume the closing brace if present
    if (check(TokenType::RBRACE)) {
        Token closeBraceToken = consume(TokenType::RBRACE, "Expect '}' after block");
    } else {
        std::cerr << "Warning: Block ended without closing brace" << std::endl;
    }
    
    SourceLocation loc;
    loc.filename = std::string(braceToken.lexeme);
    loc.line = static_cast<int>(braceToken.line);
    loc.column = static_cast<int>(braceToken.column);
    loc.length = 1; // Just the opening brace length for simplicity
    
    std::cout << "Completed block with " << statements.size() << " statements" << std::endl;
    
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

// Statement parsing methods
std::shared_ptr<Statement> Parser::parseStatement() {
    // First, check for statement keywords
    if (match({TokenType::KW_IF})) {
        return parseIfStatement();
    }
    
    if (match({TokenType::KW_WHILE})) {
        return parseWhileStatement();
    }
    
    if (match({TokenType::KW_FOR})) {
        return parseForStatement();
    }
    
    if (match({TokenType::KW_RETURN})) {
        return parseReturnStatement();
    }
    
    if (match({TokenType::KW_BREAK})) {
        return parseBreakStatement();
    }
    
    if (match({TokenType::KW_CONTINUE})) {
        return parseContinueStatement();
    }
    
    if (match({TokenType::KW_THROW})) {
        return parseThrowStatement();
    }
    
    if (match({TokenType::KW_TRY})) {
        return parseTryCatchStatement();
    }
    
    if (match({TokenType::KW_ASM})) {
        return parseASMStatement();
    }
    
    if (match({TokenType::LBRACE})) {
        return parseBlock();
    }
    
    // Check for qualified object instantiation: A::B::C{} d;
    if (check(TokenType::IDENTIFIER)) {
        // Save current position in case we need to rewind
        size_t startPos = current;
        
        // Try to parse a potentially qualified type name
        bool hasScope = false;
        
        // Record all tokens for rewind
        std::vector<Token> tokens;
        tokens.push_back(advance()); // First identifier
        
        // Check for scope resolution operator
        while (check(TokenType::OP_DOUBLE_COLON) && current + 1 < this->tokens.size() && 
               this->tokens[current + 1].type == TokenType::IDENTIFIER) {
            hasScope = true;
            tokens.push_back(advance()); // ::
            tokens.push_back(advance()); // Identifier
        }
        
        // Check for {} pattern indicating object instantiation
        if (match({TokenType::LBRACE})) {
            tokens.push_back(previous()); // {
            
            // Skip anything inside the braces
            while (!check(TokenType::RBRACE) && !isAtEnd()) {
                tokens.push_back(advance());
            }
            
            if (match({TokenType::RBRACE})) {
                tokens.push_back(previous()); // }
                
                // Check for variable name
                if (check(TokenType::IDENTIFIER)) {
                    Token varNameToken = advance();
                    
                    // Check for semicolon
                    if (match({TokenType::SEMICOLON})) {
                        // This is an object instantiation
                        
                        // Create the qualified type expression
                        std::shared_ptr<Expression> typeExpr = nullptr;
                        
                        // First token is the start of the type
                        Token firstToken = tokens[0];
                        typeExpr = std::make_shared<IdentifierExpression>(
                            SourceLocation{
                                std::string(firstToken.lexeme),
                                static_cast<int>(firstToken.line),
                                static_cast<int>(firstToken.column),
                                static_cast<int>(firstToken.length)
                            },
                            std::string(firstToken.getValueString())
                        );
                        
                        // Build the scope resolution chain
                        for (size_t i = 1; i < tokens.size() && i + 1 < tokens.size(); i += 2) {
                            if (tokens[i].type == TokenType::OP_DOUBLE_COLON && 
                                tokens[i+1].type == TokenType::IDENTIFIER) {
                                
                                typeExpr = std::make_shared<ScopeResolutionExpression>(
                                    SourceLocation{
                                        std::string(tokens[i].lexeme),
                                        static_cast<int>(tokens[i].line),
                                        static_cast<int>(tokens[i].column),
                                        static_cast<int>(tokens[i].length)
                                    },
                                    typeExpr,
                                    std::string(tokens[i+1].getValueString())
                                );
                            }
                        }
                        
                        // Create object instantiation expression
                        auto objInstExpr = std::make_shared<ObjectInstantiationExpression>(
                            SourceLocation{
                                std::string(firstToken.lexeme),
                                static_cast<int>(firstToken.line),
                                static_cast<int>(firstToken.column),
                                static_cast<int>(varNameToken.length)
                            },
                            "", // Empty for qualified types
                            std::string(varNameToken.getValueString())
                        );
                        
                        // Set qualified type expression
                        objInstExpr->setTypeExpr(typeExpr);
                        
                        std::cout << "Parsed qualified object instantiation" << std::endl;
                        
                        // Return expression statement
                        return std::make_shared<ExpressionStatement>(
                            SourceLocation{
                                std::string(firstToken.lexeme),
                                static_cast<int>(firstToken.line),
                                static_cast<int>(firstToken.column),
                                static_cast<int>(varNameToken.length)
                            },
                            objInstExpr
                        );
                    }
                }
            }
        }
        
        // Not an object instantiation, rewind
        current = startPos;
    }
    
    // If no other statement type matches, parse an expression statement
    return parseExpressionStatement();
}

std::shared_ptr<IfStatement> Parser::parseIfStatement() {
    Token ifToken = previous();
    
    consume(TokenType::LPAREN, "Expect '(' after 'if'");
    std::shared_ptr<Expression> condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after if condition");
    
    std::shared_ptr<Statement> thenBranch = parseStatement();
    std::shared_ptr<Statement> elseBranch = nullptr;
    
    if (match({TokenType::KW_ELSE})) {
        elseBranch = parseStatement();
    }
    
    SourceLocation loc;
    loc.filename = std::string(ifToken.lexeme);
    loc.line = static_cast<int>(ifToken.line);
    loc.column = static_cast<int>(ifToken.column);
    loc.length = static_cast<int>(ifToken.length);
    
    return std::make_shared<IfStatement>(loc, condition, thenBranch, elseBranch);
}

std::shared_ptr<WhileStatement> Parser::parseWhileStatement() {
    Token whileToken = previous();
    
    consume(TokenType::LPAREN, "Expect '(' after 'while'");
    std::shared_ptr<Expression> condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after while condition");
    
    std::shared_ptr<Statement> body = parseStatement();
    
    SourceLocation loc;
    loc.filename = std::string(whileToken.lexeme);
    loc.line = static_cast<int>(whileToken.line);
    loc.column = static_cast<int>(whileToken.column);
    loc.length = static_cast<int>(whileToken.length);
    
    return std::make_shared<WhileStatement>(loc, condition, body);
}

std::shared_ptr<ForStatement> Parser::parseForStatement() {
    Token forToken = previous();
    
    consume(TokenType::LPAREN, "Expect '(' after 'for'");
    
    // Parse initializer
    std::shared_ptr<Statement> initializer;
    if (match({TokenType::SEMICOLON})) {
        initializer = nullptr;
    } else if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) || 
              check(TokenType::KW_CHAR) || check(TokenType::IDENTIFIER)) {
        initializer = std::dynamic_pointer_cast<Statement>(parseDeclaration());
    } else {
        initializer = parseExpressionStatement();
    }
    
    // Parse condition
    std::shared_ptr<Expression> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition");
    
    // Parse increment
    std::shared_ptr<Expression> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parseExpression();
    }
    consume(TokenType::RPAREN, "Expect ')' after for clauses");
    
    // Parse body
    std::shared_ptr<Statement> body = parseStatement();
    
    SourceLocation loc;
    loc.filename = std::string(forToken.lexeme);
    loc.line = static_cast<int>(forToken.line);
    loc.column = static_cast<int>(forToken.column);
    loc.length = static_cast<int>(forToken.length);
    
    return std::make_shared<ForStatement>(loc, initializer, condition, increment, body);
}

std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    Token returnToken = previous();
    std::cout << "Parsing return statement at line " << returnToken.line << std::endl;
    
    std::shared_ptr<Expression> value = nullptr;
    
    // Check if there's an expression after return
    if (!check(TokenType::SEMICOLON)) {
        try {
            value = parseExpression();
            std::cout << "Return statement has expression value" << std::endl;
        } catch (const ParseError& e) {
            std::cerr << "Error parsing return expression: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Return statement has no expression (void return)" << std::endl;
    }
    
    // Consume the semicolon
    consume(TokenType::SEMICOLON, "Expect ';' after return value");
    
    SourceLocation loc;
    loc.filename = std::string(returnToken.lexeme);
    loc.line = static_cast<int>(returnToken.line);
    loc.column = static_cast<int>(returnToken.column);
    loc.length = static_cast<int>(returnToken.length);
    
    return std::make_shared<ReturnStatement>(loc, value);
}

std::shared_ptr<BreakStatement> Parser::parseBreakStatement() {
    Token breakToken = previous();
    consume(TokenType::SEMICOLON, "Expect ';' after 'break'");
    
    SourceLocation loc;
    loc.filename = std::string(breakToken.lexeme);
    loc.line = static_cast<int>(breakToken.line);
    loc.column = static_cast<int>(breakToken.column);
    loc.length = static_cast<int>(breakToken.length);
    
    return std::make_shared<BreakStatement>(loc);
}

std::shared_ptr<ContinueStatement> Parser::parseContinueStatement() {
    Token continueToken = previous();
    consume(TokenType::SEMICOLON, "Expect ';' after 'continue'");
    
    SourceLocation loc;
    loc.filename = std::string(continueToken.lexeme);
    loc.line = static_cast<int>(continueToken.line);
    loc.column = static_cast<int>(continueToken.column);
    loc.length = static_cast<int>(continueToken.length);
    
    return std::make_shared<ContinueStatement>(loc);
}

std::shared_ptr<ThrowStatement> Parser::parseThrowStatement() {
    Token throwToken = previous();
    
    std::shared_ptr<Expression> exception = parseExpression();
    
    std::shared_ptr<Statement> handler = nullptr;
    if (match({TokenType::LBRACE})) {
        handler = parseBlock();
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after throw statement");
    
    SourceLocation loc;
    loc.filename = std::string(throwToken.lexeme);
    loc.line = static_cast<int>(throwToken.line);
    loc.column = static_cast<int>(throwToken.column);
    loc.length = static_cast<int>(throwToken.length);
    
    return std::make_shared<ThrowStatement>(loc, exception, handler);
}

std::shared_ptr<TryCatchStatement> Parser::parseTryCatchStatement() {
    Token tryToken = previous();
    
    std::shared_ptr<Statement> tryBlock = parseBlock();
    
    std::vector<TryCatchStatement::CatchClause> catchClauses;
    
    while (match({TokenType::KW_CATCH})) {
        consume(TokenType::LPAREN, "Expect '(' after 'catch'");
        
        std::shared_ptr<Type> exceptionType = parseType();
        Token exceptionVarToken = consume(TokenType::IDENTIFIER, "Expect exception variable name");
        
        consume(TokenType::RPAREN, "Expect ')' after catch parameter");
        
        std::shared_ptr<Statement> catchBody = parseBlock();
        
        catchClauses.push_back({
            std::string(exceptionVarToken.getValueString()),
            exceptionType,
            catchBody
        });
    }
    
    SourceLocation loc;
    loc.filename = std::string(tryToken.lexeme);
    loc.line = static_cast<int>(tryToken.line);
    loc.column = static_cast<int>(tryToken.column);
    loc.length = static_cast<int>(tryToken.length);
    
    return std::make_shared<TryCatchStatement>(loc, tryBlock, catchClauses);
}

std::shared_ptr<ASMStatement> Parser::parseASMStatement() {
    Token asmToken = previous();
    
    consume(TokenType::LBRACE, "Expect '{' after 'asm'");
    
    std::string asmCode;
    
    // Collect all tokens until we find the closing brace
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        asmCode += std::string(advance().lexeme) + " ";
    }
    
    consume(TokenType::RBRACE, "Expect '}' after inline assembly");
    
    SourceLocation loc;
    loc.filename = std::string(asmToken.lexeme);
    loc.line = static_cast<int>(asmToken.line);
    loc.column = static_cast<int>(asmToken.column);
    loc.length = static_cast<int>(asmToken.length);
    
    return std::make_shared<ASMStatement>(loc, asmCode);
}

std::shared_ptr<ExpressionStatement> Parser::parseExpressionStatement() {
    Token startToken = peek();
    std::cout << "Parsing expression statement starting with: " 
              << tokenTypeToString(startToken.type) 
              << ", Lexeme: " << std::string(startToken.lexeme) 
              << std::endl;
    
    // Check for object instantiation pattern: [identifier] [{}] [identifier] [;]
    if (check(TokenType::IDENTIFIER)) {
        size_t currentPos = current;
        Token objectTypeToken = advance(); // Consume object type
        
        // Check for {} after the type name
        if (match({TokenType::LBRACE}) && match({TokenType::RBRACE})) {
            // Check if next token is an identifier (the object name)
            if (check(TokenType::IDENTIFIER)) {
                Token objectNameToken = advance(); // Consume object name
                
                // Create source location
                SourceLocation loc;
                loc.filename = std::string(startToken.lexeme);
                loc.line = static_cast<int>(startToken.line);
                loc.column = static_cast<int>(startToken.column);
                loc.length = static_cast<int>(objectNameToken.length);
                
                // Create an ObjectInstantiationExpression (we'll need to add this to AST)
                auto expr = std::make_shared<ObjectInstantiationExpression>(
                    loc,
                    std::string(objectTypeToken.getValueString()),
                    std::string(objectNameToken.getValueString())
                );
                
                // Expect a semicolon
                consume(TokenType::SEMICOLON, "Expect ';' after object instantiation");
                
                std::cout << "Created object instantiation expression: " 
                          << objectTypeToken.getValueString() << " {} " 
                          << objectNameToken.getValueString() << std::endl;
                          
                return std::make_shared<ExpressionStatement>(loc, expr);
            }
        }
        
        // If it wasn't an object instantiation, rewind and parse normally
        current = currentPos;
    }
    
    // Continue with normal expression parsing
    std::shared_ptr<Expression> expr = parseExpression();
    
    // Expect a semicolon after the expression
    consume(TokenType::SEMICOLON, "Expect ';' after expression");
    
    // Create source location
    SourceLocation loc;
    loc.filename = std::string(startToken.lexeme);
    loc.line = static_cast<int>(startToken.line);
    loc.column = static_cast<int>(startToken.column);
    loc.length = static_cast<int>(startToken.length);
    
    std::cout << "Created expression statement" << std::endl;
    return std::make_shared<ExpressionStatement>(loc, expr);
}

std::shared_ptr<Expression> Parser::parseInjectableString() {
    Token stringToken = previous();  // Get the 'i"..."' part
    
    // Create source location
    SourceLocation loc;
    loc.filename = std::string(stringToken.lexeme);
    loc.line = static_cast<int>(stringToken.line);
    loc.column = static_cast<int>(stringToken.column);
    loc.length = static_cast<int>(stringToken.length);
    
    std::string format = std::get<std::string>(stringToken.value.value);
    
    std::cout << "Parsing Injectable String: '" << format << "'" << std::endl;
    
    // Expect ':' after injectable string
    consume(TokenType::COLON, "Expect ':' after injectable string");
    
    // Expect '{' 
    consume(TokenType::LBRACE, "Expect '{' after ':'");
    
    std::vector<std::shared_ptr<Expression>> args;
    
    // Parse arguments
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            // Parse each argument (could be an identifier or other expression)
            std::shared_ptr<Expression> arg = parseExpression();
            args.push_back(arg);
            
            // Expect semicolon between arguments, or '}' to end
            if (!match({TokenType::SEMICOLON})) {
                break;
            }
        } catch (const ParseError& e) {
            std::cerr << "Error parsing injectable string argument: " << e.what() << std::endl;
            synchronize();
            break;
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after injectable string arguments");
    
    std::cout << "Parsed Injectable String with " << args.size() << " arguments" << std::endl;
    
    return std::make_shared<InjectableStringExpression>(loc, format, args);
}

std::shared_ptr<ImportDeclaration> Parser::parseImportDeclaration() {
    Token importToken = previous();
    
    // Import can be followed by a string literal path
    Token pathToken = consume(TokenType::STRING_LITERAL, "Expected string literal after 'import'");
    
    // Expect semicolon after import statement
    consume(TokenType::SEMICOLON, "Expect ';' after import statement");
    
    SourceLocation loc;
    loc.filename = std::string(importToken.lexeme);
    loc.line = static_cast<int>(importToken.line);
    loc.column = static_cast<int>(importToken.column);
    loc.length = static_cast<int>(importToken.length);
    
    // Extract the path from the string literal
    std::string path;
    if (std::holds_alternative<std::string>(pathToken.value.value)) {
        path = std::get<std::string>(pathToken.value.value);
    } else {
        throw error(pathToken, "Invalid path in import statement");
    }
    
    return std::make_shared<ImportDeclaration>(loc, path);
}

std::shared_ptr<Expression> Parser::parseIndex(std::shared_ptr<Expression> array) {
    Token startToken = previous();
    
    // Expect an index inside the brackets
    std::shared_ptr<Expression> indexExpr;
    
    // Check for slice notation with missing start index
    if (match({TokenType::COLON})) {
        if (match({TokenType::INTEGER_LITERAL})) {
            Token endIndexToken = previous();
            int64_t endIndex = std::get<int64_t>(endIndexToken.value.value);
            
            // Create custom binary expression for slicing
            SourceLocation loc;
            loc.filename = std::string(startToken.lexeme);
            loc.line = static_cast<int>(startToken.line);
            loc.column = static_cast<int>(startToken.column);
            loc.length = static_cast<int>(endIndexToken.length);
            
            indexExpr = std::make_shared<BinaryExpression>(
                loc, 
                BinaryExpression::Operator::CUSTOM, 
                std::make_shared<LiteralExpression>(
                    loc, 
                    static_cast<int64_t>(-1), // Use -1 to indicate missing start index
                    std::make_shared<PrimitiveType>(
                        loc, 
                        PrimitiveTypeKind::INT, 
                        BitWidth::_32, 
                        false
                    )
                ),
                std::make_shared<LiteralExpression>(
                    loc, 
                    endIndex, 
                    std::make_shared<PrimitiveType>(
                        loc, 
                        PrimitiveTypeKind::INT, 
                        BitWidth::_32, 
                        false
                    )
                ),
                ":"
            );
        } else {
            // Partial slice like a[:] - not implemented
            throw error(peek(), "Expected end index after ':'");
        }
    } else if (match({TokenType::INTEGER_LITERAL})) {
        Token startIndexToken = previous();
        int64_t startIndex = std::get<int64_t>(startIndexToken.value.value);
        
        // Check for colon-based slice
        if (match({TokenType::COLON})) {
            // Parse end index
            if (match({TokenType::INTEGER_LITERAL})) {
                Token endIndexToken = previous();
                int64_t endIndex = std::get<int64_t>(endIndexToken.value.value);
                
                // Create custom binary expression for slicing
                SourceLocation loc;
                loc.filename = std::string(startToken.lexeme);
                loc.line = static_cast<int>(startToken.line);
                loc.column = static_cast<int>(startToken.column);
                loc.length = static_cast<int>(endIndexToken.length);
                
                indexExpr = std::make_shared<BinaryExpression>(
                    loc, 
                    BinaryExpression::Operator::CUSTOM, 
                    std::make_shared<LiteralExpression>(
                        loc, 
                        startIndex, 
                        std::make_shared<PrimitiveType>(
                            loc, 
                            PrimitiveTypeKind::INT, 
                            BitWidth::_32, 
                            false
                        )
                    ),
                    std::make_shared<LiteralExpression>(
                        loc, 
                        endIndex, 
                        std::make_shared<PrimitiveType>(
                            loc, 
                            PrimitiveTypeKind::INT, 
                            BitWidth::_32, 
                            false
                        )
                    ),
                    ":"
                );
            } else {
                // Partial slice like a[start:]
                throw error(peek(), "Expected end index after ':'");
            }
        } else {
            // Regular index
            indexExpr = std::make_shared<LiteralExpression>(
                SourceLocation{
                    std::string(startToken.lexeme), 
                    static_cast<int>(startToken.line), 
                    static_cast<int>(startToken.column), 
                    static_cast<int>(startToken.length)
                }, 
                startIndex, 
                std::make_shared<PrimitiveType>(
                    SourceLocation{}, 
                    PrimitiveTypeKind::INT, 
                    BitWidth::_32, 
                    false
                )
            );
        }
    } else {
        throw error(peek(), "Expected index or slice");
    }
    
    // Consume closing bracket
    consume(TokenType::RBRACKET, "Expect ']' after index");
    
    // Create index expression
    SourceLocation loc;
    loc.filename = std::string(startToken.lexeme);
    loc.line = static_cast<int>(startToken.line);
    loc.column = static_cast<int>(startToken.column);
    loc.length = 1; // Bracket length
    
    return std::make_shared<IndexExpression>(loc, array, indexExpr);
}

} // namespace flux
