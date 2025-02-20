/**
 * @file statements.hpp
 * @brief Statement classes for Flux AST
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <typeinfo>
#include "../lexer/token.hpp"
#include "expressions.hpp"
#include "../forwards.hpp"

namespace flux {

// Forward declarations
class Type;

/**
 * @brief Base class for all statements in Flux
 */
class Statement {
public:
    virtual ~Statement() = default;
    virtual std::string toString() const = 0;
    
    virtual bool isBlockStmt() const { return false; }

    virtual bool isA(const std::type_info& type) const {
        return typeid(*this) == type;
    }
};

using StmtPtr = std::shared_ptr<Statement>;

/**
 * @brief Expression statement
 */
class ExpressionStmt : public Statement {
public:
    explicit ExpressionStmt(ExprPtr expression)
        : expression(std::move(expression)) {}
    
    std::string toString() const override;
    
    ExprPtr expression;
};

/**
 * @brief Block statement
 */
class BlockStmt : public Statement {
public:
    BlockStmt() = default;

    explicit BlockStmt(std::vector<StmtPtr> statements, bool isVolatile = false)
        : statements(std::move(statements)), isVolatile(isVolatile) {}
    
    std::string toString() const override;
    
    bool isBlockStmt() const override { return true; }
    
    std::vector<StmtPtr> statements;
    bool isVolatile = false;  // Whether the block is marked as volatile
};

/**
 * @brief Variable declaration
 */
class VarDeclarationStmt : public Statement {
public:
    VarDeclarationStmt(std::shared_ptr<Type> type, Token name, 
                     ExprPtr initializer = nullptr, bool isVolatile = false)
        : type(std::move(type)), name(name), 
          initializer(std::move(initializer)), isVolatile(isVolatile) {}
    
    std::string toString() const override;
    
    std::shared_ptr<Type> type;
    Token name;
    ExprPtr initializer;
    bool isVolatile;
};

/**
 * @brief If statement
 */
class IfStmt : public Statement {
public:
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch = nullptr)
        : condition(std::move(condition)), 
          thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)) {}
    
    std::string toString() const override;
    
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
};

/**
 * @brief While statement
 */
class WhileStmt : public Statement {
public:
    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    std::string toString() const override;
    
    ExprPtr condition;
    StmtPtr body;
};

/**
 * @brief For statement
 */
class ForStmt : public Statement {
public:
    ForStmt(StmtPtr initializer, ExprPtr condition, ExprPtr increment, StmtPtr body)
        : initializer(std::move(initializer)), 
          condition(std::move(condition)), 
          increment(std::move(increment)), 
          body(std::move(body)) {}
    
    std::string toString() const override;
    
    StmtPtr initializer;
    ExprPtr condition;
    ExprPtr increment;
    StmtPtr body;
};

/**
 * @brief When statement (interrupt-style programming)
 */
class WhenStmt : public Statement {
public:
    WhenStmt(ExprPtr condition, StmtPtr body, bool isVolatile, bool isAsync = false)
        : condition(std::move(condition)), 
          body(std::move(body)), 
          isVolatile(isVolatile),
          isAsync(isAsync) {}
    
    std::string toString() const override;
    
    ExprPtr condition;
    StmtPtr body;
    bool isVolatile;
    bool isAsync;
};

/**
 * @brief Assembly block
 */
class AsmStmt : public Statement {
public:
    explicit AsmStmt(std::string asmCode)
        : asmCode(std::move(asmCode)) {}
    
    std::string toString() const override;
    
    std::string asmCode;
};

/**
 * @brief Function declaration
 */
class FunctionDeclarationStmt : public Statement {
public:
    struct Parameter {
        std::shared_ptr<Type> type;
        Token name;
    };
    
    FunctionDeclarationStmt(
        std::shared_ptr<Type> returnType,
        Token name,
        std::vector<Parameter> parameters,
        StmtPtr body,
        bool isVolatile = false,
        bool isAsync = false
    ) : returnType(std::move(returnType)),
        name(name),
        parameters(std::move(parameters)),
        body(std::move(body)),
        isVolatile(isVolatile),
        isAsync(isAsync) {}
    
    std::string toString() const override;
    
    std::shared_ptr<Type> returnType;
    Token name;
    std::vector<Parameter> parameters;
    StmtPtr body;
    bool isVolatile;
    bool isAsync;
};

/**
 * @brief Return statement
 */
class ReturnStmt : public Statement {
public:
    explicit ReturnStmt(ExprPtr value = nullptr)
        : value(std::move(value)) {}
    
    std::string toString() const override;
    
    ExprPtr value;
};

/**
 * @brief Break statement
 */
class BreakStmt : public Statement {
public:
    BreakStmt() = default;
    
    std::string toString() const override;
};

/**
 * @brief Continue statement
 */
class ContinueStmt : public Statement {
public:
    ContinueStmt() = default;
    
    std::string toString() const override;
};

/**
 * @brief Class declaration
 */
class ClassDeclarationStmt : public Statement {
public:
    ClassDeclarationStmt(Token name, std::vector<StmtPtr> members)
        : name(name), members(std::move(members)) {}
    
    std::string toString() const override;
    
    Token name;
    std::vector<StmtPtr> members;
};

/**
 * @brief Object declaration
 */
class ObjectDeclarationStmt : public Statement {
public:
    ObjectDeclarationStmt(Token name, std::vector<StmtPtr> members)
        : name(name), members(std::move(members)) {}
    
    std::string toString() const override;
    
    Token name;
    std::vector<StmtPtr> members;
};

/**
 * @brief Namespace declaration
 */
class NamespaceDeclarationStmt : public Statement {
public:
    NamespaceDeclarationStmt(Token name, std::vector<StmtPtr> declarations)
        : name(name), declarations(std::move(declarations)) {}
    
    std::string toString() const override;
    
    Token name;
    std::vector<StmtPtr> declarations;
};

/**
 * @brief Struct declaration
 */
class StructDeclarationStmt : public Statement {
public:
    struct Field {
        std::shared_ptr<Type> type;
        Token name;
    };
    
    StructDeclarationStmt(Token name, std::vector<Field> fields)
        : name(name), fields(std::move(fields)) {}
    
    std::string toString() const override;
    
    Token name;
    std::vector<Field> fields;
};

/**
 * @brief Operator declaration
 */
class OperatorDeclarationStmt : public Statement {
public:
    OperatorDeclarationStmt(
        std::shared_ptr<Type> leftType,
        std::shared_ptr<Type> rightType,
        Token op,
        StmtPtr body
    ) : leftType(std::move(leftType)),
        rightType(std::move(rightType)),
        op(op),
        body(std::move(body)) {}
    
    std::string toString() const override;
    
    std::shared_ptr<Type> leftType;
    std::shared_ptr<Type> rightType;
    Token op;
    StmtPtr body;
};

/**
 * @brief Lock statement for async functions
 */
class LockStmt : public Statement {
public:
    enum class LockType {
        LOCK,       // lock()
        PRE_LOCK,   // __lock
        POST_LOCK   // lock__
    };
    
    LockStmt(LockType type, std::vector<Token> scopes, StmtPtr body = nullptr)
        : type(type), scopes(std::move(scopes)), body(std::move(body)) {}
    
    virtual ~LockStmt() = default;  // Add this line
    
    std::string toString() const override;
    
    LockType type;
    std::vector<Token> scopes;
    StmtPtr body;
};

/**
 * @brief Print statement (Python-like print() function)
 */
class PrintStmt : public Statement {
public:
    explicit PrintStmt(std::vector<ExprPtr> arguments)
        : arguments(std::move(arguments)) {}
    
    std::string toString() const override;
    
    std::vector<ExprPtr> arguments;
};

/**
 * @brief Input statement (Python-like input() function)
 */
class InputStmt : public Statement {
public:
    explicit InputStmt(ExprPtr prompt = nullptr, Token variable = Token(TokenType::IDENTIFIER, "", 0))
        : prompt(std::move(prompt)), variable(variable) {}
    
    std::string toString() const override;
    
    ExprPtr prompt;     // Optional prompt to display
    Token variable;     // Optional variable to store into
};

/**
 * @brief Open file statement (Python-like open() function)
 */
class OpenStmt : public Statement {
public:
    enum class Mode {
        READ,       // "r"
        WRITE,      // "w"
        APPEND,     // "a" 
        READ_PLUS,  // "r+"
        WRITE_PLUS, // "w+"
        APPEND_PLUS // "a+"
    };
    
    OpenStmt(ExprPtr filename, Mode mode, Token variable)
        : filename(std::move(filename)), mode(mode), variable(variable) {}
    
    std::string toString() const override;
    
    ExprPtr filename;   // Filename expression
    Mode mode;          // File open mode
    Token variable;     // Variable to store the file handle
};

} // namespace flux
