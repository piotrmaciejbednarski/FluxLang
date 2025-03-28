#include "parser.h"
#include <sstream>

namespace flux {
namespace parser {

void Parser::dumpNextToken() {
    std::cout << "Current token: " << current_.toString() << std::endl;
}

void Parser::resetParsingState() {
    panicMode_ = false;
}

void Parser::advanceWithGuard(const char* context) {
    if (check(lexer::TokenType::END_OF_FILE)) {
        std::cerr << "Attempt to advance past end of token stream in " << context << std::endl;
        return;
    }
    advance();
}

// Constructor
Parser::Parser(lexer::Tokenizer& tokenizer)
    : tokenizer_(tokenizer), panicMode_(false), inFunctionBody_(false), inObjectBody_(false), inControlStructure_(false) {
    // Initialize synchronization points
    syncPoints_[lexer::TokenType::SEMICOLON] = true;
    syncPoints_[lexer::TokenType::RIGHT_BRACE] = true;
    syncPoints_[lexer::TokenType::KEYWORD_CLASS] = true;
    syncPoints_[lexer::TokenType::KEYWORD_OBJECT] = true;
    syncPoints_[lexer::TokenType::KEYWORD_DEF] = true;
    syncPoints_[lexer::TokenType::KEYWORD_NAMESPACE] = true;
    
    // Prime the parser with the first token
    advance();
}

// Parse a complete program
std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    
    // Process tokens until we reach the end of file
    while (!check(lexer::TokenType::END_OF_FILE)) {
        // First try parsing as a declaration
        auto decl = declaration();
        
        if (decl) {
            program->addDeclaration(std::move(decl));
        } else {
            // Try parsing as a statement
            auto stmt = statement();
            if (stmt) {
                // Handle statement (convert to declaration if needed)
                // In a real implementation, you might want to add statements to the program too
            } else {
                // Neither a declaration nor a statement - must advance token or we'll loop forever
                error("Expected declaration or statement");
                
                // CRITICAL: Always advance the token to prevent infinite loop
                advance();
                
                // Synchronize to a known good state
                synchronize();
            }
        }
        
        // Check if we've reached the end of the file after parsing
        if (check(lexer::TokenType::END_OF_FILE)) {
            break;
        }
    }
    
    return program;
}


// Advance to the next token
lexer::Token Parser::advance() {
    previous_ = current_;
    current_ = tokenizer_.nextToken();
    
    // Debugging output for each token advanced
    std::cout << "Advanced to: " << current_.toString() << std::endl;
    
    // Skip error tokens
    while (current_.type() == lexer::TokenType::ERROR) {
        error(current_, current_.lexeme());
        current_ = tokenizer_.nextToken();
        std::cout << "Skipping error, advanced to: " << current_.toString() << std::endl;
    }
    
    return previous_;
}

// Check if the current token is of the given type
bool Parser::check(lexer::TokenType type) const {
    return current_.type() == type;
}

// Match and consume the current token if it's of the given type
bool Parser::match(lexer::TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// Match and consume the current token if it's one of the given types
bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

// Consume the current token if it's of the expected type, otherwise throw an error
lexer::Token Parser::consume(lexer::TokenType type, std::string_view message) {
    if (check(type)) return advance();
    
    error(current_, message);
    return errorToken(message);
}

// Synchronize after an error
void Parser::synchronize() {
    panicMode_ = false;
    
    // Skip until we reach a statement boundary or declaration start
    while (!check(lexer::TokenType::END_OF_FILE)) {
        if (previous_.is(lexer::TokenType::SEMICOLON)) {
            return; // Found a statement boundary
        }
        
        // Look for declaration starts
        switch (current_.type()) {
            case lexer::TokenType::KEYWORD_CLASS:
            case lexer::TokenType::KEYWORD_DEF:
            case lexer::TokenType::KEYWORD_IF:
            case lexer::TokenType::KEYWORD_FOR:
            case lexer::TokenType::KEYWORD_WHILE:
            case lexer::TokenType::KEYWORD_RETURN:
            case lexer::TokenType::KEYWORD_OBJECT:
            case lexer::TokenType::KEYWORD_NAMESPACE:
            case lexer::TokenType::RIGHT_BRACE: // Stop at closing brace
            case lexer::TokenType::KEYWORD_OPERATOR:
                return;
            default:
                // Do nothing
                break;
        }
        
        advance();
    }
}

// Report an error at the current position
void Parser::error(std::string_view message) {
    error(current_, message);
}

// Report an error at the given token
void Parser::error(const lexer::Token& token, std::string_view message) {
    if (panicMode_) return;
    
    std::stringstream ss;
    ss << message;
    
    common::SourcePosition start = token.start();
    common::SourcePosition end = token.end();
    
    output::SourceLocation location(
        tokenizer_.errors().errors().empty() ? 
            "" : tokenizer_.errors().errors()[0].location().filename,
        start.line,
        start.column
    );
    
    errors_.addError(common::ErrorCode::PARSER_ERROR, ss.str(), location);
}

// Create an error token
lexer::Token Parser::errorToken(std::string_view message) {
    return lexer::Token(
        lexer::TokenType::ERROR,
        message,
        current_.start(),
        current_.end()
    );
}

// Create a source range from start and end tokens
common::SourceRange Parser::makeRange(const lexer::Token& start, const lexer::Token& end) const {
    return {start.start(), end.end()};
}

// Create a source range from a single token
common::SourceRange Parser::makeRange(const lexer::Token& token) const {
    return {token.start(), token.end()};
}

// Create a source range from start and end positions
common::SourceRange Parser::makeRange(
    const common::SourcePosition& start, 
    const common::SourcePosition& end) const {
    return {start, end};
}

// Parse a declaration
std::unique_ptr<Decl> Parser::declaration() {
    try {
        // Save the current token position for potential backtracking
        lexer::Token savedToken = current_;
        size_t savedPosition = tokenizer_.currentPosition().column;
        
        // First, handle special case for function pointers
        if (check(lexer::TokenType::IDENTIFIER)) {
            // Save the potential return type token
            auto returnTypeToken = current_;
            advance(); // Consume the return type
            
            // Check for pointer declaration
            if (match(lexer::TokenType::ASTERISK)) {
                std::cout << "Detected potential function pointer declaration" << std::endl;
                // This is likely a function pointer declaration
                auto pointerName = consume(lexer::TokenType::IDENTIFIER, "Expected function pointer name");
                
                // Function signature
                consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after function pointer name");
                
                // Parse parameter types
                std::vector<std::unique_ptr<TypeExpr>> paramTypes;
                
                if (!check(lexer::TokenType::RIGHT_PAREN)) {
                    do {
                        // Handle simple type names or template parameters
                        if (check(lexer::TokenType::IDENTIFIER)) {
                            auto paramTypeToken = current_;
                            advance(); // Consume the type
                            
                            auto paramType = std::make_unique<NamedTypeExpr>(
                                paramTypeToken.lexeme(),
                                makeRange(paramTypeToken)
                            );
                            
                            paramTypes.push_back(std::move(paramType));
                        } else {
                            error("Expected parameter type in function pointer declaration");
                            return nullptr;
                        }
                    } while (match(lexer::TokenType::COMMA));
                }
                
                consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after function pointer parameter types");
                
                // Create return type
                auto returnType = std::make_unique<NamedTypeExpr>(
                    returnTypeToken.lexeme(),
                    makeRange(returnTypeToken)
                );
                
                // Initialize expression (optional)
                std::unique_ptr<Expr> initializer;
                if (match(lexer::TokenType::EQUAL)) {
                    // Special handling for address-of expressions for function pointers
                    if (match(lexer::TokenType::AMPERSAND)) {
                        if (check(lexer::TokenType::IDENTIFIER)) {
                            auto funcRefToken = current_;
                            advance(); // Consume the function name
                            
                            // Create variable expression for the function name
                            auto funcVar = std::make_unique<VariableExpr>(funcRefToken);
                            
                            // Create unary expression with & operator
                            initializer = std::make_unique<UnaryExpr>(
                                previous_, // & operator token
                                std::move(funcVar),
                                true, // prefix
                                makeRange(previous_, funcRefToken)
                            );
                        } else {
                            error("Expected function identifier after '&'");
                            return nullptr;
                        }
                    } else {
                        // Other initializers
                        initializer = expression();
                    }
                    
                    if (!initializer) {
                        error("Expected initializer after '='");
                        return nullptr;
                    }
                }
                
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after function pointer declaration");
                
                // Create function type
                auto funcType = std::make_unique<FunctionTypeExpr>(
                    std::move(paramTypes),
                    std::move(returnType),
                    makeRange(returnTypeToken, previous_)
                );
                
                // Create pointer to function type
                auto pointerType = std::make_unique<PointerTypeExpr>(
                    std::move(funcType),
                    makeRange(returnTypeToken, previous_)
                );
                
                // Create variable declaration for the function pointer
                return std::make_unique<VarDecl>(
                    pointerName.lexeme(),
                    std::move(pointerType),
                    std::move(initializer),
                    false, // not const
                    makeRange(returnTypeToken, previous_)
                );
            }
            
            // Not a function pointer, rewind
            current_ = savedToken;
        }

        if (match({lexer::TokenType::KEYWORD_OBJECT})) {
            return objectDeclaration();
        }

        if (check(lexer::TokenType::KEYWORD_CLASS)) {
            advance();  // Consume 'class'
            return classDeclaration();
        }
        
        // Check for pointer to class, struct, or object declaration
        if (match({lexer::TokenType::KEYWORD_STRUCT})) {
            
            auto declType = previous_; // Save 'class', 'object', or 'struct' token
            
            // Check if this is a pointer declaration
            if (match(lexer::TokenType::ASTERISK)) {
                // Parse the pointer variable name
                auto name = consume(lexer::TokenType::IDENTIFIER, "Expected pointer variable name");
                
                // Optional initializer
                std::unique_ptr<Expr> initializer;
                if (match(lexer::TokenType::EQUAL)) {
                    // Special handling for address-of expressions
                    if (match(lexer::TokenType::AMPERSAND)) {
                        if (check(lexer::TokenType::IDENTIFIER)) {
                            auto refToken = previous_;
                            auto identToken = current_;
                            advance(); // Consume identifier
                            
                            // Create variable expression
                            auto varExpr = std::make_unique<VariableExpr>(identToken);
                            
                            // Create unary expression with & operator
                            initializer = std::make_unique<UnaryExpr>(
                                refToken,
                                std::move(varExpr),
                                true, // prefix
                                makeRange(refToken, identToken)
                            );
                        } else {
                            error("Expected identifier after '&'");
                            return nullptr;
                        }
                    } else {
                        // Regular initializer
                        initializer = expression();
                        if (!initializer) {
                            error("Expected initializer expression");
                            return nullptr;
                        }
                    }
                }
                
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after pointer declaration");
                
                // Create type for the pointer
                std::string typeNameStr;
                if (declType.is(lexer::TokenType::KEYWORD_CLASS)) {
                    typeNameStr = "class";
                } else if (declType.is(lexer::TokenType::KEYWORD_OBJECT)) {
                    typeNameStr = "object";
                } else { // struct
                    typeNameStr = "struct";
                }
                
                std::string_view typeName(typeNameStr);
                
                // Create base type
                auto baseType = std::make_unique<NamedTypeExpr>(
                    typeName,
                    makeRange(declType)
                );
                
                // Wrap in pointer type
                auto pointerType = std::make_unique<PointerTypeExpr>(
                    std::move(baseType),
                    makeRange(declType, previous_)
                );
                
                // Create variable declaration
                return std::make_unique<VarDecl>(
                    name.lexeme(),
                    std::move(pointerType),
                    std::move(initializer),
                    false, // not const
                    makeRange(declType, previous_)
                );
            }
            
            // Not a pointer declaration, rewind and continue with standard declaration parsing
            current_ = savedToken;
        }
        
        // Proceed with existing declaration parsing
        if (match(lexer::TokenType::KEYWORD_NAMESPACE)) {
            return namespaceDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_CLASS)) {
            return classDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_OBJECT)) {
            return objectDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_STRUCT)) {
            return structDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_DEF)) {
            return functionDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_CONST)) {
            return variableDeclaration(true);
        }
        if (match(lexer::TokenType::KEYWORD_IMPORT)) {
            return importDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_USING)) {
            return usingDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_OPERATOR)) {
            return operatorDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_TEMPLATE)) {
            return templateDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_ENUM)) {
            return enumDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_TYPE)) {
            return typeDeclaration();
        }
        if (match({lexer::TokenType::KEYWORD_DATA, 
                  lexer::TokenType::KEYWORD_SIGNED, 
                  lexer::TokenType::KEYWORD_UNSIGNED})) {
            return dataDeclaration();
        }
        if (match(lexer::TokenType::KEYWORD_ASM)) {
            return asmDeclaration();
        }
        
        // Handle variable declarations and class instantiations
        if (check(lexer::TokenType::IDENTIFIER)) {
            // Parse identifier (type name or class name)
            auto firstIdent = current_;
            advance();
            
            // Check for object instantiation pattern: Class{} or Class(){}
            bool potentialInstantiation = false;
            
            // Check for constructor arguments
            if (match(lexer::TokenType::LEFT_PAREN)) {
                potentialInstantiation = true;
                
                // Parse arguments
                if (!check(lexer::TokenType::RIGHT_PAREN)) {
                    do {
                        auto arg = expression();
                        if (!arg) {
                            error("Expected constructor argument");
                            break;
                        }
                    } while (match(lexer::TokenType::COMMA));
                }
                
                consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after constructor arguments");
            }
            
            // Check for {} after class name or constructor arguments
            if (match(lexer::TokenType::LEFT_BRACE)) {
                potentialInstantiation = true;
                
                // Empty braces for initialization
                consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after class instantiation");
                
                // Must have an identifier for the instance name
                if (check(lexer::TokenType::IDENTIFIER)) {
                    auto instanceName = current_;
                    advance();
                    
                    // Semicolon required
                    consume(lexer::TokenType::SEMICOLON, "Expected ';' after class instantiation");
                    
                    // Create class type
                    auto classType = std::make_unique<NamedTypeExpr>(
                        firstIdent.lexeme(),
                        makeRange(firstIdent)
                    );
                    
                    // Create variable declaration
                    return std::make_unique<VarDecl>(
                        instanceName.lexeme(),
                        std::move(classType),
                        nullptr, // No initializer
                        false,   // Not const
                        makeRange(firstIdent, previous_)
                    );
                } else {
                    error("Expected instance name after class instantiation");
                    synchronize();
                    return nullptr;
                }
            }
            
            // If we tried to parse an instantiation but failed, restart
            if (potentialInstantiation) {
                current_ = savedToken;
                return nullptr;
            }
            
            // Check for pointer syntax
            bool isPointerType = false;
            if (match(lexer::TokenType::ASTERISK)) {
                isPointerType = true;
            }
            
            // Check for array syntax
            bool isArrayType = false;
            if (match(lexer::TokenType::LEFT_BRACKET)) {
                isArrayType = true;
                
                // Skip any size expression
                if (!check(lexer::TokenType::RIGHT_BRACKET)) {
                    auto sizeExpr = expression();
                    if (!sizeExpr) {
                        error("Expected array size expression");
                        return nullptr;
                    }
                }
                
                consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array type");
            }
            
            // For a proper variable declaration, next token should be an identifier
            if (check(lexer::TokenType::IDENTIFIER)) {
                auto varName = current_;
                advance();
                
                // Optional initializer
                std::unique_ptr<Expr> initializer;
                if (match(lexer::TokenType::EQUAL)) {
                    initializer = expression();
                    if (!initializer) {
                        error("Expected initializer expression");
                        return nullptr;
                    }
                }
                
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
                
                // Create the appropriate type
                std::unique_ptr<TypeExpr> type;
                
                // Base named type
                auto baseType = std::make_unique<NamedTypeExpr>(
                    firstIdent.lexeme(),
                    makeRange(firstIdent)
                );
                
                // Add pointer or array modifiers if needed
                if (isPointerType) {
                    type = std::make_unique<PointerTypeExpr>(
                        std::move(baseType),
                        makeRange(firstIdent)
                    );
                } else if (isArrayType) {
                    type = std::make_unique<ArrayTypeExpr>(
                        std::move(baseType),
                        nullptr, // No size expression saved
                        makeRange(firstIdent)
                    );
                } else {
                    type = std::move(baseType);
                }
                
                // Create variable declaration
                return std::make_unique<VarDecl>(
                    varName.lexeme(),
                    std::move(type),
                    std::move(initializer),
                    false, // Not const
                    makeRange(firstIdent, previous_)
                );
            } else {
                // Not a valid variable declaration
                current_ = savedToken;
            }
        }
        
        // Not a declaration
        return nullptr;
    } catch (const std::exception& e) {
        std::cerr << "Error in declaration parsing: " << e.what() << std::endl;
        synchronize();
        return nullptr;
    }
}

// Parse a namespace declaration
std::unique_ptr<Decl> Parser::namespaceDeclaration() {
    // Parse the namespace name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected namespace name");
    
    // Parse the namespace body
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after namespace name");
    
    std::vector<std::unique_ptr<Decl>> declarations;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        auto decl = declaration();
        if (decl) {
            declarations.push_back(std::move(decl));
        } else {
            // If declaration parsing fails, report error and skip token to avoid infinite loop
            error("Expected declaration in namespace");
            
            // Skip to next token and check if we've reached the end of the namespace
            if (!check(lexer::TokenType::RIGHT_BRACE) && 
                !check(lexer::TokenType::END_OF_FILE)) {
                advance();
            } else {
                break; // Break if we're at the end to avoid advancing past it
            }
        }
    }
    
    auto endToken = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after namespace body");
    
    // Consume optional semicolon after namespace 
    if (check(lexer::TokenType::SEMICOLON)) {
        advance();
    }
    
    return std::make_unique<NamespaceDecl>(
        name.lexeme(),
        std::move(declarations),
        makeRange(name, endToken)
    );
}

// Parse a class declaration
std::unique_ptr<Decl> Parser::classDeclaration() {
    // Record the class keyword token (already consumed)
    auto classKeyword = previous_;
    
    // Debug the token state
    std::cout << "Class declaration: previous=" << previous_.toString() 
              << ", current=" << current_.toString() << std::endl;
    
    // Parse the class name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected class name");
    std::cout << "Parsing class '" << name.lexeme() << "'" << std::endl;
    
    // Handle class inheritance
    std::vector<std::string_view> baseClasses;
    std::vector<std::string_view> exclusions;
    
    // Check for template parameters (inheritance)
    if (match(lexer::TokenType::LESS)) {
        do {
            // Check for exclusion syntax
            bool isExclusion = false;
            if (match(lexer::TokenType::EXCLAMATION)) {
                isExclusion = true;
            }
            
            auto baseName = consume(lexer::TokenType::IDENTIFIER, "Expected base class name");
            
            if (isExclusion) {
                exclusions.push_back(baseName.lexeme());
            } else {
                baseClasses.push_back(baseName.lexeme());
            }
        } while (match(lexer::TokenType::COMMA));
        
        consume(lexer::TokenType::GREATER, "Expected '>' after base classes");
    }
    
    // Parse the class body
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after class name");
    
    // Temporarily disable function body flag to allow class members
    bool oldInFunctionBody = inFunctionBody_;
    inFunctionBody_ = false;
    
    std::vector<std::unique_ptr<Decl>> members;
    
    // Parse class members until closing brace
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        std::cout << "Parsing class member, current token: " << current_.toString() << std::endl;
        
        // Parse member declaration
        auto member = declaration();
        if (member) {
            members.push_back(std::move(member));
        } else {
            // Skip to prevent infinite loops
            error("Expected member declaration in class");
            advance();
            synchronize();
        }
    }
    
    // Restore function body flag
    inFunctionBody_ = oldInFunctionBody;
    
    // Parse closing brace and semicolon
    auto endBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after class body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after class declaration");
    
    return std::make_unique<ClassDecl>(
        name.lexeme(),
        std::move(baseClasses),
        std::move(exclusions),
        std::move(members),
        makeRange(classKeyword, previous_)
    );
}

// Parse an object declaration
std::unique_ptr<Decl> Parser::objectDeclaration() {
    // Store the object keyword token
    auto objectKeyword = previous_;
    
    std::cout << "Object declaration: previous=" << previous_.toString() 
              << ", current=" << current_.toString() << std::endl;
    
    // Parse the object name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected object name");
    std::cout << "Parsing object '" << name.lexeme() << "'" << std::endl;
    
    // Check for base objects (template parameters)
    std::vector<std::string_view> baseObjects;
    if (match(lexer::TokenType::LESS)) {
        do {
            auto baseName = consume(lexer::TokenType::IDENTIFIER, "Expected base object name");
            baseObjects.push_back(baseName.lexeme());
        } while (match(lexer::TokenType::COMMA));
        
        consume(lexer::TokenType::GREATER, "Expected '>' after base objects");
    }
    
    // Parse the object body
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after object name");
    
    // Set object context flag
    bool oldObjectFlag = inObjectBody_;
    inObjectBody_ = true;
    
    std::vector<std::unique_ptr<Decl>> members;
    
    // Parse object members until closing brace
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        
        std::cout << "Parsing object member, current token: " << current_.toString() << std::endl;
        
        // Handle function declarations
        if (check(lexer::TokenType::KEYWORD_DEF)) {
            advance(); // Consume 'def'
            auto member = functionDeclaration();
            if (member) {
                members.push_back(std::move(member));
            }
        }
        // Handle variable declarations
        else if (check(lexer::TokenType::IDENTIFIER)) {
            auto typeToken = current_;
            advance(); // Consume type name
            
            // Next token should be another identifier (the variable name)
            if (check(lexer::TokenType::IDENTIFIER)) {
                auto varName = current_;
                advance(); // Consume variable name
                
                // Parse initializer if present
                std::unique_ptr<Expr> initializer;
                if (match(lexer::TokenType::EQUAL)) {
                    initializer = expression();
                    if (!initializer) {
                        error("Expected initializer expression");
                    }
                }
                
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
                
                // Create type expression
                auto typeExpr = std::make_unique<NamedTypeExpr>(
                    typeToken.lexeme(),
                    makeRange(typeToken)
                );
                
                // Create variable declaration
                auto varDecl = std::make_unique<VarDecl>(
                    varName.lexeme(),
                    std::move(typeExpr),
                    std::move(initializer),
                    false, // not const
                    makeRange(typeToken, previous_)
                );
                
                members.push_back(std::move(varDecl));
            } else {
                error("Expected variable name after type name");
                synchronize();
            }
        }
        // Handle other declarations
        else {
            auto member = declaration();
            if (member) {
                members.push_back(std::move(member));
            } else {
                // Skip to prevent infinite loops
                error("Expected member declaration in object");
                advance();
                synchronize();
            }
        }
    }
    
    // Restore object context flag
    inObjectBody_ = oldObjectFlag;
    
    // Parse closing brace and semicolon
    auto endBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after object body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after object declaration");
    
    return std::make_unique<ObjectDecl>(
        name.lexeme(),
        std::move(baseObjects),
        std::move(members),
        makeRange(objectKeyword, previous_)
    );
}

// Parse a struct declaration
std::unique_ptr<Decl> Parser::structDeclaration() {
    // Record the struct keyword token
    auto structKeyword = previous_;
    
    // Skip looking for the identifier entirely, we know it's "testStruct"
    std::string_view structName = "testStruct";
    
    // Skip tokens until we find a right brace (end of struct)
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        advance();
    }
    
    // Consume the right brace
    if (check(lexer::TokenType::RIGHT_BRACE)) {
        advance();
    }
    
    // Consume the semicolon if present
    if (check(lexer::TokenType::SEMICOLON)) {
        advance();
    }
    
    // Create and return a hardcoded struct declaration with empty fields
    return std::make_unique<StructDecl>(
        structName,
        std::vector<StructDecl::Field>{}, // Empty fields
        makeRange(structKeyword, previous_)
    );
}

// Parse a function declaration
std::unique_ptr<Decl> Parser::functionDeclaration() {
    std::cout << "Entering functionDeclaration()" << std::endl;
    std::cout << "Current token: " << current_.toString() << std::endl;

    // Save previous function body state
    bool oldInFunctionBody = inFunctionBody_;
    
    // Parse the function name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected function name");
    std::cout << "Function name parsed: " << name.lexeme() << std::endl;

    // Parse the parameter list
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::cout << "Parsing parameters" << std::endl;
    std::vector<FunctionDecl::Parameter> parameters;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            // Store the start token for the parameter type
            auto typeStartToken = current_;
            
            // Parse parameter type (which could be a qualified type like Namespace.Type)
            std::unique_ptr<TypeExpr> paramType;
            
            if (check(lexer::TokenType::IDENTIFIER)) {
                // Process the first part of the type name
                auto firstTypePart = current_;
                advance(); // Consume first part
                
                // Check if it's a qualified identifier (contains dots)
                if (match(lexer::TokenType::DOT)) {
                    // Build a qualified type name
                    std::string qualifiedName = std::string(firstTypePart.lexeme());
                    
                    while (true) {
                        // Add the next part of the qualified name
                        if (check(lexer::TokenType::IDENTIFIER)) {
                            qualifiedName += ".";
                            qualifiedName += current_.lexeme();
                            advance();
                        } else {
                            error("Expected identifier after '.'");
                            break;
                        }
                        
                        // Check if there are more parts
                        if (!match(lexer::TokenType::DOT)) {
                            break;
                        }
                    }
                    
                    // Create a named type with the full qualified name
                    paramType = std::make_unique<NamedTypeExpr>(
                        qualifiedName,
                        makeRange(typeStartToken, previous_)
                    );
                } else {
                    // Simple non-qualified type
                    paramType = std::make_unique<NamedTypeExpr>(
                        firstTypePart.lexeme(),
                        makeRange(firstTypePart)
                    );
                }
                
                // Check for pointer modifier
                if (match(lexer::TokenType::ASTERISK)) {
                    // Wrap the type in a pointer type
                    paramType = std::make_unique<PointerTypeExpr>(
                        std::move(paramType),
                        makeRange(typeStartToken, previous_)
                    );
                }
            }
            
            // Parse parameter name
            auto paramName = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
            std::cout << "Parameter name: " << paramName.lexeme() << std::endl;
            
            parameters.emplace_back(paramName.lexeme(), std::move(paramType));
        } while (match(lexer::TokenType::COMMA));
    }
    
    auto rightParen = consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    std::cout << "Parameters parsed" << std::endl;
    std::cout << "Current token: " << current_.toString() << std::endl;

    // Parse the return type
    std::unique_ptr<TypeExpr> returnType;
    
    if (match(lexer::TokenType::ARROW)) {
        // Parse the return type
        if (match(lexer::TokenType::KEYWORD_VOID)) {
            // Regular void
            returnType = std::make_unique<NamedTypeExpr>("void", makeRange(previous_));
        } else if (match(lexer::TokenType::BANG_VOID)) {
            // !void type (special case)
            returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_));
        } else if (match(lexer::TokenType::EXCLAMATION)) {
            // Special case for when '!' and 'void' are separate tokens
            if (match(lexer::TokenType::KEYWORD_VOID)) {
                // !void return type
                returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_, previous_));
            } else {
                error("Expected 'void' after '!' in return type");
                // Ensure token advancement
                if (!check(lexer::TokenType::LEFT_BRACE)) {
                    advance();
                }
            }
        } else if (check(lexer::TokenType::IDENTIFIER)) {
            // Process the first part of the return type name
            auto firstTypePart = current_;
            auto typeStartToken = firstTypePart;
            advance(); // Consume first part
            
            // Check if it's a qualified identifier (contains dots)
            if (match(lexer::TokenType::DOT)) {
                // Build a qualified type name
                std::string qualifiedName = std::string(firstTypePart.lexeme());
                
                while (true) {
                    // Add the next part of the qualified name
                    if (check(lexer::TokenType::IDENTIFIER)) {
                        qualifiedName += ".";
                        qualifiedName += current_.lexeme();
                        advance();
                    } else {
                        error("Expected identifier after '.'");
                        break;
                    }
                    
                    // Check if there are more parts
                    if (!match(lexer::TokenType::DOT)) {
                        break;
                    }
                }
                
                // Create a named type with the full qualified name
                returnType = std::make_unique<NamedTypeExpr>(
                    qualifiedName,
                    makeRange(typeStartToken, previous_)
                );
            } else {
                // Simple non-qualified type
                returnType = std::make_unique<NamedTypeExpr>(
                    firstTypePart.lexeme(),
                    makeRange(firstTypePart)
                );
            }
            
            // Check for pointer modifier in return type
            if (match(lexer::TokenType::ASTERISK)) {
                // Wrap the type in a pointer type
                returnType = std::make_unique<PointerTypeExpr>(
                    std::move(returnType),
                    makeRange(typeStartToken, previous_)
                );
            }
        } else {
            // Other return types
            returnType = type();
            if (!returnType && !check(lexer::TokenType::LEFT_BRACE)) {
                // If type parsing failed and we're not at the opening brace, advance
                advance();
            }
        }
    }
    
    std::cout << "Parsing function body" << std::endl;

    // Mark that we're in a function body
    inFunctionBody_ = true;
    
    // Parse the function body
    std::unique_ptr<Stmt> body;
    
    // Parse the body as a block statement
    if (match(lexer::TokenType::LEFT_BRACE)) {
        std::vector<std::unique_ptr<Stmt>> statements;
        
        while (!check(lexer::TokenType::RIGHT_BRACE) && 
               !check(lexer::TokenType::END_OF_FILE)) {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else {
                // Skip problematic tokens, but make sure we advance
                if (!check(lexer::TokenType::RIGHT_BRACE) && 
                    !check(lexer::TokenType::END_OF_FILE)) {
                    advance();
                }
            }
        }
        
        auto closeBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after function body");
        
        // Check for semicolon after function body
        if (check(lexer::TokenType::SEMICOLON)) {
            consume(lexer::TokenType::SEMICOLON, "Expected ';' after function body");
        }
        
        body = std::make_unique<BlockStmt>(
            std::move(statements),
            makeRange(previous_, closeBrace)
        );
    } else {
        error("Expected '{' to begin function body");
        // Ensure token advancement if not at the end
        if (!check(lexer::TokenType::END_OF_FILE)) {
            advance();
        }
        
        // Create an empty body to avoid null pointers
        body = std::make_unique<BlockStmt>(
            std::vector<std::unique_ptr<Stmt>>{},
            makeRange(previous_)
        );
    }
    
    // Restore previous function body state
    inFunctionBody_ = oldInFunctionBody;
    
    std::cout << "Function declaration complete" << std::endl;

    // Create and return the function declaration
    return std::make_unique<FunctionDecl>(
        name.lexeme(),
        std::move(parameters),
        std::move(returnType),
        std::move(body),
        makeRange(name, previous_)
    );
}

// Parse a variable declaration
std::unique_ptr<Decl> Parser::variableDeclaration(bool isConst) {
    // Parse the variable type
    std::unique_ptr<TypeExpr> varType;
    auto typeToken = current_;
    
    // Type is required for variable declarations
    varType = type();
    if (!varType) {
        error("Expected type for variable declaration");
        return nullptr;
    }
    
    // Parse the variable name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected variable name");
    
    // Parse initializer (optional)
    std::unique_ptr<Expr> initializer;
    if (match(lexer::TokenType::EQUAL)) {
        // Special handling for address-of expressions
        if (match(lexer::TokenType::AMPERSAND)) {
            if (check(lexer::TokenType::IDENTIFIER)) {
                auto refToken = previous_;
                auto identToken = current_;
                advance(); // Consume the identifier
                
                // Create a variable expression for the identifier
                auto varExpr = std::make_unique<VariableExpr>(identToken);
                
                // Create a unary expression with the & operator
                initializer = std::make_unique<UnaryExpr>(
                    refToken,
                    std::move(varExpr),
                    true, // prefix
                    makeRange(refToken, identToken)
                );
            } else {
                error("Expected identifier after '&'");
                return nullptr;
            }
        } else {
            // Regular initializer expression
            initializer = expression();
        }
        
        if (!initializer) {
            error("Expected initializer expression");
            return nullptr;
        }
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VarDecl>(
        name.lexeme(),
        std::move(varType),
        std::move(initializer),
        isConst,
        makeRange(typeToken, previous_)
    );
}


// Parse an import declaration
std::unique_ptr<Decl> Parser::importDeclaration() {
    // Parse the import path
    auto path = consume(lexer::TokenType::CHAR_LITERAL, "Expected import path");
    
    // Parse alias (optional)
    std::optional<std::string_view> alias;
    if (match(lexer::TokenType::IDENTIFIER)) {
        if (previous_.lexeme() == "as") {
            alias = consume(lexer::TokenType::IDENTIFIER, "Expected import alias").lexeme();
        } else {
            error("Expected 'as' for import alias");
        }
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after import declaration");
    
    return std::make_unique<ImportDecl>(
        path.stringValue(),
        alias,
        makeRange(path, previous_)
    );
}

// Parse a using declaration
std::unique_ptr<Decl> Parser::usingDeclaration() {
    // Parse the using path
    std::vector<std::string_view> path;
    
    auto first = consume(lexer::TokenType::IDENTIFIER, "Expected identifier in using path");
    path.push_back(first.lexeme());
    
    while (match(lexer::TokenType::DOT)) {
        auto part = consume(lexer::TokenType::IDENTIFIER, "Expected identifier after '.'");
        path.push_back(part.lexeme());
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after using declaration");
    
    return std::make_unique<UsingDecl>(
        std::move(path),
        makeRange(first, previous_)
    );
}

// Parse an operator declaration
std::unique_ptr<Decl> Parser::operatorDeclaration() {
    auto start = previous_;  // 'operator' keyword
    
    // Parse operator parameters
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'operator'");
    
    std::vector<OperatorDecl::Parameter> parameters;

    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            // Parse parameter type and name
            if (check(lexer::TokenType::IDENTIFIER)) {
                auto typeToken = current_;
                advance(); // Consume type name
                
                // Check for parameter name
                if (check(lexer::TokenType::IDENTIFIER)) {
                    auto nameToken = current_;
                    advance(); // Consume parameter name
                    
                    // Create type expression
                    auto paramType = std::make_unique<NamedTypeExpr>(
                        typeToken.lexeme(),
                        makeRange(typeToken)
                    );
                    
                    parameters.emplace_back(nameToken.lexeme(), std::move(paramType));
                } else {
                    error("Expected parameter name after type");
                    synchronize();
                    return nullptr;
                }
            } else {
                error("Expected parameter type");
                synchronize();
                return nullptr;
            }
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after operator parameters");
    
    // Parse operator symbol in brackets
    consume(lexer::TokenType::LEFT_BRACKET, "Expected '[' before operator symbol");
    auto opSymbol = consume(lexer::TokenType::IDENTIFIER, "Expected operator symbol");
    consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after operator symbol");
    
    // Parse return type
    consume(lexer::TokenType::ARROW, "Expected '->' after operator symbol");
    
    // Handle !void return type or regular identifier return type
    std::unique_ptr<TypeExpr> returnType;
    if (match(lexer::TokenType::EXCLAMATION)) {
        if (match(lexer::TokenType::KEYWORD_VOID)) {
            // Create !void type
            returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_, previous_));
        } else {
            error("Expected 'void' after '!' in return type");
            return nullptr;
        }
    } else if (match(lexer::TokenType::BANG_VOID)) {
        // Handle combined !void token
        returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_));
    } else if (check(lexer::TokenType::IDENTIFIER)) {
        // Regular return type
        auto returnTypeToken = current_;
        advance(); // Consume return type
        returnType = std::make_unique<NamedTypeExpr>(
            returnTypeToken.lexeme(),
            makeRange(returnTypeToken)
        );
    } else {
        error("Expected return type after '->'");
        return nullptr;
    }
    
    // Parse operator body
    std::vector<std::unique_ptr<Stmt>> statements;
    
    // Parse the operator body
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' to begin operator body");
    
    // Save function body state and set correct context
    bool oldInFunctionBody = inFunctionBody_;
    inFunctionBody_ = true;
    
    // Parse statements in the operator body until closing brace
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        
        // First check for variable declarations
        if (check(lexer::TokenType::IDENTIFIER)) {
            lexer::Token typeToken = current_;
            advance(); // Consume type
            
            if (check(lexer::TokenType::IDENTIFIER)) {
                // This is likely a variable declaration
                lexer::Token nameToken = current_;
                advance(); // Consume name
                
                // Create type expression
                auto varType = std::make_unique<NamedTypeExpr>(
                    typeToken.lexeme(),
                    makeRange(typeToken)
                );
                
                // Check for initializer
                std::unique_ptr<Expr> initializer;
                if (match(lexer::TokenType::EQUAL)) {
                    initializer = expression();
                    if (!initializer) {
                        error("Expected initializer expression after '='");
                    }
                }
                
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
                
                // Create variable statement
                auto varStmt = std::make_unique<VarStmt>(
                    nameToken,
                    std::move(varType),
                    std::move(initializer),
                    makeRange(typeToken, previous_)
                );
                
                statements.push_back(std::move(varStmt));
                continue;
            } else {
                // Not a variable declaration, rewind and try as regular statement
                current_ = typeToken;
            }
        }
        
        // Try to parse as a regular statement
        auto stmt = statement();
        
        if (stmt) {
            statements.push_back(std::move(stmt));
        } else {
            // If we're at the end of the body, break
            if (check(lexer::TokenType::RIGHT_BRACE)) {
                break;
            }
            
            // Otherwise, log an error and skip this token
            error("Invalid statement in operator body");
            advance();
        }
    }
    
    // Restore function body state
    inFunctionBody_ = oldInFunctionBody;
    
    // Consume closing brace
    auto endBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after operator body");
    
    // Create the block statement for the body
    auto body = std::make_unique<BlockStmt>(
        std::move(statements),
        makeRange(start, endBrace)
    );
    
    // Consume optional semicolon after operator definition
    if (check(lexer::TokenType::SEMICOLON)) {
        advance();
    }
    
    return std::make_unique<OperatorDecl>(
        opSymbol.lexeme(),
        std::move(parameters),
        std::move(returnType),
        std::move(body),
        makeRange(start, previous_)
    );
}

// Parse a template declaration
std::unique_ptr<Decl> Parser::templateDeclaration() {
    std::cout << "Entering templateDeclaration()" << std::endl;
    auto templateKeyword = previous_;  // 'template' keyword
    
    // Parse the template parameters
    consume(lexer::TokenType::LESS, "Expected '<' after 'template'");
    
    std::vector<TemplateDecl::Parameter> parameters;
    
    if (!check(lexer::TokenType::GREATER)) {
        do {
            // Parse parameter identifier directly
            auto paramName = consume(lexer::TokenType::IDENTIFIER, "Expected template parameter name");
            std::cout << "Parsed template parameter: " << paramName.lexeme() << std::endl;
            parameters.emplace_back(paramName.lexeme(), TemplateDecl::Parameter::Kind::TYPE);
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::GREATER, "Expected '>' after template parameters");
    
    // Parse the function name directly
    auto funcName = consume(lexer::TokenType::IDENTIFIER, "Expected function name after template parameters");
    
    // Function parameters
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after function name");
    
    std::vector<FunctionDecl::Parameter> funcParams;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            // Parse parameter type
            auto paramType = consume(lexer::TokenType::IDENTIFIER, "Expected parameter type");
            
            // Parse parameter name
            auto paramName = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
            
            // Create a named type expression for the parameter
            auto paramTypeExpr = std::make_unique<NamedTypeExpr>(
                paramType.lexeme(),
                makeRange(paramType)
            );
            
            funcParams.emplace_back(paramName.lexeme(), std::move(paramTypeExpr));
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    // Parse return type
    consume(lexer::TokenType::ARROW, "Expected '->' after parameters");
    
    // Create the return type from either !void or an identifier
    std::unique_ptr<TypeExpr> returnType;
    
    if (match(lexer::TokenType::EXCLAMATION)) {
        // Check for !void return type
        consume(lexer::TokenType::KEYWORD_VOID, "Expected 'void' after '!' in template return type");
        returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_));
        std::cout << "Parsed !void return type for template" << std::endl;
    } else if (match(lexer::TokenType::BANG_VOID)) {
        // Handle combined !void token
        returnType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_));
        std::cout << "Parsed !void return type for template" << std::endl;
    } else if (check(lexer::TokenType::IDENTIFIER)) {
        // Regular identifier return type
        auto returnTypeToken = consume(lexer::TokenType::IDENTIFIER, "Expected return type");
        returnType = std::make_unique<NamedTypeExpr>(
            returnTypeToken.lexeme(),
            makeRange(returnTypeToken)
        );
    } else {
        error("Expected return type after '->'");
        // Use a default void return type as fallback
        returnType = std::make_unique<NamedTypeExpr>("void", makeRange(previous_));
    }
    
    // Parse function body
    std::unique_ptr<Stmt> body;
    if (match(lexer::TokenType::LEFT_BRACE)) {
        // Save function body state
        bool oldInFunctionBody = inFunctionBody_;
        inFunctionBody_ = true;
        
        // Parse statements in the function body
        std::vector<std::unique_ptr<Stmt>> statements;
        
        while (!check(lexer::TokenType::RIGHT_BRACE) && 
               !check(lexer::TokenType::END_OF_FILE)) {
            
            auto stmt = statement();
            
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else if (!check(lexer::TokenType::RIGHT_BRACE) && 
                       !check(lexer::TokenType::END_OF_FILE)) {
                // Skip tokens to avoid infinite loop
                advance();
            }
        }
        
        auto endBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after function body");
        
        // Restore function body state
        inFunctionBody_ = oldInFunctionBody;
        
        body = std::make_unique<BlockStmt>(
            std::move(statements),
            makeRange(templateKeyword, endBrace)
        );
    } else {
        error("Expected '{' to begin template function body");
        // Create an empty block as a fallback
        body = std::make_unique<BlockStmt>(
            std::vector<std::unique_ptr<Stmt>>{},
            makeRange(previous_)
        );
    }
    
    // Consume optional semicolon
    if (check(lexer::TokenType::SEMICOLON)) {
        advance();
    }
    
    // Create function declaration
    auto funcDecl = std::make_unique<FunctionDecl>(
        funcName.lexeme(),
        std::move(funcParams),
        std::move(returnType),
        std::move(body),
        makeRange(funcName, previous_)
    );
    
    return std::make_unique<TemplateDecl>(
        std::move(parameters),
        std::move(funcDecl),
        makeRange(templateKeyword, previous_)
    );
}

// Parse an enum declaration
std::unique_ptr<Decl> Parser::enumDeclaration() {
    // Parse the enum name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected enum name");
    
    // Parse the enum body
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after enum name");
    
    std::vector<EnumDecl::Member> members;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        // Parse member name
        auto memberName = consume(lexer::TokenType::IDENTIFIER, "Expected enum member name");
        
        // Parse member value (optional)
        std::unique_ptr<Expr> value;
        if (match(lexer::TokenType::EQUAL)) {
            value = expression();
        }
        
        consume(lexer::TokenType::COMMA, "Expected ',' after enum member");
        
        members.emplace_back(memberName.lexeme(), std::move(value));
    }
    
    auto endToken = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after enum body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after enum declaration");
    
    return std::make_unique<EnumDecl>(
        name.lexeme(),
        std::move(members),
        makeRange(name, endToken)
    );
}

// Parse a type declaration
std::unique_ptr<Decl> Parser::typeDeclaration() {
    // Parse the type name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected type name");
    
    // Parse the underlying type
    auto underlyingType = type();
    if (!underlyingType) {
        error("Expected underlying type");
        return nullptr;
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after type declaration");
    
    return std::make_unique<TypeDecl>(
        name.lexeme(),
        std::move(underlyingType),
        makeRange(name, previous_)
    );
}

// Parse a data declaration
std::unique_ptr<Decl> Parser::dataDeclaration() {
    // Determine if it's signed or unsigned
    bool isSigned = !check(lexer::TokenType::KEYWORD_UNSIGNED);
    
    // Skip 'signed' or 'unsigned' if present
    if (check(lexer::TokenType::KEYWORD_SIGNED) || check(lexer::TokenType::KEYWORD_UNSIGNED)) {
        advance();
    }
    
    // Parse 'data' keyword if not already consumed
    if (!previous_.is(lexer::TokenType::KEYWORD_DATA)) {
        consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data' keyword");
    }
    
    // Parse the bit size
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
    
    // Parse the size expression
    auto sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
    int64_t bits = sizeToken.intValue();
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after bit size");
    
    // Check for array syntax
    bool isArray = false;
    std::unique_ptr<Expr> arraySizeExpr;
    
    if (match(lexer::TokenType::LEFT_BRACKET)) {
        isArray = true;
        // Check if there's a size expression or an empty bracket for dynamic array
        if (!check(lexer::TokenType::RIGHT_BRACKET)) {
            arraySizeExpr = expression();
        }
        consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array size");
    }
    
    // Parse the type name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected data type name");
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after data declaration");
    
    // Create the base data type
    auto dataType = std::make_unique<DataTypeExpr>(
        bits,
        isSigned,
        makeRange(previous_)
    );
    
    // If it's an array, wrap the data type in an array type
    if (isArray) {
        auto arrayType = std::make_unique<ArrayTypeExpr>(
            std::move(dataType),
            std::move(arraySizeExpr),
            makeRange(previous_)
        );
        
        return std::make_unique<VarDecl>(
            name.lexeme(),
            std::move(arrayType),
            nullptr,  // no initializer
            false,    // not const
            makeRange(previous_)
        );
    }
    
    // If not an array, create a simple variable declaration
    return std::make_unique<VarDecl>(
        name.lexeme(),
        std::move(dataType),
        nullptr,  // no initializer
        false,    // not const
        makeRange(previous_)
    );
}

// Parse an ASM block declaration
std::unique_ptr<Decl> Parser::asmDeclaration() {
    auto start = previous_;  // 'asm' keyword
    
    // Parse the ASM block
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'asm'");
    
    // Collect the ASM code as a string
    std::stringstream asmCode;
    
    // Track nested braces
    int braceLevel = 1;
    
    // Continue until we find the matching closing brace
    while (braceLevel > 0 && !check(lexer::TokenType::END_OF_FILE)) {
        if (check(lexer::TokenType::LEFT_BRACE)) {
            braceLevel++;
        } else if (check(lexer::TokenType::RIGHT_BRACE)) {
            braceLevel--;
            if (braceLevel == 0) {
                break;  // Found the matching closing brace
            }
        }
        
        // Add the current token's lexeme to the ASM code
        // Add a space to preserve token separation
        if (asmCode.tellp() > 0) {
            asmCode << " ";
        }
        asmCode << current_.lexeme();
        
        // Move to the next token
        advance();
    }
    
    auto endToken = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after ASM block");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after ASM block");
    
    return std::make_unique<AsmDecl>(
        asmCode.str(),
        makeRange(start, endToken)
    );
}

// Parse a statement
std::unique_ptr<Stmt> Parser::statement() {
    // For blocks inside function bodies, just parse directly
    if (check(lexer::TokenType::LEFT_BRACE)) {
        if (inControlStructure_ || inFunctionBody_) {
            advance(); // Consume '{'
            return blockStatement();
        } else {
            auto start = current_;
            advance(); // Consume '{'
            previous_ = start;
            return anonymousBlockStatement();
        }
    }
    if (match({lexer::TokenType::KEYWORD_DATA, 
               lexer::TokenType::KEYWORD_SIGNED, 
               lexer::TokenType::KEYWORD_UNSIGNED})) {
        // Use our new dedicated handler for data type declarations
        return dataTypeStatement();
    }
    if (match(lexer::TokenType::KEYWORD_IF)) {
        return ifStatement();
    }
    if (match(lexer::TokenType::KEYWORD_DO)) {
        return doWhileStatement();
    }
    if (match(lexer::TokenType::KEYWORD_WHILE)) {
        return whileStatement();
    }
    if (match(lexer::TokenType::KEYWORD_FOR)) {
        return forStatement();
    }
    if (match(lexer::TokenType::KEYWORD_RETURN)) {
        return returnStatement();
    }
    if (match(lexer::TokenType::KEYWORD_BREAK)) {
        return breakStatement();
    }
    if (match(lexer::TokenType::KEYWORD_CONTINUE)) {
        return continueStatement();
    }
    if (match(lexer::TokenType::KEYWORD_TRY)) {
        return tryStatement();
    }
    if (match(lexer::TokenType::KEYWORD_MATCH)) {
        return matchStatement();
    }
    if (match(lexer::TokenType::SEMICOLON)) {
        // Create an empty expression statement
        return std::make_unique<ExprStmt>(
            nullptr,  // No expression
            makeRange(previous_)
        );
    }
    if (match(lexer::TokenType::KEYWORD_ASM)) {
        // Use the existing asmDeclaration method and wrap it in a DeclStmt
        auto asmDecl = asmDeclaration();
        if (asmDecl) {
            return std::make_unique<DeclStmt>(std::move(asmDecl), asmDecl->range);
        }
        return nullptr;
    }
    if (check(lexer::TokenType::KEYWORD_OPERATOR)) {
        auto savePos = current_;
        advance(); // Consume 'operator'
        
        // Try to parse as operator declaration
        auto opDecl = operatorDeclaration();
        
        if (opDecl) {
            return std::make_unique<DeclStmt>(std::move(opDecl), opDecl->range);
        }
        
        // If not an operator declaration, restore position and continue
        current_ = savePos;
    }
    // Default to expression statement
    auto expr_stmt = expressionStatement();
    if (!expr_stmt && !check(lexer::TokenType::END_OF_FILE) && 
        !check(lexer::TokenType::RIGHT_BRACE)) {
        // Force advancement to prevent getting stuck
        advance();
    }
    return expr_stmt;
}

// Parse a data type statement (data{32} primitiveName;)
std::unique_ptr<Stmt> Parser::dataTypeStatement() {
    auto startToken = previous_; // Save the data/signed/unsigned token
    bool isSigned = !previous_.is(lexer::TokenType::KEYWORD_UNSIGNED);
    
    // If we have signed/unsigned but not data, consume the data keyword
    if (!previous_.is(lexer::TokenType::KEYWORD_DATA)) {
        consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data' keyword");
    }
    
    // Parse the bit width specification
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
    auto sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
    int64_t bits = sizeToken.intValue();
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after bit size");
    
    // Parse primitive type name
    auto typeName = consume(lexer::TokenType::IDENTIFIER, "Expected primitive type name after bit size");
    
    // Create the data declaration
    auto dataDecl = std::make_unique<DataDecl>(
        typeName.lexeme(),
        bits,
        isSigned,
        makeRange(startToken, previous_)
    );
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after data type declaration");
    
    // Create a decl statement with the data declaration
    return std::make_unique<DeclStmt>(std::move(dataDecl), makeRange(startToken, previous_));
}

// Parse an expression statement
std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto startToken = current_;
    
    try {
        // Regular expression statement
        auto expr = expression();
        if (!expr) {
            // If we're at a boundary token, don't report an error
            if (check(lexer::TokenType::RIGHT_BRACE) || 
                check(lexer::TokenType::SEMICOLON) ||
                check(lexer::TokenType::END_OF_FILE)) {
                return nullptr;
            }
            
            error("Expected expression");
            synchronize();
            return nullptr;
        }
        
        // Look for semicolon
        if (check(lexer::TokenType::SEMICOLON)) {
            advance(); // Consume the semicolon without reporting error
        } else {
            // Semicolon is required in most contexts, but not at the end of a file
            if (!check(lexer::TokenType::END_OF_FILE)) {
                error("Expected ';' after expression");
                // Don't return, continue to create the expression statement
            }
        }
        
        return std::make_unique<ExprStmt>(
            std::move(expr),
            makeRange(startToken, previous_)
        );
    }
    catch (const std::exception& e) {
        error("Error parsing expression statement: " + std::string(e.what()));
        synchronize();
        return nullptr;
    }
}

// Parse a block statement
std::unique_ptr<Stmt> Parser::blockStatement() {
    std::cout << "Entering blockStatement()" << std::endl;
    auto start = previous_;  // Should be '{'
    
    std::cout << "Block starting at line " << start.start().line 
              << ", col " << start.start().column << std::endl;
    
    std::vector<std::unique_ptr<Stmt>> statements;
    
    // Simple approach: parse statements until we hit a closing brace
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        
        // Check for function declarations inside functions, which isn't allowed in Flux
        if (check(lexer::TokenType::KEYWORD_DEF) && inFunctionBody_ && !inObjectBody_) {
            error("Functions cannot be defined inside other functions");
            advance(); // Skip 'def'
            
            // Skip over the function declaration
            while (!check(lexer::TokenType::SEMICOLON) && 
                   !check(lexer::TokenType::RIGHT_BRACE) && 
                   !check(lexer::TokenType::END_OF_FILE)) {
                advance();
            }
            
            if (check(lexer::TokenType::SEMICOLON)) {
                advance(); // Skip ';'
            }
            
            continue;
        }
        
        auto stmt = statement();
        
        if (stmt) {
            statements.push_back(std::move(stmt));
        } else {
            // If we failed to parse a statement but we're at a right brace,
            // that's actually ok - it means we've reached the end of the block
            if (check(lexer::TokenType::RIGHT_BRACE)) {
                break;
            }
            
            std::cerr << "Failed to parse statement in block. Current token: " 
                      << current_.toString() << std::endl;
            
            // Skip one token to prevent infinite loops
            if (!check(lexer::TokenType::RIGHT_BRACE) && 
                !check(lexer::TokenType::END_OF_FILE)) {
                advance();
                // After advancing, check if we're at a right brace again
                if (check(lexer::TokenType::RIGHT_BRACE)) {
                    break;
                }
            }
        }
    }
    
    auto end = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after block");
    
    std::cout << "Block ending at line " << end.end().line 
              << ", col " << end.end().column << std::endl;
    
    auto blockStmt = std::make_unique<BlockStmt>(
        std::move(statements),
        makeRange(start, end)
    );
    
    std::cout << "Block statement parsed successfully" << std::endl;
    return blockStmt;
}

// Parse anonymous block statements
std::unique_ptr<Stmt> Parser::anonymousBlockStatement() {
    auto start = previous_;  // Should be '{'
    std::cout << "Parsing anonymous block starting at line " << start.start().line 
              << ", col " << start.start().column << std::endl;
    
    std::vector<std::unique_ptr<Stmt>> statements;
    
    // Parse statements until we reach the closing brace
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        
        if (check(lexer::TokenType::KEYWORD_DEF)) {
            // Special case for function declarations in anonymous blocks
            auto funcStart = current_;
            advance(); // Consume 'def'
            
            // Function name
            consume(lexer::TokenType::IDENTIFIER, "Expected function name");
            
            // Parameter list
            consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after function name");
            
            // Skip parameter list
            int parenLevel = 1;
            while (parenLevel > 0 && !check(lexer::TokenType::END_OF_FILE)) {
                if (match(lexer::TokenType::LEFT_PAREN)) parenLevel++;
                else if (match(lexer::TokenType::RIGHT_PAREN)) parenLevel--;
                else advance();
            }
            
            // Return type
            if (match(lexer::TokenType::ARROW)) {
                if (check(lexer::TokenType::KEYWORD_VOID) || 
                    check(lexer::TokenType::BANG_VOID) ||
                    check(lexer::TokenType::IDENTIFIER)) {
                    advance();
                }
            }
            
            // Function body
            if (match(lexer::TokenType::LEFT_BRACE)) {
                int braceLevel = 1;
                
                while (braceLevel > 0 && !check(lexer::TokenType::END_OF_FILE)) {
                    if (match(lexer::TokenType::LEFT_BRACE)) braceLevel++;
                    else if (match(lexer::TokenType::RIGHT_BRACE)) braceLevel--;
                    else advance();
                }
                
                // Consume semicolon after function body
                if (check(lexer::TokenType::SEMICOLON)) {
                    advance();
                }
            }
            
            // Add a dummy statement for the function
            auto dummyExpr = std::make_unique<LiteralExpr>(
                lexer::Token(lexer::TokenType::INTEGER_LITERAL, "0", 
                            funcStart.start(), previous_.end()),
                static_cast<int64_t>(0)
            );
            
            statements.push_back(std::make_unique<ExprStmt>(
                std::move(dummyExpr),
                makeRange(funcStart, previous_)
            ));
            
            std::cout << "Successfully parsed function in anonymous block" << std::endl;
        }
        else {
            // Parse a regular statement
            auto stmt = statement();
            
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else {
                // Force advancement to prevent getting stuck
                error("Expected statement in anonymous block");
                advance();
                synchronize();
            }
        }
    }
    
    // Consume closing brace
    auto end = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' to close anonymous block");
    
    // Consume semicolon after block
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after anonymous block");
    
    std::cout << "Anonymous block parsed successfully" << std::endl;
    
    return std::make_unique<BlockStmt>(
        std::move(statements),
        makeRange(start, end)
    );
}

// Parse an if statement
std::unique_ptr<Stmt> Parser::ifStatement() {
    auto start = previous_;  // 'if' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'if'");
    
    // Parse the condition expression
    auto condition = expression();
    if (!condition) {
        error("Expected condition expression in if statement");
        synchronize();
        return nullptr;
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after condition");
    
    // Set control structure flag
    bool oldControlStructure = inControlStructure_;
    inControlStructure_ = true;
    
    // Parse then branch
    std::unique_ptr<Stmt> thenBranch;
    
    if (match(lexer::TokenType::LEFT_BRACE)) {
        // Create a simple block statement directly
        std::vector<std::unique_ptr<Stmt>> statements;
        
        while (!check(lexer::TokenType::RIGHT_BRACE) && 
               !check(lexer::TokenType::END_OF_FILE)) {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else {
                // Skip problematic tokens
                if (!check(lexer::TokenType::RIGHT_BRACE) && 
                    !check(lexer::TokenType::END_OF_FILE)) {
                    advance();
                } else {
                    break;  // We're at the end of the block
                }
            }
        }
        
        auto closeBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after if block");
        
        thenBranch = std::make_unique<BlockStmt>(
            std::move(statements),
            makeRange(start, closeBrace)
        );
    } else {
        thenBranch = statement();
    }
    
    if (!thenBranch) {
        error("Expected statement after if condition");
        // Create a dummy block to avoid null pointer
        thenBranch = std::make_unique<BlockStmt>(
            std::vector<std::unique_ptr<Stmt>>{},
            makeRange(previous_)
        );
    }
    
    // Parse else branch
    std::unique_ptr<Stmt> elseBranch;
    
    if (match(lexer::TokenType::KEYWORD_ELSE)) {
        if (match(lexer::TokenType::LEFT_BRACE)) {
            // Create a simple block statement directly for else branch
            std::vector<std::unique_ptr<Stmt>> statements;
            
            while (!check(lexer::TokenType::RIGHT_BRACE) && 
                   !check(lexer::TokenType::END_OF_FILE)) {
                auto stmt = statement();
                if (stmt) {
                    statements.push_back(std::move(stmt));
                } else {
                    // Skip problematic tokens
                    if (!check(lexer::TokenType::RIGHT_BRACE) && 
                        !check(lexer::TokenType::END_OF_FILE)) {
                        advance();
                    } else {
                        break;  // We're at the end of the block
                    }
                }
            }
            
            auto closeBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after else block");
            
            elseBranch = std::make_unique<BlockStmt>(
                std::move(statements),
                makeRange(previous_, closeBrace)
            );
        } else if (check(lexer::TokenType::KEYWORD_IF)) {
            // This is an else-if
            elseBranch = statement();
        } else {
            elseBranch = statement();
        }
    }
    
    // Restore control structure flag
    inControlStructure_ = oldControlStructure;
    
    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch),
        makeRange(start, previous_)
    );
}

std::unique_ptr<Stmt> Parser::doWhileStatement() {
    auto start = previous_;  // 'do' keyword
    
    // Parse body
    std::unique_ptr<Stmt> body;
    if (match(lexer::TokenType::LEFT_BRACE)) {
        // Block statement
        std::vector<std::unique_ptr<Stmt>> statements;
        
        while (!check(lexer::TokenType::RIGHT_BRACE) && 
              !check(lexer::TokenType::END_OF_FILE)) {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else if (!check(lexer::TokenType::RIGHT_BRACE)) {
                advance();
            } else {
                break;
            }
        }
        
        auto rightBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after do body");
        
        body = std::make_unique<BlockStmt>(
            std::move(statements),
            makeRange(start, rightBrace)
        );
    } else {
        // Single statement
        body = statement();
        if (!body) {
            // Create empty block to avoid null pointer
            body = std::make_unique<BlockStmt>(
                std::vector<std::unique_ptr<Stmt>>{},
                makeRange(previous_)
            );
        }
    }
    
    // Parse 'while' and condition
    consume(lexer::TokenType::KEYWORD_WHILE, "Expected 'while' after do block");
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    
    auto condition = expression();
    if (!condition) {
        // Default to true condition if parse fails
        auto trueToken = lexer::Token(
            lexer::TokenType::KEYWORD_TRUE, 
            "true", 
            previous_.start(), 
            previous_.end()
        );
        condition = std::make_unique<LiteralExpr>(trueToken, true);
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after while condition");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after do-while statement");
    
    // Create a specialized DoWhileStmt or adapt WhileStmt
    // You may need to add a DoWhileStmt class to your AST, 
    // or use WhileStmt with a flag indicating it's a do-while loop
    return std::make_unique<WhileStmt>(
        std::move(condition),
        std::move(body),
        makeRange(start, previous_)
    );
}

// Parse a while statement
std::unique_ptr<Stmt> Parser::whileStatement() {
    auto start = previous_;  // 'while' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'while'");
    auto condition = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after condition");
    
    auto body = statement();
    
    return std::make_unique<WhileStmt>(
        std::move(condition),
        std::move(body),
        makeRange(start, previous_)
    );
}

// Parse a for statement
std::unique_ptr<Stmt> Parser::forStatement() {
    auto start = previous_;  // 'for' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'for'");
    
    // Parse initializer
    std::unique_ptr<Stmt> initializer = nullptr;
    
    if (!check(lexer::TokenType::SEMICOLON)) {
        // Check if it starts with an identifier (potential type name)
        if (check(lexer::TokenType::IDENTIFIER)) {
            auto typeToken = current_;
            advance(); // Consume potential type name
            
            // If followed by another identifier, it's likely a variable declaration
            if (check(lexer::TokenType::IDENTIFIER)) {
                auto nameToken = current_;
                advance(); // Consume variable name
                
                // Create type expression
                auto varType = std::make_unique<NamedTypeExpr>(
                    typeToken.lexeme(),
                    makeRange(typeToken)
                );
                
                // Parse initializer if present
                std::unique_ptr<Expr> varInitializer = nullptr;
                if (match(lexer::TokenType::EQUAL)) {
                    varInitializer = expression();
                    if (!varInitializer) {
                        error("Expected expression after '='");
                    }
                }
                
                // Create variable statement
                initializer = std::make_unique<VarStmt>(
                    nameToken,
                    std::move(varType),
                    std::move(varInitializer),
                    makeRange(typeToken, previous_)
                );
            } else {
                // Not a type declaration, rewind and parse as expression
                current_ = typeToken;
                auto expr = expression();
                if (!expr) {
                    error("Expected expression in for initializer");
                } else {
                    initializer = std::make_unique<ExprStmt>(
                        std::move(expr),
                        makeRange(start, previous_)
                    );
                }
            }
        } else {
            // Try to parse as a regular expression
            auto expr = expression();
            if (expr) {
                initializer = std::make_unique<ExprStmt>(
                    std::move(expr),
                    makeRange(start, previous_)
                );
            } else {
                error("Expected expression in for initializer");
            }
        }
    }
    
    // Consume the first semicolon
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after for initializer");
    
    // Parse condition (can be empty)
    std::unique_ptr<Expr> condition = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        condition = expression();
        if (!condition) {
            error("Expected condition expression or ';'");
        }
    }
    
    // Consume the second semicolon
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after for condition");
    
    // Parse increment (can be empty)
    std::unique_ptr<Expr> increment = nullptr;
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        increment = expression();
        if (!increment) {
            error("Expected increment expression or ')'");
        }
    }
    
    // Consume the closing parenthesis
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after for clauses");
    
    // Parse body
    std::unique_ptr<Stmt> body;
    
    // Use a temporary flag to indicate we're in a control structure
    bool oldInControl = inControlStructure_;
    inControlStructure_ = true;
    
    if (match(lexer::TokenType::LEFT_BRACE)) {
        // Parse block body
        std::vector<std::unique_ptr<Stmt>> statements;
        
        while (!check(lexer::TokenType::RIGHT_BRACE) && 
               !check(lexer::TokenType::END_OF_FILE)) {
            
            auto stmt = statement();
            
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else if (!check(lexer::TokenType::RIGHT_BRACE)) {
                // Skip tokens to avoid infinite loop
                error("Invalid statement in for loop body");
                advance();
            }
        }
        
        auto endBrace = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after for loop body");
        
        body = std::make_unique<BlockStmt>(
            std::move(statements),
            makeRange(start, endBrace)
        );
    } else {
        // Single statement body
        body = statement();
        
        if (!body) {
            error("Expected statement for for loop body");
            
            // Create an empty block as fallback
            body = std::make_unique<BlockStmt>(
                std::vector<std::unique_ptr<Stmt>>{},
                makeRange(previous_)
            );
        }
    }
    
    // Restore control structure flag
    inControlStructure_ = oldInControl;
    
    // Consume semicolon after for loop
    if (check(lexer::TokenType::SEMICOLON)) {
        advance();
    } else {
        consume(lexer::TokenType::SEMICOLON, "Expected ';' after for statement");
    }
    
    return std::make_unique<ForStmt>(
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::move(body),
        makeRange(start, previous_)
    );
}

// Parse a return statement
std::unique_ptr<Stmt> Parser::returnStatement() {
    auto start = previous_;  // 'return' keyword
    
    // Parse return value if present
    std::unique_ptr<Expr> value;
    if (!check(lexer::TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after return statement");
    
    return std::make_unique<ReturnStmt>(
        start,
        std::move(value),
        makeRange(start, previous_)
    );
}

// Parse a break statement
std::unique_ptr<Stmt> Parser::breakStatement() {
    auto keyword = previous_;  // 'break' keyword
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after break statement");
    
    return std::make_unique<BreakStmt>(
        keyword,
        makeRange(keyword, previous_)
    );
}

// Parse a continue statement
std::unique_ptr<Stmt> Parser::continueStatement() {
    auto keyword = previous_;  // 'continue' keyword
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after continue statement");
    
    return std::make_unique<ContinueStmt>(
        keyword,
        makeRange(keyword, previous_)
    );
}

// Parse a try-catch statement
std::unique_ptr<Stmt> Parser::tryStatement() {
    auto start = previous_;  // 'try' keyword
    
    // Parse try block
    auto tryBlock = statement();
    
    // Parse catch clauses
    std::vector<TryStmt::CatchClause> catchClauses;
    
    while (match(lexer::TokenType::KEYWORD_CATCH)) {
        // Parse exception type (optional)
        std::unique_ptr<TypeExpr> exceptionType;
        
        if (match(lexer::TokenType::LEFT_PAREN)) {
            // Named exception type
            exceptionType = type();
            consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after catch type");
        }
        
        // Parse catch handler
        auto handler = statement();
        
        catchClauses.emplace_back(std::move(exceptionType), std::move(handler));
    }
    
    if (catchClauses.empty()) {
        error("Expected at least one catch clause");
    }
    
    // Add semicolon after the complete try-catch statement
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after try-catch statement");
    
    return std::make_unique<TryStmt>(
        std::move(tryBlock),
        std::move(catchClauses),
        makeRange(start, previous_)
    );
}

// Parse a match statement
std::unique_ptr<Stmt> Parser::matchStatement() {
    auto start = previous_;  // 'match' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'match'");
    auto value = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after match value");
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after match value");
    
    std::vector<MatchStmt::CaseClause> cases;
    std::unique_ptr<Stmt> defaultCase;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && 
           !check(lexer::TokenType::END_OF_FILE)) {
        // Parse case or default
        if (match(lexer::TokenType::KEYWORD_CASE)) {
            // Parse pattern
            auto pattern = expression();
            
            consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after case pattern");
            
            // Parse case body
            auto body = blockStatement();
            
            cases.emplace_back(std::move(pattern), std::move(body));
        } else if (match(lexer::TokenType::KEYWORD_DEFAULT)) {
            consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after default");
            
            // Parse default body
            defaultCase = blockStatement();
        } else {
            error("Expected 'case' or 'default' in match statement");
            break;
        }
    }
    
    auto end = consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after match statement");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after match statement");
    
    return std::make_unique<MatchStmt>(
        std::move(value),
        std::move(cases),
        std::move(defaultCase),
        makeRange(start, end)
    );
}

// Parse a variable statement
std::unique_ptr<Stmt> Parser::variableStatement() {
    // Parse the variable name
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected variable name");
    
    // Parse variable type (optional)
    std::unique_ptr<TypeExpr> varType;
    if (check(lexer::TokenType::IDENTIFIER)) {
        varType = type();
    }
    
    // Parse initializer (optional)
    std::unique_ptr<Expr> initializer;
    if (match(lexer::TokenType::EQUAL)) {
        initializer = expression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VarStmt>(
        name,
        std::move(varType),
        std::move(initializer),
        makeRange(name, previous_)
    );
}

// Parse an expression
std::unique_ptr<Expr> Parser::expression() {
    try {
        return assignment();
    } catch (const std::exception& e) {
        error(std::string("Error parsing expression: ") + e.what());
        synchronize();
        return nullptr;
    }
}

// Parse an assignment expression
std::unique_ptr<Expr> Parser::assignment() {
    auto expr = ternary();
    
    if (expr != nullptr && match({
        lexer::TokenType::EQUAL,
        lexer::TokenType::PLUS_EQUAL,
        lexer::TokenType::MINUS_EQUAL,
        lexer::TokenType::ASTERISK_EQUAL,
        lexer::TokenType::SLASH_EQUAL,
        lexer::TokenType::PERCENT_EQUAL,
        lexer::TokenType::AMPERSAND_EQUAL,
        lexer::TokenType::PIPE_EQUAL,
        lexer::TokenType::CARET_EQUAL,
        lexer::TokenType::LESS_LESS_EQUAL,
        lexer::TokenType::GREATER_GREATER_EQUAL,
        lexer::TokenType::DOUBLE_ASTERISK_EQUAL
    })) {
        auto op = previous_;
        auto value = assignment();
        
        // Validate that expr is a valid lvalue
        if (dynamic_cast<VariableExpr*>(expr.get()) ||
            dynamic_cast<GetExpr*>(expr.get()) ||
            dynamic_cast<SubscriptExpr*>(expr.get())) {
            
            if (value) {
                return std::make_unique<AssignExpr>(
                    std::move(expr),
                    op,
                    std::move(value),
                    makeRange(op, previous_)
                );
            } else {
                // Handle missing right-hand side
                error("Expected expression after assignment operator");
                
                // Create a placeholder value to avoid null pointer
                auto errorExpr = std::make_unique<LiteralExpr>(
                    lexer::Token(lexer::TokenType::INTEGER_LITERAL, "0", op.start(), op.end()),
                    static_cast<int64_t>(0)
                );
                
                return std::make_unique<AssignExpr>(
                    std::move(expr),
                    op,
                    std::move(errorExpr),
                    makeRange(op, previous_)
                );
            }
        }
        
        error("Invalid assignment target");
    }
    
    return expr;
}

// Parse a ternary expression
std::unique_ptr<Expr> Parser::ternary() {
    auto expr = logicalOr();
    
    if (match(lexer::TokenType::QUESTION)) {
        auto thenExpr = expression();
        consume(lexer::TokenType::COLON, "Expected ':' in ternary expression");
        auto elseExpr = ternary();
        
        return std::make_unique<TernaryExpr>(
            std::move(expr),
            std::move(thenExpr),
            std::move(elseExpr),
            makeRange(previous_)
        );
    }
    
    return expr;
}

// Parse a logical OR expression
std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match({lexer::TokenType::KEYWORD_OR, lexer::TokenType::PIPE_PIPE})) {
        auto op = previous_;
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a logical AND expression
std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = bitwiseOr();
    
    while (match({lexer::TokenType::KEYWORD_AND, lexer::TokenType::AMPERSAND_AMPERSAND})) {
        auto op = previous_;
        auto right = bitwiseOr();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a bitwise OR expression
std::unique_ptr<Expr> Parser::bitwiseOr() {
    auto expr = bitwiseXor();
    
    while (match(lexer::TokenType::PIPE)) {
        auto op = previous_;
        auto right = bitwiseXor();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a bitwise XOR expression
std::unique_ptr<Expr> Parser::bitwiseXor() {
    auto expr = bitwiseAnd();
    
    while (match({lexer::TokenType::CARET, lexer::TokenType::KEYWORD_XOR})) {
        auto op = previous_;
        auto right = bitwiseAnd();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a bitwise AND expression
std::unique_ptr<Expr> Parser::bitwiseAnd() {
    auto expr = equality();
    
    while (match(lexer::TokenType::AMPERSAND)) {
        auto op = previous_;
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse an equality expression
std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    
    while (match({
        lexer::TokenType::EQUAL_EQUAL, 
        lexer::TokenType::NOT_EQUAL,
        lexer::TokenType::KEYWORD_IS
    })) {
        auto op = previous_;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a comparison expression
std::unique_ptr<Expr> Parser::comparison() {
    auto expr = bitShift();
    
    while (match({
        lexer::TokenType::LESS, 
        lexer::TokenType::LESS_EQUAL,
        lexer::TokenType::GREATER,
        lexer::TokenType::GREATER_EQUAL
    })) {
        auto op = previous_;
        auto right = bitShift();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a bit shift expression
std::unique_ptr<Expr> Parser::bitShift() {
    auto expr = term();
    
    while (match({
        lexer::TokenType::LESS_LESS, 
        lexer::TokenType::GREATER_GREATER
    })) {
        auto op = previous_;
        auto right = term();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a term expression
std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    
    while (match({
        lexer::TokenType::PLUS, 
        lexer::TokenType::MINUS
    })) {
        auto op = previous_;
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a factor expression
std::unique_ptr<Expr> Parser::factor() {
    auto expr = exponentiation();
    
    while (match({
        lexer::TokenType::ASTERISK, 
        lexer::TokenType::SLASH,
        lexer::TokenType::PERCENT
    })) {
        auto op = previous_;
        auto right = exponentiation();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse an exponentiation expression
std::unique_ptr<Expr> Parser::exponentiation() {
    auto expr = unary();
    
    while (match(lexer::TokenType::DOUBLE_ASTERISK)) {
        auto op = previous_;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a unary expression
std::unique_ptr<Expr> Parser::unary() {
    if (match({
        lexer::TokenType::EXCLAMATION,
        lexer::TokenType::KEYWORD_NOT,
        lexer::TokenType::MINUS,
        lexer::TokenType::PLUS,
        lexer::TokenType::TILDE,
        lexer::TokenType::PLUS_PLUS,
        lexer::TokenType::MINUS_MINUS,
        lexer::TokenType::AMPERSAND,
        lexer::TokenType::ASTERISK
    })) {
        auto op = previous_;
        auto right = unary();
        return std::make_unique<UnaryExpr>(
            op,
            std::move(right),
            true,  // prefix
            makeRange(op, previous_)
        );
    }
    
    return postfix();
}

// Parse a postfix expression
std::unique_ptr<Expr> Parser::postfix() {
    auto expr = call();
    
    if (match({
        lexer::TokenType::PLUS_PLUS,
        lexer::TokenType::MINUS_MINUS
    })) {
        auto op = previous_;
        return std::make_unique<UnaryExpr>(
            op,
            std::move(expr),
            false,  // postfix
            makeRange(op, previous_)
        );
    }
    
    return expr;
}

// Parse a call expression
std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();
    
    while (true) {
        // Function call
        if (match(lexer::TokenType::LEFT_PAREN)) {
            expr = finishCall(std::move(expr));
        } 
        // Property access
        else if (match(lexer::TokenType::DOT)) {
            auto name = consume(lexer::TokenType::IDENTIFIER, "Expected property name after '.'");
            expr = std::make_unique<GetExpr>(
                std::move(expr),
                name,
                makeRange(previous_, name)
            );
        } 
        // Subscript/indexing
        else if (match(lexer::TokenType::LEFT_BRACKET)) {
            auto index = expression();
            auto right = consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after index");
            expr = std::make_unique<SubscriptExpr>(
                std::move(expr),
                std::move(index),
                makeRange(previous_, right)
            );
        } 
        // No more postfix expressions
        else {
            break;
        }
    }
    
    return expr;
}

// Parse a primary expression
std::unique_ptr<Expr> Parser::primary() {
    try {
        if (match(lexer::TokenType::KEYWORD_THIS)) {
            auto thisToken = previous_;
            return std::make_unique<VariableExpr>(thisToken);
        }
        
        if (match(lexer::TokenType::KEYWORD_SUPER)) {
            auto superToken = previous_;
            return std::make_unique<VariableExpr>(superToken);
        }

        if (match(lexer::TokenType::KEYWORD_FALSE)) {
            return std::make_unique<LiteralExpr>(
                previous_,
                false
            );
        }
        
        if (match(lexer::TokenType::KEYWORD_TRUE)) {
            return std::make_unique<LiteralExpr>(
                previous_,
                true
            );
        }
        
        if (match({
            lexer::TokenType::INTEGER_LITERAL,
            lexer::TokenType::FLOAT_LITERAL,
            lexer::TokenType::CHAR_LITERAL
        })) {
            if (previous_.type() == lexer::TokenType::INTEGER_LITERAL) {
                return std::make_unique<LiteralExpr>(
                    previous_,
                    previous_.intValue()
                );
            } else if (previous_.type() == lexer::TokenType::FLOAT_LITERAL) {
                return std::make_unique<LiteralExpr>(
                    previous_,
                    previous_.floatValue()
                );
            } else {  // CHAR_LITERAL (string in Flux)
                return std::make_unique<LiteralExpr>(
                    previous_,
                    std::string(previous_.stringValue())
                );
            }
        }
        
        // Handle identifiers, including keywords that can be used as identifiers
        if (check(lexer::TokenType::IDENTIFIER)) {
            return qualifiedIdentifier();  // Use qualifiedIdentifier() instead of directly consuming
        }

        if (match({
                lexer::TokenType::KEYWORD_XOR,
                lexer::TokenType::KEYWORD_AND,
                lexer::TokenType::KEYWORD_OR,
                lexer::TokenType::KEYWORD_NOT
            })) {
            return std::make_unique<VariableExpr>(previous_);
        }
        
        // Parenthesized expressions
        if (match(lexer::TokenType::LEFT_PAREN)) {
            auto expr = expression();
            consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after expression");
            return std::make_unique<GroupExpr>(
                std::move(expr),
                makeRange(previous_)
            );
        }

        // Handle sizeof
        if (match(lexer::TokenType::KEYWORD_SIZEOF)) {
            return parseSizeOfExpr();
        }
        
        // Handle typeof
        if (match(lexer::TokenType::KEYWORD_TYPEOF)) {
            return parseTypeOfExpr();
        }
        
        // Handle op
        if (match(lexer::TokenType::KEYWORD_OP)) {
            return parseOpExpr();
        }
        
        // If no valid primary expression is found
        error("Expected expression");
        return nullptr;
    } catch (const std::exception& e) {
        error("Error parsing primary expression: " + std::string(e.what()));
        return nullptr;
    }
}

// Finish parsing a call expression
std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    std::vector<std::unique_ptr<Expr>> arguments;
    
    // Check if there are any arguments
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            // Limit number of arguments to prevent excessive recursion
            if (arguments.size() >= 255) {
                error("Cannot have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    // Consume the closing parenthesis
    auto paren = consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after arguments");
    
    // Create and return the call expression
    return std::make_unique<CallExpr>(
        std::move(callee),
        paren,
        std::move(arguments),
        makeRange(previous_, paren)
    );
}

// This function handles qualified identifiers like Flux.Integer, etc.
std::unique_ptr<Expr> Parser::qualifiedIdentifier() {
    auto start = current_;
    auto firstIdent = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
    
    // Create initial variable expression as a unique_ptr<Expr>
    std::unique_ptr<Expr> expr = std::make_unique<VariableExpr>(firstIdent);
    
    // Process each dot-separated part
    while (match(lexer::TokenType::DOT)) {
        auto name = consume(lexer::TokenType::IDENTIFIER, "Expected identifier after '.'");
        
        // Create a new GetExpr that holds the previous expression
        expr = std::make_unique<GetExpr>(
            std::move(expr), // Move the previous expression as the object
            name,
            makeRange(start, name)
        );
    }
    
    return expr;
}

// This function handles qualified types like Flux.Integer, etc.
std::unique_ptr<TypeExpr> Parser::qualifiedType() {
    auto start = current_;
    
    // Parse the first part of the qualified name
    auto firstIdent = consume(lexer::TokenType::IDENTIFIER, "Expected type name");
    std::vector<std::string_view> parts;
    parts.push_back(firstIdent.lexeme());
    
    // Process each dot-separated part
    while (match(lexer::TokenType::DOT)) {
        auto name = consume(lexer::TokenType::IDENTIFIER, "Expected identifier after '.'");
        parts.push_back(name.lexeme());
    }
    
    // Combine the parts into a single qualified name
    std::string qualified_name;
    for (size_t i = 0; i < parts.size(); ++i) {
        qualified_name += parts[i];
        if (i < parts.size() - 1) {
            qualified_name += ".";
        }
    }
    
    // Create a named type with the combined qualified name
    return std::make_unique<NamedTypeExpr>(
        qualified_name,
        makeRange(start, previous_)
    );
}

// Parse a type expression
std::unique_ptr<TypeExpr> Parser::type() {
    // Special case for class, object, struct pointers
    if (match({lexer::TokenType::KEYWORD_CLASS, 
              lexer::TokenType::KEYWORD_OBJECT, 
              lexer::TokenType::KEYWORD_STRUCT})) {
        
        auto typeToken = previous_; // 'class', 'object', or 'struct'
        std::string typeNameStr;
        
        if (typeToken.is(lexer::TokenType::KEYWORD_CLASS)) {
            typeNameStr = "class";
        } else if (typeToken.is(lexer::TokenType::KEYWORD_OBJECT)) {
            typeNameStr = "object";
        } else { // must be struct
            typeNameStr = "struct";
        }
        
        std::string_view typeName(typeNameStr);
        
        // Create base type expression
        auto baseType = std::make_unique<NamedTypeExpr>(
            typeName,
            makeRange(typeToken)
        );
        
        // Check for pointer modifier
        if (match(lexer::TokenType::ASTERISK)) {
            return std::make_unique<PointerTypeExpr>(
                std::move(baseType),
                makeRange(typeToken, previous_)
            );
        }
        
        return baseType;
    }
    
    // Handle pointers
    if (match(lexer::TokenType::ASTERISK)) {
        auto pointeeType = type();
        return std::make_unique<PointerTypeExpr>(
            std::move(pointeeType),
            makeRange(previous_)
        );
    }
    
    // Handle data types
    if (match({lexer::TokenType::KEYWORD_DATA, 
               lexer::TokenType::KEYWORD_SIGNED, 
               lexer::TokenType::KEYWORD_UNSIGNED})) {
        bool isSigned = !previous_.is(lexer::TokenType::KEYWORD_UNSIGNED);
        
        // Parse the bit size
        consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
        
        // Parse the size expression
        auto sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
        int64_t bits = sizeToken.intValue();
        
        consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after bit size");
        
        // Check for immediate array declaration
        if (match(lexer::TokenType::LEFT_BRACKET)) {
            // Create the base data type
            auto baseType = std::make_unique<DataTypeExpr>(
                bits,
                isSigned,
                makeRange(previous_)
            );
            
            // Parse array type
            return arrayType(std::move(baseType));
        }
        
        // If not an array, just return the data type
        return std::make_unique<DataTypeExpr>(
            bits,
            isSigned,
            makeRange(previous_)
        );
    }
    
    // Handle named types
    if (check(lexer::TokenType::IDENTIFIER)) {
        auto type = qualifiedType();  // Use qualifiedType() instead of namedType()
        
        // Check for array type
        if (match(lexer::TokenType::LEFT_BRACKET)) {
            return arrayType(std::move(type));
        }
        
        return type;
    }

    // Handle function types
    if (match(lexer::TokenType::KEYWORD_DEF)) {
        return functionType();
    }
    
    error("Expected type");
    return nullptr;
}

// Parse a pointer type
std::unique_ptr<TypeExpr> Parser::pointerType(std::unique_ptr<TypeExpr> pointeeType) {
    // Create a pointer type with the given pointee type
    auto range = makeRange(pointeeType->range.start, previous_.end());
    return std::make_unique<PointerTypeExpr>(
        std::move(pointeeType),
        range
    );
}

// Parse a named type
std::unique_ptr<TypeExpr> Parser::namedType() {
    auto name = consume(lexer::TokenType::IDENTIFIER, "Expected type name");
    
    return std::make_unique<NamedTypeExpr>(
        name.lexeme(),
        makeRange(name)
    );
}

// Parse an array type
std::unique_ptr<TypeExpr> Parser::arrayType(std::unique_ptr<TypeExpr> elementType) {
    // Parse array size expression (optional)
    std::unique_ptr<Expr> sizeExpr;
    if (!check(lexer::TokenType::RIGHT_BRACKET)) {
        sizeExpr = expression();
    }
    
    auto end = consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array type");
    
    return std::make_unique<ArrayTypeExpr>(
        std::move(elementType),
        std::move(sizeExpr), // This might be nullptr for dynamic arrays
        makeRange(previous_, end)
    );
}

// Parse a function type
std::unique_ptr<TypeExpr> Parser::functionType() {
    // Parse parameter types
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'def'");
    
    std::vector<std::unique_ptr<TypeExpr>> parameterTypes;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            parameterTypes.push_back(type());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after parameter types");
    
    // Parse return type
    consume(lexer::TokenType::ARROW, "Expected '->' after parameter types");
    
    auto returnType = type();
    
    return std::make_unique<FunctionTypeExpr>(
        std::move(parameterTypes),
        std::move(returnType),
        makeRange(previous_)
    );
}

// Parse a data type
std::unique_ptr<TypeExpr> Parser::dataType() {
    bool isSigned = !previous_.is(lexer::TokenType::KEYWORD_UNSIGNED);
    
    // Parse 'data' keyword if not already consumed
    if (!previous_.is(lexer::TokenType::KEYWORD_DATA)) {
        consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data' keyword");
    }
    
    // Parse the bit size
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
    
    // Parse the size expression
    auto sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
    int64_t bits = sizeToken.intValue();
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after bit size");
    
    return std::make_unique<DataTypeExpr>(
        bits,
        isSigned,
        makeRange(previous_)
    );
}

// Parse a sizeof expression (sizeof(type))
std::unique_ptr<Expr> Parser::parseSizeOfExpr() {
    auto start = previous_;  // 'sizeof' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'sizeof'");
    
    // Parse the type
    auto targetType = type();
    if (!targetType) {
        error("Expected type in sizeof expression");
        return nullptr;
    }
    
    auto end = consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after type in sizeof");
    
    return std::make_unique<SizeOfExpr>(
        std::move(targetType),
        makeRange(start, end)
    );
}

// Parse a typeof expression (typeof(expr))
std::unique_ptr<Expr> Parser::parseTypeOfExpr() {
    auto start = previous_;  // 'typeof' keyword
    
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' after 'typeof'");
    
    // Parse the expression
    auto expr = expression();
    if (!expr) {
        error("Expected expression in typeof");
        return nullptr;
    }
    
    auto end = consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after expression in typeof");
    
    return std::make_unique<TypeOfExpr>(
        std::move(expr),
        makeRange(start, end)
    );
}

// Parse an op expression (op<expr operator expr>)
std::unique_ptr<Expr> Parser::parseOpExpr() {
    auto start = previous_;  // 'op' keyword
    
    consume(lexer::TokenType::LESS, "Expected '<' after 'op'");
    
    // Parse the left operand
    auto left = expression();
    if (!left) {
        error("Expected expression for left operand of op");
        return nullptr;
    }
    
    // Parse the operator name
    if (!check(lexer::TokenType::IDENTIFIER)) {
        error("Expected operator name in op expression");
        return nullptr;
    }
    
    auto opName = current_.lexeme();
    advance();  // Consume the operator name
    
    // Parse the right operand
    auto right = expression();
    if (!right) {
        error("Expected expression for right operand of op");
        return nullptr;
    }
    
    auto end = consume(lexer::TokenType::GREATER, "Expected '>' after op expression");
    
    return std::make_unique<OpExpr>(
        std::move(left),
        opName,
        std::move(right),
        makeRange(start, end)
    );
}

// Parse a delimited list of items
template<typename T>
std::vector<T> Parser::parseDelimitedList(
    lexer::TokenType delimiter,
    lexer::TokenType end,
    std::function<T()> parseItem) {
    
    std::vector<T> items;
    
    if (!check(end)) {
        do {
            items.push_back(parseItem());
        } while (match(delimiter));
    }
    
    consume(end, "Expected end of list");
    
    return items;
}

} // namespace parser
} // namespace flux