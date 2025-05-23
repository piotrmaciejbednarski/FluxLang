#include "parser.h"
#include <iostream>
#include <cassert>
#include <algorithm>

namespace flux {
namespace parser {

// Constructor with tokenizer
Parser::Parser(lexer::Tokenizer& tokenizer) 
    : tokenizer_(tokenizer), current_(lexer::Token()), previous_(lexer::Token()) {
    
    // Initialize synchronization points for error recovery
    syncPoints_[lexer::TokenType::KEYWORD_CLASS] = true;
    syncPoints_[lexer::TokenType::KEYWORD_OBJECT] = true;
    syncPoints_[lexer::TokenType::KEYWORD_DEF] = true;
    syncPoints_[lexer::TokenType::KEYWORD_NAMESPACE] = true;
    syncPoints_[lexer::TokenType::KEYWORD_STRUCT] = true;
    syncPoints_[lexer::TokenType::KEYWORD_ENUM] = true;
    syncPoints_[lexer::TokenType::KEYWORD_IMPORT] = true;
    syncPoints_[lexer::TokenType::KEYWORD_USING] = true;
    syncPoints_[lexer::TokenType::KEYWORD_RETURN] = true;
    syncPoints_[lexer::TokenType::KEYWORD_IF] = true;
    syncPoints_[lexer::TokenType::KEYWORD_WHILE] = true;
    syncPoints_[lexer::TokenType::KEYWORD_FOR] = true;
    syncPoints_[lexer::TokenType::KEYWORD_SWITCH] = true;
    syncPoints_[lexer::TokenType::KEYWORD_TRY] = true;
    syncPoints_[lexer::TokenType::SEMICOLON] = true;
    syncPoints_[lexer::TokenType::LEFT_BRACE] = true;
    syncPoints_[lexer::TokenType::RIGHT_BRACE] = true;
    
    // Get the first token
    advance();
}

// Parse a complete program
std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    
    try {
        resetParsingState();
        
        // Parse declarations until end of file
        while (!check(lexer::TokenType::END_OF_FILE)) {
            if (panicMode_) {
                synchronize();
                continue;
            }
            
            auto decl = declaration();
            if (decl) {
                program->addDeclaration(std::move(decl));
            }
        }
        
    } catch (const common::Error& e) {
        errors_.addError(e);
    }
    
    return program;
}

// Token handling methods
lexer::Token Parser::advance() {
    previous_ = current_;
    current_ = tokenizer_.nextToken();
    
    if (current_.type() == lexer::TokenType::ERROR) {
        error(current_.lexeme());
    }
    
    return previous_;
}

void Parser::advanceWithGuard(const char* context) {
    if (check(lexer::TokenType::END_OF_FILE)) {
        error("Unexpected end of file in " + std::string(context));
    }
    advance();
}

bool Parser::check(lexer::TokenType type) const {
    return current_.type() == type;
}

bool Parser::checkAny(std::initializer_list<lexer::TokenType> types) const {
    return current_.isOneOf(types);
}

bool Parser::checkNext(lexer::TokenType type) {
    lexer::Token next = tokenizer_.peekToken();
    return next.type() == type;
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    if (checkAny(types)) {
        advance();
        return true;
    }
    return false;
}

lexer::Token Parser::consume(lexer::TokenType type, std::string_view message) {
    if (check(type)) {
        return advance();
    }
    
    error(message);
    return errorToken(message);
}

void Parser::dumpNextToken() {
    std::cout << "Next token: " << current_.toString() << std::endl;
}

void Parser::resetParsingState() {
    panicMode_ = false;
    inFunctionBody_ = false;
    inObjectBody_ = false;
    inControlStructure_ = false;
    inTemplateDeclaration_ = false;
}

void Parser::synchronize() {
    panicMode_ = false;
    
    while (!check(lexer::TokenType::END_OF_FILE)) {
        if (previous_.type() == lexer::TokenType::SEMICOLON) {
            return;
        }
        
        if (syncPoints_[current_.type()]) {
            return;
        }
        
        advance();
    }
}

// Error reporting methods
void Parser::error(std::string_view message) {
    error(current_, message);
}

void Parser::error(const lexer::Token& token, std::string_view message) {
    if (panicMode_) return;
    
    panicMode_ = true;
    
    output::SourceLocation location(
        tokenizer_.source()->filename(),
        token.start().line,
        token.start().column,
        tokenizer_.source()->getLineText(token.start().line),
        token.start().column - 1,
        token.end().column - token.start().column
    );
    
    errors_.addError(common::ErrorCode::PARSER_ERROR, message, location);
}

lexer::Token Parser::errorToken(std::string_view message) {
    return lexer::Token(lexer::TokenType::ERROR, message, current_.start(), current_.end());
}

// Declaration parsing methods
std::unique_ptr<Decl> Parser::declaration() {
    try {
        // Handle section declarations
        if (match(lexer::TokenType::KEYWORD_SECTION)) {
            return sectionDeclaration();
        }
        
        // Handle template declarations
        if (match(lexer::TokenType::KEYWORD_TEMPLATE)) {
            return templateDeclaration();
        }
        
        // Handle ASM blocks
        if (match(lexer::TokenType::KEYWORD_ASM)) {
            return asmDeclaration();
        }
        
        // Handle import declarations
        if (match(lexer::TokenType::KEYWORD_IMPORT)) {
            return importDeclaration();
        }
        
        // Handle using declarations
        if (match(lexer::TokenType::KEYWORD_USING)) {
            return usingDeclaration();
        }
        
        // Handle namespace declarations
        if (match(lexer::TokenType::KEYWORD_NAMESPACE)) {
            return namespaceDeclaration();
        }
        
        // Handle class declarations
        if (match(lexer::TokenType::KEYWORD_CLASS)) {
            return classDeclaration();
        }
        
        // Handle object declarations
        if (match(lexer::TokenType::KEYWORD_OBJECT)) {
            return objectDeclaration();
        }
        
        // Handle struct declarations
        if (match(lexer::TokenType::KEYWORD_STRUCT)) {
            return structDeclaration();
        }
        
        // Handle enum declarations
        if (match(lexer::TokenType::KEYWORD_ENUM)) {
            return enumDeclaration();
        }
        
        // Handle operator declarations
        if (match(lexer::TokenType::KEYWORD_OPERATOR)) {
            return operatorDeclaration();
        }
        
        // Handle function declarations
        if (match(lexer::TokenType::KEYWORD_DEF)) {
            return functionDeclaration();
        }
        
        // Handle const/volatile variable declarations
        bool isConst = match(lexer::TokenType::KEYWORD_CONST);
        bool isVolatile = match(lexer::TokenType::KEYWORD_VOLATILE);
        
        if (isConst || isVolatile) {
            return variableDeclaration(isConst, isVolatile);
        }
        
        // Handle data type declarations
        if (checkAny({lexer::TokenType::KEYWORD_SIGNED, lexer::TokenType::KEYWORD_UNSIGNED, lexer::TokenType::KEYWORD_DATA})) {
            return dataDeclaration();
        }
        
        // Handle identifier-based declarations
        if (check(lexer::TokenType::IDENTIFIER)) {
            return identifierDeclaration();
        }
        
        error("Expected declaration");
        return nullptr;
        
    } catch (const common::Error& e) {
        synchronize();
        throw;
    }
}

std::unique_ptr<Decl> Parser::identifierDeclaration() {
    // Look ahead to determine declaration type
    lexer::Token next = tokenizer_.peekToken();
    
    // Type declaration (name: type)
    if (next.type() == lexer::TokenType::COLON) {
        lexer::Token name = advance();
        consume(lexer::TokenType::COLON, "Expected ':'");
        
        auto underlyingType = type();
        consume(lexer::TokenType::SEMICOLON, "Expected ';'");
        
        return std::make_unique<TypeDecl>(
            name.lexeme(),
            std::move(underlyingType),
            makeRange(name, previous_)
        );
    }
    
    // Variable declaration
    return variableDeclaration(false, false);
}

std::unique_ptr<Decl> Parser::namespaceDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected namespace name");
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<std::unique_ptr<Decl>> declarations;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        auto decl = declaration();
        if (decl) {
            declarations.push_back(std::move(decl));
        }
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<NamespaceDecl>(
        name.lexeme(),
        std::move(declarations),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::classDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected class name");
    
    std::vector<std::string_view> baseClasses;
    std::vector<std::string_view> exclusions;
    
    // Handle inheritance
    if (match(lexer::TokenType::LESS)) {
        baseClasses = parseInheritanceList();
        
        // Handle exclusions {!name}
        if (match(lexer::TokenType::LEFT_BRACE) && match(lexer::TokenType::EXCLAMATION)) {
            exclusions = parseExclusionList();
            consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
        }
    }
    
    // Check for forward declaration
    if (match(lexer::TokenType::SEMICOLON)) {
        return std::make_unique<ClassDecl>(
            name.lexeme(),
            std::move(baseClasses),
            std::move(exclusions),
            std::vector<std::unique_ptr<Decl>>(),
            makeRange(name, previous_),
            true  // isForwardDeclaration
        );
    }
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<std::unique_ptr<Decl>> members;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        auto member = declaration();
        if (member) {
            members.push_back(std::move(member));
        }
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ClassDecl>(
        name.lexeme(),
        std::move(baseClasses),
        std::move(exclusions),
        std::move(members),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::objectDeclaration() {
    // Check for template object
    if (match(lexer::TokenType::KEYWORD_TEMPLATE)) {
        return objectTemplateDeclaration();
    }
    
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected object name");
    
    std::vector<std::string_view> baseObjects;
    if (match(lexer::TokenType::LESS)) {
        baseObjects = parseInheritanceList();
    }
    
    // Check for forward declaration
    if (match(lexer::TokenType::SEMICOLON)) {
        return std::make_unique<ObjectDecl>(
            name.lexeme(),
            std::move(baseObjects),
            std::vector<std::unique_ptr<Decl>>(),
            makeRange(name, previous_),
            true  // isForwardDeclaration
        );
    }
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    bool oldInObjectBody = inObjectBody_;
    inObjectBody_ = true;
    
    std::vector<std::unique_ptr<Decl>> members = parseObjectMembers();
    
    inObjectBody_ = oldInObjectBody;
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ObjectDecl>(
        name.lexeme(),
        std::move(baseObjects),
        std::move(members),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::objectTemplateDeclaration() {
    consume(lexer::TokenType::LESS, "Expected '<'");
    
    std::vector<std::string_view> templateParams;
    do {
        lexer::Token param = consume(lexer::TokenType::IDENTIFIER, "Expected template parameter");
        templateParams.push_back(param.lexeme());
    } while (match(lexer::TokenType::COMMA));
    
    consume(lexer::TokenType::GREATER, "Expected '>'");
    
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected object name");
    
    std::vector<std::string_view> baseObjects;
    if (match(lexer::TokenType::LESS)) {
        baseObjects = parseInheritanceList();
    }
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    bool oldInObjectBody = inObjectBody_;
    inObjectBody_ = true;
    
    std::vector<std::unique_ptr<Decl>> members = parseObjectMembers();
    
    inObjectBody_ = oldInObjectBody;
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ObjectDecl>(
        name.lexeme(),
        std::move(baseObjects),
        std::move(members),
        makeRange(name, previous_),
        false,  // isForwardDeclaration
        true,   // isTemplate
        std::move(templateParams)
    );
}

std::unique_ptr<Decl> Parser::structDeclaration() {
    bool isPacked = false;
    std::optional<size_t> alignment;
    
    if (match(lexer::TokenType::KEYWORD_PACKED)) {
        isPacked = true;
    }
    
    if (match(lexer::TokenType::KEYWORD_ALIGN)) {
        alignment = parseAlignmentAttribute();
    }
    
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected struct name");
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<StructDecl::Field> fields;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        bool isVolatile = match(lexer::TokenType::KEYWORD_VOLATILE);
        
        auto fieldType = type();
        lexer::Token fieldName = consume(lexer::TokenType::IDENTIFIER, "Expected field name");
        
        std::optional<size_t> fieldAlignment;
        if (match(lexer::TokenType::KEYWORD_ALIGN)) {
            fieldAlignment = parseAlignmentAttribute();
        }
        
        consume(lexer::TokenType::SEMICOLON, "Expected ';'");
        
        fields.emplace_back(
            fieldName.lexeme(),
            std::move(fieldType),
            fieldAlignment,
            isVolatile
        );
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<StructDecl>(
        name.lexeme(),
        std::move(fields),
        makeRange(name, previous_),
        isPacked,
        alignment
    );
}

std::unique_ptr<Decl> Parser::functionDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected function name");
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    
    std::vector<FunctionDecl::Parameter> parameters;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            auto paramType = type();
            lexer::Token paramName = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
            parameters.emplace_back(paramName.lexeme(), std::move(paramType));
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    std::unique_ptr<TypeExpr> returnType;
    if (match(lexer::TokenType::ARROW)) {
        returnType = type();
    }
    
    // Check for function prototype
    if (match(lexer::TokenType::SEMICOLON)) {
        return std::make_unique<FunctionDecl>(
            name.lexeme(),
            std::move(parameters),
            std::move(returnType),
            nullptr,
            makeRange(name, previous_),
            true  // isPrototype
        );
    }
    
    bool oldInFunctionBody = inFunctionBody_;
    inFunctionBody_ = true;
    
    auto body = blockStatement();
    
    inFunctionBody_ = oldInFunctionBody;
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after function body");
    
    return std::make_unique<FunctionDecl>(
        name.lexeme(),
        std::move(parameters),
        std::move(returnType),
        std::move(body),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::variableDeclaration(bool isConst, bool isVolatile) {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected variable name");
    
    std::unique_ptr<TypeExpr> varType;
    std::unique_ptr<Expr> initializer;
    
    // Parse type annotation (name: type)
    if (match(lexer::TokenType::COLON)) {
        varType = type();
    }
    
    // Parse initializer (name = value)
    if (match(lexer::TokenType::EQUAL)) {
        initializer = expression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<VarDecl>(
        name.lexeme(),
        std::move(varType),
        std::move(initializer),
        isConst,
        isVolatile,
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::importDeclaration() {
    lexer::Token path = consume(lexer::TokenType::CHAR_LITERAL, "Expected import path");
    
    std::optional<std::string_view> alias;
    if (match(lexer::TokenType::KEYWORD_AS)) {
        lexer::Token aliasToken = consume(lexer::TokenType::IDENTIFIER, "Expected alias name");
        alias = aliasToken.lexeme();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ImportDecl>(
        path.stringValue(),
        alias,
        makeRange(path, previous_)
    );
}

std::unique_ptr<Decl> Parser::usingDeclaration() {
    std::vector<std::string_view> path;
    
    do {
        lexer::Token part = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
        path.push_back(part.lexeme());
    } while (match(lexer::TokenType::DOT));
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<UsingDecl>(
        std::move(path),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Decl> Parser::operatorDeclaration() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    
    std::vector<OperatorDecl::Parameter> parameters;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            auto paramType = type();
            lexer::Token paramName = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
            parameters.emplace_back(paramName.lexeme(), std::move(paramType));
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    consume(lexer::TokenType::LEFT_BRACKET, "Expected '['");
    
    lexer::Token opToken = advance(); // Get the operator symbol
    
    consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']'");
    consume(lexer::TokenType::ARROW, "Expected '->'");
    
    auto returnType = type();
    auto body = blockStatement();
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<OperatorDecl>(
        opToken.lexeme(),
        std::move(parameters),
        std::move(returnType),
        std::move(body),
        makeRange(opToken, previous_)
    );
}

std::unique_ptr<Decl> Parser::templateDeclaration() {
    auto params = parseTemplateParameters();
    
    std::unique_ptr<Decl> decl;
    
    // Check if this is a template function without 'def' keyword
    if (check(lexer::TokenType::IDENTIFIER) && checkNext(lexer::TokenType::LEFT_PAREN)) {
        // Parse template function
        lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected function name");
        consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
        
        std::vector<FunctionDecl::Parameter> parameters;
        
        if (!check(lexer::TokenType::RIGHT_PAREN)) {
            do {
                auto paramType = type();
                lexer::Token paramName = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
                parameters.emplace_back(paramName.lexeme(), std::move(paramType));
            } while (match(lexer::TokenType::COMMA));
        }
        
        consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
        
        std::unique_ptr<TypeExpr> returnType;
        if (match(lexer::TokenType::ARROW)) {
            returnType = type();
        }
        
        auto body = blockStatement();
        consume(lexer::TokenType::SEMICOLON, "Expected ';'");
        
        decl = std::make_unique<FunctionDecl>(
            name.lexeme(),
            std::move(parameters),
            std::move(returnType),
            std::move(body),
            makeRange(name, previous_)
        );
    } else {
        // Parse regular declaration
        decl = declaration();
    }
    
    if (!decl) {
        error("Expected declaration after template parameters");
        return nullptr;
    }
    
    return std::make_unique<TemplateDecl>(
        std::move(params),
        std::move(decl),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Decl> Parser::enumDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected enum name");
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<EnumDecl::Member> members;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        lexer::Token memberName = consume(lexer::TokenType::IDENTIFIER, "Expected enum member name");
        
        std::unique_ptr<Expr> value;
        if (match(lexer::TokenType::EQUAL)) {
            value = expression();
        }
        
        members.emplace_back(memberName.lexeme(), std::move(value));
        
        if (!match(lexer::TokenType::COMMA)) {
            break;
        }
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<EnumDecl>(
        name.lexeme(),
        std::move(members),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::typeDeclaration() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected type name");
    consume(lexer::TokenType::COLON, "Expected ':'");
    
    auto underlyingType = type();
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<TypeDecl>(
        name.lexeme(),
        std::move(underlyingType),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Decl> Parser::dataDeclaration() {
    bool isSigned = true;
    
    // Check what we matched
    if (previous_.type() == lexer::TokenType::KEYWORD_UNSIGNED) {
        isSigned = false;
    } else if (previous_.type() == lexer::TokenType::KEYWORD_SIGNED) {
        isSigned = true;
    } else if (previous_.type() == lexer::TokenType::KEYWORD_DATA) {
        // Already have 'data', keep default signed
    } else {
        // We're at the start, need to match the modifier
        if (match(lexer::TokenType::KEYWORD_UNSIGNED)) {
            isSigned = false;
        } else if (match(lexer::TokenType::KEYWORD_SIGNED)) {
            isSigned = true;
        }
    }
    
    // Consume 'data' if we haven't already
    if (!check(lexer::TokenType::LEFT_BRACE)) {
        consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data'");
    }
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    lexer::Token sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
    int64_t bits = sizeToken.intValue();
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    // Handle array syntax data{8}[]
    if (match(lexer::TokenType::LEFT_BRACKET)) {
        consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']'");
    }
    
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected data type name");
    
    std::optional<size_t> alignment;
    if (match(lexer::TokenType::KEYWORD_ALIGN)) {
        alignment = parseAlignmentAttribute();
    }
    
    bool isVolatile = match(lexer::TokenType::KEYWORD_VOLATILE);
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<DataDecl>(
        name.lexeme(),
        bits,
        isSigned,
        makeRange(name, previous_),
        alignment,
        isVolatile
    );
}

std::unique_ptr<Decl> Parser::asmDeclaration() {
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::string asmCode;
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        asmCode += std::string(current_.lexeme()) + " ";
        advance();
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<AsmDecl>(
        asmCode,
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Decl> Parser::sectionDeclaration() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    
    lexer::Token sectionName = consume(lexer::TokenType::CHAR_LITERAL, "Expected section name");
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    auto attribute = parseSectionAttribute();
    auto decl = declaration();
    std::unique_ptr<Expr> addressExpr = parseAddressSpecifier();
    
    return std::make_unique<SectionDecl>(
        sectionName.stringValue(),
        attribute,
        std::move(decl),
        std::move(addressExpr),
        makeRange(sectionName, previous_)
    );
}

// Helper methods for parsing declarations
std::optional<size_t> Parser::parseAlignmentAttribute() {
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    lexer::Token alignValue = consume(lexer::TokenType::INTEGER_LITERAL, "Expected alignment value");
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    return static_cast<size_t>(alignValue.intValue());
}

SectionDecl::Attribute Parser::parseSectionAttribute() {
    if (match(lexer::TokenType::DOT)) {
        if (match(lexer::TokenType::IDENTIFIER)) {
            std::string_view attr = previous_.lexeme();
            if (attr == "rodata") return SectionDecl::Attribute::RODATA;
            if (attr == "rwdata") return SectionDecl::Attribute::RWDATA;
            if (attr == "exec") return SectionDecl::Attribute::EXEC;
            if (attr == "noinit") return SectionDecl::Attribute::NOINIT;
            if (attr == "persist") return SectionDecl::Attribute::PERSIST;
        }
    }
    return SectionDecl::Attribute::NONE;
}

std::unique_ptr<Expr> Parser::parseAddressSpecifier() {
    if (match(lexer::TokenType::KEYWORD_ADDRESS)) {
        consume(lexer::TokenType::LESS, "Expected '<'");
        auto addressExpr = expression();
        consume(lexer::TokenType::GREATER, "Expected '>'");
        return addressExpr;
    }
    return nullptr;
}

std::unique_ptr<Decl> Parser::parseTemplateParameterList() {
    // This is a placeholder - template parameter parsing would be more complex
    error("Template parameter list parsing not implemented");
    return nullptr;
}

std::vector<std::unique_ptr<Decl>> Parser::parseObjectMembers() {
    std::vector<std::unique_ptr<Decl>> members;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        auto member = declaration();
        if (member) {
            members.push_back(std::move(member));
        }
    }
    
    return members;
}

std::vector<std::string_view> Parser::parseInheritanceList() {
    std::vector<std::string_view> baseList;
    
    do {
        lexer::Token baseName = consume(lexer::TokenType::IDENTIFIER, "Expected base class name");
        baseList.push_back(baseName.lexeme());
    } while (match(lexer::TokenType::COMMA));
    
    consume(lexer::TokenType::GREATER, "Expected '>'");
    
    return baseList;
}

std::vector<std::string_view> Parser::parseExclusionList() {
    std::vector<std::string_view> exclusions;
    
    do {
        lexer::Token excludeName = consume(lexer::TokenType::IDENTIFIER, "Expected exclusion name");
        exclusions.push_back(excludeName.lexeme());
    } while (match(lexer::TokenType::COMMA));
    
    return exclusions;
}

std::vector<TemplateDecl::Parameter> Parser::parseTemplateParameters() {
    consume(lexer::TokenType::LESS, "Expected '<'");
    
    std::vector<TemplateDecl::Parameter> params;
    
    if (!check(lexer::TokenType::GREATER)) {
        do {
            lexer::Token paramName = consume(lexer::TokenType::IDENTIFIER, "Expected template parameter name");
            
            if (match(lexer::TokenType::COLON)) {
                // Value parameter with type
                auto paramType = type();
                params.emplace_back(paramName.lexeme(), TemplateDecl::Parameter::Kind::VALUE, std::move(paramType));
            } else {
                // Type parameter
                params.emplace_back(paramName.lexeme(), TemplateDecl::Parameter::Kind::TYPE);
            }
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::GREATER, "Expected '>'");
    
    return params;
}

// Statement parsing methods
std::unique_ptr<Stmt> Parser::statement() {
    try {
        // Skip empty statements
        if (match(lexer::TokenType::SEMICOLON)) {
            return statement();
        }
        
        // Handle declarations that can appear in statement context
        if (checkAny({
            lexer::TokenType::KEYWORD_CONST,
            lexer::TokenType::KEYWORD_VOLATILE,
            lexer::TokenType::KEYWORD_SIGNED,
            lexer::TokenType::KEYWORD_UNSIGNED,
            lexer::TokenType::KEYWORD_DATA
        })) {
            auto decl = declaration();
            return std::make_unique<DeclStmt>(std::move(decl), makeRange(current_, previous_));
        }
        
        // Control flow statements
        if (match(lexer::TokenType::KEYWORD_IF)) {
            return ifStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_WHILE)) {
            return whileStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_DO)) {
            return doWhileStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_FOR)) {
            return forStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_SWITCH)) {
            return switchStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_TRY)) {
            return tryStatement();
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
        
        if (match(lexer::TokenType::KEYWORD_THROW)) {
            return throwStatement();
        }
        
        if (match(lexer::TokenType::KEYWORD_ASSERT)) {
            return assertStatement();
        }
        
        // Block statements
        if (match(lexer::TokenType::LEFT_BRACE)) {
            return anonymousBlockStatement();
        }
        
        // Identifier-based statements (variable declarations, assignments, calls)
        if (check(lexer::TokenType::IDENTIFIER)) {
            return identifierStatement();
        }
        
        // Default to expression statement
        return expressionStatement();
        
    } catch (const common::Error& e) {
        synchronize();
        throw;
    }
}

std::unique_ptr<Stmt> Parser::identifierStatement() {
    // Look ahead to determine statement type
    lexer::Token next = tokenizer_.peekToken();
    
    // Variable declaration (name: type = value)
    if (next.type() == lexer::TokenType::COLON) {
        return variableStatement();
    }
    
    // Variable declaration with initialization (name = value)
    if (next.type() == lexer::TokenType::EQUAL) {
        return variableStatement();
    }
    
    // Default to expression statement
    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::dataTypeStatement() {
    error("Unexpected 'data' keyword in statement context");
    return nullptr;
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto expr = expression();
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ExprStmt>(std::move(expr), makeRange(current_, previous_));
}

std::unique_ptr<Stmt> Parser::anonymousBlockStatement() {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        auto stmt = statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    return std::make_unique<BlockStmt>(std::move(statements), makeRange(current_, previous_));
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        auto stmt = statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    return std::make_unique<BlockStmt>(std::move(statements), makeRange(current_, previous_));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto condition = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    auto thenBranch = statement();
    
    std::unique_ptr<Stmt> elseBranch;
    if (match(lexer::TokenType::KEYWORD_ELSE)) {
        elseBranch = statement();
    }
    
    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::doWhileStatement() {
    bool oldInControl = inControlStructure_;
    inControlStructure_ = true;
    
    auto body = statement();
    
    consume(lexer::TokenType::KEYWORD_WHILE, "Expected 'while'");
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto condition = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    inControlStructure_ = oldInControl;
    
    // Convert do-while to while with initial execution
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(body));
    
    auto blockBody = std::make_unique<BlockStmt>(std::move(stmts), makeRange(current_, previous_));
    
    return std::make_unique<WhileStmt>(
        std::move(condition),
        std::move(blockBody),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto condition = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    bool oldInControl = inControlStructure_;
    inControlStructure_ = true;
    
    auto body = statement();
    
    inControlStructure_ = oldInControl;
    
    return std::make_unique<WhileStmt>(
        std::move(condition),
        std::move(body),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::forStatement() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    
    std::unique_ptr<Stmt> initializer;
    if (match(lexer::TokenType::SEMICOLON)) {
        initializer = nullptr;
    } else {
        initializer = statement();
    }
    
    std::unique_ptr<Expr> condition;
    if (!check(lexer::TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    std::unique_ptr<Expr> increment;
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        increment = expression();
    }
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    bool oldInControl = inControlStructure_;
    inControlStructure_ = true;
    
    auto body = statement();
    
    inControlStructure_ = oldInControl;
    
    return std::make_unique<ForStmt>(
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::move(body),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    lexer::Token keyword = previous_;
    
    std::unique_ptr<Expr> value;
    if (!check(lexer::TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ReturnStmt>(keyword, std::move(value), makeRange(keyword, previous_));
}

std::unique_ptr<Stmt> Parser::breakStatement() {
    lexer::Token keyword = previous_;
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    if (!inControlStructure_) {
        error("'break' can only be used inside loops or switch statements");
    }
    
    return std::make_unique<BreakStmt>(keyword, makeRange(keyword, previous_));
}

std::unique_ptr<Stmt> Parser::continueStatement() {
    lexer::Token keyword = previous_;
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    if (!inControlStructure_) {
        error("'continue' can only be used inside loops");
    }
    
    return std::make_unique<ContinueStmt>(keyword, makeRange(keyword, previous_));
}

std::unique_ptr<Stmt> Parser::throwStatement() {
    lexer::Token keyword = previous_;
    
    std::unique_ptr<Expr> message;
    if (match(lexer::TokenType::LEFT_PAREN)) {
        message = expression();
        consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    }
    
    std::unique_ptr<Stmt> body;
    if (match(lexer::TokenType::LEFT_BRACE)) {
        body = anonymousBlockStatement();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<ThrowStmt>(
        keyword,
        std::move(message),
        std::move(body),
        makeRange(keyword, previous_)
    );
}

std::unique_ptr<Stmt> Parser::tryStatement() {
    auto tryBlock = blockStatement();
    
    std::vector<TryStmt::CatchClause> catchClauses;
    
    while (match(lexer::TokenType::KEYWORD_CATCH)) {
        consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
        
        std::unique_ptr<TypeExpr> exceptionType;
        if (!check(lexer::TokenType::RIGHT_PAREN)) {
            exceptionType = type();
        }
        
        consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
        
        auto handler = blockStatement();
        
        catchClauses.emplace_back(std::move(exceptionType), std::move(handler));
    }
    
    if (catchClauses.empty()) {
        error("Expected at least one catch clause");
    }
    
    return std::make_unique<TryStmt>(
        std::move(tryBlock),
        std::move(catchClauses),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::switchStatement() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto value = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{'");
    
    std::vector<SwitchStmt::CaseClause> cases;
    std::unique_ptr<Stmt> defaultCase;
    
    bool oldInControl = inControlStructure_;
    inControlStructure_ = true;
    
    while (!check(lexer::TokenType::RIGHT_BRACE) && !check(lexer::TokenType::END_OF_FILE)) {
        if (match(lexer::TokenType::KEYWORD_CASE)) {
            consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
            auto pattern = expression();
            consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
            
            auto caseBody = blockStatement();
            
            cases.emplace_back(std::move(pattern), std::move(caseBody));
        } else if (match(lexer::TokenType::KEYWORD_DEFAULT)) {
            if (defaultCase) {
                error("Multiple default cases");
            }
            defaultCase = blockStatement();
        } else {
            error("Expected 'case' or 'default'");
            break;
        }
    }
    
    inControlStructure_ = oldInControl;
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    return std::make_unique<SwitchStmt>(
        std::move(value),
        std::move(cases),
        std::move(defaultCase),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<Stmt> Parser::variableStatement() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected variable name");
    
    std::unique_ptr<TypeExpr> varType;
    std::unique_ptr<Expr> initializer;
    
    if (match(lexer::TokenType::COLON)) {
        varType = type();
    }
    
    if (match(lexer::TokenType::EQUAL)) {
        initializer = expression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    return std::make_unique<VarStmt>(
        name,
        std::move(varType),
        std::move(initializer),
        makeRange(name, previous_)
    );
}

std::unique_ptr<Stmt> Parser::assertStatement() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto condition = expression();
    
    std::unique_ptr<Expr> message;
    if (match(lexer::TokenType::COMMA)) {
        message = expression();
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    
    // Convert assert to if statement that throws
    auto throwStmt = std::make_unique<ThrowStmt>(
        lexer::Token(),
        message ? std::move(message) : std::make_unique<LiteralExpr>(
            lexer::Token(), "Assertion failed"
        ),
        nullptr,
        makeRange(current_, previous_)
    );
    
    auto notCondition = std::make_unique<UnaryExpr>(
        lexer::Token(lexer::TokenType::EXCLAMATION, "!", current_.start(), current_.end()),
        std::move(condition),
        true,
        makeRange(current_, previous_)
    );
    
    return std::make_unique<IfStmt>(
        std::move(notCondition),
        std::move(throwStmt),
        nullptr,
        makeRange(current_, previous_)
    );
}

// Expression parsing methods (precedence climbing)
std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = cast();
    
    if (match({
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
        lexer::Token op = previous_;
        auto value = assignment();
        
        return std::make_unique<AssignExpr>(
            std::move(expr),
            op,
            std::move(value),
            makeRange(expr->range.start, value->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::cast() {
    auto expr = ternary();
    
    if (match(lexer::TokenType::COLON)) {
        auto targetType = type();
        
        return std::make_unique<CastExpr>(
            std::move(targetType),
            std::move(expr),
            makeRange(expr->range.start, previous_.end())
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::ternary() {
    auto expr = logicalOr();
    
    if (match(lexer::TokenType::QUESTION)) {
        auto thenExpr = expression();
        consume(lexer::TokenType::COLON, "Expected ':'");
        auto elseExpr = ternary();
        
        return std::make_unique<TernaryExpr>(
            std::move(expr),
            std::move(thenExpr),
            std::move(elseExpr),
            makeRange(expr->range.start, elseExpr->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match({lexer::TokenType::KEYWORD_OR, lexer::TokenType::PIPE_PIPE})) {
        lexer::Token op = previous_;
        auto right = logicalAnd();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = bitwiseOr();
    
    while (match({lexer::TokenType::KEYWORD_AND, lexer::TokenType::AMPERSAND_AMPERSAND})) {
        lexer::Token op = previous_;
        auto right = bitwiseOr();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseOr() {
    auto expr = bitwiseXor();
    
    while (match(lexer::TokenType::PIPE)) {
        lexer::Token op = previous_;
        auto right = bitwiseXor();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseXor() {
    auto expr = bitwiseAnd();
    
    while (match({lexer::TokenType::CARET, lexer::TokenType::KEYWORD_XOR})) {
        lexer::Token op = previous_;
        auto right = bitwiseAnd();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseAnd() {
    auto expr = equality();
    
    while (match(lexer::TokenType::AMPERSAND)) {
        lexer::Token op = previous_;
        auto right = equality();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = is();
    
    while (match({lexer::TokenType::EQUAL_EQUAL, lexer::TokenType::NOT_EQUAL})) {
        lexer::Token op = previous_;
        auto right = is();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::is() {
    auto expr = comparison();
    
    while (match({lexer::TokenType::KEYWORD_IS, lexer::TokenType::KEYWORD_NOT})) {
        lexer::Token op = previous_;
        auto right = comparison();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = bitShift();
    
    while (match({
        lexer::TokenType::GREATER,
        lexer::TokenType::GREATER_EQUAL,
        lexer::TokenType::LESS,
        lexer::TokenType::LESS_EQUAL
    })) {
        lexer::Token op = previous_;
        auto right = bitShift();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::bitShift() {
    auto expr = term();
    
    while (match({lexer::TokenType::LESS_LESS, lexer::TokenType::GREATER_GREATER})) {
        lexer::Token op = previous_;
        auto right = term();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    
    while (match({lexer::TokenType::MINUS, lexer::TokenType::PLUS})) {
        lexer::Token op = previous_;
        auto right = factor();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = exponentiation();
    
    while (match({lexer::TokenType::SLASH, lexer::TokenType::ASTERISK, lexer::TokenType::PERCENT})) {
        lexer::Token op = previous_;
        auto right = exponentiation();
        
        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::exponentiation() {
    auto expr = unary();
    
    if (match(lexer::TokenType::DOUBLE_ASTERISK)) {
        lexer::Token op = previous_;
        auto right = exponentiation(); // Right-associative
        
        return std::make_unique<BinaryExpr>(
            std::move(expr),
            op,
            std::move(right),
            makeRange(expr->range.start, right->range.end)
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({
        lexer::TokenType::EXCLAMATION,
        lexer::TokenType::MINUS,
        lexer::TokenType::PLUS,
        lexer::TokenType::TILDE,
        lexer::TokenType::KEYWORD_NOT,
        lexer::TokenType::PLUS_PLUS,
        lexer::TokenType::MINUS_MINUS,
        lexer::TokenType::ASTERISK_PTR,
        lexer::TokenType::AT_REF
    })) {
        lexer::Token op = previous_;
        auto right = unary();
        
        return std::make_unique<UnaryExpr>(
            op,
            std::move(right),
            true, // prefix
            makeRange(op.start(), right->range.end)
        );
    }
    
    return postfix();
}

std::unique_ptr<Expr> Parser::postfix() {
    auto expr = call();
    
    while (match({lexer::TokenType::PLUS_PLUS, lexer::TokenType::MINUS_MINUS})) {
        lexer::Token op = previous_;
        
        expr = std::make_unique<UnaryExpr>(
            op,
            std::move(expr),
            false, // postfix
            makeRange(expr->range.start, op.end())
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();
    
    while (true) {
        if (match(lexer::TokenType::LEFT_PAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(lexer::TokenType::DOT)) {
            lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected property name");
            
            expr = std::make_unique<GetExpr>(
                std::move(expr),
                name,
                makeRange(expr->range.start, name.end())
            );
        } else if (match(lexer::TokenType::LEFT_BRACKET)) {
            auto index = expression();
            consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']'");
            
            expr = std::make_unique<SubscriptExpr>(
                std::move(expr),
                std::move(index),
                makeRange(expr->range.start, previous_.end())
            );
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::primary() {
    // Literals
    if (match(lexer::TokenType::KEYWORD_TRUE)) {
        return std::make_unique<LiteralExpr>(previous_, true);
    }
    
    if (match(lexer::TokenType::KEYWORD_FALSE)) {
        return std::make_unique<LiteralExpr>(previous_, false);
    }
    
    if (match(lexer::TokenType::INTEGER_LITERAL)) {
        return std::make_unique<LiteralExpr>(previous_, previous_.intValue());
    }
    
    if (match(lexer::TokenType::FLOAT_LITERAL)) {
        return std::make_unique<LiteralExpr>(previous_, previous_.floatValue());
    }
    
    if (match(lexer::TokenType::CHAR_LITERAL)) {
        return std::make_unique<LiteralExpr>(previous_, std::string(previous_.stringValue()));
    }
    
    // Keywords as expressions
    if (match(lexer::TokenType::KEYWORD_THIS)) {
        return std::make_unique<VariableExpr>(previous_);
    }
    
    // Special expressions
    if (match(lexer::TokenType::KEYWORD_SIZEOF)) {
        return parseSizeOfExpr();
    }
    
    if (match(lexer::TokenType::KEYWORD_TYPEOF)) {
        return parseTypeOfExpr();
    }
    
    if (match(lexer::TokenType::KEYWORD_OP)) {
        return parseOpExpr();
    }
    
    if (match(lexer::TokenType::KEYWORD_ADDRESS)) {
        return parseAddressOfExpr();
    }
    
    // Parenthesized expression
    if (match(lexer::TokenType::LEFT_PAREN)) {
        auto expr = expression();
        consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
        
        return std::make_unique<GroupExpr>(
            std::move(expr),
            makeRange(previous_, current_)
        );
    }
    
    // Array literal
    if (match(lexer::TokenType::LEFT_BRACKET)) {
        return parseArrayLiteral();
    }
    
    // Dictionary literal
    if (match(lexer::TokenType::LEFT_BRACE)) {
        return parseDictionaryLiteral();
    }
    
    // Identifiers and qualified names
    if (check(lexer::TokenType::IDENTIFIER)) {
        return qualifiedIdentifier();
    }
    
    // Injectable string (simplified)
    if (match(lexer::TokenType::ISTRING_START)) {
        std::vector<std::string_view> textParts;
        std::vector<std::unique_ptr<Expr>> exprParts;
        
        return std::make_unique<IStringExpr>(
            std::move(textParts),
            std::move(exprParts),
            makeRange(previous_, current_)
        );
    }
    
    error("Expected expression");
    return nullptr;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    std::vector<std::unique_ptr<Expr>> arguments;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    lexer::Token paren = consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    return std::make_unique<CallExpr>(
        std::move(callee),
        paren,
        std::move(arguments),
        makeRange(callee->range.start, paren.end())
    );
}

std::unique_ptr<Expr> Parser::qualifiedIdentifier() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
    
    // Handle qualified access (Flux.Integer, namespace.type, etc.)
    if (match(lexer::TokenType::DOT)) {
        std::unique_ptr<Expr> object = std::make_unique<VariableExpr>(name);
        
        while (true) {
            lexer::Token member = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
            
            object = std::make_unique<GetExpr>(
                std::move(object),
                member,
                makeRange(name.start(), member.end())
            );
            
            if (!match(lexer::TokenType::DOT)) {
                break;
            }
        }
        
        return object;
    }
    
    // Handle scope resolution (::)
    if (match(lexer::TokenType::DOUBLE_COLON)) {
        error("Scope resolution operator not yet implemented");
        return std::make_unique<VariableExpr>(name);
    }
    
    return std::make_unique<VariableExpr>(name);
}

std::unique_ptr<Expr> Parser::parseSizeOfExpr() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto targetType = type();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    return std::make_unique<SizeOfExpr>(
        std::move(targetType),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Expr> Parser::parseTypeOfExpr() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '('");
    auto expr = expression();
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')'");
    
    return std::make_unique<TypeOfExpr>(
        std::move(expr),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Expr> Parser::parseOpExpr() {
    consume(lexer::TokenType::LESS, "Expected '<'");
    
    auto left = expression();
    
    lexer::Token opToken = advance();
    std::string_view operatorName = opToken.lexeme();
    
    auto right = expression();
    
    consume(lexer::TokenType::GREATER, "Expected '>'");
    
    return std::make_unique<OpExpr>(
        std::move(left),
        operatorName,
        std::move(right),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Expr> Parser::parseArrayLiteral() {
    std::vector<std::unique_ptr<Expr>> elements;
    
    if (!check(lexer::TokenType::RIGHT_BRACKET)) {
        do {
            elements.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']'");
    
    return std::make_unique<ArrayExpr>(
        std::move(elements),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Expr> Parser::parseDictionaryLiteral() {
    std::vector<std::unique_ptr<Expr>> elements;
    
    if (!check(lexer::TokenType::RIGHT_BRACE)) {
        do {
            auto key = expression();
            consume(lexer::TokenType::COLON, "Expected ':'");
            auto value = expression();
            
            // Create a key-value pair as an array
            std::vector<std::unique_ptr<Expr>> pair;
            pair.push_back(std::move(key));
            pair.push_back(std::move(value));
            
            elements.push_back(std::make_unique<ArrayExpr>(
                std::move(pair),
                makeRange(current_, previous_)
            ));
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}'");
    
    return std::make_unique<ArrayExpr>(
        std::move(elements),
        makeRange(previous_, current_)
    );
}

std::unique_ptr<Expr> Parser::parseAddressOfExpr() {
    consume(lexer::TokenType::LESS, "Expected '<'");
    auto addressValue = expression();
    consume(lexer::TokenType::GREATER, "Expected '>'");
    
    return std::make_unique<AddressOfExpr>(
        std::move(addressValue),
        makeRange(previous_, current_)
    );
}

// Type expression parsing methods
std::unique_ptr<TypeExpr> Parser::type() {
    if (check(lexer::TokenType::END_OF_FILE)) {
        error("Unexpected end of file while parsing type");
        return nullptr;
    }
    
    return qualifiedType();
}

std::unique_ptr<TypeExpr> Parser::namedType() {
    lexer::Token name = consume(lexer::TokenType::IDENTIFIER, "Expected type name");
    
    std::string qualifiedName = std::string(name.lexeme());
    common::SourcePosition start = name.start();
    common::SourcePosition end = name.end();
    
    // Handle qualified names like Flux.Integer
    while (match(lexer::TokenType::DOT)) {
        lexer::Token part = consume(lexer::TokenType::IDENTIFIER, "Expected identifier after '.'");
        qualifiedName += ".";
        qualifiedName += part.lexeme();
        end = part.end();
    }
    
    return std::make_unique<NamedTypeExpr>(
        qualifiedName,
        makeRange(start, end)
    );
}

std::unique_ptr<TypeExpr> Parser::arrayType(std::unique_ptr<TypeExpr> elementType) {
    consume(lexer::TokenType::LEFT_BRACKET, "Expected '[' for array type");
    
    std::unique_ptr<Expr> sizeExpr;
    if (!check(lexer::TokenType::RIGHT_BRACKET)) {
        sizeExpr = expression();
    }
    
    consume(lexer::TokenType::RIGHT_BRACKET, "Expected ']' after array size");
    
    return std::make_unique<ArrayTypeExpr>(
        std::move(elementType),
        std::move(sizeExpr),
        makeRange(elementType->range.start, previous_.end())
    );
}

std::unique_ptr<TypeExpr> Parser::pointerType(std::unique_ptr<TypeExpr> pointeeType) {
    bool isConst = false;
    bool isVolatile = false;
    
    // Handle const/volatile modifiers
    if (match(lexer::TokenType::KEYWORD_CONST)) {
        isConst = true;
    }
    if (match(lexer::TokenType::KEYWORD_VOLATILE)) {
        isVolatile = true;
    }
    
    consume(lexer::TokenType::ASTERISK_PTR, "Expected '*' for pointer type");
    
    return std::make_unique<PointerTypeExpr>(
        std::move(pointeeType),
        makeRange(pointeeType->range.start, previous_.end()),
        isVolatile,
        isConst
    );
}

std::unique_ptr<TypeExpr> Parser::functionType() {
    consume(lexer::TokenType::LEFT_PAREN, "Expected '(' for function type");
    
    std::vector<std::unique_ptr<TypeExpr>> parameterTypes;
    
    if (!check(lexer::TokenType::RIGHT_PAREN)) {
        do {
            parameterTypes.push_back(type());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RIGHT_PAREN, "Expected ')' after function parameters");
    consume(lexer::TokenType::ARROW, "Expected '->' after function parameters");
    
    auto returnType = type();
    
    return std::make_unique<FunctionTypeExpr>(
        std::move(parameterTypes),
        std::move(returnType),
        makeRange(current_, previous_)
    );
}

std::unique_ptr<TypeExpr> Parser::dataType() {
    bool isSigned = match(lexer::TokenType::KEYWORD_SIGNED);
    if (!isSigned) {
        match(lexer::TokenType::KEYWORD_UNSIGNED);
    }
    
    consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data' keyword");
    consume(lexer::TokenType::LEFT_BRACE, "Expected '{' after 'data'");
    
    lexer::Token sizeToken = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit size");
    int64_t bits = sizeToken.intValue();
    
    consume(lexer::TokenType::RIGHT_BRACE, "Expected '}' after bit size");
    
    std::optional<size_t> alignment;
    bool isVolatile = false;
    
    if (match(lexer::TokenType::KEYWORD_ALIGN)) {
        alignment = parseAlignmentAttribute();
    }
    
    if (match(lexer::TokenType::KEYWORD_VOLATILE)) {
        isVolatile = true;
    }
    
    return std::make_unique<DataTypeExpr>(
        bits,
        isSigned,
        makeRange(current_, previous_),
        alignment,
        isVolatile
    );
}

std::unique_ptr<TypeExpr> Parser::qualifiedType() {
    std::unique_ptr<TypeExpr> baseType;
    
    // Handle type modifiers
    bool isConst = match(lexer::TokenType::KEYWORD_CONST);
    bool isVolatile = match(lexer::TokenType::KEYWORD_VOLATILE);
    
    // Handle data types
    if (checkAny({lexer::TokenType::KEYWORD_SIGNED, lexer::TokenType::KEYWORD_UNSIGNED})) {
        baseType = dataType();
    }
    // Handle function types
    else if (check(lexer::TokenType::LEFT_PAREN)) {
        baseType = functionType();
    }
    // Handle named types
    else if (check(lexer::TokenType::IDENTIFIER)) {
        baseType = namedType();
    }
    // Handle void type
    else if (match(lexer::TokenType::KEYWORD_VOID)) {
        baseType = std::make_unique<NamedTypeExpr>("void", makeRange(previous_, previous_));
    }
    // Handle !void type
    else if (match(lexer::TokenType::BANG_VOID)) {
        baseType = std::make_unique<NamedTypeExpr>("!void", makeRange(previous_, previous_));
    }
    else {
        error("Expected type");
        return nullptr;
    }
    
    // Safety check
    if (!baseType) {
        return nullptr;
    }
    
    // Handle array and pointer modifiers
    while (true) {
        if (check(lexer::TokenType::LEFT_BRACKET)) {
            baseType = arrayType(std::move(baseType));
        } else if (check(lexer::TokenType::ASTERISK_PTR) || 
                   (check(lexer::TokenType::ASTERISK) && !checkNext(lexer::TokenType::EQUAL))) {
            baseType = pointerType(std::move(baseType));
        } else {
            break;
        }
        
        // Safety check in loop
        if (!baseType) {
            break;
        }
    }
    
    return baseType;
}

bool Parser::isTypeModifier(const lexer::Token& token) const {
    return token.isOneOf({
        lexer::TokenType::KEYWORD_CONST,
        lexer::TokenType::KEYWORD_VOLATILE,
        lexer::TokenType::KEYWORD_SIGNED,
        lexer::TokenType::KEYWORD_UNSIGNED
    });
}

// Helper methods
common::SourceRange Parser::makeRange(const lexer::Token& start, const lexer::Token& end) const {
    return {start.start(), end.end()};
}

common::SourceRange Parser::makeRange(const lexer::Token& token) const {
    return {token.start(), token.end()};
}

common::SourceRange Parser::makeRange(const common::SourcePosition& start, const common::SourcePosition& end) const {
    return {start, end};
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
    
    consume(end, "Expected end delimiter");
    
    return items;
}

} // namespace parser
} // namespace flux