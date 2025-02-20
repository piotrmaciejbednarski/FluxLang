/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree definitions for Flux
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include "../lexer/token.hpp"

namespace flux {

// Forward declarations
class Expression;
class Statement;

/**
 * @brief Type representation in Flux AST
 */
class Type {
public:
    enum class TypeKind {
        PRIMITIVE,    // int, float, char, bool, void
        ARRAY,        // Type[]
        POINTER,      // Type*
        REFERENCE,    // Type&
        CLASS,        // Custom class
        OBJECT,       // Custom object
        STRUCT,       // Struct type
        FUNCTION      // Function type
    };
    
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
    virtual TypeKind getKind() const = 0;
};

/**
 * @brief Primitive type (int, float, char, bool, void)
 */
class PrimitiveType : public Type {
public:
    enum class PrimitiveKind {
        INT,
        FLOAT,
        CHAR,
        BOOL,
        VOID,
        STRING
    };
    
    PrimitiveType(PrimitiveKind kind, int bitWidth = 0)
        : kind(kind), bitWidth(bitWidth) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::PRIMITIVE; }
    
    PrimitiveKind kind;
    int bitWidth;  // For types with bit width specification (e.g., int{32})
};

/**
 * @brief Array type
 */
class ArrayType : public Type {
public:
    ArrayType(std::shared_ptr<Type> elementType, std::optional<size_t> size = std::nullopt)
        : elementType(std::move(elementType)), size(size) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::ARRAY; }
    
    std::shared_ptr<Type> elementType;
    std::optional<size_t> size;  // Fixed size if specified
};

/**
 * @brief Pointer type
 */
class PointerType : public Type {
public:
    explicit PointerType(std::shared_ptr<Type> pointeeType)
        : pointeeType(std::move(pointeeType)) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::POINTER; }
    
    std::shared_ptr<Type> pointeeType;
};

/**
 * @brief Class type
 */
class ClassType : public Type {
public:
    explicit ClassType(std::string name)
        : name(std::move(name)) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::CLASS; }
    
    std::string name;
};

/**
 * @brief Object type
 */
class ObjectType : public Type {
public:
    explicit ObjectType(std::string name)
        : name(std::move(name)) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::OBJECT; }
    
    std::string name;
};

/**
 * @brief Struct type
 */
class StructType : public Type {
public:
    explicit StructType(std::string name)
        : name(std::move(name)) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::STRUCT; }
    
    std::string name;
};

/**
 * @brief Function type (for function pointers)
 */
class FunctionType : public Type {
public:
    FunctionType(std::shared_ptr<Type> returnType, std::vector<std::shared_ptr<Type>> parameterTypes)
        : returnType(std::move(returnType)), parameterTypes(std::move(parameterTypes)) {}
    
    std::string toString() const override;
    TypeKind getKind() const override { return TypeKind::FUNCTION; }
    
    std::shared_ptr<Type> returnType;
    std::vector<std::shared_ptr<Type>> parameterTypes;
};

/**
 * @brief AST visitor interface
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Visit expressions
    virtual void visitBinaryExpr(class BinaryExpr& expr) = 0;
    virtual void visitUnaryExpr(class UnaryExpr& expr) = 0;
    virtual void visitGroupingExpr(class GroupingExpr& expr) = 0;
    virtual void visitLiteralExpr(class LiteralExpr& expr) = 0;
    virtual void visitVariableExpr(class VariableExpr& expr) = 0;
    virtual void visitAssignExpr(class AssignExpr& expr) = 0;
    virtual void visitLogicalExpr(class LogicalExpr& expr) = 0;
    virtual void visitCallExpr(class CallExpr& expr) = 0;
    virtual void visitArrayAccessExpr(class ArrayAccessExpr& expr) = 0;
    virtual void visitMemberAccessExpr(class MemberAccessExpr& expr) = 0;
    virtual void visitAddressOfExpr(class AddressOfExpr& expr) = 0;
    virtual void visitDereferenceExpr(class DereferenceExpr& expr) = 0;
    virtual void visitInputExpr(class InputExpr& expr) = 0;
    virtual void visitOpenExpr(class OpenExpr& expr) = 0;
    
    // Visit statements
    virtual void visitExpressionStmt(class ExpressionStmt& stmt) = 0;
    virtual void visitBlockStmt(class BlockStmt& stmt) = 0;
    virtual void visitVarDeclarationStmt(class VarDeclarationStmt& stmt) = 0;
    virtual void visitIfStmt(class IfStmt& stmt) = 0;
    virtual void visitWhileStmt(class WhileStmt& stmt) = 0;
    virtual void visitForStmt(class ForStmt& stmt) = 0;
    virtual void visitWhenStmt(class WhenStmt& stmt) = 0;
    virtual void visitAsmStmt(class AsmStmt& stmt) = 0;
    virtual void visitFunctionDeclarationStmt(class FunctionDeclarationStmt& stmt) = 0;
    virtual void visitReturnStmt(class ReturnStmt& stmt) = 0;
    virtual void visitBreakStmt(class BreakStmt& stmt) = 0;
    virtual void visitContinueStmt(class ContinueStmt& stmt) = 0;
    virtual void visitClassDeclarationStmt(class ClassDeclarationStmt& stmt) = 0;
    virtual void visitObjectDeclarationStmt(class ObjectDeclarationStmt& stmt) = 0;
    virtual void visitNamespaceDeclarationStmt(class NamespaceDeclarationStmt& stmt) = 0;
    virtual void visitStructDeclarationStmt(class StructDeclarationStmt& stmt) = 0;
    virtual void visitOperatorDeclarationStmt(class OperatorDeclarationStmt& stmt) = 0;
    virtual void visitLockStmt(class LockStmt& stmt) = 0;
    virtual void visitPrintStmt(class PrintStmt& stmt) = 0;
    virtual void visitInputStmt(class InputStmt& stmt) = 0;
    virtual void visitOpenStmt(class OpenStmt& stmt) = 0;
};

/**
 * @brief Program AST node (root of the AST)
 */
class Program {
public:
    explicit Program(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    std::vector<std::shared_ptr<Statement>> statements;
};

} // namespace flux
