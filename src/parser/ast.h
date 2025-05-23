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

class Expr;
class Stmt;
class TypeExpr;
class Decl;

template<typename R> class ExprVisitor;
template<typename R> class StmtVisitor;
template<typename R> class TypeExprVisitor;
template<typename R> class DeclVisitor;

class Expr {
public:
    common::SourceRange range;
    
    explicit Expr(const common::SourceRange& range) : range(range) {}
    virtual ~Expr() = default;
    
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const = 0;
    
    template<typename R>
    R accept(ExprVisitor<R>& visitor) const;
    
    virtual std::unique_ptr<Expr> clone() const = 0;
};

class Stmt {
public:
    common::SourceRange range;
    
    explicit Stmt(const common::SourceRange& range) : range(range) {}
    virtual ~Stmt() = default;
    
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const = 0;
    
    template<typename R>
    R accept(StmtVisitor<R>& visitor) const;
    
    virtual std::unique_ptr<Stmt> clone() const = 0;
};

class TypeExpr {
public:
    common::SourceRange range;
    
    explicit TypeExpr(const common::SourceRange& range) : range(range) {}
    virtual ~TypeExpr() = default;
    
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const = 0;
    
    template<typename R>
    R accept(TypeExprVisitor<R>& visitor) const;
    
    virtual std::unique_ptr<TypeExpr> clone() const = 0;
};

class Decl {
public:
    common::SourceRange range;
    std::string_view name;
    
    Decl(const common::SourceRange& range, std::string_view name) 
        : range(range), name(name) {}
    virtual ~Decl() = default;
    
    virtual void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const = 0;
    
    template<typename R>
    R accept(DeclVisitor<R>& visitor) const;
    
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

class LiteralExpr : public Expr {
public:
    using LiteralValue = std::variant<int64_t, double, std::string, bool>;
    
    LiteralValue value;
    lexer::Token token;
    
    LiteralExpr(const lexer::Token& token, LiteralValue value)
        : Expr({token.start(), token.end()}), value(std::move(value)), token(token) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class VariableExpr : public Expr {
public:
    std::string_view name;
    lexer::Token token;
    
    VariableExpr(const lexer::Token& token)
        : Expr({token.start(), token.end()}), name(token.lexeme()), token(token) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class UnaryExpr : public Expr {
public:
    lexer::Token op;
    std::unique_ptr<Expr> right;
    bool prefix;
    
    UnaryExpr(const lexer::Token& op, std::unique_ptr<Expr> right, bool prefix,
              const common::SourceRange& range)
        : Expr(range), op(op), right(std::move(right)), prefix(prefix) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    lexer::Token op;
    std::unique_ptr<Expr> right;
    
    BinaryExpr(std::unique_ptr<Expr> left, const lexer::Token& op, std::unique_ptr<Expr> right,
              const common::SourceRange& range)
        : Expr(range), left(std::move(left)), op(op), right(std::move(right)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class GroupExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;
    
    GroupExpr(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Expr(range), expression(std::move(expression)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    lexer::Token paren;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(std::unique_ptr<Expr> callee, const lexer::Token& paren,
             std::vector<std::unique_ptr<Expr>> arguments, const common::SourceRange& range)
        : Expr(range), callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class GetExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    lexer::Token name;
    
    GetExpr(std::unique_ptr<Expr> object, const lexer::Token& name,
            const common::SourceRange& range)
        : Expr(range), object(std::move(object)), name(name) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class SetExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    lexer::Token name;
    std::unique_ptr<Expr> value;
    
    SetExpr(std::unique_ptr<Expr> object, const lexer::Token& name,
            std::unique_ptr<Expr> value, const common::SourceRange& range)
        : Expr(range), object(std::move(object)), name(name), value(std::move(value)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class ArrayExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    
    ArrayExpr(std::vector<std::unique_ptr<Expr>> elements, const common::SourceRange& range)
        : Expr(range), elements(std::move(elements)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class DictExpr : public Expr {
public:
    struct KeyValuePair {
        std::unique_ptr<Expr> key;
        std::unique_ptr<Expr> value;
        
        KeyValuePair(std::unique_ptr<Expr> key, std::unique_ptr<Expr> value)
            : key(std::move(key)), value(std::move(value)) {}
    };
    
    std::vector<KeyValuePair> pairs;
    
    DictExpr(std::vector<KeyValuePair> pairs, const common::SourceRange& range)
        : Expr(range), pairs(std::move(pairs)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class SubscriptExpr : public Expr {
public:
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    
    SubscriptExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index,
                 const common::SourceRange& range)
        : Expr(range), array(std::move(array)), index(std::move(index)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class TernaryExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> thenExpr;
    std::unique_ptr<Expr> elseExpr;
    
    TernaryExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenExpr,
               std::unique_ptr<Expr> elseExpr, const common::SourceRange& range)
        : Expr(range), condition(std::move(condition)), thenExpr(std::move(thenExpr)),
          elseExpr(std::move(elseExpr)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class IStringExpr : public Expr {
public:
    std::vector<std::string_view> textParts;
    std::vector<std::unique_ptr<Expr>> exprParts;
    
    IStringExpr(std::vector<std::string_view> textParts,
               std::vector<std::unique_ptr<Expr>> exprParts,
               const common::SourceRange& range)
        : Expr(range), textParts(std::move(textParts)), exprParts(std::move(exprParts)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class CastExpr : public Expr {
public:
    std::unique_ptr<TypeExpr> targetType;
    std::unique_ptr<Expr> expression;
    
    CastExpr(std::unique_ptr<TypeExpr> targetType, std::unique_ptr<Expr> expression,
            const common::SourceRange& range)
        : Expr(range), targetType(std::move(targetType)), expression(std::move(expression)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class AssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    lexer::Token op;
    std::unique_ptr<Expr> value;
    
    AssignExpr(std::unique_ptr<Expr> target, const lexer::Token& op,
              std::unique_ptr<Expr> value, const common::SourceRange& range)
        : Expr(range), target(std::move(target)), op(op), value(std::move(value)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class SizeOfExpr : public Expr {
public:
    std::unique_ptr<TypeExpr> targetType;
    
    SizeOfExpr(std::unique_ptr<TypeExpr> targetType, const common::SourceRange& range)
        : Expr(range), targetType(std::move(targetType)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class TypeOfExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;
    
    TypeOfExpr(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Expr(range), expression(std::move(expression)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class OpExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    std::string_view operatorName;
    std::unique_ptr<Expr> right;
    
    OpExpr(std::unique_ptr<Expr> left, std::string_view operatorName, std::unique_ptr<Expr> right,
          const common::SourceRange& range)
        : Expr(range), left(std::move(left)), operatorName(operatorName), right(std::move(right)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class AddressOfExpr : public Expr {
public:
    std::unique_ptr<Expr> variable;
    
    AddressOfExpr(std::unique_ptr<Expr> variable, const common::SourceRange& range)
        : Expr(range), variable(std::move(variable)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class DereferenceExpr : public Expr {
public:
    std::unique_ptr<Expr> pointer;
    
    DereferenceExpr(std::unique_ptr<Expr> pointer, const common::SourceRange& range)
        : Expr(range), pointer(std::move(pointer)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class ScopeExpr : public Expr {
public:
    std::vector<std::string_view> path;
    
    ScopeExpr(std::vector<std::string_view> path, const common::SourceRange& range)
        : Expr(range), path(std::move(path)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Expr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Expr> clone() const override;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    
    ExprStmt(std::unique_ptr<Expr> expression, const common::SourceRange& range)
        : Stmt(range), expression(std::move(expression)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, const common::SourceRange& range)
        : Stmt(range), statements(std::move(statements)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class VarStmt : public Stmt {
public:
    lexer::Token name;
    std::unique_ptr<TypeExpr> type;
    std::unique_ptr<Expr> initializer;
    bool isConst;
    bool isVolatile;
    
    VarStmt(const lexer::Token& name, std::unique_ptr<TypeExpr> type,
           std::unique_ptr<Expr> initializer, bool isConst, bool isVolatile,
           const common::SourceRange& range)
        : Stmt(range), name(name), type(std::move(type)), initializer(std::move(initializer)),
          isConst(isConst), isVolatile(isVolatile) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
          std::unique_ptr<Stmt> elseBranch, const common::SourceRange& range)
        : Stmt(range), condition(std::move(condition)), thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body,
             const common::SourceRange& range)
        : Stmt(range), condition(std::move(condition)), body(std::move(body)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class DoWhileStmt : public Stmt {
public:
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> condition;
    
    DoWhileStmt(std::unique_ptr<Stmt> body, std::unique_ptr<Expr> condition,
               const common::SourceRange& range)
        : Stmt(range), body(std::move(body)), condition(std::move(condition)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
    
    ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
           std::unique_ptr<Expr> increment, std::unique_ptr<Stmt> body,
           const common::SourceRange& range)
        : Stmt(range), initializer(std::move(initializer)), condition(std::move(condition)),
          increment(std::move(increment)), body(std::move(body)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class ForInStmt : public Stmt {
public:
    lexer::Token iterator;
    std::optional<lexer::Token> keyVar;
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<Stmt> body;
    
    ForInStmt(const lexer::Token& iterator, std::optional<lexer::Token> keyVar,
             std::unique_ptr<Expr> iterable, std::unique_ptr<Stmt> body,
             const common::SourceRange& range)
        : Stmt(range), iterator(iterator), keyVar(keyVar), 
          iterable(std::move(iterable)), body(std::move(body)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class ReturnStmt : public Stmt {
public:
    lexer::Token keyword;
    std::unique_ptr<Expr> value;
    
    ReturnStmt(const lexer::Token& keyword, std::unique_ptr<Expr> value,
              const common::SourceRange& range)
        : Stmt(range), keyword(keyword), value(std::move(value)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class BreakStmt : public Stmt {
public:
    lexer::Token keyword;
    
    BreakStmt(const lexer::Token& keyword, const common::SourceRange& range)
        : Stmt(range), keyword(keyword) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class ContinueStmt : public Stmt {
public:
    lexer::Token keyword;
    
    ContinueStmt(const lexer::Token& keyword, const common::SourceRange& range)
        : Stmt(range), keyword(keyword) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class ThrowStmt : public Stmt {
public:
    lexer::Token keyword;
    std::unique_ptr<Expr> message;
    std::unique_ptr<Stmt> body;
    
    ThrowStmt(const lexer::Token& keyword, std::unique_ptr<Expr> message,
             std::unique_ptr<Stmt> body, const common::SourceRange& range)
        : Stmt(range), keyword(keyword), message(std::move(message)), body(std::move(body)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class TryStmt : public Stmt {
public:
    struct CatchClause {
        std::unique_ptr<TypeExpr> exceptionType;
        std::optional<lexer::Token> exceptionVar;
        std::unique_ptr<Stmt> handler;
        
        CatchClause(std::unique_ptr<TypeExpr> exceptionType, 
                   std::optional<lexer::Token> exceptionVar,
                   std::unique_ptr<Stmt> handler)
            : exceptionType(std::move(exceptionType)), 
              exceptionVar(exceptionVar),
              handler(std::move(handler)) {}
    };
    
    std::unique_ptr<Stmt> tryBlock;
    std::vector<CatchClause> catchClauses;
    
    TryStmt(std::unique_ptr<Stmt> tryBlock, std::vector<CatchClause> catchClauses,
           const common::SourceRange& range)
        : Stmt(range), tryBlock(std::move(tryBlock)), catchClauses(std::move(catchClauses)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class SwitchStmt : public Stmt {
public:
    struct CaseClause {
        std::unique_ptr<Expr> pattern;
        std::unique_ptr<Stmt> body;
        
        CaseClause(std::unique_ptr<Expr> pattern, std::unique_ptr<Stmt> body)
            : pattern(std::move(pattern)), body(std::move(body)) {}
    };
    
    std::unique_ptr<Expr> value;
    std::vector<CaseClause> cases;
    std::unique_ptr<Stmt> defaultCase;
    
    SwitchStmt(std::unique_ptr<Expr> value, std::vector<CaseClause> cases,
             std::unique_ptr<Stmt> defaultCase, const common::SourceRange& range)
        : Stmt(range), value(std::move(value)), cases(std::move(cases)),
          defaultCase(std::move(defaultCase)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class AssertStmt : public Stmt {
public:
    lexer::Token keyword;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> message;
    
    AssertStmt(const lexer::Token& keyword, std::unique_ptr<Expr> condition,
              std::unique_ptr<Expr> message, const common::SourceRange& range)
        : Stmt(range), keyword(keyword), condition(std::move(condition)), 
          message(std::move(message)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Stmt*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Stmt> clone() const override;
};

class NamedTypeExpr : public TypeExpr {
public:
    std::string_view name;
    
    NamedTypeExpr(std::string_view name, const common::SourceRange& range)
        : TypeExpr(range), name(name) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class ArrayTypeExpr : public TypeExpr {
public:
    std::unique_ptr<TypeExpr> elementType;
    std::unique_ptr<Expr> sizeExpr;
    
    ArrayTypeExpr(std::unique_ptr<TypeExpr> elementType, std::unique_ptr<Expr> sizeExpr,
                 const common::SourceRange& range)
        : TypeExpr(range), elementType(std::move(elementType)), sizeExpr(std::move(sizeExpr)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class PointerTypeExpr : public TypeExpr {
public:
    std::unique_ptr<TypeExpr> pointeeType;
    bool isVolatile;
    bool isConst;
    
    PointerTypeExpr(std::unique_ptr<TypeExpr> pointeeType, const common::SourceRange& range,
                   bool isVolatile = false, bool isConst = false)
        : TypeExpr(range), pointeeType(std::move(pointeeType)), 
          isVolatile(isVolatile), isConst(isConst) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class FunctionTypeExpr : public TypeExpr {
public:
    std::vector<std::unique_ptr<TypeExpr>> parameterTypes;
    std::unique_ptr<TypeExpr> returnType;
    
    FunctionTypeExpr(std::vector<std::unique_ptr<TypeExpr>> parameterTypes,
                    std::unique_ptr<TypeExpr> returnType,
                    const common::SourceRange& range)
        : TypeExpr(range), parameterTypes(std::move(parameterTypes)),
          returnType(std::move(returnType)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class DataTypeExpr : public TypeExpr {
public:
    int64_t bits;
    bool isSigned;
    std::optional<size_t> alignment;
    bool isVolatile;
    
    DataTypeExpr(int64_t bits, bool isSigned, const common::SourceRange& range,
                std::optional<size_t> alignment = std::nullopt, bool isVolatile = false)
        : TypeExpr(range), bits(bits), isSigned(isSigned),
          alignment(alignment), isVolatile(isVolatile) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class TemplateTypeExpr : public TypeExpr {
public:
    std::string_view name;
    std::vector<std::unique_ptr<TypeExpr>> arguments;
    
    TemplateTypeExpr(std::string_view name, std::vector<std::unique_ptr<TypeExpr>> arguments,
                    const common::SourceRange& range)
        : TypeExpr(range), name(name), arguments(std::move(arguments)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const TypeExpr*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<TypeExpr> clone() const override;
};

class FunctionDecl : public Decl {
public:
    struct Parameter {
        std::string_view name;
        std::unique_ptr<TypeExpr> type;
        
        Parameter(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    std::vector<Parameter> parameters;
    std::unique_ptr<TypeExpr> returnType;
    std::unique_ptr<Stmt> body;
    bool isPrototype;
    
    FunctionDecl(std::string_view name, std::vector<Parameter> parameters,
                std::unique_ptr<TypeExpr> returnType, std::unique_ptr<Stmt> body,
                const common::SourceRange& range, bool isPrototype = false)
        : Decl(range, name), parameters(std::move(parameters)),
          returnType(std::move(returnType)), body(std::move(body)),
          isPrototype(isPrototype) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class VarDecl : public Decl {
public:
    std::unique_ptr<TypeExpr> type;
    std::unique_ptr<Expr> initializer;
    bool isConst;
    bool isVolatile;
    
    VarDecl(std::string_view name, std::unique_ptr<TypeExpr> type,
           std::unique_ptr<Expr> initializer, bool isConst, bool isVolatile,
           const common::SourceRange& range)
        : Decl(range, name), type(std::move(type)),
          initializer(std::move(initializer)), isConst(isConst), isVolatile(isVolatile) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class ObjectDecl : public Decl {
public:
    std::vector<std::string_view> baseObjects;
    std::vector<std::string_view> templateParams;
    std::vector<std::unique_ptr<Decl>> members;
    bool isForwardDeclaration;
    bool isTemplate;
    
    ObjectDecl(std::string_view name, 
              std::vector<std::string_view> baseObjects,
              std::vector<std::string_view> templateParams,
              std::vector<std::unique_ptr<Decl>> members,
              const common::SourceRange& range,
              bool isForwardDeclaration = false,
              bool isTemplate = false)
        : Decl(range, name), baseObjects(std::move(baseObjects)),
          templateParams(std::move(templateParams)),
          members(std::move(members)), isForwardDeclaration(isForwardDeclaration),
          isTemplate(isTemplate) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class StructDecl : public Decl {
public:
    struct Field {
        std::string_view name;
        std::unique_ptr<TypeExpr> type;
        
        Field(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    std::vector<Field> fields;
    
    StructDecl(std::string_view name, std::vector<Field> fields,
              const common::SourceRange& range)
        : Decl(range, name), fields(std::move(fields)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class NamespaceDecl : public Decl {
public:
    std::vector<std::unique_ptr<Decl>> declarations;
    
    NamespaceDecl(std::string_view name, std::vector<std::unique_ptr<Decl>> declarations,
                 const common::SourceRange& range)
        : Decl(range, name), declarations(std::move(declarations)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class ImportDecl : public Decl {
public:
    std::string_view path;
    std::optional<std::string_view> alias;
    
    ImportDecl(std::string_view path, std::optional<std::string_view> alias,
              const common::SourceRange& range)
        : Decl(range, path), path(path), alias(alias) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class UsingDecl : public Decl {
public:
    std::vector<std::string_view> path;
    
    UsingDecl(std::vector<std::string_view> path, const common::SourceRange& range)
        : Decl(range, path.back()), path(std::move(path)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class OperatorDecl : public Decl {
public:
    struct Parameter {
        std::string_view name;
        std::unique_ptr<TypeExpr> type;
        
        Parameter(std::string_view name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };
    
    std::vector<Parameter> parameters;
    std::string_view op;
    std::unique_ptr<TypeExpr> returnType;
    std::unique_ptr<Stmt> body;
    bool isPrototype;
    
    OperatorDecl(std::string_view op, std::vector<Parameter> parameters,
                std::unique_ptr<TypeExpr> returnType, std::unique_ptr<Stmt> body,
                const common::SourceRange& range, bool isPrototype = false)
        : Decl(range, op), op(op), parameters(std::move(parameters)),
          returnType(std::move(returnType)), body(std::move(body)),
          isPrototype(isPrototype) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class DataDecl : public Decl {
public:
    int64_t bits;
    bool isSigned;
    bool isVolatile;
    
    DataDecl(std::string_view name, int64_t bits, bool isSigned,
            const common::SourceRange& range, bool isVolatile = false)
        : Decl(range, name), bits(bits), isSigned(isSigned), isVolatile(isVolatile) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class EnumDecl : public Decl {
public:
    struct Member {
        std::string_view name;
        std::unique_ptr<Expr> value;
        
        Member(std::string_view name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {}
    };
    
    std::vector<Member> members;
    
    EnumDecl(std::string_view name, std::vector<Member> members,
            const common::SourceRange& range)
        : Decl(range, name), members(std::move(members)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class TemplateDecl : public Decl {
public:
    struct Parameter {
        std::string_view name;
        
        enum class Kind {
            TYPE,
            VALUE
        };
        
        Kind kind;
        std::unique_ptr<TypeExpr> type;
        
        Parameter(std::string_view name, Kind kind)
            : name(name), kind(kind), type(nullptr) {}
        
        Parameter(std::string_view name, Kind kind, std::unique_ptr<TypeExpr> type)
            : name(name), kind(kind), type(std::move(type)) {}
    };
    
    std::vector<Parameter> parameters;
    std::unique_ptr<Decl> declaration;
    
    TemplateDecl(std::vector<Parameter> parameters, std::unique_ptr<Decl> declaration,
                const common::SourceRange& range)
        : Decl(range, declaration->name), parameters(std::move(parameters)),
          declaration(std::move(declaration)) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

class AsmDecl : public Decl {
public:
    std::string_view code;
    
    AsmDecl(std::string_view code, const common::SourceRange& range)
        : Decl(range, "asm"), code(code) {}
    
    void accept(void* visitor, void* result, void (*visit)(void*, const Decl*, void*)) const override {
        visit(visitor, this, result);
    }
    
    std::unique_ptr<Decl> clone() const override;
};

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
    virtual R visitDictExpr(const DictExpr& expr) = 0;
    virtual R visitSubscriptExpr(const SubscriptExpr& expr) = 0;
    virtual R visitTernaryExpr(const TernaryExpr& expr) = 0;
    virtual R visitIStringExpr(const IStringExpr& expr) = 0;
    virtual R visitCastExpr(const CastExpr& expr) = 0;
    virtual R visitAssignExpr(const AssignExpr& expr) = 0;
    virtual R visitSizeOfExpr(const SizeOfExpr& expr) = 0;
    virtual R visitTypeOfExpr(const TypeOfExpr& expr) = 0;
    virtual R visitOpExpr(const OpExpr& expr) = 0;
    virtual R visitAddressOfExpr(const AddressOfExpr& expr) = 0;
    virtual R visitDereferenceExpr(const DereferenceExpr& expr) = 0;
    virtual R visitScopeExpr(const ScopeExpr& expr) = 0;
    
private:
    friend class Expr;
    static void dispatch(void* visitor, const Expr* expr, void* result) {
        auto* typed_visitor = static_cast<ExprVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* e = dynamic_cast<const LiteralExpr*>(expr)) {
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
        } else if (auto* e = dynamic_cast<const DictExpr*>(expr)) {
            *typed_result = typed_visitor->visitDictExpr(*e);
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
        } else if (auto* e = dynamic_cast<const SizeOfExpr*>(expr)) {
            *typed_result = typed_visitor->visitSizeOfExpr(*e);
        } else if (auto* e = dynamic_cast<const TypeOfExpr*>(expr)) {
            *typed_result = typed_visitor->visitTypeOfExpr(*e);
        } else if (auto* e = dynamic_cast<const OpExpr*>(expr)) {
            *typed_result = typed_visitor->visitOpExpr(*e);
        } else if (auto* e = dynamic_cast<const AddressOfExpr*>(expr)) {
            *typed_result = typed_visitor->visitAddressOfExpr(*e);
        } else if (auto* e = dynamic_cast<const DereferenceExpr*>(expr)) {
            *typed_result = typed_visitor->visitDereferenceExpr(*e);
        } else if (auto* e = dynamic_cast<const ScopeExpr*>(expr)) {
            *typed_result = typed_visitor->visitScopeExpr(*e);
        }
    }
};

template<typename R>
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    
    virtual R visitExprStmt(const ExprStmt& stmt) = 0;
    virtual R visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual R visitVarStmt(const VarStmt& stmt) = 0;
    virtual R visitIfStmt(const IfStmt& stmt) = 0;
    virtual R visitWhileStmt(const WhileStmt& stmt) = 0;
    virtual R visitDoWhileStmt(const DoWhileStmt& stmt) = 0;
    virtual R visitForStmt(const ForStmt& stmt) = 0;
    virtual R visitForInStmt(const ForInStmt& stmt) = 0;
    virtual R visitReturnStmt(const ReturnStmt& stmt) = 0;
    virtual R visitBreakStmt(const BreakStmt& stmt) = 0;
    virtual R visitContinueStmt(const ContinueStmt& stmt) = 0;
    virtual R visitThrowStmt(const ThrowStmt& stmt) = 0;
    virtual R visitTryStmt(const TryStmt& stmt) = 0;
    virtual R visitSwitchStmt(const SwitchStmt& stmt) = 0;
    virtual R visitAssertStmt(const AssertStmt& stmt) = 0;
    virtual R visitDeclStmt(const DeclStmt& stmt) = 0;
    
private:
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
        } else if (auto* s = dynamic_cast<const DoWhileStmt*>(stmt)) {
            *typed_result = typed_visitor->visitDoWhileStmt(*s);
        } else if (auto* s = dynamic_cast<const ForStmt*>(stmt)) {
            *typed_result = typed_visitor->visitForStmt(*s);
        } else if (auto* s = dynamic_cast<const ForInStmt*>(stmt)) {
            *typed_result = typed_visitor->visitForInStmt(*s);
        } else if (auto* s = dynamic_cast<const ReturnStmt*>(stmt)) {
            *typed_result = typed_visitor->visitReturnStmt(*s);
        } else if (auto* s = dynamic_cast<const BreakStmt*>(stmt)) {
            *typed_result = typed_visitor->visitBreakStmt(*s);
        } else if (auto* s = dynamic_cast<const ContinueStmt*>(stmt)) {
            *typed_result = typed_visitor->visitContinueStmt(*s);
        } else if (auto* s = dynamic_cast<const ThrowStmt*>(stmt)) {
            *typed_result = typed_visitor->visitThrowStmt(*s);
        } else if (auto* s = dynamic_cast<const TryStmt*>(stmt)) {
            *typed_result = typed_visitor->visitTryStmt(*s);
        } else if (auto* s = dynamic_cast<const SwitchStmt*>(stmt)) {
            *typed_result = typed_visitor->visitSwitchStmt(*s);
        } else if (auto* s = dynamic_cast<const AssertStmt*>(stmt)) {
            *typed_result = typed_visitor->visitAssertStmt(*s);
        } else if (auto* s = dynamic_cast<const DeclStmt*>(stmt)) {
            *typed_result = typed_visitor->visitDeclStmt(*s);
        }
    }
};

template<typename R>
class DeclVisitor {
public:
    virtual ~DeclVisitor() = default;
    
    virtual R visitFunctionDecl(const FunctionDecl& decl) = 0;
    virtual R visitVarDecl(const VarDecl& decl) = 0;
    virtual R visitObjectDecl(const ObjectDecl& decl) = 0;
    virtual R visitStructDecl(const StructDecl& decl) = 0;
    virtual R visitNamespaceDecl(const NamespaceDecl& decl) = 0;
    virtual R visitImportDecl(const ImportDecl& decl) = 0;
    virtual R visitUsingDecl(const UsingDecl& decl) = 0;
    virtual R visitOperatorDecl(const OperatorDecl& decl) = 0;
    virtual R visitDataDecl(const DataDecl& decl) = 0;
    virtual R visitEnumDecl(const EnumDecl& decl) = 0;
    virtual R visitTemplateDecl(const TemplateDecl& decl) = 0;
    virtual R visitAsmDecl(const AsmDecl& decl) = 0;
    
private:
    friend class Decl;
    static void dispatch(void* visitor, const Decl* decl, void* result) {
        auto* typed_visitor = static_cast<DeclVisitor<R>*>(visitor);
        auto* typed_result = static_cast<R*>(result);
        
        if (auto* d = dynamic_cast<const FunctionDecl*>(decl)) {
            *typed_result = typed_visitor->visitFunctionDecl(*d);
        } else if (auto* d = dynamic_cast<const VarDecl*>(decl)) {
            *typed_result = typed_visitor->visitVarDecl(*d);
        } else if (auto* d = dynamic_cast<const ObjectDecl*>(decl)) {
            *typed_result = typed_visitor->visitObjectDecl(*d);
        } else if (auto* d = dynamic_cast<const StructDecl*>(decl)) {
            *typed_result = typed_visitor->visitStructDecl(*d);
        } else if (auto* d = dynamic_cast<const NamespaceDecl*>(decl)) {
            *typed_result = typed_visitor->visitNamespaceDecl(*d);
        } else if (auto* d = dynamic_cast<const ImportDecl*>(decl)) {
            *typed_result = typed_visitor->visitImportDecl(*d);
        } else if (auto* d = dynamic_cast<const UsingDecl*>(decl)) {
            *typed_result = typed_visitor->visitUsingDecl(*d);
        } else if (auto* d = dynamic_cast<const OperatorDecl*>(decl)) {
            *typed_result = typed_visitor->visitOperatorDecl(*d);
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

template<typename R>
class TypeExprVisitor {
public:
    virtual ~TypeExprVisitor() = default;
    
    virtual R visitNamedTypeExpr(const NamedTypeExpr& type) = 0;
    virtual R visitArrayTypeExpr(const ArrayTypeExpr& type) = 0;
    virtual R visitPointerTypeExpr(const PointerTypeExpr& type) = 0;
    virtual R visitFunctionTypeExpr(const FunctionTypeExpr& type) = 0;
    virtual R visitDataTypeExpr(const DataTypeExpr& type) = 0;
    virtual R visitTemplateTypeExpr(const TemplateTypeExpr& type) = 0;
    
private:
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
        } else if (auto* t = dynamic_cast<const TemplateTypeExpr*>(type)) {
            *typed_result = typed_visitor->visitTemplateTypeExpr(*t);
        }
    }
};

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

class Program {
public:
    std::vector<std::unique_ptr<Decl>> declarations;
    
    Program() = default;
    
    void addDeclaration(std::unique_ptr<Decl> declaration) {
        declarations.push_back(std::move(declaration));
    }
    
    std::unique_ptr<Program> clone() const {
        auto cloned = std::make_unique<Program>();
        for (const auto& decl : declarations) {
            cloned->addDeclaration(decl->clone());
        }
        return cloned;
    }
};

} // namespace parser
} // namespace flux