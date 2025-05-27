#include "parser.h"
#include <algorithm>
#include <sstream>

namespace flux {
namespace parser {

Parser::Parser(std::shared_ptr<common::Source> source, common::Arena& arena)
    : source_(source), arena_(arena), tokenizer_(source, arena), current_token_index_(0) {
    tokenizeSource();
}

Parser::~Parser() {
}

void Parser::tokenizeSource() {
    tokens_ = tokenizer_.tokenizeAll();
    
    // Copy tokenizer errors to parser errors
    for (const auto& error : tokenizer_.getErrors().errors()) {
        errors_.addError(error);
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>(common::SourceRange{});
    
    // Parse global declarations
    auto declarations = parseGlobalDeclarations();
    for (auto* decl : declarations) {
        program->addDeclaration(decl);
    }
    
    // Check for end of file
    if (!isAtEnd()) {
        reportError("Expected end of file");
    }
    
    return program;
}

const lexer::Token& Parser::currentToken() const {
    if (current_token_index_ >= tokens_.size()) {
        static const lexer::Token eof_token(lexer::TokenType::END_OF_FILE, "", common::SourcePosition{});
        return eof_token;
    }
    return tokens_[current_token_index_];
}

const lexer::Token& Parser::peekToken(size_t offset) const {
    size_t index = current_token_index_ + offset;
    if (index >= tokens_.size()) {
        static const lexer::Token eof_token(lexer::TokenType::END_OF_FILE, "", common::SourcePosition{});
        return eof_token;
    }
    return tokens_[index];
}

void Parser::advance() {
    if (current_token_index_ < tokens_.size()) {
        current_token_index_++;
    }
}

bool Parser::isAtEnd() const {
    return current_token_index_ >= tokens_.size() || 
           currentToken().type == lexer::TokenType::END_OF_FILE;
}

bool Parser::check(lexer::TokenType type) const {
    return currentToken().type == type;
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::reportError(const std::string& message) {
    reportError(message, currentToken());
}

void Parser::reportError(const std::string& message, const lexer::Token& token) {
    output::SourceLocation location(
        source_->filename(),
        static_cast<int>(token.position.line),
        static_cast<int>(token.position.column)
    );
    errors_.addError(common::ErrorCode::PARSER_ERROR, message, location);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (currentToken().type == lexer::TokenType::SEMICOLON) {
            advance();
            return;
        }
        
        switch (currentToken().type) {
            case lexer::TokenType::KEYWORD_IMPORT:
            case lexer::TokenType::KEYWORD_USING:
            case lexer::TokenType::KEYWORD_NAMESPACE:
            case lexer::TokenType::KEYWORD_OBJECT:
            case lexer::TokenType::KEYWORD_STRUCT:
            case lexer::TokenType::KEYWORD_DEF:
            case lexer::TokenType::KEYWORD_TEMPLATE:
            case lexer::TokenType::KEYWORD_OPERATOR:
            case lexer::TokenType::KEYWORD_IF:
            case lexer::TokenType::KEYWORD_WHILE:
            case lexer::TokenType::KEYWORD_FOR:
            case lexer::TokenType::KEYWORD_SWITCH:
            case lexer::TokenType::KEYWORD_RETURN:
            case lexer::TokenType::KEYWORD_BREAK:
            case lexer::TokenType::KEYWORD_CONTINUE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

lexer::Token Parser::consume(lexer::TokenType type, const std::string& message) {
    if (check(type)) {
        lexer::Token token = currentToken();
        advance();
        return token;
    }
    
    reportError(message);
    synchronize();
    return currentToken();
}

common::SourceRange Parser::makeRange(const lexer::Token& start, const lexer::Token& end) const {
    return common::SourceRange{start.position, end.position};
}

common::SourceRange Parser::makeRange(size_t start_token_index, size_t end_token_index) const {
    if (start_token_index >= tokens_.size() || end_token_index >= tokens_.size()) {
        return common::SourceRange{};
    }
    return makeRange(tokens_[start_token_index], tokens_[end_token_index]);
}

common::SourceRange Parser::getCurrentRange() const {
    return common::SourceRange{currentToken().position, currentToken().position};
}

std::vector<Declaration*> Parser::parseGlobalDeclarations() {
    std::vector<Declaration*> declarations;
    
    while (!isAtEnd()) {
        try {
            Declaration* decl = parseGlobalDeclaration();
            if (decl) {
                declarations.push_back(decl);
            }
        } catch (...) {
            synchronize();
        }
    }
    
    return declarations;
}

Declaration* Parser::parseGlobalDeclaration() {
    switch (currentToken().type) {
        case lexer::TokenType::KEYWORD_IMPORT:
            return parseImportDeclaration();
        case lexer::TokenType::KEYWORD_USING:
            return parseUsingDeclaration();
        case lexer::TokenType::KEYWORD_NAMESPACE:
            return parseNamespaceDeclaration();
        case lexer::TokenType::KEYWORD_OBJECT:
            return parseObjectDeclaration();
        case lexer::TokenType::KEYWORD_STRUCT:
            return parseStructDeclaration();
        case lexer::TokenType::KEYWORD_DEF:
            return parseFunctionDeclaration();
        case lexer::TokenType::KEYWORD_TEMPLATE:
            return parseTemplateDeclaration();
        case lexer::TokenType::KEYWORD_OPERATOR:
            return parseOperatorDeclaration();
        default:
            // Try parsing as variable declaration
            if (check(lexer::TokenType::IDENTIFIER) || 
                check(lexer::TokenType::KEYWORD_DATA) ||
                check(lexer::TokenType::KEYWORD_CONST) ||
                check(lexer::TokenType::KEYWORD_VOLATILE) ||
                check(lexer::TokenType::KEYWORD_SIGNED) ||
                check(lexer::TokenType::KEYWORD_UNSIGNED) ||
                check(lexer::TokenType::KEYWORD_VOID) ||
                check(lexer::TokenType::KEYWORD_AUTO)) {
                auto* var_decl = parseVariableDeclaration();
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
                return var_decl;
            }
            
            reportError("Expected global declaration");
            return nullptr;
    }
}

ImportDeclaration* Parser::parseImportDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_IMPORT, "Expected 'import'");
    
    auto string_token = consume(lexer::TokenType::STRING_LITERAL, "Expected module path");
    std::string_view module_path = string_token.processed_text.empty() ? 
                                  string_token.text : string_token.processed_text;
    
    std::optional<std::string_view> alias;
    if (match(lexer::TokenType::KEYWORD_AS)) {
        auto identifier_token = consume(lexer::TokenType::IDENTIFIER, "Expected identifier after 'as'");
        alias = identifier_token.text;
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after import declaration");
    
    auto range = makeRange(start_index, current_token_index_ - 1);
    return allocate<ImportDeclaration>(range, module_path, alias);
}

UsingDeclaration* Parser::parseUsingDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_USING, "Expected 'using'");
    
    auto* using_decl = allocate<UsingDeclaration>(getCurrentRange());
    
    // Parse qualified identifier list
    do {
        std::string_view name = parseQualifiedIdentifier();
        using_decl->addName(name);
    } while (match(lexer::TokenType::COMMA));
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after using declaration");
    
    return using_decl;
}

NamespaceDeclaration* Parser::parseNamespaceDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_NAMESPACE, "Expected 'namespace'");
    
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected namespace name");
    
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after namespace name");
    
    auto* namespace_decl = allocate<NamespaceDeclaration>(getCurrentRange(), name_token.text);
    
    // Parse namespace members
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        Declaration* member = parseGlobalDeclaration();
        if (member) {
            namespace_decl->addMember(member);
        }
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after namespace body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after namespace declaration");
    
    return namespace_decl;
}

ObjectDeclaration* Parser::parseObjectDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_OBJECT, "Expected 'object'");
    
    // Check for template keyword
    TemplateParameterList* template_params = nullptr;
    if (match(lexer::TokenType::KEYWORD_TEMPLATE)) {
        template_params = parseTemplateParameterList();
    }
    
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected object name");
    
    // Parse template parameters if not prefixed with template keyword
    if (!template_params && check(lexer::TokenType::LESS_THAN)) {
        advance();
        template_params = parseTemplateParameterList();
        consume(lexer::TokenType::GREATER_THAN, "Expected '>' after template parameters");
    }
    
    // Parse inheritance clause
    Type* parent_type = nullptr;
    if (match(lexer::TokenType::LESS_THAN)) {
        parent_type = parseType();
        consume(lexer::TokenType::GREATER_THAN, "Expected '>' after parent type");
    }
    
    // Check for forward declaration
    if (match(lexer::TokenType::SEMICOLON)) {
        auto range = makeRange(start_index, current_token_index_ - 1);
        return allocate<ObjectDeclaration>(range, name_token.text, template_params, parent_type, true);
    }
    
    // Parse object body
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after object declaration");
    
    auto* object_decl = allocate<ObjectDeclaration>(getCurrentRange(), name_token.text, 
                                                   template_params, parent_type, false);
    
    auto members = parseObjectMembers();
    for (auto* member : members) {
        object_decl->addMember(member);
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after object body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after object declaration");
    
    return object_decl;
}

std::vector<Declaration*> Parser::parseObjectMembers() {
    std::vector<Declaration*> members;
    
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        Declaration* member = parseObjectMember();
        if (member) {
            members.push_back(member);
        }
    }
    
    return members;
}

Declaration* Parser::parseObjectMember() {
    switch (currentToken().type) {
        case lexer::TokenType::KEYWORD_DEF:
            return parseFunctionDeclaration();
        case lexer::TokenType::KEYWORD_OBJECT:
            return parseObjectDeclaration();
        case lexer::TokenType::KEYWORD_STRUCT:
            return parseStructDeclaration();
        case lexer::TokenType::KEYWORD_TEMPLATE:
            return parseTemplateDeclaration();
        default:
            // Variable declaration
            auto* var_decl = parseVariableDeclaration();
            consume(lexer::TokenType::SEMICOLON, "Expected ';' after member variable declaration");
            return var_decl;
    }
}

StructDeclaration* Parser::parseStructDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_STRUCT, "Expected 'struct'");
    
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected struct name");
    
    // Parse template parameters
    TemplateParameterList* template_params = nullptr;
    if (check(lexer::TokenType::LESS_THAN)) {
        advance();
        template_params = parseTemplateParameterList();
        consume(lexer::TokenType::GREATER_THAN, "Expected '>' after template parameters");
    }
    
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after struct name");
    
    auto* struct_decl = allocate<StructDeclaration>(getCurrentRange(), name_token.text, template_params);
    
    auto members = parseStructMembers();
    for (auto* member : members) {
        struct_decl->addMember(member);
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after struct body");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after struct declaration");
    
    return struct_decl;
}

std::vector<Declaration*> Parser::parseStructMembers() {
    std::vector<Declaration*> members;
    
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        Declaration* member = parseStructMember();
        if (member) {
            members.push_back(member);
        }
    }
    
    return members;
}

Declaration* Parser::parseStructMember() {
    auto* var_decl = parseVariableDeclaration();
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after struct member");
    return var_decl;
}

FunctionDeclaration* Parser::parseFunctionDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_DEF, "Expected 'def'");
    
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected function name");
    
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after function name");
    
    ParameterList* params = nullptr;
    if (!check(lexer::TokenType::PAREN_CLOSE)) {
        params = parseParameterList();
    }
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after parameters");
    consume(lexer::TokenType::ARROW, "Expected '->' after parameters");
    
    Type* return_type = parseType();
    
    auto* func_decl = allocate<FunctionDeclaration>(getCurrentRange(), name_token.text, 
                                                   params, return_type);
    
    // Parse function body
    if (check(lexer::TokenType::BRACE_OPEN)) {
        Statement* body = parseCompoundStatement();
        func_decl->setBody(body);
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after function declaration");
    
    return func_decl;
}

ParameterList* Parser::parseParameterList() {
    auto* param_list = allocate<ParameterList>(getCurrentRange());
    
    do {
        Parameter* param = parseParameter();
        param_list->addParameter(param);
    } while (match(lexer::TokenType::COMMA));
    
    return param_list;
}

Parameter* Parser::parseParameter() {
    Type* type = parseType();
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
    
    return allocate<Parameter>(getCurrentRange(), type, name_token.text);
}

TemplateDeclaration* Parser::parseTemplateDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_TEMPLATE, "Expected 'template'");
    
    consume(lexer::TokenType::LESS_THAN, "Expected '<' after 'template'");
    TemplateParameterList* params = parseTemplateParameterList();
    consume(lexer::TokenType::GREATER_THAN, "Expected '>' after template parameters");
    
    Declaration* declaration = nullptr;
    switch (currentToken().type) {
        case lexer::TokenType::KEYWORD_DEF:
            declaration = parseFunctionDeclaration();
            break;
        case lexer::TokenType::KEYWORD_OBJECT:
            declaration = parseObjectDeclaration();
            break;
        case lexer::TokenType::KEYWORD_STRUCT:
            declaration = parseStructDeclaration();
            break;
        default:
            reportError("Expected function, object, or struct declaration after template");
            return nullptr;
    }
    
    auto range = makeRange(start_index, current_token_index_ - 1);
    return allocate<TemplateDeclaration>(range, params, declaration);
}

TemplateParameterList* Parser::parseTemplateParameterList() {
    auto* param_list = allocate<TemplateParameterList>(getCurrentRange());
    
    do {
        TemplateParameter* param = parseTemplateParameter();
        param_list->addParameter(param);
    } while (match(lexer::TokenType::COMMA));
    
    return param_list;
}

TemplateParameter* Parser::parseTemplateParameter() {
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected template parameter name");
    return allocate<TemplateParameter>(getCurrentRange(), name_token.text);
}

OperatorDeclaration* Parser::parseOperatorDeclaration() {
    size_t start_index = current_token_index_;
    consume(lexer::TokenType::KEYWORD_OPERATOR, "Expected 'operator'");
    
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'operator'");
    ParameterList* params = parseParameterList();
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after parameters");
    
    consume(lexer::TokenType::BRACKET_OPEN, "Expected '[' before operator symbol");
    std::string operator_symbol = parseOperatorSymbol();
    consume(lexer::TokenType::BRACKET_CLOSE, "Expected ']' after operator symbol");
    
    consume(lexer::TokenType::ARROW, "Expected '->' after operator symbol");
    Type* return_type = parseType();
    
    Statement* body = parseCompoundStatement();
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after operator declaration");
    
    auto range = makeRange(start_index, current_token_index_ - 1);
    return allocate<OperatorDeclaration>(range, params, operator_symbol, return_type, body);
}

std::string Parser::parseOperatorSymbol() {
    std::string symbol;
    
    // Parse operator symbol (can be identifier or sequence of symbols)
    if (check(lexer::TokenType::IDENTIFIER)) {
        symbol = std::string(currentToken().text);
        advance();
    } else {
        // Parse symbol sequence
        while (!check(lexer::TokenType::BRACKET_CLOSE) && !isAtEnd()) {
            symbol += std::string(currentToken().text);
            advance();
        }
    }
    
    return symbol;
}

VariableDeclaration* Parser::parseVariableDeclaration() {
    Type* type = parseType();
    auto name_token = consume(lexer::TokenType::IDENTIFIER, "Expected variable name");
    
    Expression* initializer = nullptr;
    if (match(lexer::TokenType::ASSIGN)) {
        initializer = parseExpression();
    }
    
    return allocate<VariableDeclaration>(getCurrentRange(), type, name_token.text, initializer);
}

Type* Parser::parseType() {
    Type* base_type = parseBasicType();
    
    // Handle pointer, array, and other type modifiers
    while (true) {
        if (check(lexer::TokenType::MULTIPLY)) {
            base_type = parsePointerType(base_type);
        } else if (check(lexer::TokenType::BRACKET_OPEN)) {
            base_type = parseArrayType(base_type);
        } else {
            break;
        }
    }
    
    return base_type;
}

Type* Parser::parseBasicType() {
    // Handle const and volatile qualifiers
    bool is_const = match(lexer::TokenType::KEYWORD_CONST);
    bool is_volatile = match(lexer::TokenType::KEYWORD_VOLATILE);
    
    // Handle signedness for data types
    bool is_signed = false;
    bool is_unsigned = false;
    if (match(lexer::TokenType::KEYWORD_SIGNED)) {
        is_signed = true;
    } else if (match(lexer::TokenType::KEYWORD_UNSIGNED)) {
        is_unsigned = true;
    }
    
    switch (currentToken().type) {
        case lexer::TokenType::KEYWORD_VOID:
            advance();
            return allocate<BuiltinType>(getCurrentRange(), BuiltinType::VOID);
            
        case lexer::TokenType::KEYWORD_AUTO:
            advance();
            return allocate<BuiltinType>(getCurrentRange(), BuiltinType::AUTO);
            
        case lexer::TokenType::KEYWORD_DATA:
            return parseDataType();
            
        case lexer::TokenType::IDENTIFIER: {
            auto name_token = currentToken();
            advance();
            
            // Check for template instantiation
            if (check(lexer::TokenType::LESS_THAN)) {
                return parseTemplateInstantiation(name_token.text);
            }
            
            // Check for qualified name
            if (check(lexer::TokenType::SCOPE_RESOLUTION)) {
                // TODO: Handle qualified types
                return allocate<NamedType>(getCurrentRange(), name_token.text);
            }
            
            return allocate<NamedType>(getCurrentRange(), name_token.text);
        }
        
        default:
            reportError("Expected type");
            return allocate<BuiltinType>(getCurrentRange(), BuiltinType::VOID);
    }
}

Type* Parser::parsePointerType(Type* base_type) {
    consume(lexer::TokenType::MULTIPLY, "Expected '*'");
    
    // Check for const/volatile after *
    bool is_const = match(lexer::TokenType::KEYWORD_CONST);
    bool is_volatile = match(lexer::TokenType::KEYWORD_VOLATILE);
    
    return allocate<PointerType>(getCurrentRange(), base_type, is_const, is_volatile);
}

Type* Parser::parseArrayType(Type* base_type) {
    consume(lexer::TokenType::BRACKET_OPEN, "Expected '['");
    
    Expression* size = nullptr;
    if (!check(lexer::TokenType::BRACKET_CLOSE)) {
        size = parseExpression();
    }
    
    consume(lexer::TokenType::BRACKET_CLOSE, "Expected ']'");
    
    return allocate<ArrayType>(getCurrentRange(), base_type, size);
}

Type* Parser::parseFunctionPointerType(Type* return_type) {
    consume(lexer::TokenType::MULTIPLY, "Expected '*'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after '*'");
    
    auto* func_ptr_type = allocate<FunctionPointerType>(getCurrentRange(), return_type);
    
    if (!check(lexer::TokenType::PAREN_CLOSE)) {
        auto param_types = parseTypeList();
        for (auto* type : param_types) {
            func_ptr_type->addParameterType(type);
        }
    }
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')'");
    
    return func_ptr_type;
}

Type* Parser::parseTemplateInstantiation(std::string_view base_name) {
    consume(lexer::TokenType::LESS_THAN, "Expected '<'");
    
    auto* template_type = allocate<TemplateInstantiationType>(getCurrentRange(), base_name);
    
    auto arguments = parseTemplateArgumentList();
    for (auto* arg : arguments) {
        template_type->addArgument(arg);
    }
    
    consume(lexer::TokenType::GREATER_THAN, "Expected '>'");
    
    return template_type;
}

DataType* Parser::parseDataType() {
    consume(lexer::TokenType::KEYWORD_DATA, "Expected 'data'");
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after 'data'");
    
    auto bit_width_token = consume(lexer::TokenType::INTEGER_LITERAL, "Expected bit width");
    int bit_width = static_cast<int>(bit_width_token.integer_value);
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after bit width");
    
    // TODO: Handle signedness from earlier parsing
    return allocate<DataType>(getCurrentRange(), bit_width, false);
}

std::vector<Type*> Parser::parseTypeList() {
    std::vector<Type*> types;
    
    do {
        types.push_back(parseType());
    } while (match(lexer::TokenType::COMMA));
    
    return types;
}

Statement* Parser::parseStatement() {
    switch (currentToken().type) {
        case lexer::TokenType::BRACE_OPEN:
            return parseCompoundStatement();
        case lexer::TokenType::KEYWORD_IF:
            return parseIfStatement();
        case lexer::TokenType::KEYWORD_WHILE:
            return parseWhileStatement();
        case lexer::TokenType::KEYWORD_FOR:
            return parseForStatement();
        case lexer::TokenType::KEYWORD_SWITCH:
            return parseSwitchStatement();
        case lexer::TokenType::KEYWORD_RETURN:
            return parseReturnStatement();
        case lexer::TokenType::KEYWORD_BREAK:
            return parseBreakStatement();
        case lexer::TokenType::KEYWORD_CONTINUE:
            return parseContinueStatement();
        case lexer::TokenType::KEYWORD_ASM:
            return parseAssemblyStatement();
        case lexer::TokenType::KEYWORD_TRY:
            return parseTryStatement();
        case lexer::TokenType::KEYWORD_THROW:
            return parseThrowStatement();
        case lexer::TokenType::KEYWORD_ASSERT:
            return parseAssertStatement();
        default:
            // Check for variable declaration or expression statement
            if (check(lexer::TokenType::IDENTIFIER) && peekToken().type == lexer::TokenType::IDENTIFIER) {
                // This looks like a variable declaration
                auto* var_decl = parseVariableDeclaration();
                consume(lexer::TokenType::SEMICOLON, "Expected ';' after variable declaration");
                // For now, wrap in expression statement (TODO: add VariableDeclarationStatement)
                return allocate<ExpressionStatement>(getCurrentRange(), nullptr);
            } else {
                return parseExpressionStatement();
            }
    }
}

CompoundStatement* Parser::parseCompoundStatement() {
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{'");
    
    auto* compound = allocate<CompoundStatement>(getCurrentRange());
    
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        Statement* stmt = parseStatement();
        if (stmt) {
            compound->addStatement(stmt);
        }
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}'");
    
    return compound;
}

ExpressionStatement* Parser::parseExpressionStatement() {
    Expression* expr = nullptr;
    
    if (!check(lexer::TokenType::SEMICOLON)) {
        expr = parseExpression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after expression");
    
    return allocate<ExpressionStatement>(getCurrentRange(), expr);
}

IfStatement* Parser::parseIfStatement() {
    consume(lexer::TokenType::KEYWORD_IF, "Expected 'if'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'if'");
    
    Expression* condition = parseExpression();
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after if condition");
    
    Statement* then_stmt = parseStatement();
    
    Statement* else_stmt = nullptr;
    if (match(lexer::TokenType::KEYWORD_ELSE)) {
        else_stmt = parseStatement();
    }
    
    return allocate<IfStatement>(getCurrentRange(), condition, then_stmt, else_stmt);
}

WhileStatement* Parser::parseWhileStatement() {
    consume(lexer::TokenType::KEYWORD_WHILE, "Expected 'while'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'while'");
    
    Expression* condition = parseExpression();
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after while condition");
    
    Statement* body = parseStatement();
    
    return allocate<WhileStatement>(getCurrentRange(), condition, body);
}

ForStatement* Parser::parseForStatement() {
    consume(lexer::TokenType::KEYWORD_FOR, "Expected 'for'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'for'");
    
    // Check for range-based for loop
    if (check(lexer::TokenType::IDENTIFIER)) {
        auto var_token = currentToken();
        advance();
        
        // Check for index variable (var, index in ...)
        std::optional<std::string_view> index_var;
        if (match(lexer::TokenType::COMMA)) {
            index_var = var_token.text;
            var_token = consume(lexer::TokenType::IDENTIFIER, "Expected index variable name");
        }
        
        if (match(lexer::TokenType::KEYWORD_IN)) {
            Expression* range_expr = parseExpression();
            consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after for range");
            
            Statement* body = parseStatement();
            
            return allocate<ForStatement>(getCurrentRange(), var_token.text, range_expr, body, index_var);
        } else {
            // This is not a range-based for loop, backtrack
            // TODO: Implement proper backtracking
            reportError("Invalid for loop syntax");
            return nullptr;
        }
    }
    
    // Traditional for loop
    Statement* init = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        if (check(lexer::TokenType::IDENTIFIER) && peekToken().type == lexer::TokenType::IDENTIFIER) {
            // Variable declaration
            auto* var_decl = parseVariableDeclaration();
            // TODO: Wrap in declaration statement
            init = allocate<ExpressionStatement>(getCurrentRange(), nullptr);
        } else {
            init = parseExpressionStatement();
        }
    } else {
        consume(lexer::TokenType::SEMICOLON, "Expected ';'");
    }
    
    Expression* condition = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after for condition");
    
    Expression* increment = nullptr;
    if (!check(lexer::TokenType::PAREN_CLOSE)) {
        increment = parseExpression();
    }
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after for increment");
    
    Statement* body = parseStatement();
    
    return allocate<ForStatement>(getCurrentRange(), init, condition, increment, body);
}

SwitchStatement* Parser::parseSwitchStatement() {
    consume(lexer::TokenType::KEYWORD_SWITCH, "Expected 'switch'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'switch'");
    
    Expression* expr = parseExpression();
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after switch expression");
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after switch");
    
    auto* switch_stmt = allocate<SwitchStatement>(getCurrentRange(), expr);
    
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        CaseStatement* case_stmt = parseCaseStatement();
        if (case_stmt) {
            switch_stmt->addCase(case_stmt);
        }
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after switch body");
    
    return switch_stmt;
}

CaseStatement* Parser::parseCaseStatement() {
    consume(lexer::TokenType::KEYWORD_CASE, "Expected 'case'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'case'");
    
    Expression* value = nullptr;
    bool is_default = false;
    
    if (match(lexer::TokenType::KEYWORD_DEFAULT)) {
        is_default = true;
    } else {
        value = parseExpression();
    }
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after case value");
    
    Statement* body = parseCompoundStatement();
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after case statement");
    
    return allocate<CaseStatement>(getCurrentRange(), value, body, is_default);
}

ReturnStatement* Parser::parseReturnStatement() {
    consume(lexer::TokenType::KEYWORD_RETURN, "Expected 'return'");
    
    Expression* value = nullptr;
    if (!check(lexer::TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after return statement");
    
    return allocate<ReturnStatement>(getCurrentRange(), value);
}

BreakStatement* Parser::parseBreakStatement() {
    consume(lexer::TokenType::KEYWORD_BREAK, "Expected 'break'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after break");
    
    return allocate<BreakStatement>(getCurrentRange());
}

ContinueStatement* Parser::parseContinueStatement() {
    consume(lexer::TokenType::KEYWORD_CONTINUE, "Expected 'continue'");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after continue");
    
    return allocate<ContinueStatement>(getCurrentRange());
}

AssemblyStatement* Parser::parseAssemblyStatement() {
    consume(lexer::TokenType::KEYWORD_ASM, "Expected 'asm'");
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{' after 'asm'");
    
    // Parse assembly code (everything until closing brace)
    std::string assembly_code;
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        assembly_code += std::string(currentToken().text);
        if (currentToken().type == lexer::TokenType::NEWLINE) {
            assembly_code += "\n";
        } else {
            assembly_code += " ";
        }
        advance();
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after assembly code");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after assembly statement");
    
    return allocate<AssemblyStatement>(getCurrentRange(), assembly_code);
}

TryStatement* Parser::parseTryStatement() {
    consume(lexer::TokenType::KEYWORD_TRY, "Expected 'try'");
    
    Statement* try_block = parseCompoundStatement();
    
    consume(lexer::TokenType::KEYWORD_CATCH, "Expected 'catch' after try block");
    
    Statement* catch_block = parseCompoundStatement();
    
    return allocate<TryStatement>(getCurrentRange(), try_block, catch_block);
}

ThrowStatement* Parser::parseThrowStatement() {
    consume(lexer::TokenType::KEYWORD_THROW, "Expected 'throw'");
    
    Expression* expr = parseExpression();
    
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after throw statement");
    
    return allocate<ThrowStatement>(getCurrentRange(), expr);
}

AssertStatement* Parser::parseAssertStatement() {
    consume(lexer::TokenType::KEYWORD_ASSERT, "Expected 'assert'");
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'assert'");
    
    Expression* condition = parseExpression();
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after assert condition");
    consume(lexer::TokenType::SEMICOLON, "Expected ';' after assert statement");
    
    return allocate<AssertStatement>(getCurrentRange(), condition);
}

Statement* Parser::parseAnonymousBlock() {
    return parseCompoundStatement();
}

std::vector<Statement*> Parser::parseStatementList() {
    std::vector<Statement*> statements;
    
    while (!check(lexer::TokenType::BRACE_CLOSE) && !isAtEnd()) {
        Statement* stmt = parseStatement();
        if (stmt) {
            statements.push_back(stmt);
        }
    }
    
    return statements;
}

Expression* Parser::parseExpression() {
    return parseAssignmentExpression();
}

Expression* Parser::parseAssignmentExpression() {
    Expression* expr = parseConditionalExpression();
    
    if (isAssignmentOperator(currentToken().type)) {
        lexer::TokenType op = currentToken().type;
        advance();
        Expression* right = parseAssignmentExpression();
        return allocate<AssignmentExpression>(getCurrentRange(), op, expr, right);
    }
    
    return expr;
}

Expression* Parser::parseConditionalExpression() {
    Expression* expr = parseBinaryExpression(0);
    
    if (match(lexer::TokenType::CONDITIONAL)) {
        Expression* true_expr = parseExpression();
        consume(lexer::TokenType::COLON, "Expected ':' after ternary true expression");
        Expression* false_expr = parseConditionalExpression();
        return allocate<ConditionalExpression>(getCurrentRange(), expr, true_expr, false_expr);
    }
    
    return expr;
}

Expression* Parser::parseBinaryExpression(int min_precedence) {
    Expression* left = parseUnaryExpression();
    
    while (isBinaryOperator(currentToken().type)) {
        lexer::TokenType op = currentToken().type;
        int precedence = getOperatorPrecedence(op);
        
        if (precedence < min_precedence) {
            break;
        }
        
        advance();
        
        int next_min_prec = isRightAssociative(op) ? precedence : precedence + 1;
        Expression* right = parseBinaryExpression(next_min_prec);
        
        left = allocate<BinaryExpression>(getCurrentRange(), op, left, right);
    }
    
    return left;
}

Expression* Parser::parseUnaryExpression() {
    if (isUnaryOperator(currentToken().type)) {
        lexer::TokenType op = currentToken().type;
        advance();
        Expression* operand = parseUnaryExpression();
        return allocate<UnaryExpression>(getCurrentRange(), op, operand, UnaryExpression::PREFIX);
    }
    
    if (match(lexer::TokenType::KEYWORD_SIZEOF)) {
        return parseSizeofExpression();
    }
    
    if (match(lexer::TokenType::KEYWORD_TYPEOF)) {
        return parseTypeofExpression();
    }
    
    return parsePostfixExpression();
}

Expression* Parser::parsePostfixExpression() {
    Expression* expr = parsePrimaryExpression();
    
    while (true) {
        if (match(lexer::TokenType::BRACKET_OPEN)) {
            Expression* index = parseExpression();
            consume(lexer::TokenType::BRACKET_CLOSE, "Expected ']' after array subscript");
            expr = allocate<SubscriptExpression>(getCurrentRange(), expr, index);
        } else if (match(lexer::TokenType::PAREN_OPEN)) {
            expr = parseCallExpression(expr);
        } else if (match(lexer::TokenType::MEMBER_ACCESS)) {
            auto member_token = consume(lexer::TokenType::IDENTIFIER, "Expected member name after '.'");
            expr = allocate<MemberExpression>(getCurrentRange(), expr, member_token.text);
        } else if (match(lexer::TokenType::SCOPE_RESOLUTION)) {
            auto member_token = consume(lexer::TokenType::IDENTIFIER, "Expected member name after '::'");
            expr = allocate<MemberExpression>(getCurrentRange(), expr, member_token.text);
        } else if (match({lexer::TokenType::INCREMENT, lexer::TokenType::DECREMENT})) {
            lexer::TokenType op = tokens_[current_token_index_ - 1].type;
            expr = allocate<UnaryExpression>(getCurrentRange(), op, expr, UnaryExpression::POSTFIX);
        } else {
            break;
        }
    }
    
    return expr;
}

Expression* Parser::parsePrimaryExpression() {
    // Handle parenthesized expressions
    if (match(lexer::TokenType::PAREN_OPEN)) {
        // Check for cast expression
        if (check(lexer::TokenType::IDENTIFIER) || check(lexer::TokenType::KEYWORD_DATA) ||
            check(lexer::TokenType::KEYWORD_VOID) || check(lexer::TokenType::KEYWORD_AUTO)) {
            // This might be a cast, try to parse type
            size_t saved_pos = current_token_index_;
            try {
                Type* type = parseType();
                if (match(lexer::TokenType::PAREN_CLOSE)) {
                    Expression* expr = parseUnaryExpression();
                    return allocate<CastExpression>(getCurrentRange(), type, expr);
                }
            } catch (...) {
                // Not a cast, restore position
                current_token_index_ = saved_pos;
            }
        }
        
        // Regular parenthesized expression
        Expression* expr = parseExpression();
        consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after expression");
        return expr;
    }
    
    // Handle literals
    if (currentToken().isLiteral()) {
        return parseLiteral();
    }
    
    // Handle identifiers
    if (check(lexer::TokenType::IDENTIFIER)) {
        auto name_token = currentToken();
        advance();
        return allocate<IdentifierExpression>(getCurrentRange(), name_token.text);
    }
    
    // Handle array literals
    if (check(lexer::TokenType::BRACKET_OPEN)) {
        return parseArrayLiteral();
    }
    
    // Handle dictionary literals
    if (check(lexer::TokenType::BRACE_OPEN)) {
        // Could be dictionary literal or anonymous function
        // For now, assume dictionary literal
        return parseDictionaryLiteral();
    }
    
    // Handle i-string literals
    if (check(lexer::TokenType::I_STRING_LITERAL)) {
        return parseIStringLiteral();
    }
    
    reportError("Expected expression");
    return allocate<IntegerLiteral>(getCurrentRange(), 0);
}

CallExpression* Parser::parseCallExpression(Expression* function) {
    auto* call_expr = allocate<CallExpression>(getCurrentRange(), function);
    
    if (!check(lexer::TokenType::PAREN_CLOSE)) {
        auto arguments = parseArgumentList();
        for (auto* arg : arguments) {
            call_expr->addArgument(arg);
        }
    }
    
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after function arguments");
    
    return call_expr;
}

MemberExpression* Parser::parseMemberExpression(Expression* object) {
    auto member_token = consume(lexer::TokenType::IDENTIFIER, "Expected member name");
    return allocate<MemberExpression>(getCurrentRange(), object, member_token.text);
}

SubscriptExpression* Parser::parseSubscriptExpression(Expression* array) {
    Expression* index = parseExpression();
    consume(lexer::TokenType::BRACKET_CLOSE, "Expected ']' after array index");
    return allocate<SubscriptExpression>(getCurrentRange(), array, index);
}

CastExpression* Parser::parseCastExpression() {
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' before cast type");
    Type* type = parseType();
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after cast type");
    Expression* expr = parseUnaryExpression();
    return allocate<CastExpression>(getCurrentRange(), type, expr);
}

SizeofExpression* Parser::parseSizeofExpression() {
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'sizeof'");
    Type* type = parseType();
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after sizeof type");
    return allocate<SizeofExpression>(getCurrentRange(), type);
}

TypeofExpression* Parser::parseTypeofExpression() {
    consume(lexer::TokenType::PAREN_OPEN, "Expected '(' after 'typeof'");
    Expression* expr = parseExpression();
    consume(lexer::TokenType::PAREN_CLOSE, "Expected ')' after typeof expression");
    return allocate<TypeofExpression>(getCurrentRange(), expr);
}

Expression* Parser::parseLiteral() {
    switch (currentToken().type) {
        case lexer::TokenType::INTEGER_LITERAL:
            return parseIntegerLiteral();
        case lexer::TokenType::FLOAT_LITERAL:
            return parseFloatLiteral();
        case lexer::TokenType::STRING_LITERAL:
            return parseStringLiteral();
        case lexer::TokenType::CHARACTER_LITERAL:
            return parseCharacterLiteral();
        case lexer::TokenType::DATA_LITERAL:
            return parseDataLiteral();
        case lexer::TokenType::I_STRING_LITERAL:
            return parseIStringLiteral();
        default:
            reportError("Expected literal");
            return allocate<IntegerLiteral>(getCurrentRange(), 0);
    }
}

IntegerLiteral* Parser::parseIntegerLiteral() {
    auto token = consume(lexer::TokenType::INTEGER_LITERAL, "Expected integer literal");
    return allocate<IntegerLiteral>(getCurrentRange(), token.integer_value);
}

FloatLiteral* Parser::parseFloatLiteral() {
    auto token = consume(lexer::TokenType::FLOAT_LITERAL, "Expected float literal");
    return allocate<FloatLiteral>(getCurrentRange(), token.float_value);
}

StringLiteral* Parser::parseStringLiteral() {
    auto token = consume(lexer::TokenType::STRING_LITERAL, "Expected string literal");
    std::string_view value = token.processed_text.empty() ? token.text : token.processed_text;
    return allocate<StringLiteral>(getCurrentRange(), value);
}

CharacterLiteral* Parser::parseCharacterLiteral() {
    auto token = consume(lexer::TokenType::CHARACTER_LITERAL, "Expected character literal");
    char value = token.processed_text.empty() ? 
                 (token.text.empty() ? '\0' : token.text[0]) : 
                 (token.processed_text.empty() ? '\0' : token.processed_text[0]);
    return allocate<CharacterLiteral>(getCurrentRange(), value);
}

DataLiteral* Parser::parseDataLiteral() {
    auto token = consume(lexer::TokenType::DATA_LITERAL, "Expected data literal");
    std::string_view bit_data = token.processed_text.empty() ? token.text : token.processed_text;
    return allocate<DataLiteral>(getCurrentRange(), bit_data);
}

ArrayLiteral* Parser::parseArrayLiteral() {
    consume(lexer::TokenType::BRACKET_OPEN, "Expected '['");
    
    auto* array_literal = allocate<ArrayLiteral>(getCurrentRange());
    
    if (!check(lexer::TokenType::BRACKET_CLOSE)) {
        // Check for array comprehension
        // This is a simplified check - in a real parser you'd need more lookahead
        if (check(lexer::TokenType::IDENTIFIER)) {
            // Could be array comprehension, but for now just parse as regular array
            auto elements = parseExpressionList();
            for (auto* element : elements) {
                array_literal->addElement(element);
            }
        } else {
            auto elements = parseExpressionList();
            for (auto* element : elements) {
                array_literal->addElement(element);
            }
        }
    }
    
    consume(lexer::TokenType::BRACKET_CLOSE, "Expected ']' after array elements");
    
    return array_literal;
}

DictionaryLiteral* Parser::parseDictionaryLiteral() {
    consume(lexer::TokenType::BRACE_OPEN, "Expected '{'");
    
    auto* dict_literal = allocate<DictionaryLiteral>(getCurrentRange());
    
    if (!check(lexer::TokenType::BRACE_CLOSE)) {
        do {
            Expression* key = parseExpression();
            consume(lexer::TokenType::COLON, "Expected ':' after dictionary key");
            Expression* value = parseExpression();
            dict_literal->addPair(key, value);
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::BRACE_CLOSE, "Expected '}' after dictionary elements");
    
    return dict_literal;
}

IStringLiteral* Parser::parseIStringLiteral() {
    auto token = consume(lexer::TokenType::I_STRING_LITERAL, "Expected i-string literal");
    
    std::string_view format = token.processed_text.empty() ? token.text : token.processed_text;
    auto* istring = allocate<IStringLiteral>(getCurrentRange(), format);
    
    // The tokenizer should have already parsed the expressions, but for now
    // we'll leave the expressions empty and handle i-string parsing later
    
    return istring;
}

ArrayComprehension* Parser::parseArrayComprehension() {
    // This is a placeholder implementation
    // Array comprehensions need more complex parsing logic
    reportError("Array comprehensions not yet implemented");
    return nullptr;
}

AnonymousFunction* Parser::parseAnonymousFunction() {
    Statement* body = parseCompoundStatement();
    return allocate<AnonymousFunction>(getCurrentRange(), body);
}

std::vector<Expression*> Parser::parseExpressionList() {
    std::vector<Expression*> expressions;
    
    do {
        expressions.push_back(parseExpression());
    } while (match(lexer::TokenType::COMMA));
    
    return expressions;
}

std::vector<Expression*> Parser::parseArgumentList() {
    return parseExpressionList();
}

std::string_view Parser::parseQualifiedIdentifier() {
    // For now, just parse a simple identifier
    // TODO: Handle qualified names like namespace::identifier
    auto token = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
    return token.text;
}

std::vector<std::string_view> Parser::parseIdentifierList() {
    std::vector<std::string_view> identifiers;
    
    do {
        auto token = consume(lexer::TokenType::IDENTIFIER, "Expected identifier");
        identifiers.push_back(token.text);
    } while (match(lexer::TokenType::COMMA));
    
    return identifiers;
}

int Parser::getOperatorPrecedence(lexer::TokenType type) const {
    switch (type) {
        case lexer::TokenType::COMMA: return 1;
        
        case lexer::TokenType::LOGICAL_OR: return 4;
        case lexer::TokenType::LOGICAL_AND: return 5;
        
        case lexer::TokenType::BITWISE_OR: return 6;
        case lexer::TokenType::BITWISE_XOR: return 7;
        case lexer::TokenType::BITWISE_AND: return 8;
        
        case lexer::TokenType::EQUAL:
        case lexer::TokenType::NOT_EQUAL: return 9;
        
        case lexer::TokenType::LESS_THAN:
        case lexer::TokenType::LESS_EQUAL:
        case lexer::TokenType::GREATER_THAN:
        case lexer::TokenType::GREATER_EQUAL: return 10;
        
        case lexer::TokenType::SHIFT_LEFT:
        case lexer::TokenType::SHIFT_RIGHT: return 11;
        
        case lexer::TokenType::PLUS:
        case lexer::TokenType::MINUS: return 12;
        
        case lexer::TokenType::MULTIPLY:
        case lexer::TokenType::DIVIDE:
        case lexer::TokenType::MODULO: return 13;
        
        case lexer::TokenType::POWER:
        case lexer::TokenType::EXPONENT: return 14;
        
        default: return 0;
    }
}

bool Parser::isRightAssociative(lexer::TokenType type) const {
    switch (type) {
        case lexer::TokenType::POWER:
        case lexer::TokenType::EXPONENT:
            return true;
        default:
            return false;
    }
}

bool Parser::isBinaryOperator(lexer::TokenType type) const {
    return getOperatorPrecedence(type) > 0;
}

bool Parser::isUnaryOperator(lexer::TokenType type) const {
    switch (type) {
        case lexer::TokenType::PLUS:
        case lexer::TokenType::MINUS:
        case lexer::TokenType::LOGICAL_NOT:
        case lexer::TokenType::BITWISE_NOT:
        case lexer::TokenType::ADDRESS_OF:
        case lexer::TokenType::MULTIPLY: // dereference
        case lexer::TokenType::INCREMENT:
        case lexer::TokenType::DECREMENT:
            return true;
        default:
            return false;
    }
}

bool Parser::isAssignmentOperator(lexer::TokenType type) const {
    switch (type) {
        case lexer::TokenType::ASSIGN:
        case lexer::TokenType::ASSIGN_ADD:
        case lexer::TokenType::ASSIGN_SUB:
        case lexer::TokenType::ASSIGN_MUL:
        case lexer::TokenType::ASSIGN_DIV:
        case lexer::TokenType::ASSIGN_MOD:
        case lexer::TokenType::ASSIGN_AND:
        case lexer::TokenType::ASSIGN_OR:
        case lexer::TokenType::ASSIGN_XOR:
        case lexer::TokenType::ASSIGN_SHL:
        case lexer::TokenType::ASSIGN_SHR:
        case lexer::TokenType::ASSIGN_POW:
        case lexer::TokenType::ASSIGN_EXP:
            return true;
        default:
            return false;
    }
}

std::vector<Type*> Parser::parseTemplateArgumentList() {
    std::vector<Type*> arguments;
    
    do {
        arguments.push_back(parseTemplateArgument());
    } while (match(lexer::TokenType::COMMA));
    
    return arguments;
}

Type* Parser::parseTemplateArgument() {
    // For now, template arguments are just types
    // Later we may need to handle expression arguments as well
    return parseType();
}

} // namespace parser
} // namespace flux