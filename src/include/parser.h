#ifndef FLUX_PARSER_H
#define FLUX_PARSER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "lexer.h"
#include "ast.h"
#include "error.h"

namespace flux {

// Forward declarations
class SymbolTable;
class TypeRegistry;

class Parser {
private:
    // Input tokens and current position
    std::vector<Token> tokens;
    size_t current;
    std::string_view filename;
    
    // Symbol tables and type system
    std::shared_ptr<SymbolTable> currentScope;
    std::shared_ptr<TypeRegistry> typeRegistry;
    
    // Track current namespace and class for scoping
    std::string currentNamespace;
    std::string currentClass;
    
    // Error handling and recovery
    bool panicMode;
    bool hadError;
    std::vector<TokenType> synchronizationPoints;
    
    // Helper methods for parsing
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::initializer_list<TokenType>& types);
    bool isAtEnd() const;
    
    // Expect a token of a specific type, or report an error
    Token consume(TokenType type, const std::string& message);
    
    // Synchronize after an error to continue parsing
    void synchronize();
    
    // Error reporting
    void error(const std::string& message);
    void error(const Token& token, const std::string& message);
    void errorAt(const Token& token, const std::string& message);

    // Lexer
    std::string trimLexeme(std::string_view lexeme);
    
    // AST
    AstLocation makeLocation(int startLine, int startColumn, int endLine, int endColumn);
    AstLocation makeLocation(const Token& start, const Token& end);
    
    // Parse declarations
    std::shared_ptr<Program> parseProgram();
    std::shared_ptr<NamespaceDeclaration> parseNamespace();
    std::shared_ptr<ClassDeclaration> parseClass();
    std::shared_ptr<StructDeclaration> parseStruct();
    std::shared_ptr<ObjectDeclaration> parseObject();
    std::shared_ptr<FunctionDeclaration> parseFunction(std::shared_ptr<Type> optionalReturnType = nullptr);
    std::shared_ptr<ImportDeclaration> parseImport();
    std::shared_ptr<UnionDeclaration> parseUnion();
    std::shared_ptr<TypedefDeclaration> parseTypedef();
    
    // Parse statements
    std::shared_ptr<Statement> parseStatement();
    std::shared_ptr<BlockStatement> parseBlock();
    std::shared_ptr<ExpressionStatement> parseExpressionStatement();
    std::shared_ptr<IfStatement> parseIfStatement();
    std::shared_ptr<WhileStatement> parseWhileStatement();
    std::shared_ptr<ForStatement> parseForStatement();
    std::shared_ptr<ReturnStatement> parseReturnStatement();
    std::shared_ptr<BreakStatement> parseBreakStatement();
    std::shared_ptr<ContinueStatement> parseContinueStatement();
    std::shared_ptr<ThrowStatement> parseThrowStatement();
    std::shared_ptr<TryCatchStatement> parseTryCatchStatement();
    std::shared_ptr<VariableDeclaration> parseVariableDeclaration();
    std::shared_ptr<AsmStatement> parseAsmStatement();
    
    // Parse expressions with precedence climbing
    std::shared_ptr<Expression> parseExpression();
    std::shared_ptr<Expression> parseAssignment();
    std::shared_ptr<Expression> parseLogicalOr();
    std::shared_ptr<Expression> parseLogicalAnd();
    std::shared_ptr<Expression> parseBitwiseOr();
    std::shared_ptr<Expression> parseBitwiseXor();
    std::shared_ptr<Expression> parseBitwiseAnd();
    std::shared_ptr<Expression> parseEquality();
    std::shared_ptr<Expression> parseComparison();
    std::shared_ptr<Expression> parseShift();
    std::shared_ptr<Expression> parseTerm();
    std::shared_ptr<Expression> parseFactor();
    std::shared_ptr<Expression> parseExponent();
    std::shared_ptr<Expression> parseUnary();
    std::shared_ptr<Expression> parsePostfix();
    std::shared_ptr<Expression> parsePrimary();
    
    // Parse type specifications
    std::shared_ptr<Type> parseType();
    std::shared_ptr<PrimitiveType> parsePrimitiveType();
    std::shared_ptr<PointerType> parsePointerType();
    std::shared_ptr<StructType> parseStructType();
    std::shared_ptr<UnionType> parseUnionType();
    std::shared_ptr<FunctionType> parseFunctionType();
    int parseBitWidth();  // For int{32}, float{64}, etc.
    
    // Parse helper methods
    std::vector<Parameter> parseParameterList();
    std::vector<StructField> parseFieldList();
    std::shared_ptr<Expression> finishCall(std::shared_ptr<Expression> callee);
    std::shared_ptr<Expression> finishIndexAccess(std::shared_ptr<Expression> array);
    std::shared_ptr<Expression> finishMemberAccess(std::shared_ptr<Expression> object);
    std::shared_ptr<Expression> finishArrowAccess(std::shared_ptr<Expression> pointer);
    std::shared_ptr<Expression> parseLiteral();
    std::shared_ptr<Expression> parseIdentifier();
    std::shared_ptr<Expression> parseArrayLiteral();
    std::shared_ptr<Expression> parseStringInterpolation();
    
    // Scope and symbol management
    void beginScope();
    void endScope();
    void enterNamespace(const std::string& name);
    void exitNamespace();
    void enterClass(const std::string& name);
    void exitClass();
    
    // Type checking and inference
    std::shared_ptr<Type> inferType(const std::shared_ptr<Expression>& expr);
    bool checkTypes(const std::shared_ptr<Type>& expected, const std::shared_ptr<Type>& actual);
    
    // Variable handling methods
    void registerGlobalVariable(const std::shared_ptr<VariableDeclaration>& varDecl);
    bool checkVariableInitialization(const std::shared_ptr<VariableDeclaration>& varDecl);
    std::shared_ptr<VariableDeclaration> processGlobalVariable(
        const std::string& name, 
        std::shared_ptr<Type> type,
        std::shared_ptr<Expression> initializer,
        const AstLocation& location);
    
public:
    Parser();
    ~Parser();
    
    // Parse tokens into an AST
    std::shared_ptr<Program> parse(const std::vector<Token>& tokens, std::string_view filename);
    
    // Reset the parser state for reuse
    void reset();
    
    // Access parsing results
    bool hasError() const { return hadError; }
    std::shared_ptr<SymbolTable> getGlobalScope() const;
    std::shared_ptr<TypeRegistry> getTypeRegistry() const { return typeRegistry; }
};

// Helper class for managing symbols
// Helper class for managing symbols
class SymbolTable : public std::enable_shared_from_this<SymbolTable> {
public:
    enum class SymbolKind {
        VARIABLE,
        FUNCTION,
        CLASS,
        STRUCT,
        OBJECT,
        NAMESPACE,
        TYPEDEF,
        PARAMETER,
        GLOBAL_VARIABLE
    };
    
    struct Symbol {
        std::string name;
        SymbolKind kind;
        std::shared_ptr<Type> type;
        bool isDefined;
        AstLocation location;
        
        // For variables/parameters
        bool isConst;
        
        // For functions
        std::vector<Parameter> parameters;
    };
    
private:
    std::unordered_map<std::string, Symbol> symbols;
    std::shared_ptr<SymbolTable> parent;
    
public:
    SymbolTable() : parent(nullptr) {}
    explicit SymbolTable(std::shared_ptr<SymbolTable> parent) : parent(parent) {}
    
    // Symbol management
    bool define(const Symbol& symbol);
    bool declare(const Symbol& symbol);
    Symbol* lookup(const std::string& name);
    Symbol* lookupLocal(const std::string& name);
    
    // Check if a symbol is global
    bool isGlobal(const std::string& name) {
        Symbol* symbol = lookup(name);
        return symbol && symbol->kind == SymbolKind::GLOBAL_VARIABLE;
    }
    
    // Define a global variable
    bool defineGlobal(const Symbol& symbol) {
        Symbol globalSymbol = symbol;
        globalSymbol.kind = SymbolKind::GLOBAL_VARIABLE;
        
        // Globals are always defined in the root scope
        std::shared_ptr<SymbolTable> rootScope = getGlobalScope();
        return rootScope->define(globalSymbol);
    }
    
    // Get the global (root) scope
    std::shared_ptr<SymbolTable> getGlobalScope() {
        std::shared_ptr<SymbolTable> scope = shared_from_this();
        
        while (scope->parent != nullptr) {
            scope = scope->parent;
        }
        
        return scope;
    }
    
    // Scope handling
    std::shared_ptr<SymbolTable> createChildScope();
    std::shared_ptr<SymbolTable> getParent() const { return parent; }
    
    // Utility methods
    std::vector<Symbol> getAllSymbols() const;
    void clear();
};
// Registry for types, to avoid duplication
class TypeRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<Type>> types;
    
public:
    TypeRegistry();
    
    // Register built-in types
    void initializeBuiltinTypes();
    
    // Type management
    void registerType(const std::string& name, std::shared_ptr<Type> type);
    std::shared_ptr<Type> getType(const std::string& name);
    bool hasType(const std::string& name) const;
    
    // Common types
    std::shared_ptr<Type> getVoidType();
    std::shared_ptr<Type> getBoolType();
    std::shared_ptr<Type> getIntType(int bitWidth = 32, bool isUnsigned = false);
    std::shared_ptr<Type> getFloatType(int bitWidth = 32);
    std::shared_ptr<Type> getStringType();
    std::shared_ptr<Type> getPointerType(std::shared_ptr<Type> pointeeType);
    
    // Type utilities
    std::string getTypeName(const std::shared_ptr<Type>& type);
    bool areTypesCompatible(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b);
    std::shared_ptr<Type> getCommonType(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b);
    
    // Clear all registered types
    void clear();
};

} // namespace flux

#endif // FLUX_PARSER_H
