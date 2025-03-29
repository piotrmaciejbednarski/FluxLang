#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <optional>

#include "../common/source.h"
#include "../lexer/token.h"
#include "../common/arena.h"

namespace flux {
namespace parser {

// Forward declarations for all AST node types
class Expr;
class Stmt;
class TypeExpr;
class Decl;

// Forward declarations for visitor classes
template<typename R> class ExprVisitor;
template<typename R> class StmtVisitor;
template<typename R> class TypeExprVisitor;
template<typename R> class DeclVisitor;

// Base class for all expressions
class Expr {
public:
    // Source range for error reporting
    common::SourceRange range;
    
    // Constructor
    explicit Expr(const common::SourceRange& range) : range(range) {}
    
    // Virtual destructor
    virtual ~Expr() = default;
    
    // Type-erased accept method for the visitor pattern
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const = 0;
    
    // Helper template method to make accept easier to use
    template<typename R>
    R accept(ExprVisitor<R>& visitor) const;
    
    // Clone method for copying expressions
    virtual std::unique_ptr<Expr> clone() const = 0;
};

// SizeOf expression (sizeof(type))
class SizeOfExpr : public Expr {
public:
    // The type to get the size of
    std::unique_ptr<TypeExpr> targetType;
    
    // Constructor
    SizeOfExpr(std::unique_ptr<TypeExpr> targetType, const common::SourceRange& range)
        : Expr(range), targetType(std::move(targetType)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// TypeOf expression (typeof(expr))
class TypeOfExpr : public Expr {
public:
    // The expression to get the type of
    std::unique_ptr<Expr> expression;
    
    // Constructor
    TypeOfExpr(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Expr(range), expression(std::move(expression)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Op expression (op<expr operator expr>)
class OpExpr : public Expr {
public:
    // Left operand
    std::unique_ptr<Expr> left;
    
    // Operator name (e.g., "pow", "add")
    std::string_view operatorName;
    
    // Right operand
    std::unique_ptr<Expr> right;
    
    // Constructor
    OpExpr(std::unique_ptr<Expr> left, std::string_view operatorName, std::unique_ptr<Expr> right,
          const common::SourceRange& range)
        : Expr(range), left(std::move(left)), operatorName(operatorName), right(std::move(right)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Base class for all statements
class Stmt {
public:
    // Source range for error reporting
    common::SourceRange range;
    
    // Constructor
    explicit Stmt(const common::SourceRange& range) : range(range) {}
    
    // Virtual destructor
    virtual ~Stmt() = default;
    
    // Type-erased accept method for the visitor pattern
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const = 0;
    
    // Helper template method to make accept easier to use
    template<typename R>
    R accept(StmtVisitor<R>& visitor) const;
    
    // Clone method for copying statements
    virtual std::unique_ptr<Stmt> clone() const = 0;
};

// Base class for all type expressions
class TypeExpr {
public:
    // Source range for error reporting
    common::SourceRange range;
    
    // Constructor
    explicit TypeExpr(const common::SourceRange& range) : range(range) {}
    
    // Virtual destructor
    virtual ~TypeExpr() = default;
    
    // Type-erased accept method for the visitor pattern
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const = 0;
    
    // Helper template method to make accept easier to use
    template<typename R>
    R accept(TypeExprVisitor<R>& visitor) const;
    
    // Clone method for copying type expressions
    virtual std::unique_ptr<TypeExpr> clone() const = 0;
};

// Base class for all declarations
class Decl {
public:
    // Source range for error reporting
    common::SourceRange range;
    
    // Name of the declared entity
    std::string_view name;
    
    // Constructor
    Decl(const common::SourceRange& range, std::string_view name) 
        : range(range), name(name) {}
    
    // Virtual destructor
    virtual ~Decl() = default;
    
    // Type-erased accept method for the visitor pattern
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const = 0;
    
    // Helper template method to make accept easier to use
    template<typename R>
    R accept(DeclVisitor<R>& visitor) const;
    
    // Clone method for copying declarations
    virtual std::unique_ptr<Decl> clone() const = 0;
};

class DeclStmt : public Stmt {
public:
    std::unique_ptr<Decl> declaration;
    
    DeclStmt(std::unique_ptr<Decl> declaration, const common::SourceRange& range)
        : Stmt(range), declaration(std::move(declaration)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override {
        return std::make_unique<DeclStmt>(declaration->clone(), range);
    }
};

// Expression AST nodes

// Literal expression (integers, floats, strings, booleans)
class LiteralExpr : public Expr {
public:
    // Variant to hold different literal types
    using LiteralValue = std::variant<int64_t, double, std::string, bool>;
    
    // Literal value
    LiteralValue value;
    
    // Token representing the literal
    lexer::Token token;
    
    // Constructor
    LiteralExpr(const lexer::Token& token, LiteralValue value)
        : Expr({token.start(), token.end()}), value(std::move(value)), token(token) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Variable expression (reference to a variable)
class VariableExpr : public Expr {
public:
    // Name of the variable
    std::string_view name;
    
    // Token representing the variable
    lexer::Token token;
    
    // Constructor
    VariableExpr(const lexer::Token& token)
        : Expr({token.start(), token.end()}), name(token.lexeme()), token(token) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Unary expression (e.g., -x, !x, ~x, ++x, --x)
class UnaryExpr : public Expr {
public:
    // Unary operator token
    lexer::Token op;
    
    // Operand expression
    std::unique_ptr<Expr> right;
    
    // Prefix or postfix
    bool prefix;
    
    // Constructor
    UnaryExpr(const lexer::Token& op, std::unique_ptr<Expr> right, bool prefix,
              const common::SourceRange& range)
        : Expr(range), op(op), right(std::move(right)), prefix(prefix) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Binary expression (e.g., a + b, a * b)
class BinaryExpr : public Expr {
public:
    // Left operand
    std::unique_ptr<Expr> left;
    
    // Binary operator token
    lexer::Token op;
    
    // Right operand
    std::unique_ptr<Expr> right;
    
    // Constructor
    BinaryExpr(std::unique_ptr<Expr> left, const lexer::Token& op, std::unique_ptr<Expr> right,
              const common::SourceRange& range)
        : Expr(range), left(std::move(left)), op(op), right(std::move(right)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Group expression (parenthesized expression)
class GroupExpr : public Expr {
public:
    // Grouped expression
    std::unique_ptr<Expr> expression;
    
    // Constructor
    GroupExpr(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Expr(range), expression(std::move(expression)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Call expression (function call)
class CallExpr : public Expr {
public:
    // Callee expression
    std::unique_ptr<Expr> callee;
    
    // Opening parenthesis token
    lexer::Token paren;
    
    // Arguments
    std::vector<std::unique_ptr<Expr>> arguments;
    
    // Constructor
    CallExpr(std::unique_ptr<Expr> callee, const lexer::Token& paren,
             std::vector<std::unique_ptr<Expr>> arguments, const common::SourceRange& range)
        : Expr(range), callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Get expression (property access, e.g., obj.prop)
class GetExpr : public Expr {
public:
    // Object expression
    std::unique_ptr<Expr> object;
    
    // Property name token
    lexer::Token name;
    
    // Constructor
    GetExpr(std::unique_ptr<Expr> object, const lexer::Token& name,
            const common::SourceRange& range)
        : Expr(range), object(std::move(object)), name(name) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Set expression (property assignment, e.g., obj.prop = value)
class SetExpr : public Expr {
public:
    // Object expression
    std::unique_ptr<Expr> object;
    
    // Property name token
    lexer::Token name;
    
    // Value expression
    std::unique_ptr<Expr> value;
    
    // Constructor
    SetExpr(std::unique_ptr<Expr> object, const lexer::Token& name,
            std::unique_ptr<Expr> value, const common::SourceRange& range)
        : Expr(range), object(std::move(object)), name(name), value(std::move(value)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Array/List expression (e.g., [1, 2, 3])
class ArrayExpr : public Expr {
public:
    // Array elements
    std::vector<std::unique_ptr<Expr>> elements;
    
    // Constructor
    ArrayExpr(std::vector<std::unique_ptr<Expr>> elements, const common::SourceRange& range)
        : Expr(range), elements(std::move(elements)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Subscript expression (array access, e.g., arr[index])
class SubscriptExpr : public Expr {
public:
    // Array expression
    std::unique_ptr<Expr> array;
    
    // Index expression
    std::unique_ptr<Expr> index;
    
    // Constructor
    SubscriptExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index,
                 const common::SourceRange& range)
        : Expr(range), array(std::move(array)), index(std::move(index)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Ternary expression (conditional expression, e.g., cond ? then : else)
class TernaryExpr : public Expr {
public:
    // Condition expression
    std::unique_ptr<Expr> condition;
    
    // Then expression
    std::unique_ptr<Expr> thenExpr;
    
    // Else expression
    std::unique_ptr<Expr> elseExpr;
    
    // Constructor
    TernaryExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenExpr,
               std::unique_ptr<Expr> elseExpr, const common::SourceRange& range)
        : Expr(range), condition(std::move(condition)), thenExpr(std::move(thenExpr)),
          elseExpr(std::move(elseExpr)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// IString expression (injectable string, e.g., i"Hello {name}")
class IStringExpr : public Expr {
public:
    // Text parts
    std::vector<std::string_view> textParts;
    
    // Expression parts
    std::vector<std::unique_ptr<Expr>> exprParts;
    
    // Constructor
    IStringExpr(std::vector<std::string_view> textParts,
               std::vector<std::unique_ptr<Expr>> exprParts,
               const common::SourceRange& range)
        : Expr(range), textParts(std::move(textParts)), exprParts(std::move(exprParts)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Cast expression (type cast, e.g., (int)x)
class CastExpr : public Expr {
public:
    // Target type
    std::unique_ptr<TypeExpr> targetType;
    
    // Expression to cast
    std::unique_ptr<Expr> expression;
    
    // Constructor
    CastExpr(std::unique_ptr<TypeExpr> targetType, std::unique_ptr<Expr> expression,
            const common::SourceRange& range)
        : Expr(range), targetType(std::move(targetType)), expression(std::move(expression)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Assignment expression (e.g., x = value)
class AssignExpr : public Expr {
public:
    // Target expression (lvalue)
    std::unique_ptr<Expr> target;
    
    // Assignment operator (=, +=, -=, etc.)
    lexer::Token op;
    
    // Value expression
    std::unique_ptr<Expr> value;
    
    // Constructor
    AssignExpr(std::unique_ptr<Expr> target, const lexer::Token& op,
              std::unique_ptr<Expr> value, const common::SourceRange& range)
        : Expr(range), target(std::move(target)), op(op), value(std::move(value)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Expr> clone() const override;
};

// Statement AST nodes

// Expression statement
class ExprStmt : public Stmt {
public:
    // Expression
    std::unique_ptr<Expr> expression;
    
    // Constructor
    ExprStmt(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Stmt(range), expression(std::move(expression)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Block statement (compound statement, e.g., { ... })
class BlockStmt : public Stmt {
public:
    // Statements in the block
    std::vector<std::unique_ptr<Stmt>> statements;
    
    // Constructor
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, const common::SourceRange& range)
        : Stmt(range), statements(std::move(statements)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Variable declaration statement
class VarStmt : public Stmt {
public:
    // Variable name token
    lexer::Token name;
    
    // Variable type (optional)
    std::unique_ptr<TypeExpr> type;
    
    // Initializer expression (optional)
    std::unique_ptr<Expr> initializer;
    
    // Constructor
    VarStmt(const lexer::Token& name, std::unique_ptr<TypeExpr> type,
           std::unique_ptr<Expr> initializer, const common::SourceRange& range)
        : Stmt(range), name(name), type(std::move(type)), initializer(std::move(initializer)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// If statement
class IfStmt : public Stmt {
public:
    // Condition expression
    std::unique_ptr<Expr> condition;
    
    // Then statement
    std::unique_ptr<Stmt> thenBranch;
    
    // Else statement (optional)
    std::unique_ptr<Stmt> elseBranch;
    
    // Constructor
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
          std::unique_ptr<Stmt> elseBranch, const common::SourceRange& range)
        : Stmt(range), condition(std::move(condition)), thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// While statement
class WhileStmt : public Stmt {
public:
    // Condition expression
    std::unique_ptr<Expr> condition;
    
    // Body statement
    std::unique_ptr<Stmt> body;
    
    // Constructor
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body,
             const common::SourceRange& range)
        : Stmt(range), condition(std::move(condition)), body(std::move(body)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// For statement
class ForStmt : public Stmt {
public:
    // Initializer statement (optional)
    std::unique_ptr<Stmt> initializer;
    
    // Condition expression (optional)
    std::unique_ptr<Expr> condition;
    
    // Increment expression (optional)
    std::unique_ptr<Expr> increment;
    
    // Body statement
    std::unique_ptr<Stmt> body;
    
    // Constructor
    ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
           std::unique_ptr<Expr> increment, std::unique_ptr<Stmt> body,
           const common::SourceRange& range)
        : Stmt(range), initializer(std::move(initializer)), condition(std::move(condition)),
          increment(std::move(increment)), body(std::move(body)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Return statement
class ReturnStmt : public Stmt {
public:
    // Return keyword token
    lexer::Token keyword;
    
    // Return value expression (optional)
    std::unique_ptr<Expr> value;
    
    // Constructor
    ReturnStmt(const lexer::Token& keyword, std::unique_ptr<Expr> value,
              const common::SourceRange& range)
        : Stmt(range), keyword(keyword), value(std::move(value)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Break statement
class BreakStmt : public Stmt {
public:
    // Break keyword token
    lexer::Token keyword;
    
    // Constructor
    BreakStmt(const lexer::Token& keyword, const common::SourceRange& range)
        : Stmt(range), keyword(keyword) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Continue statement
class ContinueStmt : public Stmt {
public:
    // Continue keyword token
    lexer::Token keyword;
    
    // Constructor
    ContinueStmt(const lexer::Token& keyword, const common::SourceRange& range)
        : Stmt(range), keyword(keyword) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

class ThrowStmt : public Stmt {
public:
    // Throw keyword token
    lexer::Token keyword;
    
    // Exception message expression (optional)
    std::unique_ptr<Expr> message;
    
    // Body block to execute after throwing (optional)
    std::unique_ptr<Stmt> body;
    
    // Constructor
    ThrowStmt(const lexer::Token& keyword, std::unique_ptr<Expr> message,
             std::unique_ptr<Stmt> body, const common::SourceRange& range)
        : Stmt(range), keyword(keyword), message(std::move(message)), body(std::move(body)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Try-catch statement
class TryStmt : public Stmt {
public:
    // Try block
    std::unique_ptr<Stmt> tryBlock;
    
    // Catch clauses (catch blocks with exception types)
    struct CatchClause {
        // Exception type (can be null for a catch-all clause)
        std::unique_ptr<TypeExpr> exceptionType;
        
        // Catch block
        std::unique_ptr<Stmt> handler;
        
        // Constructor
        CatchClause(std::unique_ptr<TypeExpr> exceptionType, std::unique_ptr<Stmt> handler)
            : exceptionType(std::move(exceptionType)), handler(std::move(handler)) {}
    };
    
    // Catch clauses
    std::vector<CatchClause> catchClauses;
    
    // Constructor
    TryStmt(std::unique_ptr<Stmt> tryBlock, std::vector<CatchClause> catchClauses,
           const common::SourceRange& range)
        : Stmt(range), tryBlock(std::move(tryBlock)), catchClauses(std::move(catchClauses)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Match statement (similar to switch but more powerful)
class SwitchStmt : public Stmt {
public:
    // Match expression
    std::unique_ptr<Expr> value;
    
    // Case clauses
    struct CaseClause {
        // Case pattern expression
        std::unique_ptr<Expr> pattern;
        
        // Case body
        std::unique_ptr<Stmt> body;
        
        // Constructor
        CaseClause(std::unique_ptr<Expr> pattern, std::unique_ptr<Stmt> body)
            : pattern(std::move(pattern)), body(std::move(body)) {}
    };
    
    // Case clauses
    std::vector<CaseClause> cases;
    
    // Default case (optional)
    std::unique_ptr<Stmt> defaultCase;
    
    // Constructor
    SwitchStmt(std::unique_ptr<Expr> value, std::vector<CaseClause> cases,
             std::unique_ptr<Stmt> defaultCase, const common::SourceRange& range)
        : Stmt(range), value(std::move(value)), cases(std::move(cases)),
          defaultCase(std::move(defaultCase)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Stmt> clone() const override;
};

// Declaration AST nodes

// Function declaration
class FunctionDecl : public Decl {
public:
    // Parameters (name and type)
    struct Parameter {
        // Parameter name
        std::string_view name;
        
        // Parameter type (optional)
        std::unique_ptr<TypeExpr> type;
        
        // Constructor
        Parameter(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    // Parameter list
    std::vector<Parameter> parameters;
    
    // Return type (optional)
    std::unique_ptr<TypeExpr> returnType;
    
    // Function body
    std::unique_ptr<Stmt> body;
    
    // Is this a function prototype (declaration without body)?
    bool isPrototype;
    
    // Constructor
    FunctionDecl(std::string_view name, std::vector<Parameter> parameters,
                std::unique_ptr<TypeExpr> returnType, std::unique_ptr<Stmt> body,
                const common::SourceRange& range, bool isPrototype = false)
        : Decl(range, name), parameters(std::move(parameters)),
          returnType(std::move(returnType)), body(std::move(body)),
          isPrototype(isPrototype) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Variable declaration
class VarDecl : public Decl {
public:
    // Variable type (optional)
    std::unique_ptr<TypeExpr> type;
    
    // Initializer expression (optional)
    std::unique_ptr<Expr> initializer;
    
    // Is this a constant?
    bool isConst;
    
    // Constructor
    VarDecl(std::string_view name, std::unique_ptr<TypeExpr> type,
           std::unique_ptr<Expr> initializer, bool isConst,
           const common::SourceRange& range)
        : Decl(range, name), type(std::move(type)),
          initializer(std::move(initializer)), isConst(isConst) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Class declaration
class ClassDecl : public Decl {
public:
    // Base classes (inheritance)
    std::vector<std::string_view> baseClasses;
    
    // Exclusion list for inheritance (using {!name} syntax)
    std::vector<std::string_view> exclusions;
    
    // Member declarations
    std::vector<std::unique_ptr<Decl>> members;
    
    // Is this a forward declaration?
    bool isForwardDeclaration;
    
    // Constructor
    ClassDecl(std::string_view name, std::vector<std::string_view> baseClasses,
             std::vector<std::string_view> exclusions,
             std::vector<std::unique_ptr<Decl>> members,
             const common::SourceRange& range,
             bool isForwardDeclaration = false)
        : Decl(range, name), baseClasses(std::move(baseClasses)),
          exclusions(std::move(exclusions)), members(std::move(members)),
          isForwardDeclaration(isForwardDeclaration) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

class ObjectDecl : public Decl {
public:
    // Base objects (inheritance)
    std::vector<std::string_view> baseObjects;
    
    // Member declarations
    std::vector<std::unique_ptr<Decl>> members;
    
    // Is this a forward declaration?
    bool isForwardDeclaration;
    
    // Constructor
    ObjectDecl(std::string_view name, std::vector<std::string_view> baseObjects,
              std::vector<std::unique_ptr<Decl>> members,
              const common::SourceRange& range,
              bool isForwardDeclaration = false)
        : Decl(range, name), baseObjects(std::move(baseObjects)),
          members(std::move(members)), isForwardDeclaration(isForwardDeclaration) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Struct declaration
class StructDecl : public Decl {
public:
    // Fields (members of the struct)
    struct Field {
        // Field name
        std::string_view name;
        
        // Field type
        std::unique_ptr<TypeExpr> type;
        
        // Constructor
        Field(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    // Fields
    std::vector<Field> fields;
    
    // Constructor
    StructDecl(std::string_view name, std::vector<Field> fields,
              const common::SourceRange& range)
        : Decl(range, name), fields(std::move(fields)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Namespace declaration
class NamespaceDecl : public Decl {
public:
    // Declarations in the namespace
    std::vector<std::unique_ptr<Decl>> declarations;
    
    // Constructor
    NamespaceDecl(std::string_view name, std::vector<std::unique_ptr<Decl>> declarations,
                 const common::SourceRange& range)
        : Decl(range, name), declarations(std::move(declarations)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Import declaration
class ImportDecl : public Decl {
public:
    // File path to import
    std::string_view path;
    
    // Alias for the import (optional)
    std::optional<std::string_view> alias;
    
    // Constructor
    ImportDecl(std::string_view path, std::optional<std::string_view> alias,
              const common::SourceRange& range)
        : Decl(range, path), path(path), alias(alias) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Operator declaration
class OperatorDecl : public Decl {
public:
    // Parameters (similar to function parameters)
    struct Parameter {
        // Parameter name
        std::string_view name;
        
        // Parameter type
        std::unique_ptr<TypeExpr> type;
        
        // Constructor
        Parameter(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    // Parameters
    std::vector<Parameter> parameters;
    
    // Operator symbol
    std::string_view op;
    
    // Return type
    std::unique_ptr<TypeExpr> returnType;
    
    // Function body
    std::unique_ptr<Stmt> body;
    
    // Is this a prototype declaration?
    bool isPrototype;
    
    // Constructor
    OperatorDecl(std::string_view op, std::vector<Parameter> parameters,
                std::unique_ptr<TypeExpr> returnType, std::unique_ptr<Stmt> body,
                const common::SourceRange& range, bool isPrototype = false)
        : Decl(range, op), op(op), parameters(std::move(parameters)),
          returnType(std::move(returnType)), body(std::move(body)),
          isPrototype(isPrototype) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Using declaration (e.g., using std.io)
class UsingDecl : public Decl {
public:
    // Path to the imported module
    std::vector<std::string_view> path;
    
    // Constructor
    UsingDecl(std::vector<std::string_view> path, const common::SourceRange& range)
        : Decl(range, path.back()), path(std::move(path)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Type declaration (typedef or type alias)
class TypeDecl : public Decl {
public:
    // Underlying type
    std::unique_ptr<TypeExpr> underlyingType;
    
    // Constructor
    TypeDecl(std::string_view name, std::unique_ptr<TypeExpr> underlyingType,
            const common::SourceRange& range)
        : Decl(range, name), underlyingType(std::move(underlyingType)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Data type declaration (raw bit storage)
class DataDecl : public Decl {
public:
    // Size in bits
    int64_t bits;
    
    // Signed or unsigned
    bool isSigned;
    
    // Constructor
    DataDecl(std::string_view name, int64_t bits, bool isSigned,
            const common::SourceRange& range)
        : Decl(range, name), bits(bits), isSigned(isSigned) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Enum declaration
class EnumDecl : public Decl {
public:
    // Enum members
    struct Member {
        // Member name
        std::string_view name;
        
        // Value expression (optional)
        std::unique_ptr<Expr> value;
        
        // Constructor
        Member(std::string_view name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {}
    };
    
    // Members
    std::vector<Member> members;
    
    // Constructor
    EnumDecl(std::string_view name, std::vector<Member> members,
            const common::SourceRange& range)
        : Decl(range, name), members(std::move(members)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Template declaration
class TemplateDecl : public Decl {
public:
    // Template parameters
    struct Parameter {
        // Parameter name
        std::string_view name;
        
        // Parameter kind (type, non-type, etc.)
        enum class Kind {
            TYPE,
            VALUE
        };
        
        // Parameter kind
        Kind kind;
        
        // For value parameters, the type
        std::unique_ptr<TypeExpr> type;
        
        // Constructor for type parameters
        Parameter(std::string_view name, Kind kind)
            : name(name), kind(kind), type(nullptr) {}
        
        // Constructor for value parameters
        Parameter(std::string_view name, Kind kind, std::unique_ptr<TypeExpr> type)
            : name(name), kind(kind), type(std::move(type)) {}
    };
    
    // Template parameters
    std::vector<Parameter> parameters;
    
    // Declaration being templated
    std::unique_ptr<Decl> declaration;
    
    // Constructor
    TemplateDecl(std::vector<Parameter> parameters, std::unique_ptr<Decl> declaration,
                const common::SourceRange& range)
        : Decl(range, declaration->name), parameters(std::move(parameters)),
          declaration(std::move(declaration)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// ASM block declaration
class AsmDecl : public Decl {
public:
    // Assembly code
    std::string_view code;
    
    // Constructor
    AsmDecl(std::string_view code, const common::SourceRange& range)
        : Decl(range, "asm"), code(code) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<Decl> clone() const override;
};

// Type AST nodes

// Named type
class NamedTypeExpr : public TypeExpr {
public:
    // Type name
    std::string_view name;
    
    // Constructor
    NamedTypeExpr(std::string_view name, const common::SourceRange& range)
        : TypeExpr(range), name(name) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<TypeExpr> clone() const override;
};

// Array type
class ArrayTypeExpr : public TypeExpr {
public:
    // Element type
    std::unique_ptr<TypeExpr> elementType;
    
    // Size expression (optional, nullptr for dynamic arrays)
    std::unique_ptr<Expr> sizeExpr;
    
    // Constructor
    ArrayTypeExpr(std::unique_ptr<TypeExpr> elementType, std::unique_ptr<Expr> sizeExpr,
                 const common::SourceRange& range)
        : TypeExpr(range), elementType(std::move(elementType)), sizeExpr(std::move(sizeExpr)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<TypeExpr> clone() const override;
};

// Pointer type
class PointerTypeExpr : public TypeExpr {
public:
    // Pointee type
    std::unique_ptr<TypeExpr> pointeeType;
    
    // Constructor
    PointerTypeExpr(std::unique_ptr<TypeExpr> pointeeType, const common::SourceRange& range)
        : TypeExpr(range), pointeeType(std::move(pointeeType)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<TypeExpr> clone() const override;
};

// Function type
class FunctionTypeExpr : public TypeExpr {
public:
    // Parameter types
    std::vector<std::unique_ptr<TypeExpr>> parameterTypes;
    
    // Return type
    std::unique_ptr<TypeExpr> returnType;
    
    // Constructor
    FunctionTypeExpr(std::vector<std::unique_ptr<TypeExpr>> parameterTypes,
                    std::unique_ptr<TypeExpr> returnType,
                    const common::SourceRange& range)
        : TypeExpr(range), parameterTypes(std::move(parameterTypes)),
          returnType(std::move(returnType)) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<TypeExpr> clone() const override;
};

// Data type (raw bit storage)
class DataTypeExpr : public TypeExpr {
public:
    // Size in bits
    int64_t bits;
    
    // Signed or unsigned
    bool isSigned;
    
    // Constructor
    DataTypeExpr(int64_t bits, bool isSigned, const common::SourceRange& range)
        : TypeExpr(range), bits(bits), isSigned(isSigned) {}
    
    // Type-erased visitor implementation
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    // Clone method
    std::unique_ptr<TypeExpr> clone() const override;
};

// Visitor interfaces

// Expression visitor
template<typename R>
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    
    virtual R visitLiteralExpr(const LiteralExpr& expr) = 0;
    virtual R visitVariableExpr(const VariableExpr& expr) = 0;
    virtual R visitUnaryExpr(const UnaryExpr& expr) = 0;
    virtual R visitBinaryExpr(const BinaryExpr& expr) = 0;
    virtual R visitGroupExpr(const GroupExpr& expr) = 0;
    virtual R visitCallExpr(const CallExpr& expr) = 0;
    virtual R visitGetExpr(const GetExpr& expr) = 0;
    virtual R visitSetExpr(const SetExpr& expr) = 0;
    virtual R visitArrayExpr(const ArrayExpr& expr) = 0;
    virtual R visitSubscriptExpr(const SubscriptExpr& expr) = 0;
    virtual R visitTernaryExpr(const TernaryExpr& expr) = 0;
    virtual R visitIStringExpr(const IStringExpr& expr) = 0;
    virtual R visitCastExpr(const CastExpr& expr) = 0;
    virtual R visitAssignExpr(const AssignExpr& expr) = 0;
    virtual R visitSizeOfExpr(const SizeOfExpr& expr) = 0;
    virtual R visitTypeOfExpr(const TypeOfExpr& expr) = 0;
    virtual R visitOpExpr(const OpExpr& expr) = 0;
    
private:
    // Type-erased visitor dispatch function
    friend class Expr;
    static void dispatch(void* visitor, const Expr* expr, void* result) {
        auto* typed_visitor = static_cast<ExprVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* e = dynamic_cast<const SizeOfExpr*>(expr)) {
            *typed_result = typed_visitor->visitSizeOfExpr(*e);
        } else if (auto* e = dynamic_cast<const TypeOfExpr*>(expr)) {
            *typed_result = typed_visitor->visitTypeOfExpr(*e);
        } else if (auto* e = dynamic_cast<const OpExpr*>(expr)) {
            *typed_result = typed_visitor->visitOpExpr(*e);
        } else if (auto* e = dynamic_cast<const LiteralExpr*>(expr)) {
            *typed_result = typed_visitor->visitLiteralExpr(*e);
        } else if (auto* e = dynamic_cast<const VariableExpr*>(expr)) {
            *typed_result = typed_visitor->visitVariableExpr(*e);
        } else if (auto* e = dynamic_cast<const UnaryExpr*>(expr)) {
            *typed_result = typed_visitor->visitUnaryExpr(*e);
        } else if (auto* e = dynamic_cast<const BinaryExpr*>(expr)) {
            *typed_result = typed_visitor->visitBinaryExpr(*e);
        } else if (auto* e = dynamic_cast<const GroupExpr*>(expr)) {
            *typed_result = typed_visitor->visitGroupExpr(*e);
        } else if (auto* e = dynamic_cast<const CallExpr*>(expr)) {
            *typed_result = typed_visitor->visitCallExpr(*e);
        } else if (auto* e = dynamic_cast<const GetExpr*>(expr)) {
            *typed_result = typed_visitor->visitGetExpr(*e);
        } else if (auto* e = dynamic_cast<const SetExpr*>(expr)) {
            *typed_result = typed_visitor->visitSetExpr(*e);
        } else if (auto* e = dynamic_cast<const ArrayExpr*>(expr)) {
            *typed_result = typed_visitor->visitArrayExpr(*e);
        } else if (auto* e = dynamic_cast<const SubscriptExpr*>(expr)) {
            *typed_result = typed_visitor->visitSubscriptExpr(*e);
        } else if (auto* e = dynamic_cast<const TernaryExpr*>(expr)) {
            *typed_result = typed_visitor->visitTernaryExpr(*e);
        } else if (auto* e = dynamic_cast<const IStringExpr*>(expr)) {
            *typed_result = typed_visitor->visitIStringExpr(*e);
        } else if (auto* e = dynamic_cast<const CastExpr*>(expr)) {
            *typed_result = typed_visitor->visitCastExpr(*e);
        } else if (auto* e = dynamic_cast<const AssignExpr*>(expr)) {
            *typed_result = typed_visitor->visitAssignExpr(*e);
        }
    }
};

// Statement visitor
template<typename R>
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    
    virtual R visitExprStmt(const ExprStmt& stmt) = 0;
    virtual R visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual R visitVarStmt(const VarStmt& stmt) = 0;
    virtual R visitIfStmt(const IfStmt& stmt) = 0;
    virtual R visitWhileStmt(const WhileStmt& stmt) = 0;
    virtual R visitForStmt(const ForStmt& stmt) = 0;
    virtual R visitReturnStmt(const ReturnStmt& stmt) = 0;
    virtual R visitBreakStmt(const BreakStmt& stmt) = 0;
    virtual R visitContinueStmt(const ContinueStmt& stmt) = 0;
    virtual R visitTryStmt(const TryStmt& stmt) = 0;
    virtual R visitSwitchStmt(const SwitchStmt& stmt) = 0;
    virtual R visitThrowStmt(const ThrowStmt& stmt) = 0;
    
private:
    // Type-erased visitor dispatch function
    friend class Stmt;
    static void dispatch(void* visitor, const Stmt* stmt, void* result) {
        auto* typed_visitor = static_cast<StmtVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* s = dynamic_cast<const ExprStmt*>(stmt)) {
            *typed_result = typed_visitor->visitExprStmt(*s);
        } else if (auto* s = dynamic_cast<const BlockStmt*>(stmt)) {
            *typed_result = typed_visitor->visitBlockStmt(*s);
        } else if (auto* s = dynamic_cast<const VarStmt*>(stmt)) {
            *typed_result = typed_visitor->visitVarStmt(*s);
        } else if (auto* s = dynamic_cast<const IfStmt*>(stmt)) {
            *typed_result = typed_visitor->visitIfStmt(*s);
        } else if (auto* s = dynamic_cast<const WhileStmt*>(stmt)) {
            *typed_result = typed_visitor->visitWhileStmt(*s);
        } else if (auto* s = dynamic_cast<const ForStmt*>(stmt)) {
            *typed_result = typed_visitor->visitForStmt(*s);
        } else if (auto* s = dynamic_cast<const ReturnStmt*>(stmt)) {
            *typed_result = typed_visitor->visitReturnStmt(*s);
        } else if (auto* s = dynamic_cast<const BreakStmt*>(stmt)) {
            *typed_result = typed_visitor->visitBreakStmt(*s);
        } else if (auto* s = dynamic_cast<const ContinueStmt*>(stmt)) {
            *typed_result = typed_visitor->visitContinueStmt(*s);
        } else if (auto* s = dynamic_cast<const TryStmt*>(stmt)) {
            *typed_result = typed_visitor->visitTryStmt(*s);
        } else if (auto* s = dynamic_cast<const SwitchStmt*>(stmt)) {
            *typed_result = typed_visitor->visitSwitchStmt(*s);
        } else if (auto* s = dynamic_cast<const ThrowStmt*>(stmt)) {
            *typed_result = typed_visitor->visitThrowStmt(*s);
        }
    }
};

// Declaration visitor
template<typename R>
class DeclVisitor {
public:
    virtual ~DeclVisitor() = default;
    
    virtual R visitFunctionDecl(const FunctionDecl& decl) = 0;
    virtual R visitVarDecl(const VarDecl& decl) = 0;
    virtual R visitClassDecl(const ClassDecl& decl) = 0;
    virtual R visitObjectDecl(const ObjectDecl& decl) = 0;
    virtual R visitStructDecl(const StructDecl& decl) = 0;
    virtual R visitNamespaceDecl(const NamespaceDecl& decl) = 0;
    virtual R visitImportDecl(const ImportDecl& decl) = 0;
    virtual R visitOperatorDecl(const OperatorDecl& decl) = 0;
    virtual R visitUsingDecl(const UsingDecl& decl) = 0;
    virtual R visitTypeDecl(const TypeDecl& decl) = 0;
    virtual R visitDataDecl(const DataDecl& decl) = 0;
    virtual R visitEnumDecl(const EnumDecl& decl) = 0;
    virtual R visitTemplateDecl(const TemplateDecl& decl) = 0;
    virtual R visitAsmDecl(const AsmDecl& decl) = 0;
    
private:
    // Type-erased visitor dispatch function
    friend class Decl;
    static void dispatch(void* visitor, const Decl* decl, void* result) {
        auto* typed_visitor = static_cast<DeclVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* d = dynamic_cast<const FunctionDecl*>(decl)) {
            *typed_result = typed_visitor->visitFunctionDecl(*d);
        } else if (auto* d = dynamic_cast<const VarDecl*>(decl)) {
            *typed_result = typed_visitor->visitVarDecl(*d);
        } else if (auto* d = dynamic_cast<const ClassDecl*>(decl)) {
            *typed_result = typed_visitor->visitClassDecl(*d);
        } else if (auto* d = dynamic_cast<const ObjectDecl*>(decl)) {
            *typed_result = typed_visitor->visitObjectDecl(*d);
        } else if (auto* d = dynamic_cast<const StructDecl*>(decl)) {
            *typed_result = typed_visitor->visitStructDecl(*d);
        } else if (auto* d = dynamic_cast<const NamespaceDecl*>(decl)) {
            *typed_result = typed_visitor->visitNamespaceDecl(*d);
        } else if (auto* d = dynamic_cast<const ImportDecl*>(decl)) {
            *typed_result = typed_visitor->visitImportDecl(*d);
        } else if (auto* d = dynamic_cast<const OperatorDecl*>(decl)) {
            *typed_result = typed_visitor->visitOperatorDecl(*d);
        } else if (auto* d = dynamic_cast<const UsingDecl*>(decl)) {
            *typed_result = typed_visitor->visitUsingDecl(*d);
        } else if (auto* d = dynamic_cast<const TypeDecl*>(decl)) {
            *typed_result = typed_visitor->visitTypeDecl(*d);
        } else if (auto* d = dynamic_cast<const DataDecl*>(decl)) {
            *typed_result = typed_visitor->visitDataDecl(*d);
        } else if (auto* d = dynamic_cast<const EnumDecl*>(decl)) {
            *typed_result = typed_visitor->visitEnumDecl(*d);
        } else if (auto* d = dynamic_cast<const TemplateDecl*>(decl)) {
            *typed_result = typed_visitor->visitTemplateDecl(*d);
        } else if (auto* d = dynamic_cast<const AsmDecl*>(decl)) {
            *typed_result = typed_visitor->visitAsmDecl(*d);
        }
    }
};

// Type expression visitor
template<typename R>
class TypeExprVisitor {
public:
    virtual ~TypeExprVisitor() = default;
    
    virtual R visitNamedTypeExpr(const NamedTypeExpr& type) = 0;
    virtual R visitArrayTypeExpr(const ArrayTypeExpr& type) = 0;
    virtual R visitPointerTypeExpr(const PointerTypeExpr& type) = 0;
    virtual R visitFunctionTypeExpr(const FunctionTypeExpr& type) = 0;
    virtual R visitDataTypeExpr(const DataTypeExpr& type) = 0;
    
private:
    // Type-erased visitor dispatch function
    friend class TypeExpr;
    static void dispatch(void* visitor, const TypeExpr* type, void* result) {
        auto* typed_visitor = static_cast<TypeExprVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* t = dynamic_cast<const NamedTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitNamedTypeExpr(*t);
        } else if (auto* t = dynamic_cast<const ArrayTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitArrayTypeExpr(*t);
        } else if (auto* t = dynamic_cast<const PointerTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitPointerTypeExpr(*t);
        } else if (auto* t = dynamic_cast<const FunctionTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitFunctionTypeExpr(*t);
        } else if (auto* t = dynamic_cast<const DataTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitDataTypeExpr(*t);
        }
    }
};

// Template implementations for accept methods

template<typename R>
R Expr::accept(ExprVisitor<R>& visitor) const {
    R result;
    accept(&visitor, &result, &ExprVisitor<R>::dispatch);
    return result;
}

template<typename R>
R Stmt::accept(StmtVisitor<R>& visitor) const {
    R result;
    accept(&visitor, &result, &StmtVisitor<R>::dispatch);
    return result;
}

template<typename R>
R TypeExpr::accept(TypeExprVisitor<R>& visitor) const {
    R result;
    accept(&visitor, &result, &TypeExprVisitor<R>::dispatch);
    return result;
}

template<typename R>
R Decl::accept(DeclVisitor<R>& visitor) const {
    R result;
    accept(&visitor, &result, &DeclVisitor<R>::dispatch);
    return result;
}

// Program AST node (root of the AST)
class Program {
public:
    // Declarations in the program
    std::vector<std::unique_ptr<Decl>> declarations;
    
    // Constructor
    Program() = default;
    
    // Add a declaration to the program
    void addDeclaration(std::unique_ptr<Decl> declaration) {
        declarations.push_back(std::move(declaration));
    }
};

} // namespace parser
} // namespace flux