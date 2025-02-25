#include "include/parser.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;

namespace flux {

// Global error reporter instance
ErrorReporter errorReporter;

// Constructor and destructor
Parser::Parser() 
    : current(0), filename(""), panicMode(false), hadError(false) {
    currentScope = std::make_shared<SymbolTable>();
    typeRegistry = std::make_shared<TypeRegistry>();
    typeRegistry->initializeBuiltinTypes();
    
    synchronizationPoints = {
        TokenType::CLASS, TokenType::FUNCTION, TokenType::NAMESPACE,
        TokenType::OBJECT, TokenType::STRUCT, TokenType::IMPORT,
        TokenType::TYPEDEF, TokenType::UNION, TokenType::SEMICOLON
    };
}

Parser::~Parser() {
    // Nothing to clean up
}

std::string Parser::trimLexeme(std::string_view lexeme) {
    std::string cleanLexeme;
    for (char c : lexeme) {
        if (!std::isspace(c)) {
            cleanLexeme += c;
        }
    }
    return cleanLexeme;
}

// Core parsing methods
std::shared_ptr<Program> Parser::parse(const std::vector<Token>& tokens, std::string_view filename) {
    this->tokens = tokens;
    this->current = 0;
    this->filename = filename;
    this->hadError = false;
    this->panicMode = false;
    
    auto program = parseProgram();
    
    if (isAtEnd() && !hadError) {
        return program;
    }
    
    return nullptr;
}

void Parser::reset() {
    current = 0;
    hadError = false;
    panicMode = false;
    currentScope = std::make_shared<SymbolTable>();
    typeRegistry = std::make_shared<TypeRegistry>();
    typeRegistry->initializeBuiltinTypes();
}

// Token navigation methods
Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::initializer_list<TokenType>& types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_FILE;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    errorAt(peek(), message);
    throw std::runtime_error(message);
}

// Error handling methods
void Parser::synchronize() {
    panicMode = false;
    
    while (!isAtEnd()) {
        // Stop at semicolon or known statement starts
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::FUNCTION:
            case TokenType::NAMESPACE:
            case TokenType::OBJECT:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::error(const std::string& message) {
    errorAt(peek(), message);
}

void Parser::error(const Token& token, const std::string& message) {
    errorAt(token, message);
}

void Parser::errorAt(const Token& token, const std::string& message) {
    if (panicMode) return;
    panicMode = true;
    hadError = true;
    
    SourceLocation location = token.getLocation();
    errorReporter.reportError(ErrorType::SYNTAX_ERROR, message, location);
}

// AST creation helper
AstLocation Parser::makeLocation(const Token& start, const Token& end) {
    return AstLocation(
        filename,
        start.line, start.column,
        end.line, end.column
    );
}

// Overload
AstLocation Parser::makeLocation(int startLine, int startColumn, int endLine, int endColumn) {
    return AstLocation(filename, startLine, startColumn, endLine, endColumn);
}

// Parsing declarations
std::shared_ptr<Program> Parser::parseProgram() {
    auto program = std::make_shared<Program>(AstLocation(filename, 0, 0, 0, 0));
    
    while (!isAtEnd()) {
        try {
            std::shared_ptr<AstNode> declaration;
            Token currentToken = peek();
            
            // Handle both proper token types and identifiers that should be keywords
            if (match(TokenType::NAMESPACE) || (check(TokenType::IDENTIFIER) && peek().lexeme == "namespace" && (advance(), true))) {
                declaration = parseNamespace();
            } 
            else if (match(TokenType::CLASS) || (check(TokenType::IDENTIFIER) && peek().lexeme == "class" && (advance(), true))) {
                declaration = parseClass();
            } 
            else if (match(TokenType::STRUCT) || (check(TokenType::IDENTIFIER) && peek().lexeme == "struct" && (advance(), true))) {
                declaration = parseStruct();
            } 
            else if (match(TokenType::TYPEDEF) || (check(TokenType::IDENTIFIER) && peek().lexeme == "typedef" && (advance(), true))) {
                declaration = parseTypedef();
            } 
            else if (match(TokenType::UNION) || (check(TokenType::IDENTIFIER) && peek().lexeme == "union" && (advance(), true))) {
                declaration = parseUnion();
            }
            else if (match(TokenType::FUNCTION) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "function" && (advance(), true))) {
                // Consume any type tokens if present (for return type)
                std::shared_ptr<Type> returnType = nullptr;
                if (match(TokenType::INT) || match(TokenType::FLOAT) || 
                    match(TokenType::VOID) || match(TokenType::BOOL) || 
                    match(TokenType::STRING) || 
                    check(TokenType::IDENTIFIER)) {
                    // If it's an identifier, consume it
                    if (check(TokenType::IDENTIFIER)) {
                        advance();
                    }
                    
                    // Optional bit width specification for int/float
                    if (match(TokenType::LEFT_BRACE)) {
                        Token bitWidthToken = consume(TokenType::INT_LITERAL, "Expected: bit width");
                        consume(TokenType::RIGHT_BRACE, "Expected: '}'");
                    }
                }
                
                //Token nameToken = consume(TokenType::IDENTIFIER, "Expect function name");
                
                // Parse function, using previously determined return type
                declaration = parseFunction(returnType);
            }
            else if (match(TokenType::IMPORT) || (check(TokenType::IDENTIFIER) && peek().lexeme == "import" && (advance(), true))) {
                declaration = parseImport();
            } 
            else if (match(TokenType::OBJECT) || (check(TokenType::IDENTIFIER) && peek().lexeme == "object" && (advance(), true))) {
                declaration = parseObject();
            } 
            else if (match(TokenType::CONST) || (check(TokenType::IDENTIFIER) && peek().lexeme == "const" && (advance(), true))) {
                error("Const declarations not yet implemented");
                synchronize();
                continue;
            }
            else if (match({TokenType::INT, TokenType::FLOAT, TokenType::VOID, TokenType::BOOL, TokenType::STRING}) || 
                (check(TokenType::IDENTIFIER) && 
                    (trimLexeme(peek().lexeme) == "int" || 
                     trimLexeme(peek().lexeme) == "float" || 
                     trimLexeme(peek().lexeme) == "void" || 
                     trimLexeme(peek().lexeme) == "bool" || 
                     trimLexeme(peek().lexeme) == "string"))) {
                
                // Consume type token if it was an identifier
                if (check(TokenType::IDENTIFIER)) {
                    advance();
                }
                
                // Optional bit-width specification
                if (match(TokenType::LEFT_BRACE)) {
                    consume(TokenType::INT_LITERAL, "Expected: bit width");
                    consume(TokenType::RIGHT_BRACE, "Expected: '}'");
                }
                
                // Ensure we have an identifier for function/variable name
                if (check(TokenType::IDENTIFIER)) {
                    Token nameToken = advance();
                    
                    // Check for function declaration
                    if (check(TokenType::LEFT_PAREN)) {
                        // Roll back to the name token for function parsing
                        current = current - 1;
                        declaration = parseFunction();
                    }
                    else {
                        // Potential global variable
                        std::shared_ptr<Expression> initializer = nullptr;
                        if (match(TokenType::EQUAL)) {
                            initializer = parseExpression();
                        }
                        
                        consume(TokenType::SEMICOLON, "Expected: ';'");
                        
                        // Note: You might want to create a method to process global variables
                        declaration = processGlobalVariable(
                            trimLexeme(nameToken.lexeme), 
                            // You'll need to create the type based on the previous context
                            typeRegistry->getIntType(), // Placeholder
                            initializer,
                            makeLocation(nameToken, previous())
                        );
                    }
                }
                else {
                    error("Expected: identifier");
                    synchronize();
                    continue;
                }
            }
            else if (match(TokenType::IDENTIFIER)) {
                // This could be a variable declaration, function declaration, or other type
                Token nameToken = previous();
                
                // Try to parse as a function
                if (check(TokenType::IDENTIFIER)) {
                    Token returnTypeToken = nameToken;
                    Token funcNameToken = peek();
                    
                    // Check if this looks like a function declaration
                    size_t lookAhead = current;
                    while (lookAhead < tokens.size() && tokens[lookAhead].type != TokenType::LEFT_PAREN && 
                           tokens[lookAhead].type != TokenType::SEMICOLON && 
                           tokens[lookAhead].type != TokenType::END_OF_FILE) {
                        lookAhead++;
                    }
                    
                    if (lookAhead < tokens.size() && tokens[lookAhead].type == TokenType::LEFT_PAREN) {
                        // This is likely a function declaration
                        current--; // Go back to type token
                        declaration = parseFunction();
                    } else {
                        // This is likely a variable declaration
                        // Handle initializer if present
                        Token typeToken = nameToken;
                        Token varNameToken = advance();
                        
                        std::shared_ptr<Type> type = std::make_shared<PrimitiveType>(Type::Kind::INT); // Simplified
                        
                        std::shared_ptr<Expression> initializer = nullptr;
                        if (match(TokenType::EQUAL)) {
                            initializer = parseExpression();
                        }
                        
                        consume(TokenType::SEMICOLON, "Expected: ';'");
                        
                        // Create variable declaration node
                        auto varDecl = std::make_shared<VariableDeclaration>(
                            trimLexeme(varNameToken.lexeme),
                            type,
                            initializer,
                            makeLocation(typeToken, previous())
                        );
                        
                        declaration = varDecl;
                    }
                } else {
                    error("Expected: declaration");
                    synchronize();
                    continue;
                }
            }
            else if (match(TokenType::PRINT)) {
                declaration = parseStatement();
            }
            else if (match(TokenType::SEMICOLON)) {
                // Skip standalone semicolons at global scope
                continue;
            }
            else {
                error("[parseProgram()] Expected: declaration");
                synchronize();
                continue;
            }
            
            if (declaration) {
                program->addDeclaration(declaration);
            }
        } catch (const std::exception& e) {
            synchronize();
        }
    }
    
    return program;
}

std::shared_ptr<NamespaceDeclaration> Parser::parseNamespace() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    auto namespaceDecl = std::make_shared<NamespaceDeclaration>(
        trimLexeme(nameToken.lexeme), 
        makeLocation(nameToken, nameToken)
    );
    
    enterNamespace(trimLexeme(nameToken.lexeme));
    
    consume(TokenType::LEFT_BRACE, "Expected: '{'");
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::CLASS)) {
            auto classDecl = parseClass();
            namespaceDecl->addClass(classDecl);
        } 
        else if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "class") {
            advance(); // Consume the "class" token
            auto classDecl = parseClass();
            namespaceDecl->addClass(classDecl);
        }
        else if (match(TokenType::SEMICOLON)) {
            // Skip any standalone semicolons inside namespace
            continue;
        } else {
            error("Only classes are allowed in namespaces");
            synchronize();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    exitNamespace();
    
    return namespaceDecl;
}

std::shared_ptr<ClassDeclaration> Parser::parseClass() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    auto classDecl = std::make_shared<ClassDeclaration>(
        trimLexeme(nameToken.lexeme), 
        makeLocation(nameToken, nameToken)
    );
    
    enterClass(trimLexeme(nameToken.lexeme));
    
    consume(TokenType::LEFT_BRACE, "Expected: '{'");
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        // Handle semicolons inside class definition
        if (match(TokenType::SEMICOLON)) {
            continue;
        } 
        // Handle object declarations
        else if (match(TokenType::OBJECT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "object")) {
            if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "object") {
                advance(); // Consume the object token if it was an identifier
            }
            
            Token objectNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            consume(TokenType::LEFT_BRACE, "Expected: '{'");
            
            auto objType = std::make_shared<ObjectType>(trimLexeme(objectNameToken.lexeme));
            classDecl->addObject(objType);
            
            // Parse the object body
            int braceCount = 1;
            while (braceCount > 0 && !isAtEnd()) {
                if (match(TokenType::LEFT_BRACE)) {
                    braceCount++;
                } else if (match(TokenType::RIGHT_BRACE)) {
                    braceCount--;
                    if (braceCount == 0) break;
                } else {
                    advance();
                }
            }
            
            // Handle optional semicolon after object block
            match(TokenType::SEMICOLON);
        }
        // Handle struct declarations inside class
        else if (match(TokenType::STRUCT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "struct")) {
            if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "struct") {
                advance(); // Consume the struct token if it was an identifier
            }
            
            // Check if this is an anonymous struct or a named one
            if (match(TokenType::LEFT_BRACE)) {
                // Anonymous struct - parse struct fields
                int braceCount = 1;
                while (braceCount > 0 && !isAtEnd()) {
                    if (match(TokenType::LEFT_BRACE)) {
                        braceCount++;
                    } else if (match(TokenType::RIGHT_BRACE)) {
                        braceCount--;
                        if (braceCount == 0) break;
                    } else {
                        advance();
                    }
                }
                
                // Check if there's a field name after the closing brace
                if (check(TokenType::IDENTIFIER)) {
                    Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
                    auto structType = std::make_shared<StructType>("anonymous");
                    classDecl->addField(StructField(trimLexeme(fieldNameToken.lexeme), structType));
                    consume(TokenType::SEMICOLON, "Expected: ';'");
                } else {
                    error("Expecteded: field name after anonymous struct");
                    synchronize();
                }
            } else {
                // Named struct
                Token structNameToken = previous();
                if (check(TokenType::IDENTIFIER)) {
                    structNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
                }
                
                // Now expect a LEFT_BRACE
                consume(TokenType::LEFT_BRACE, "Expected: '{'");
                
                // Skip through the struct body
                int braceCount = 1;
                while (braceCount > 0 && !isAtEnd()) {
                    if (match(TokenType::LEFT_BRACE)) {
                        braceCount++;
                    } else if (match(TokenType::RIGHT_BRACE)) {
                        braceCount--;
                        if (braceCount == 0) break;
                    } else {
                        advance();
                    }
                }
                
                // Check if there's a field name after the closing brace
                if (check(TokenType::IDENTIFIER)) {
                    Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
                    auto structType = std::make_shared<StructType>(
                        structNameToken.type == TokenType::IDENTIFIER ? trimLexeme(structNameToken.lexeme) : "anonymous");
                    classDecl->addField(StructField(trimLexeme(fieldNameToken.lexeme), structType));
                    consume(TokenType::SEMICOLON, "Expected: ';'");
                } else {
                    // It's a struct type definition without a field
                    consume(TokenType::SEMICOLON, "Expected: ';'");
                }
            }
        }
        // Handle union declarations similarly to structs
        else if (match(TokenType::UNION) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "union")) {
            if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "union") {
                advance(); // Consume the union token if it was an identifier
            }
            
            // Parse the union similar to struct
            if (match(TokenType::LEFT_BRACE)) {
                // Skip through the union body
                int braceCount = 1;
                while (braceCount > 0 && !isAtEnd()) {
                    if (match(TokenType::LEFT_BRACE)) {
                        braceCount++;
                    } else if (match(TokenType::RIGHT_BRACE)) {
                        braceCount--;
                        if (braceCount == 0) break;
                    } else {
                        advance();
                    }
                }
                
                // Check if there's a field name after the closing brace
                if (check(TokenType::IDENTIFIER)) {
                    Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
                    auto unionType = std::make_shared<UnionType>("anonymous");
                    classDecl->addField(StructField(trimLexeme(fieldNameToken.lexeme), unionType));
                    consume(TokenType::SEMICOLON, "Expected: ';'");
                } else {
                    error("Expecteded: identifier");
                    synchronize();
                }
            }
        }
        // Handle field declarations
        else if (check(TokenType::IDENTIFIER)) {
            Token typeToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            auto fieldType = std::make_shared<PrimitiveType>(Type::Kind::INT); // Simplified type
            classDecl->addField(StructField(trimLexeme(fieldNameToken.lexeme), fieldType));
            
            consume(TokenType::SEMICOLON, "Expected: ';'");
        }
        else {
            error("Unexpected token in class");
            synchronize();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    exitClass();
    
    return classDecl;
}

std::shared_ptr<StructDeclaration> Parser::parseStruct() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    std::vector<StructField> fields;
    
    if (match(TokenType::LEFT_BRACE)) {
        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
            auto fieldType = parseType();
            
            Token fieldNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            fields.emplace_back(std::string(fieldNameToken.lexeme), fieldType);
            
            consume(TokenType::SEMICOLON, "Expected: ';'");
        }
        
        consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    }
    
    return std::make_shared<StructDeclaration>(
        std::string(nameToken.lexeme), 
        fields, 
        makeLocation(nameToken, previous())
    );
}

std::shared_ptr<ObjectDeclaration> Parser::parseObject() {
    Token nameToken = previous(); // The object name was already consumed
    
    auto objectDecl = std::make_shared<ObjectDeclaration>(
        trimLexeme(nameToken.lexeme), 
        makeLocation(previous(), nameToken)
    );
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::SEMICOLON)) {
            // Skip standalone semicolons
            continue;
        }
        // Handle any type name (primitive or custom)
        else if (match(TokenType::VOID) || match(TokenType::INT) || match(TokenType::FLOAT) || 
                check(TokenType::IDENTIFIER)) {
            
            // Capture the type token
            Token typeToken = previous();
            
            // If we're looking at an identifier, consume it as a type name
            if (check(TokenType::IDENTIFIER)) {
                typeToken = advance(); // Consume the type name
            }
            
            // Handle bit width specification for int/float
            int bitWidth = 32;
            if ((trimLexeme(typeToken.lexeme) == "int" || trimLexeme(typeToken.lexeme) == "float") && 
                check(TokenType::LEFT_BRACE)) {
                consume(TokenType::LEFT_BRACE, "Expected: '{'");
                Token bitWidthToken = consume(TokenType::INT_LITERAL, "Expected: integer bit width");
                bitWidth = static_cast<int>(bitWidthToken.intValue);
                consume(TokenType::RIGHT_BRACE, "Expected: '}'");
            }
            
            // Create the appropriate type
            std::shared_ptr<Type> returnType;
            if (trimLexeme(typeToken.lexeme) == "int") {
                returnType = std::make_shared<PrimitiveType>(Type::Kind::INT, bitWidth);
            } else if (trimLexeme(typeToken.lexeme) == "float") {
                returnType = std::make_shared<PrimitiveType>(Type::Kind::FLOAT, bitWidth);
            } else if (trimLexeme(typeToken.lexeme) == "void") {
                returnType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
            } else {
                // Custom type (assume it exists)
                returnType = typeRegistry->getType(trimLexeme(typeToken.lexeme));
                if (!returnType) {
                    returnType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
                }
            }
            
            // Parse function or field name
            Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            // Function declaration - check for opening parenthesis
            if (match(TokenType::LEFT_PAREN)) {
                // Parse parameters
                std::vector<Parameter> parameters;
                
                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        // Parameter type
                        std::shared_ptr<Type> paramType;
                        
                        if (match(TokenType::INT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "int")) {
                            if (check(TokenType::IDENTIFIER)) advance(); // Consume 'int'
                            paramType = std::make_shared<PrimitiveType>(Type::Kind::INT);
                        } else if (match(TokenType::FLOAT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "float")) {
                            if (check(TokenType::IDENTIFIER)) advance(); // Consume 'float'
                            paramType = std::make_shared<PrimitiveType>(Type::Kind::FLOAT);
                        } else if (match(TokenType::VOID) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "void")) {
                            if (check(TokenType::IDENTIFIER)) advance(); // Consume 'void'
                            paramType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
                        } else if (match(TokenType::IDENTIFIER)) {
                            // Custom type
                            paramType = typeRegistry->getType(trimLexeme(previous().lexeme));
                            if (!paramType) {
                                paramType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
                            }
                        } else {
                            error("Expected: datatype");
                            paramType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
                        }
                        
                        // Check for pointer
                        if (match(TokenType::STAR)) {
                            paramType = std::make_shared<PointerType>(paramType);
                        }
                        
                        // Parameter name
                        Token paramNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
                        
                        parameters.emplace_back(trimLexeme(paramNameToken.lexeme), paramType);
                    } while (match(TokenType::COMMA));
                }
                
                consume(TokenType::RIGHT_PAREN, "Expected: ')'");
                
                // Parse function body
                consume(TokenType::LEFT_BRACE, "Expected: '{'");
                
                // Skip function body content
                int braceCount = 1;
                while (braceCount > 0 && !isAtEnd()) {
                    if (match(TokenType::LEFT_BRACE)) {
                        braceCount++;
                    } else if (match(TokenType::RIGHT_BRACE)) {
                        braceCount--;
                    } else {
                        advance();
                    }
                }
                
                // Handle optional semicolon after function body
                match(TokenType::SEMICOLON);
                
                // Add the method to the object
                auto methodDecl = std::make_shared<FunctionDeclaration>(
                    trimLexeme(nameToken.lexeme),
                    returnType,
                    parameters,
                    nullptr, // No body for now
                    makeLocation(typeToken, previous())
                );
                
                objectDecl->addMethod(methodDecl);
            }
            // Field declaration
            else {
                auto fieldType = returnType;
                objectDecl->addField(StructField(trimLexeme(nameToken.lexeme), fieldType));
                
                consume(TokenType::SEMICOLON, "Expected: ';'");
            }
        }
        else {
            error("Unexpected token in object declaration");
            synchronize();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    
    // Handle optional semicolon after object closing brace
    match(TokenType::SEMICOLON);
    
    return objectDecl;
}

std::shared_ptr<FunctionDeclaration> Parser::parseFunction(std::shared_ptr<Type> optionalReturnType) {    
    Token nameToken = advance();
    
    auto returnType = parseType();
    
    consume(TokenType::LEFT_PAREN, "Expected: '('");
    std::vector<Parameter> parameters = parseParameterList();
    consume(TokenType::RIGHT_PAREN, "Expected: ')'");
    
    auto functionDecl = std::make_shared<FunctionDeclaration>(
        std::string(nameToken.lexeme), 
        returnType,
        parameters,
        nullptr,
        makeLocation(previous(), nameToken)
    );
    
    consume(TokenType::LEFT_BRACE, "Expected: '{'");
    
    beginScope();
    
    for (const auto& param : parameters) {
        SymbolTable::Symbol symbol;
        symbol.name = param.name;
        symbol.kind = SymbolTable::SymbolKind::PARAMETER;
        symbol.type = param.type;
        symbol.isDefined = true;
        currentScope->define(symbol);
    }
    
    auto body = parseBlock();
    functionDecl->setBody(body);
    
    endScope();
    
    consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return functionDecl;
}

std::shared_ptr<ImportDeclaration> Parser::parseImport() {
    Token pathToken = consume(TokenType::STRING_LITERAL, "Expect import path as string");
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    // Read file, if access fails give err,
    // else replace the import line with the import source.
    
    return std::make_shared<ImportDeclaration>(
        std::string(pathToken.lexeme), 
        makeLocation(pathToken, previous())
    );
}

std::shared_ptr<UnionDeclaration> Parser::parseUnion() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    std::vector<StructField> variants;
    
    if (match(TokenType::LEFT_BRACE)) {
        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
            auto variantType = parseType();
            
            Token variantNameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            variants.emplace_back(std::string(variantNameToken.lexeme), variantType);
            
            consume(TokenType::SEMICOLON, "Expected: ';'");
        }
        
        consume(TokenType::RIGHT_BRACE, "Expected: '}'");
        consume(TokenType::SEMICOLON, "Expected ';'");
    }
    
    return std::make_shared<UnionDeclaration>(
        std::string(nameToken.lexeme), 
        variants, 
        makeLocation(nameToken, previous())
    );
}

std::shared_ptr<TypedefDeclaration> Parser::parseTypedef() {
    // The TYPEDEF token was already consumed in parseProgram
    
    // Check for unsigned/signed modifiers
    bool isUnsigned = false;
    if (match(TokenType::UNSIGNED) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "unsigned")) {
        isUnsigned = true;
        // If "unsigned" was an identifier, we need to consume it
        if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "unsigned") {
            advance();
        }
    }
    
    // Get the base type (int, float, etc.)
    std::shared_ptr<Type> baseType;
    
    if (match(TokenType::INT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "int")) {
        // If "int" was an identifier, consume it
        if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "int") {
            advance();
        }
        
        // Check for bit width specification
        int bitWidth = 32; // Default
        if (match(TokenType::LEFT_BRACE)) {
            if (match(TokenType::INT_LITERAL)) {
                bitWidth = static_cast<int>(previous().intValue);
            }
            consume(TokenType::RIGHT_BRACE, "Expected: '}' after bit width");
        }
        
        baseType = std::make_shared<PrimitiveType>(Type::Kind::INT, bitWidth, isUnsigned);
    }
    else if (match(TokenType::FLOAT) || (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "float")) {
        // If "float" was an identifier, consume it
        if (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "float") {
            advance();
        }
        
        // Check for bit width specification
        int bitWidth = 32; // Default
        if (match(TokenType::LEFT_BRACE)) {
            if (match(TokenType::INT_LITERAL)) {
                bitWidth = static_cast<int>(previous().intValue);
            }
            consume(TokenType::RIGHT_BRACE, "Expected: '}' after bit width");
        }
        
        baseType = std::make_shared<PrimitiveType>(Type::Kind::FLOAT, bitWidth);
    }
    else {
        error("Expected type name in typedef");
        
        // Create a default type to continue parsing
        baseType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
    }
    
    // Get the typedef name
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<TypedefDeclaration>(
        trimLexeme(nameToken.lexeme),
        baseType,
        makeLocation(previous(), nameToken)
    );
}

// Parse statements
std::shared_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::LEFT_BRACE)) return parseBlock();
    if (match(TokenType::IF)) return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    if (match(TokenType::FOR)) return parseForStatement();
    if (match(TokenType::RETURN)) return parseReturnStatement();
    if (match(TokenType::BREAK)) return parseBreakStatement();
    if (match(TokenType::CONTINUE)) return parseContinueStatement();
    if (match(TokenType::THROW)) return parseThrowStatement();
    if (match(TokenType::TRY)) return parseTryCatchStatement();
    if (match(TokenType::ASM)) return parseAsmStatement();
    if (match(TokenType::PRINT)) return parsePrintStatement();
    
    return parseExpressionStatement();
}

bool Parser::isInFunctionOrClassContext() const {
	if (!currentFunction.empty())
	{
		return true;
	}
	
	if (!currentClass.empty())
	{
		return true;
	}
	
	return false;
}

std::shared_ptr<BlockStatement> Parser::parseBlock() {
    Token startToken = previous();
    auto block = std::make_shared<BlockStatement>(
        makeLocation(startToken, Token(TokenType::RIGHT_BRACE, "}", startToken.line, startToken.column))
    );
    
    bool isAnonymousBlock = true;
    bool hasDeclarations = false;    
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
    {
        if (match(TokenType::LEFT_BRACE))
        {
            auto nestedBlock = parseBlock();
            if (nestedBlock)
            {
                block->addStatement(nestedBlock);
                
                if (!hasDeclarations)
                {
                    consume(TokenType::SEMICOLON, "Expected: ';'");
                }
            }
        }
        else
        {
            auto statement = parseStatement();
            if (statement)
            {
                block->addStatement(statement);
                hasDeclarations = true;
                isAnonymousBlock = false;
            }
        }
        
        auto statement = parseStatement();
        if (statement) {
            block->addStatement(statement);
        }
    }
    
    Token endToken = consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    
    if (isAnonymousBlock && !hasDeclarations && !isInFunctionOrClassContext())
    {
    	consume(TokenType::SEMICOLON, "Expected: ';'");
    }
    
    return block;
}

std::shared_ptr<ExpressionStatement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<ExpressionStatement>(
        expr,
        makeLocation(Token(TokenType::SEMICOLON, ";", expr->getLocation().startLine, expr->getLocation().startColumn), previous())
    );
}

std::shared_ptr<IfStatement> Parser::parseIfStatement() {
    consume(TokenType::LEFT_PAREN, "Expected: '('");
    
    auto condition = parseExpression();
    
    consume(TokenType::RIGHT_PAREN, "Expected: ')'");
    
    auto thenBranch = parseStatement();
    
    std::shared_ptr<Statement> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = parseStatement();
    }
    
    return std::make_shared<IfStatement>(
        condition,
        thenBranch,
        elseBranch,
        makeLocation(Token(TokenType::IF, "if", thenBranch->getLocation().startLine, thenBranch->getLocation().startColumn),
                     elseBranch ? Token(TokenType::ELSE, "else", elseBranch->getLocation().endLine, elseBranch->getLocation().endColumn) : previous())
    );
}

std::shared_ptr<WhileStatement> Parser::parseWhileStatement() {
    consume(TokenType::LEFT_PAREN, "Expected: '('");
    
    auto condition = parseExpression();
    
    consume(TokenType::RIGHT_PAREN, "Expected: ')'");
    
    auto body = parseStatement();
    
    return std::make_shared<WhileStatement>(
        condition,
        body,
        makeLocation(Token(TokenType::WHILE, "while", body->getLocation().startLine, body->getLocation().startColumn), previous())
    );
}

std::shared_ptr<ForStatement> Parser::parseForStatement() {
    consume(TokenType::LEFT_PAREN, "Expected: '('");
    
    std::shared_ptr<Statement> initializer = nullptr;
    if (match(TokenType::SEMICOLON)) {
        // No initializer
    } else {
        initializer = parseExpressionStatement();
    }
    
    std::shared_ptr<Expression> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    std::shared_ptr<Expression> increment = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        increment = parseExpression();
    }
    
    consume(TokenType::RIGHT_PAREN, "Expected: ')'");
    
    auto body = parseStatement();
    
    return std::make_shared<ForStatement>(
        initializer,
        condition,
        increment,
        body,
        makeLocation(Token(TokenType::FOR, "for", body->getLocation().startLine, body->getLocation().startColumn), previous())
    );
}

std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    Token returnToken = previous();
    
    std::shared_ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<ReturnStatement>(
        value, 
        makeLocation(returnToken, previous())
    );
}

std::shared_ptr<BreakStatement> Parser::parseBreakStatement() {
    Token breakToken = previous();
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<BreakStatement>(
        makeLocation(breakToken, previous())
    );
}

std::shared_ptr<ContinueStatement> Parser::parseContinueStatement() {
    Token continueToken = previous();
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<ContinueStatement>(
        makeLocation(continueToken, previous())
    );
}

std::shared_ptr<ThrowStatement> Parser::parseThrowStatement() {
    auto exception = parseExpression();
    
    std::shared_ptr<Statement> handler = nullptr;
    if (match(TokenType::LEFT_BRACE)) {
        handler = parseBlock();
    }
    
    consume(TokenType::SEMICOLON, "Expected: ';'");
    
    return std::make_shared<ThrowStatement>(
        exception,
        handler,
        makeLocation(Token(TokenType::THROW, "throw", exception->getLocation().startLine, exception->getLocation().startColumn), previous())
    );
}

std::shared_ptr<TryCatchStatement> Parser::parseTryCatchStatement() {
    consume(TokenType::LEFT_BRACE, "Expected: '{'");
    auto tryBlock = parseBlock();
    
    consume(TokenType::CATCH, "Expected: catch after try block");
    
    consume(TokenType::LEFT_PAREN, "Expected: '('");
    
    Token exceptionVarToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
    
    consume(TokenType::RIGHT_PAREN, "Expect: ')'");
    
    consume(TokenType::LEFT_BRACE, "Expect: '{'");
    auto catchBlock = parseBlock();
    
    return std::make_shared<TryCatchStatement>(
        tryBlock,
        catchBlock,
        std::string(exceptionVarToken.lexeme),
        makeLocation(Token(TokenType::TRY, "try", tryBlock->getLocation().startLine, tryBlock->getLocation().startColumn),
                     Token(TokenType::CATCH, "catch", catchBlock->getLocation().endLine, catchBlock->getLocation().endColumn))
    );
}

std::shared_ptr<AsmStatement> Parser::parseAsmStatement() {
    Token asmToken = previous();
    
    consume(TokenType::LEFT_BRACE, "Expected: '{'");
    
    std::string asmCode;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        asmCode += std::string(advance().lexeme) + " ";
    }
    
    Token endToken = consume(TokenType::RIGHT_BRACE, "Expected: '}'");
    
    return std::make_shared<AsmStatement>(
        asmCode, 
        makeLocation(asmToken, endToken)
    );
}

std::shared_ptr<Expression> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match({TokenType::EQUAL, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL, 
               TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL, TokenType::MODULO_EQUAL,
               TokenType::AND_EQUAL, TokenType::OR_EQUAL, TokenType::XOR_EQUAL})) {
        Token operatorToken = previous();
        auto value = parseExpression();

        // Check for valid assignment targets
        if (std::dynamic_pointer_cast<VariableExpression>(expr) || 
            std::dynamic_pointer_cast<MemberExpression>(expr) ||
            std::dynamic_pointer_cast<IndexExpression>(expr) ||
            std::dynamic_pointer_cast<ArrowExpression>(expr)) {
            
            // For simple assignment, return a unique assignment expression
            return std::make_shared<BinaryExpression>(
                [&]() {
                    switch (operatorToken.type) {
                        case TokenType::EQUAL: return BinaryOp::EQ;
                        case TokenType::PLUS_EQUAL: return BinaryOp::ADD;
                        case TokenType::MINUS_EQUAL: return BinaryOp::SUB;
                        case TokenType::STAR_EQUAL: return BinaryOp::MUL;
                        case TokenType::SLASH_EQUAL: return BinaryOp::DIV;
                        case TokenType::MODULO_EQUAL: return BinaryOp::MOD;
                        case TokenType::AND_EQUAL: return BinaryOp::BITAND;
                        case TokenType::OR_EQUAL: return BinaryOp::BITOR;
                        case TokenType::XOR_EQUAL: return BinaryOp::BITXOR;
                        default: return BinaryOp::EQ;
                    }
                }(),
                expr,
                value,
                makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                             value->getLocation().endLine, value->getLocation().endColumn)
            );
        } else {
            error("Invalid assignment target.");
            return expr;
        }
    }
    
    return expr;
}

// Parse expressions
std::shared_ptr<Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::shared_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match({TokenType::OR, TokenType::LOGICAL_OR})) {
        Token operatorToken = previous();
        auto right = parseLogicalAnd();
        
        BinaryOp op = operatorToken.type == TokenType::OR ? 
            BinaryOp::OR : BinaryOp::LOGICAL_OR;
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseBitwiseOr();
    
    while (match({TokenType::AND, TokenType::LOGICAL_AND})) {
        Token operatorToken = previous();
        auto right = parseBitwiseOr();
        
        BinaryOp op = operatorToken.type == TokenType::AND ? 
            BinaryOp::AND : BinaryOp::LOGICAL_AND;
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseBitwiseOr() {
    auto expr = parseBitwiseXor();
    
    while (match(TokenType::BIT_OR)) {
        auto right = parseBitwiseXor();
        
        expr = std::make_shared<BinaryExpression>(
            BinaryOp::BITOR, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseBitwiseXor() {
    auto expr = parseBitwiseAnd();
    
    while (match({TokenType::BIT_XOR, TokenType::XOR})) {
        auto right = parseBitwiseAnd();
        
        expr = std::make_shared<BinaryExpression>(
            BinaryOp::BITXOR, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseBitwiseAnd() {
    auto expr = parseEquality();
    
    while (match(TokenType::BIT_AND)) {
        auto right = parseEquality();
        
        expr = std::make_shared<BinaryExpression>(
            BinaryOp::BITAND, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL, TokenType::IS})) {
        Token operatorToken = previous();
        auto right = parseComparison();
        
        BinaryOp op;
        switch (operatorToken.type) {
            case TokenType::EQUAL_EQUAL: op = BinaryOp::EQ; break;
            case TokenType::BANG_EQUAL: op = BinaryOp::NE; break;
            case TokenType::IS: op = BinaryOp::EQ; break; // Simplified for now
            default: 
                error("Unexpected equality operator");
                op = BinaryOp::EQ;
        }
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}


std::shared_ptr<Expression> Parser::parseComparison() {
    auto expr = parseShift();
    
    while (match({TokenType::LESS, TokenType::LESS_EQUAL, 
                  TokenType::GREATER, TokenType::GREATER_EQUAL})) {
        Token operatorToken = previous();
        auto right = parseShift();
        
        BinaryOp op;
        switch (operatorToken.type) {
            case TokenType::LESS: op = BinaryOp::LT; break;
            case TokenType::LESS_EQUAL: op = BinaryOp::LE; break;
            case TokenType::GREATER: op = BinaryOp::GT; break;
            case TokenType::GREATER_EQUAL: op = BinaryOp::GE; break;
            default: 
                error("Unexpected comparison operator");
                op = BinaryOp::LT;
        }
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseShift() {
    auto expr = parseTerm();
    
    while (match({TokenType::LEFT_SHIFT, TokenType::RIGHT_SHIFT})) {
        Token operatorToken = previous();
        auto right = parseTerm();
        
        BinaryOp op = operatorToken.type == TokenType::LEFT_SHIFT ? 
            BinaryOp::SHIFTLEFT : BinaryOp::SHIFTRIGHT;
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token operatorToken = previous();
        auto right = parseFactor();
        
        BinaryOp op = operatorToken.type == TokenType::PLUS ? 
            BinaryOp::ADD : BinaryOp::SUB;
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseFactor() {
    auto expr = parseExponent();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::MODULO})) {
        Token operatorToken = previous();
        auto right = parseExponent();
        
        BinaryOp op;
        switch (operatorToken.type) {
            case TokenType::STAR: op = BinaryOp::MUL; break;
            case TokenType::SLASH: op = BinaryOp::DIV; break;
            case TokenType::MODULO: op = BinaryOp::MOD; break;
            default: 
                error("Unexpected factor operator");
                op = BinaryOp::MUL;
        }
        
        expr = std::make_shared<BinaryExpression>(
            op, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseExponent() {
    auto expr = parseUnary();
    
    while (match(TokenType::EXPONENT)) {
        auto right = parseUnary();
        
        expr = std::make_shared<BinaryExpression>(
            BinaryOp::EXPONENT, 
            expr, 
            right, 
            makeLocation(expr->getLocation().startLine, expr->getLocation().startColumn, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::BANG, TokenType::MINUS, TokenType::BIT_NOT, 
               TokenType::STAR, TokenType::ADDRESS_OF})) {
        Token operatorToken = previous();
        auto right = parseUnary();
        
        UnaryOp op;
        switch (operatorToken.type) {
            case TokenType::BANG: op = UnaryOp::NOT; break;
            case TokenType::MINUS: op = UnaryOp::NEGATE; break;
            case TokenType::BIT_NOT: op = UnaryOp::BITNOT; break;
            case TokenType::STAR: op = UnaryOp::DEREFERENCE; break;
            case TokenType::ADDRESS_OF: op = UnaryOp::ADDRESS_OF; break;
            default: 
                error("Unexpected unary operator");
                op = UnaryOp::NEGATE;
        }
        
        return std::make_shared<UnaryExpression>(
            op, 
            right, 
            makeLocation(operatorToken.line, operatorToken.column, 
                         right->getLocation().endLine, right->getLocation().endColumn)
        );
    }
    
    return parsePostfix();
}

std::shared_ptr<Expression> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall(expr);
        }
        else if (match(TokenType::LEFT_BRACKET)) {
            expr = finishIndexAccess(expr);
        }
        else if (match(TokenType::DOT)) {
            expr = finishMemberAccess(expr);
        }
        else if (match(TokenType::ARROW)) {
            expr = finishArrowAccess(expr);
        }
        else if (match({TokenType::INCREMENT, TokenType::DECREMENT})) {
            // Handle postfix increment/decrement
        }
        else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Statement> Parser::parsePrintStatement() {
    Token printToken = previous(); // PRINT token
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'print'");
    auto expr = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement");
    
    return std::make_shared<PrintStatement>(
        expr,
        makeLocation(printToken, previous())
    );
}

std::shared_ptr<Expression> Parser::parsePrimary() {
    // Literal parsing
    if (match({TokenType::INT_LITERAL, TokenType::FLOAT_LITERAL, 
               TokenType::STRING_LITERAL, TokenType::CHAR_LITERAL})) {
        Token literalToken = previous();
        std::shared_ptr<Type> type;
        LiteralExpression::LiteralValue value;
        
        switch (literalToken.type) {
            case TokenType::INT_LITERAL:
                type = typeRegistry->getIntType(32, false);
                value = static_cast<int>(literalToken.intValue);
                break;
            case TokenType::FLOAT_LITERAL:
                type = typeRegistry->getFloatType(32);
                value = literalToken.floatValue;
                break;
            case TokenType::STRING_LITERAL:
                type = typeRegistry->getStringType();
                value = std::string(literalToken.lexeme);
                break;
            case TokenType::CHAR_LITERAL:
                type = typeRegistry->getIntType(8, false);
                value = static_cast<int>(literalToken.lexeme[0]);
                break;
            default:
                type = typeRegistry->getVoidType();
                value = 0;
        }
        
        return std::make_shared<LiteralExpression>(
            value, 
            type,
            makeLocation(literalToken, literalToken)
        );
    }
    
    // Boolean literals
    if (match({TokenType::TRUE, TokenType::FALSE})) {
        Token boolToken = previous();
        return std::make_shared<LiteralExpression>(
            LiteralExpression::LiteralValue(boolToken.type == TokenType::TRUE),
            typeRegistry->getBoolType(),
            makeLocation(boolToken, boolToken)
        );
    }
    
    // Nullptr literal
    if (match(TokenType::NULLPTR)) {
        Token nullToken = previous();
        return std::make_shared<LiteralExpression>(
            LiteralExpression::LiteralValue(0),
            typeRegistry->getPointerType(typeRegistry->getVoidType()),
            makeLocation(nullToken, nullToken)
        );
    }
    
    // Identifier parsing
    if (match(TokenType::IDENTIFIER)) {
        Token identToken = previous();
        return std::make_shared<VariableExpression>(
            std::string(identToken.lexeme),
            makeLocation(identToken, identToken)
        );
    }
    
    // Parenthesized expression
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected: ')'");
        return expr;
    }
    
    // Array literal
    if (match(TokenType::LEFT_BRACKET)) {
        return parseArrayLiteral();
    }
    
    error("Expected: expression");
    return nullptr;
}

std::shared_ptr<Expression> Parser::parseArrayLiteral() {
    std::vector<std::shared_ptr<Expression>> elements;
    Token startToken = previous();
    
    if (!check(TokenType::RIGHT_BRACKET)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    Token endToken = consume(TokenType::RIGHT_BRACKET, "Expected: ']'");
    
    return std::make_shared<ArrayLiteralExpression>(
        elements,
        makeLocation(startToken, endToken)
    );
}

std::shared_ptr<Expression> Parser::finishCall(std::shared_ptr<Expression> callee) {
    std::vector<std::shared_ptr<Expression>> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    Token endToken = consume(TokenType::RIGHT_PAREN, "Expected: ')'");
    
    return std::make_shared<CallExpression>(
        callee,
        arguments,
        makeLocation(callee->getLocation().startLine, callee->getLocation().startColumn, 
                     endToken.line, endToken.column)
    );
}

std::shared_ptr<Expression> Parser::finishIndexAccess(std::shared_ptr<Expression> array) {
    auto index = parseExpression();
    Token endToken = consume(TokenType::RIGHT_BRACKET, "Expected: ']'");
    
    return std::make_shared<IndexExpression>(
        array,
        index,
        makeLocation(array->getLocation().startLine, array->getLocation().startColumn, 
                     endToken.line, endToken.column)
    );
}

std::shared_ptr<Expression> Parser::finishMemberAccess(std::shared_ptr<Expression> object) {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: member identifier");
    
    return std::make_shared<MemberExpression>(
        object,
        std::string(nameToken.lexeme),
        makeLocation(object->getLocation().startLine, object->getLocation().startColumn, 
                     nameToken.line, nameToken.column)
    );
}

std::shared_ptr<Expression> Parser::finishArrowAccess(std::shared_ptr<Expression> pointer) {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected: member identifier");
    
    return std::make_shared<ArrowExpression>(
        pointer,
        std::string(nameToken.lexeme),
        makeLocation(pointer->getLocation().startLine, pointer->getLocation().startColumn, 
                     nameToken.line, nameToken.column)
    );
}

std::shared_ptr<Type> Parser::parseType() {
    bool isUnsigned = false;
    int bitWidth = 32; // Default bit width
    
    // Optional unsigned modifier
    if (match(TokenType::UNSIGNED) || 
        (check(TokenType::IDENTIFIER) && trimLexeme(peek().lexeme) == "unsigned")) {
        if (check(TokenType::IDENTIFIER)) advance();
        isUnsigned = true;
    }
    
    std::shared_ptr<Type> baseType = nullptr;
    Token typeToken = (previous().type == TokenType::IDENTIFIER) ? previous() : peek();
    
    // Primitive type handling
    if (match({TokenType::INT, TokenType::FLOAT, TokenType::VOID, 
               TokenType::BOOL, TokenType::STRING}) || 
        (check(TokenType::IDENTIFIER) && 
         (trimLexeme(peek().lexeme) == "int" || 
          trimLexeme(peek().lexeme) == "float" || 
          trimLexeme(peek().lexeme) == "void" || 
          trimLexeme(peek().lexeme) == "bool" || 
          trimLexeme(peek().lexeme) == "string"))) {
        
        // Consume identifier type token if needed
        typeToken = (previous().type == TokenType::IDENTIFIER) ? previous() : peek();
        if (typeToken.type == TokenType::IDENTIFIER) advance();
        
        std::string typeName = trimLexeme(typeToken.lexeme);
        
        // Prevent bit-width for void, bool, and string
        if ((typeName == "void" || typeName == "bool" || typeName == "string")) {
            // Disallow bit-width specification
            if (match(TokenType::LEFT_BRACE)) {
                error("Cannot specify bit-width for " + typeName + " type");
                consume(TokenType::INT_LITERAL, "");
                consume(TokenType::RIGHT_BRACE, "");
            }
        }
        
        // Optional bit-width specification for int and float
        if ((typeName == "int" || typeName == "float") && 
            match(TokenType::LEFT_BRACE)) {
            Token bitWidthToken = consume(TokenType::INT_LITERAL, "Expected: bit width");
            bitWidth = static_cast<int>(bitWidthToken.intValue);
            consume(TokenType::RIGHT_BRACE, "Expected: '}'");
        }
        
        // Create appropriate type
        if (typeName == "int") {
            baseType = typeRegistry->getIntType(bitWidth, isUnsigned);
        }
        else if (typeName == "float") {
            baseType = typeRegistry->getFloatType(bitWidth);
        }
        else if (typeName == "void") {
            baseType = typeRegistry->getVoidType();
        }
        else if (typeName == "bool") {
            baseType = typeRegistry->getBoolType();
        }
        else if (typeName == "string") {
            baseType = typeRegistry->getStringType();
        }
    }
    else if (match(TokenType::IDENTIFIER)) {
        // Handle custom type names
        std::string customTypeName = std::string(previous().lexeme);
        baseType = typeRegistry->getType(customTypeName);
        
        if (!baseType) {
            error("Unknown type: " + customTypeName);
            baseType = std::make_shared<PrimitiveType>(Type::Kind::INT, 32);
        }
    }
    else {
        error("Expected: identifier");
        baseType = std::make_shared<PrimitiveType>(Type::Kind::VOID);
    }
    
    // Check for pointer
    if (match(TokenType::STAR)) {
        return std::make_shared<PointerType>(baseType);
    }
    
    return baseType;
}

int Parser::parseBitWidth() {
    if (!isAtEnd() && peek().type == TokenType::INT_LITERAL) {
        Token token = advance();
        return static_cast<int>(token.intValue);
    }
    
    error("Expected: bit width");
    return 32; // Default bit width
}

std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            auto type = parseType();
            Token nameToken = consume(TokenType::IDENTIFIER, "Expected: identifier");
            
            parameters.emplace_back(std::string(nameToken.lexeme), type);
        } while (match(TokenType::COMMA));
    }
    
    return parameters;
}

void Parser::beginScope() {
    currentScope = currentScope->createChildScope();
}

void Parser::endScope() {
    currentScope = currentScope->getParent();
}

void Parser::enterNamespace(const std::string& name) {
    currentNamespace = name;
    beginScope();
}

void Parser::exitNamespace() {
    currentNamespace = "";
    endScope();
}

void Parser::enterClass(const std::string& name) {
    currentClass = name;
    beginScope();
}

void Parser::exitClass() {
    currentClass = "";
    endScope();
}

std::shared_ptr<SymbolTable> Parser::getGlobalScope() const {
    std::shared_ptr<SymbolTable> scope = currentScope;
    
    while (scope->getParent() != nullptr) {
        scope = scope->getParent();
    }
    
    return scope;
}

// SymbolTable implementation
bool SymbolTable::define(const Symbol& symbol) {
    if (lookupLocal(symbol.name)) {
        return false; // Symbol already defined in this scope
    }
    
    symbols[symbol.name] = symbol;
    return true;
}

bool SymbolTable::declare(const Symbol& symbol) {
    if (lookupLocal(symbol.name)) {
        return false; // Symbol already declared in this scope
    }
    
    Symbol declaredSymbol = symbol;
    declaredSymbol.isDefined = false;
    
    symbols[symbol.name] = declaredSymbol;
    return true;
}

SymbolTable::Symbol* SymbolTable::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &it->second;
    }
    
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

SymbolTable::Symbol* SymbolTable::lookupLocal(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &it->second;
    }
    
    return nullptr;
}

std::shared_ptr<SymbolTable> SymbolTable::createChildScope() {
    return std::make_shared<SymbolTable>(shared_from_this());
}

std::vector<SymbolTable::Symbol> SymbolTable::getAllSymbols() const {
    std::vector<Symbol> result;
    
    for (const auto& pair : symbols) {
        result.push_back(pair.second);
    }
    
    return result;
}

void SymbolTable::clear() {
    symbols.clear();
}

// TypeRegistry implementation
TypeRegistry::TypeRegistry() {
    initializeBuiltinTypes();
}

void TypeRegistry::initializeBuiltinTypes() {
    // Register the basic types according to the Flux language spec
    registerType("void", std::make_shared<PrimitiveType>(Type::Kind::VOID));
    registerType("bool", std::make_shared<PrimitiveType>(Type::Kind::BOOL));
    registerType("int", std::make_shared<PrimitiveType>(Type::Kind::INT, 32, false));
    registerType("float", std::make_shared<PrimitiveType>(Type::Kind::FLOAT, 32));
    registerType("string", std::make_shared<PrimitiveType>(Type::Kind::STRING));
}

void TypeRegistry::registerType(const std::string& name, std::shared_ptr<Type> type) {
    types[name] = type;
}

void Parser::registerGlobalVariable(const std::shared_ptr<VariableDeclaration>& varDecl) {
    // Create a symbol for the variable
    SymbolTable::Symbol symbol;
    symbol.name = varDecl->getName();
    symbol.kind = SymbolTable::SymbolKind::GLOBAL_VARIABLE;
    symbol.type = varDecl->getType();
    symbol.isDefined = true;
    symbol.location = varDecl->getLocation();
    symbol.isConst = false; // Assuming non-const for now
    
    // Define it in the global scope
    currentScope->defineGlobal(symbol);
}

bool Parser::checkVariableInitialization(const std::shared_ptr<VariableDeclaration>& varDecl) {
    if (!varDecl->getInitializer()) {
        return true; // No initializer to check
    }
    
    auto varType = varDecl->getType();
    auto initType = inferType(varDecl->getInitializer());
    
    if (!initType) {
        error("Could not infer type of initializer");
        return false;
    }
    
    if (!checkTypes(varType, initType)) {
        error("Type mismatch in variable initialization");
        return false;
    }
    
    return true;
}

std::shared_ptr<VariableDeclaration> Parser::processGlobalVariable(
    const std::string& name, 
    std::shared_ptr<Type> type,
    std::shared_ptr<Expression> initializer,
    const AstLocation& location) {
    
    // Create the variable declaration
    auto varDecl = std::make_shared<VariableDeclaration>(
        name, type, initializer, location, true // isGlobal = true
    );
    
    // Register it in the symbol table
    registerGlobalVariable(varDecl);
    
    // Check type compatibility if there's an initializer
    checkVariableInitialization(varDecl);
    
    return varDecl;
}

std::shared_ptr<Type> TypeRegistry::getType(const std::string& name) {
    auto it = types.find(name);
    if (it != types.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool TypeRegistry::hasType(const std::string& name) const {
    return types.find(name) != types.end();
}

std::shared_ptr<Type> TypeRegistry::getVoidType() {
    return getType("void");
}

std::shared_ptr<Type> TypeRegistry::getBoolType() {
    return getType("bool");
}

std::shared_ptr<Type> TypeRegistry::getIntType(int bitWidth, bool isUnsigned) {
    // If requesting default int, return the cached one
    if (bitWidth == 32 && !isUnsigned) {
        return getType("int");
    }
    
    // Otherwise create a new PrimitiveType
    return std::make_shared<PrimitiveType>(Type::Kind::INT, bitWidth, isUnsigned);
}

std::shared_ptr<Type> TypeRegistry::getFloatType(int bitWidth) {
    // If requesting default float, return the cached one
    if (bitWidth == 32) {
        return getType("float");
    }
    
    // Otherwise create a new PrimitiveType
    return std::make_shared<PrimitiveType>(Type::Kind::FLOAT, bitWidth);
}

std::shared_ptr<Type> TypeRegistry::getStringType() {
    return getType("string");
}

std::shared_ptr<Type> TypeRegistry::getPointerType(std::shared_ptr<Type> pointeeType) {
    // Create a pointer type
    return std::make_shared<PointerType>(pointeeType);
}

std::string TypeRegistry::getTypeName(const std::shared_ptr<Type>& type) {
    return type->toString();
}

bool TypeRegistry::areTypesCompatible(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b) {
    return a->isEquivalentTo(b.get());
}

std::shared_ptr<Type> Parser::inferType(const std::shared_ptr<Expression>& expr) {
    if (!expr) return typeRegistry->getVoidType();

    // Try to get type directly from the expression
    auto existingType = expr->getType();
    if (existingType) return existingType;

    // Infer type based on expression type
    if (auto literalExpr = std::dynamic_pointer_cast<LiteralExpression>(expr)) {
        return literalExpr->getType();
    }
    else if (auto varExpr = std::dynamic_pointer_cast<VariableExpression>(expr)) {
        // Look up variable type in symbol table
        auto symbol = currentScope->lookup(varExpr->getName());
        if (symbol) {
            varExpr->setType(symbol->type);
            return symbol->type;
        }
    }
    else if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        // Infer type for binary expressions
        auto leftType = inferType(binaryExpr->getLeft());
        auto rightType = inferType(binaryExpr->getRight());
        
        // Determine result type based on operator and operand types
        auto commonType = typeRegistry->getCommonType(leftType, rightType);
        binaryExpr->setResultType(commonType);
        return commonType;
    }
    else if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
        // Infer type for unary expressions
        auto operandType = inferType(unaryExpr->getOperand());
        
        // Some unary operations might change type
        switch (unaryExpr->getOperator()) {
            case UnaryOp::NOT:
                return typeRegistry->getBoolType();
            case UnaryOp::NEGATE:
                return operandType;
            case UnaryOp::BITNOT:
                return operandType;
            case UnaryOp::DEREFERENCE:
                if (auto ptrType = std::dynamic_pointer_cast<PointerType>(operandType)) {
                    return ptrType->getPointeeType();
                }
                break;
            case UnaryOp::ADDRESS_OF:
                return typeRegistry->getPointerType(operandType);
            default:
                break;
        }
    }
    else if (auto callExpr = std::dynamic_pointer_cast<CallExpression>(expr)) {
        // Infer return type from function call
        auto calleeType = inferType(callExpr->getCallee());
        
        if (auto funcType = std::dynamic_pointer_cast<FunctionType>(calleeType)) {
            callExpr->setResultType(funcType->getReturnType());
            return funcType->getReturnType();
        }
    }
    else if (auto indexExpr = std::dynamic_pointer_cast<IndexExpression>(expr)) {
        // Infer type of array/vector indexing
        auto arrayType = inferType(indexExpr->getArray());
        
        if (auto ptrType = std::dynamic_pointer_cast<PointerType>(arrayType)) {
            indexExpr->setResultType(ptrType->getPointeeType());
            return ptrType->getPointeeType();
        }
    }
    else if (auto memberExpr = std::dynamic_pointer_cast<MemberExpression>(expr)) {
        // Infer type of struct/class member access
        auto objectType = inferType(memberExpr->getObject());
        
        if (auto structType = std::dynamic_pointer_cast<StructType>(objectType)) {
            for (const auto& field : structType->getFields()) {
                if (field.name == memberExpr->getMemberName()) {
                    memberExpr->setResultType(field.type);
                    return field.type;
                }
            }
        }
        else if (auto classType = std::dynamic_pointer_cast<ClassType>(objectType)) {
            for (const auto& field : classType->getFields()) {
                if (field.name == memberExpr->getMemberName()) {
                    memberExpr->setResultType(field.type);
                    return field.type;
                }
            }
        }
    }

    // Fallback to void type if no specific type can be inferred
    return typeRegistry->getVoidType();
}

bool Parser::checkTypes(const std::shared_ptr<Type>& expected, const std::shared_ptr<Type>& actual) {
    if (!expected || !actual) return false;

    // Exact type match
    if (expected->isEquivalentTo(actual.get())) return true;

    // Special handling for numeric types
    if (expected->getKind() == Type::Kind::INT && actual->getKind() == Type::Kind::INT) {
        auto expectedInt = std::dynamic_pointer_cast<PrimitiveType>(expected);
        auto actualInt = std::dynamic_pointer_cast<PrimitiveType>(actual);
        
        // Can assign smaller integer to larger one
        if (expectedInt->getBitWidth() >= actualInt->getBitWidth()) {
            return true;
        }
    }
    
    // Pointer type compatibility
    if (expected->getKind() == Type::Kind::POINTER) {
        if (actual->getKind() == Type::Kind::NULLPTR) return true;
        
        auto expectedPtr = std::dynamic_pointer_cast<PointerType>(expected);
        auto actualPtr = std::dynamic_pointer_cast<PointerType>(actual);
        
        if (actualPtr) {
            // Check pointee types
            return expectedPtr->getPointeeType()->isEquivalentTo(actualPtr->getPointeeType().get());
        }
    }
    
    // Float can be assigned to same or larger float
    if (expected->getKind() == Type::Kind::FLOAT && actual->getKind() == Type::Kind::FLOAT) {
        auto expectedFloat = std::dynamic_pointer_cast<PrimitiveType>(expected);
        auto actualFloat = std::dynamic_pointer_cast<PrimitiveType>(actual);
        
        if (expectedFloat->getBitWidth() >= actualFloat->getBitWidth()) {
            return true;
        }
    }
    
    // Void is compatible with everything 
    if (expected->getKind() == Type::Kind::VOID) return true;
    
    return false;
}

std::shared_ptr<Type> TypeRegistry::getCommonType(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b) {
    if (a->isEquivalentTo(b.get())) {
        return a;
    }
    
    // Handle numeric types
    if (a->getKind() == Type::Kind::INT && b->getKind() == Type::Kind::INT) {
        auto aInt = std::dynamic_pointer_cast<PrimitiveType>(a);
        auto bInt = std::dynamic_pointer_cast<PrimitiveType>(b);
        
        int maxBitWidth = std::max(aInt->getBitWidth(), bInt->getBitWidth());
        bool isUnsigned = aInt->getIsUnsigned() || bInt->getIsUnsigned();
        
        return getIntType(maxBitWidth, isUnsigned);
    }
    
    if ((a->getKind() == Type::Kind::INT && b->getKind() == Type::Kind::FLOAT) ||
        (a->getKind() == Type::Kind::FLOAT && b->getKind() == Type::Kind::INT)) {
        auto floatType = (a->getKind() == Type::Kind::FLOAT) ? a : b;
        return floatType;
    }
    
    if (a->getKind() == Type::Kind::FLOAT && b->getKind() == Type::Kind::FLOAT) {
        auto aFloat = std::dynamic_pointer_cast<PrimitiveType>(a);
        auto bFloat = std::dynamic_pointer_cast<PrimitiveType>(b);
        
        int maxBitWidth = std::max(aFloat->getBitWidth(), bFloat->getBitWidth());
        return getFloatType(maxBitWidth);
    }
    
    // Pointer type compatibility
    if (a->getKind() == Type::Kind::POINTER && b->getKind() == Type::Kind::NULLPTR) {
        return a;
    }
    
    if (a->getKind() == Type::Kind::NULLPTR && b->getKind() == Type::Kind::POINTER) {
        return b;
    }
    
    // No common type found
    return nullptr;
}

void TypeRegistry::clear() {
    types.clear();
    initializeBuiltinTypes();
}

} // namespace flux
