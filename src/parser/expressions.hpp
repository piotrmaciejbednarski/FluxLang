/**
 * @file expressions.hpp
 * @brief Expression classes for Flux AST
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../lexer/token.hpp"

namespace flux {

// Forward declarations
class Type;
class Statement;

/**
 * @brief Base class for all expressions in Flux
 */
class Expression {
public:
    virtual ~Expression() = default;
    virtual std::string toString() const = 0;

    // Prevent slicing by disabling copy and move
    Expression(const Expression&) = delete;
    Expression& operator=(const Expression&) = delete;
    Expression(Expression&&) = delete;
    Expression& operator=(Expression&&) = delete;
    
protected:
    Expression() = default;
};

using ExprPtr = std::shared_ptr<Expression>;

/**
 * @brief Base class for all literal expressions in Flux
 */
class LiteralExpr : public Expression {
public:
    virtual ~LiteralExpr() = default;
};

/**
 * @brief Integer literal (e.g., 42, 0xFF)
 */
class IntegerLiteral : public LiteralExpr {
public:
    IntegerLiteral(int64_t value, int bitWidth = 32)
        : value(value), bitWidth(bitWidth) {}
    
    std::string toString() const override;
    
    int64_t value;
    int bitWidth;  // Bit width for the integer (e.g., int{32}, int{64})
};

/**
 * @brief Float literal (e.g., 3.14, 2.5e-10)
 */
class FloatLiteral : public LiteralExpr {
public:
    FloatLiteral(double value, int bitWidth = 64)
        : value(value), bitWidth(bitWidth) {}
    
    std::string toString() const override;
    
    double value;
    int bitWidth;  // Bit width for the float (e.g., float{32}, float{64})
};

/**
 * @brief Boolean literal (true, false)
 */
class BooleanLiteral : public LiteralExpr {
public:
    explicit BooleanLiteral(bool value)
        : value(value) {}
    
    std::string toString() const override;
    
    bool value;
};

/**
 * @brief Character literal (e.g., 'a', '\n')
 */
class CharLiteral : public LiteralExpr {
public:
    explicit CharLiteral(char value)
        : value(value) {}
    
    std::string toString() const override;
    
    char value;
};

/**
 * @brief String literal (e.g., "hello")
 */
class StringLiteral : public LiteralExpr {
public:
    explicit StringLiteral(std::string value)
        : value(std::move(value)) {}
    
    std::string toString() const override;
    
    std::string value;
};

/**
 * @brief Null literal (nullptr)
 */
class NullLiteral : public LiteralExpr {
public:
    NullLiteral() = default;
    
    std::string toString() const override;
};

/**
 * @brief Array literal (e.g., [1, 2, 3])
 */
class ArrayLiteral : public LiteralExpr {
public:
    explicit ArrayLiteral(std::vector<ExprPtr> elements)
        : elements(std::move(elements)) {}
    
    std::string toString() const override;
    
    std::vector<ExprPtr> elements;
};

/**
 * @brief Character array literal (e.g., ['H', 'e', 'l', 'l', 'o'])
 */
class CharArrayLiteral : public LiteralExpr {
public:
    explicit CharArrayLiteral(std::vector<char> chars)
        : chars(std::move(chars)) {}
    
    std::string toString() const override;
    
    std::vector<char> chars;
};

/**
 * @brief Binary expression (e.g., a + b, a * b)
 */
class BinaryExpr : public Expression {
public:
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
    
    std::string toString() const override;
    
    ExprPtr left;
    Token op;
    ExprPtr right;
};

/**
 * @brief Unary expression (e.g., !a, -b)
 */
class UnaryExpr : public Expression {
public:
    UnaryExpr(Token op, ExprPtr right)
        : op(op), right(std::move(right)) {}
    
    std::string toString() const override;
    
    Token op;
    ExprPtr right;
};

/**
 * @brief Grouping expression (e.g., (a + b))
 */
class GroupingExpr : public Expression {
public:
    explicit GroupingExpr(ExprPtr expression)
        : expression(std::move(expression)) {}
    
    std::string toString() const override;
    
    ExprPtr expression;
};

/**
 * @brief Variable reference expression
 */
class VariableExpr : public Expression {
public:
    explicit VariableExpr(Token name)
        : name(name) {}
    
    std::string toString() const override;
    
    Token name;
};

/**
 * @brief Assignment expression (e.g., a = b)
 */
class AssignExpr : public Expression {
public:
    AssignExpr(Token name, ExprPtr value)
        : name(name), value(std::move(value)) {}
    
    std::string toString() const override;
    
    Token name;
    ExprPtr value;
};

/**
 * @brief Logical expression (e.g., a and b, a or b)
 */
class LogicalExpr : public Expression {
public:
    LogicalExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
    
    std::string toString() const override;
    
    ExprPtr left;
    Token op;
    ExprPtr right;
};

/**
 * @brief Function call expression (e.g., foo(a, b))
 */
class CallExpr : public Expression {
public:
    CallExpr(ExprPtr callee, Token paren, std::vector<ExprPtr> arguments)
        : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}
    
    std::string toString() const override;
    
    ExprPtr callee;
    Token paren;
    std::vector<ExprPtr> arguments;
};

/**
 * @brief Array access expression (e.g., arr[0])
 */
class ArrayAccessExpr : public Expression {
public:
    ArrayAccessExpr(ExprPtr array, ExprPtr index)
        : array(std::move(array)), index(std::move(index)) {}
    
    std::string toString() const override;
    
    ExprPtr array;
    ExprPtr index;
};

/**
 * @brief Member access expression (e.g., obj.member, ptr->member)
 */
class MemberAccessExpr : public Expression {
public:
    MemberAccessExpr(ExprPtr object, Token op, Token member)
        : object(std::move(object)), op(op), member(member) {}
    
    std::string toString() const override;
    
    ExprPtr object;
    Token op;      // . or ->
    Token member;
};

/**
 * @brief Interpolated string expression (e.g., i"{x} {y}":{x;y;})
 */
class InterpolatedStringExpr : public Expression {
public:
    InterpolatedStringExpr(std::string format, std::vector<ExprPtr> expressions)
        : format(std::move(format)), expressions(std::move(expressions)) {}
    
    std::string toString() const override;
    
    std::string format;
    std::vector<ExprPtr> expressions;
};

// Forward declaration of Type
class Type;

/**
 * @brief Type cast expression (e.g., int{32}:value)
 */
class TypeCastExpr : public Expression {
public:
    TypeCastExpr(std::shared_ptr<Type> targetType, ExprPtr expression)
        : targetType(std::move(targetType)), expression(std::move(expression)) {}
    
    std::string toString() const override;
    
    std::shared_ptr<Type> targetType;
    ExprPtr expression;
};

/**
 * @brief Address-of expression (e.g., @variable)
 */
class AddressOfExpr : public Expression {
public:
    explicit AddressOfExpr(ExprPtr expression)
        : expression(std::move(expression)) {}
    
    std::string toString() const override;
    
    ExprPtr expression;
};

/**
 * @brief Dereference expression (e.g., *pointer)
 */
class DereferenceExpr : public Expression {
public:
    explicit DereferenceExpr(ExprPtr expression)
        : expression(std::move(expression)) {}
    
    std::string toString() const override;
    
    ExprPtr expression;
};

/**
 * @brief Input expression (input("prompt"))
 */
class InputExpr : public Expression {
public:
    explicit InputExpr(ExprPtr prompt = nullptr)
        : prompt(std::move(prompt)) {}
    
    std::string toString() const override;
    
    ExprPtr prompt;     // Optional prompt to display
};

/**
 * @brief Open file expression (open("filename.txt", "r"))
 */
class OpenExpr : public Expression {
public:
    enum class Mode {
        READ,       // "r"
        WRITE,      // "w"
        APPEND,     // "a" 
        READ_PLUS,  // "r+"
        WRITE_PLUS, // "w+"
        APPEND_PLUS // "a+"
    };
    
    OpenExpr(ExprPtr filename, Mode mode)
        : filename(std::move(filename)), mode(mode) {}
    
    std::string toString() const override;
    
    ExprPtr filename;   // Filename expression
    Mode mode;          // File open mode
};

} // namespace flux
