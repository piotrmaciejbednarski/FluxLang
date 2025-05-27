#pragma once

#include "ast.h"
#include "../lexer/tokenizer.h"
#include "../lexer/token.h"
#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace flux {
namespace parser {

// Parser for the Flux programming language
class Parser {
public:
    // Constructor
    Parser(std::shared_ptr<common::Source> source, 
           common::Arena& arena = common::Arena::defaultArena());
    
    // Destructor
    ~Parser();
    
    // Parse the entire program
    std::unique_ptr<Program> parseProgram();
    
    // Get error collector
    const common::ErrorCollector& getErrors() const { return errors_; }
    
    // Check if there were any parsing errors
    bool hasErrors() const { return errors_.hasErrors(); }

private:
    std::shared_ptr<common::Source> source_;
    common::Arena& arena_;
    common::ErrorCollector errors_;
    lexer::Tokenizer tokenizer_;
    
    // Current token and position
    std::vector<lexer::Token> tokens_;
    size_t current_token_index_;
    
    // Token management
    void tokenizeSource();
    const lexer::Token& currentToken() const;
    const lexer::Token& peekToken(size_t offset = 1) const;
    void advance();
    bool isAtEnd() const;
    bool check(lexer::TokenType type) const;
    bool match(lexer::TokenType type);
    bool match(std::initializer_list<lexer::TokenType> types);
    
    // Error handling and synchronization
    void reportError(const std::string& message);
    void reportError(const std::string& message, const lexer::Token& token);
    void synchronize();
    lexer::Token consume(lexer::TokenType type, const std::string& message);
    
    // Source range helpers
    common::SourceRange makeRange(const lexer::Token& start, const lexer::Token& end) const;
    common::SourceRange makeRange(size_t start_token_index, size_t end_token_index) const;
    common::SourceRange getCurrentRange() const;
    
    // Top-level parsing
    std::vector<Declaration*> parseGlobalDeclarations();
    Declaration* parseGlobalDeclaration();
    
    // Import and using statements
    ImportDeclaration* parseImportDeclaration();
    UsingDeclaration* parseUsingDeclaration();
    
    // Namespace declarations
    NamespaceDeclaration* parseNamespaceDeclaration();
    
    // Object declarations
    ObjectDeclaration* parseObjectDeclaration();
    std::vector<Declaration*> parseObjectMembers();
    Declaration* parseObjectMember();
    
    // Struct declarations
    StructDeclaration* parseStructDeclaration();
    std::vector<Declaration*> parseStructMembers();
    Declaration* parseStructMember();
    
    // Function declarations
    FunctionDeclaration* parseFunctionDeclaration();
    ParameterList* parseParameterList();
    Parameter* parseParameter();
    
    // Template declarations
    TemplateDeclaration* parseTemplateDeclaration();
    TemplateParameterList* parseTemplateParameterList();
    TemplateParameter* parseTemplateParameter();
    
    // Operator declarations
    OperatorDeclaration* parseOperatorDeclaration();
    std::string parseOperatorSymbol();
    
    // Variable declarations
    VariableDeclaration* parseVariableDeclaration();
    
    // Type parsing
    Type* parseType();
    Type* parseBasicType();
    Type* parsePointerType(Type* base_type);
    Type* parseArrayType(Type* base_type);
    Type* parseFunctionPointerType(Type* return_type);
    Type* parseTemplateInstantiation(std::string_view base_name);
    DataType* parseDataType();
    std::vector<Type*> parseTypeList();
    
    // Statement parsing
    Statement* parseStatement();
    CompoundStatement* parseCompoundStatement();
    ExpressionStatement* parseExpressionStatement();
    IfStatement* parseIfStatement();
    WhileStatement* parseWhileStatement();
    ForStatement* parseForStatement();
    SwitchStatement* parseSwitchStatement();
    CaseStatement* parseCaseStatement();
    ReturnStatement* parseReturnStatement();
    BreakStatement* parseBreakStatement();
    ContinueStatement* parseContinueStatement();
    AssemblyStatement* parseAssemblyStatement();
    TryStatement* parseTryStatement();
    ThrowStatement* parseThrowStatement();
    AssertStatement* parseAssertStatement();
    Statement* parseAnonymousBlock();
    std::vector<Statement*> parseStatementList();
    
    // Expression parsing (precedence climbing)
    Expression* parseExpression();
    Expression* parseAssignmentExpression();
    Expression* parseConditionalExpression();
    Expression* parseBinaryExpression(int min_precedence);
    Expression* parseUnaryExpression();
    Expression* parsePostfixExpression();
    Expression* parsePrimaryExpression();
    
    // Specific expression types
    CallExpression* parseCallExpression(Expression* function);
    MemberExpression* parseMemberExpression(Expression* object);
    SubscriptExpression* parseSubscriptExpression(Expression* array);
    CastExpression* parseCastExpression();
    SizeofExpression* parseSizeofExpression();
    TypeofExpression* parseTypeofExpression();
    
    // Literal parsing
    Expression* parseLiteral();
    IntegerLiteral* parseIntegerLiteral();
    FloatLiteral* parseFloatLiteral();
    StringLiteral* parseStringLiteral();
    CharacterLiteral* parseCharacterLiteral();
    DataLiteral* parseDataLiteral();
    ArrayLiteral* parseArrayLiteral();
    DictionaryLiteral* parseDictionaryLiteral();
    IStringLiteral* parseIStringLiteral();
    ArrayComprehension* parseArrayComprehension();
    AnonymousFunction* parseAnonymousFunction();
    
    // Helper functions
    std::vector<Expression*> parseExpressionList();
    std::vector<Expression*> parseArgumentList();
    std::string_view parseQualifiedIdentifier();
    std::vector<std::string_view> parseIdentifierList();
    
    // Operator precedence and associativity
    int getOperatorPrecedence(lexer::TokenType type) const;
    bool isRightAssociative(lexer::TokenType type) const;
    bool isBinaryOperator(lexer::TokenType type) const;
    bool isUnaryOperator(lexer::TokenType type) const;
    bool isAssignmentOperator(lexer::TokenType type) const;
    
    // Template argument parsing
    std::vector<Type*> parseTemplateArgumentList();
    Type* parseTemplateArgument();
    
    // Memory management helpers
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        return arena_.alloc<T>(std::forward<Args>(args)...);
    }
};

} // namespace parser
} // namespace flux