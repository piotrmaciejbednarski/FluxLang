#pragma once

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>

#include "../common/source.h"
#include "../common/error.h"
#include "../common/arena.h"
#include "../parser/ast.h"
#include "type.h"

namespace flux {
namespace type {

// Symbol table entry for tracking declared identifiers
struct Symbol {
    std::string_view name;
    std::shared_ptr<Type> type;
    common::SourceRange declarationLocation;
    bool isConst;
    bool isVolatile;
    bool isInitialized;
    
    // For functions and operators
    bool isFunction;
    std::vector<std::shared_ptr<Type>> parameterTypes;
    std::shared_ptr<Type> returnType;
    
    // For templates
    bool isTemplate;
    std::vector<std::string_view> templateParameters;
    
    Symbol(std::string_view name, std::shared_ptr<Type> type, 
           const common::SourceRange& location, bool isConst = false, bool isVolatile = false)
        : name(name), type(std::move(type)), declarationLocation(location),
          isConst(isConst), isVolatile(isVolatile), isInitialized(false),
          isFunction(false), isTemplate(false) {}
};

// Scope for symbol resolution
class Scope {
public:
    Scope(Scope* parent = nullptr) : parent_(parent) {}
    
    // Add symbol to this scope
    void addSymbol(const Symbol& symbol);
    
    // Look up symbol in this scope only
    Symbol* findLocal(std::string_view name);
    
    // Look up symbol in this scope and parent scopes
    Symbol* find(std::string_view name);
    
    // Check if symbol exists in this scope only
    bool hasLocal(std::string_view name) const;
    
    // Get parent scope
    Scope* parent() const { return parent_; }
    
    // Get all symbols in this scope
    const std::unordered_map<std::string_view, Symbol>& symbols() const { return symbols_; }

private:
    Scope* parent_;
    std::unordered_map<std::string_view, Symbol> symbols_;
};

// Type checking context
struct TypeCheckContext {
    // Current function return type (for return statement checking)
    std::shared_ptr<Type> currentFunctionReturnType;
    
    // Are we in a loop? (for break/continue checking)
    bool inLoop = false;
    
    // Are we in a try block? (for throw checking)
    bool inTryBlock = false;
    
    // Current object type (for 'this' keyword)
    std::shared_ptr<Type> currentObjectType;
    
    // Template context
    std::unordered_map<std::string_view, std::shared_ptr<Type>> templateBindings;
    bool inTemplate = false;
};

// Type checker class
class TypeChecker : 
    public parser::ExprVisitor<std::shared_ptr<Type>>,
    public parser::StmtVisitor<void>,
    public parser::DeclVisitor<void>,
    public parser::TypeExprVisitor<std::shared_ptr<Type>> {
public:
    TypeChecker(common::Arena& arena);
    
    // Type check a complete program
    bool checkProgram(const parser::Program& program);
    
    // Get any errors that occurred during type checking
    const common::ErrorCollector& errors() const { return errors_; }
    
    // Check if there were errors during type checking
    bool hasErrors() const { return errors_.hasErrors(); }
    
    // Expression visitors
    std::shared_ptr<Type> visitLiteralExpr(const parser::LiteralExpr& expr) override;
    std::shared_ptr<Type> visitVariableExpr(const parser::VariableExpr& expr) override;
    std::shared_ptr<Type> visitUnaryExpr(const parser::UnaryExpr& expr) override;
    std::shared_ptr<Type> visitBinaryExpr(const parser::BinaryExpr& expr) override;
    std::shared_ptr<Type> visitGroupExpr(const parser::GroupExpr& expr) override;
    std::shared_ptr<Type> visitCallExpr(const parser::CallExpr& expr) override;
    std::shared_ptr<Type> visitGetExpr(const parser::GetExpr& expr) override;
    std::shared_ptr<Type> visitSetExpr(const parser::SetExpr& expr) override;
    std::shared_ptr<Type> visitArrayExpr(const parser::ArrayExpr& expr) override;
    std::shared_ptr<Type> visitSubscriptExpr(const parser::SubscriptExpr& expr) override;
    std::shared_ptr<Type> visitTernaryExpr(const parser::TernaryExpr& expr) override;
    std::shared_ptr<Type> visitIStringExpr(const parser::IStringExpr& expr) override;
    std::shared_ptr<Type> visitCastExpr(const parser::CastExpr& expr) override;
    std::shared_ptr<Type> visitAssignExpr(const parser::AssignExpr& expr) override;
    std::shared_ptr<Type> visitSizeOfExpr(const parser::SizeOfExpr& expr) override;
    std::shared_ptr<Type> visitTypeOfExpr(const parser::TypeOfExpr& expr) override;
    std::shared_ptr<Type> visitOpExpr(const parser::OpExpr& expr) override;
    std::shared_ptr<Type> visitAddressOfExpr(const parser::AddressOfExpr& expr) override;
    
    // Statement visitors
    void visitExprStmt(const parser::ExprStmt& stmt) override;
    void visitBlockStmt(const parser::BlockStmt& stmt) override;
    void visitVarStmt(const parser::VarStmt& stmt) override;
    void visitIfStmt(const parser::IfStmt& stmt) override;
    void visitWhileStmt(const parser::WhileStmt& stmt) override;
    void visitForStmt(const parser::ForStmt& stmt) override;
    void visitReturnStmt(const parser::ReturnStmt& stmt) override;
    void visitBreakStmt(const parser::BreakStmt& stmt) override;
    void visitContinueStmt(const parser::ContinueStmt& stmt) override;
    void visitTryStmt(const parser::TryStmt& stmt) override;
    void visitSwitchStmt(const parser::SwitchStmt& stmt) override;
    void visitThrowStmt(const parser::ThrowStmt& stmt) override;
    void visitDeclStmt(const parser::DeclStmt& stmt) override;
    
    // Declaration visitors
    void visitFunctionDecl(const parser::FunctionDecl& decl) override;
    void visitVarDecl(const parser::VarDecl& decl) override;
    void visitClassDecl(const parser::ClassDecl& decl) override;
    void visitObjectDecl(const parser::ObjectDecl& decl) override;
    void visitStructDecl(const parser::StructDecl& decl) override;
    void visitNamespaceDecl(const parser::NamespaceDecl& decl) override;
    void visitImportDecl(const parser::ImportDecl& decl) override;
    void visitOperatorDecl(const parser::OperatorDecl& decl) override;
    void visitUsingDecl(const parser::UsingDecl& decl) override;
    void visitTypeDecl(const parser::TypeDecl& decl) override;
    void visitDataDecl(const parser::DataDecl& decl) override;
    void visitEnumDecl(const parser::EnumDecl& decl) override;
    void visitTemplateDecl(const parser::TemplateDecl& decl) override;
    void visitAsmDecl(const parser::AsmDecl& decl) override;
    void visitSectionDecl(const parser::SectionDecl& decl) override;
    
    // Type expression visitors
    std::shared_ptr<Type> visitNamedTypeExpr(const parser::NamedTypeExpr& type) override;
    std::shared_ptr<Type> visitArrayTypeExpr(const parser::ArrayTypeExpr& type) override;
    std::shared_ptr<Type> visitPointerTypeExpr(const parser::PointerTypeExpr& type) override;
    std::shared_ptr<Type> visitFunctionTypeExpr(const parser::FunctionTypeExpr& type) override;
    std::shared_ptr<Type> visitDataTypeExpr(const parser::DataTypeExpr& type) override;

private:
    common::Arena& arena_;
    common::ErrorCollector errors_;
    
    // Symbol table management
    std::vector<std::unique_ptr<Scope>> scopes_;
    Scope* currentScope_;
    
    // Type checking context
    TypeCheckContext context_;
    
    // Built-in types
    std::shared_ptr<Type> voidType_;
    std::shared_ptr<Type> bangVoidType_;
    std::shared_ptr<Type> intType_;
    std::shared_ptr<Type> floatType_;
    std::shared_ptr<Type> stringType_;
    std::shared_ptr<Type> boolType_;
    
    // Type management
    std::unordered_map<std::string, std::shared_ptr<Type>> namedTypes_;
    std::vector<std::shared_ptr<Type>> managedTypes_;
    
    // Scope management
    void pushScope();
    void popScope();
    
    // Built-in type initialization
    void initializeBuiltinTypes();
    
    // Symbol management
    void declareSymbol(const Symbol& symbol);
    Symbol* lookupSymbol(std::string_view name);
    
    // Type checking helpers
    bool isAssignable(const Type& target, const Type& source);
    bool isCompatible(const Type& left, const Type& right);
    bool isImplicitlyConvertible(const Type& from, const Type& to);
    std::shared_ptr<Type> getCommonType(const Type& left, const Type& right);
    
    // Type creation helpers
    std::shared_ptr<Type> createBuiltinType(TypeKind kind, std::string_view name);
    std::shared_ptr<Type> createDataType(int64_t bits, bool isSigned, bool isVolatile = false);
    std::shared_ptr<Type> createArrayType(std::shared_ptr<Type> elementType, std::optional<size_t> size = std::nullopt);
    std::shared_ptr<Type> createPointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false);
    std::shared_ptr<Type> createFunctionType(std::vector<std::shared_ptr<Type>> paramTypes, std::shared_ptr<Type> returnType);
    std::shared_ptr<Type> createObjectType(std::string_view name, const parser::ObjectDecl& decl);
    std::shared_ptr<Type> createStructType(std::string_view name, const parser::StructDecl& decl);
    std::shared_ptr<Type> createEnumType(std::string_view name, const parser::EnumDecl& decl);
    
    // Binary operation type checking
    std::shared_ptr<Type> checkArithmeticOperation(const parser::BinaryExpr& expr);
    std::shared_ptr<Type> checkComparisonOperation(const parser::BinaryExpr& expr);
    std::shared_ptr<Type> checkLogicalOperation(const parser::BinaryExpr& expr);
    std::shared_ptr<Type> checkBitwiseOperation(const parser::BinaryExpr& expr);
    std::shared_ptr<Type> checkAssignmentOperation(const parser::BinaryExpr& expr);
    
    // Unary operation type checking
    std::shared_ptr<Type> checkUnaryArithmetic(const parser::UnaryExpr& expr);
    std::shared_ptr<Type> checkUnaryLogical(const parser::UnaryExpr& expr);
    std::shared_ptr<Type> checkUnaryBitwise(const parser::UnaryExpr& expr);
    std::shared_ptr<Type> checkUnaryPointer(const parser::UnaryExpr& expr);
    std::shared_ptr<Type> checkUnaryIncDec(const parser::UnaryExpr& expr);
    
    // Function call type checking
    std::shared_ptr<Type> checkFunctionCall(const parser::CallExpr& expr, const Type& calleeType);
    std::shared_ptr<Type> checkObjectConstruction(const parser::CallExpr& expr, const Type& objectType);
    
    // Array access type checking
    std::shared_ptr<Type> checkArrayAccess(const parser::SubscriptExpr& expr);
    
    // Property access type checking
    std::shared_ptr<Type> checkPropertyAccess(const parser::GetExpr& expr);
    std::shared_ptr<Type> checkPropertyAssignment(const parser::SetExpr& expr);
    
    // Template handling
    void enterTemplate(const std::vector<std::string_view>& templateParams);
    void exitTemplate();
    std::shared_ptr<Type> resolveTemplateType(std::string_view name);
    
    // Error reporting helpers
    void error(common::ErrorCode code, std::string_view message, const common::SourceRange& location);
    void error(std::string_view message, const common::SourceRange& location);
    void typeError(std::string_view message, const common::SourceRange& location);
    void incompatibleTypeError(const Type& expected, const Type& actual, const common::SourceRange& location);
    void undefinedSymbolError(std::string_view name, const common::SourceRange& location);
    
    // Source location helpers
    output::SourceLocation makeSourceLocation(const common::SourceRange& range) const;
};

} // namespace type
} // namespace flux