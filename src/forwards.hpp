/**
 * @file forwards.hpp
 * @brief Forward declarations for Flux language types
 */

#pragma once

#include <memory>
#include <vector>
#include <optional>

namespace flux {
    // Token forward declaration
    class Token;

    // Type system forward declarations
    class Type;
    class PrimitiveType;
    class ArrayType;
    class PointerType;
    class ClassType;
    class ObjectType;
    class StructType;
    class FunctionType;

    // Forward declaration of statement classes needed for virtual method declarations
    class Statement;
    class Expression;

    // Expression forward declarations
    class Expression;
    using ExprPtr = std::shared_ptr<Expression>;

    class LiteralExpr;
    class IntegerLiteral;
    class FloatLiteral;
    class BooleanLiteral;
    class CharLiteral;
    class StringLiteral;
    class NullLiteral;
    class ArrayLiteral;
    class CharArrayLiteral;
    
    class BinaryExpr;
    class UnaryExpr;
    class GroupingExpr;
    class VariableExpr;
    class AssignExpr;
    class LogicalExpr;
    class CallExpr;
    class ArrayAccessExpr;
    class MemberAccessExpr;
    class InterpolatedStringExpr;
    class TypeCastExpr;
    class AddressOfExpr;
    class DereferenceExpr;
    class InputExpr;
    class OpenExpr;

    // Statement forward declarations
    class Statement;
    using StmtPtr = std::shared_ptr<Statement>;

    class ExpressionStmt;
    class BlockStmt;
    class VarDeclarationStmt;
    class IfStmt;
    class WhileStmt;
    class ForStmt;
    class WhenStmt;
    class AsmStmt;
    class FunctionDeclarationStmt;
    class ReturnStmt;
    class BreakStmt;
    class ContinueStmt;
    class ClassDeclarationStmt;
    class ObjectDeclarationStmt;
    class NamespaceDeclarationStmt;
    class StructDeclarationStmt;
    class OperatorDeclarationStmt;
    class LockStmt;
    class PrintStmt;
    class InputStmt;
    class OpenStmt;

    // Program forward declaration
    class Program;

    // AST Visitor forward declaration
    class ASTVisitor;
} // namespace flux
