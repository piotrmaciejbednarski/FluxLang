#ifndef FLUX_AST_H
#define FLUX_AST_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include <optional>

namespace flux {

// Forward declarations
class Type;
class Expression;
class Statement;
class Declaration;

// Location information for error reporting
struct SourceLocation {
    std::string_view filename;
    int line;
    int column;
    int length;
};

// Base AST node class
class ASTNode {
public:
    ASTNode(SourceLocation loc) : location(loc) {}
    virtual ~ASTNode() = default;
    
    SourceLocation getLocation() const { return location; }
    
protected:
    SourceLocation location;
};

// ----- Types -----

// Primitive type bitwidths
enum class BitWidth {
    DEFAULT,
    _8,
    _16,
    _32,
    _64,
    _128
};

// Primitive types
enum class PrimitiveTypeKind {
    CHAR,
    INT,
    FLOAT,
    VOID,
    BOOL
};

class Type : public ASTNode {
public:
    Type(SourceLocation loc) : ASTNode(loc) {}
    virtual bool isEqual(const Type& other) const = 0;
    virtual std::string toString() const = 0;
};

class PrimitiveType : public Type {
public:
    PrimitiveType(SourceLocation loc, PrimitiveTypeKind kind, BitWidth width = BitWidth::DEFAULT, bool isUnsigned = false)
        : Type(loc), kind(kind), width(width), isUnsigned(isUnsigned) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    PrimitiveTypeKind getKind() const { return kind; }
    BitWidth getWidth() const { return width; }
    bool getIsUnsigned() const { return isUnsigned; }
    
private:
    PrimitiveTypeKind kind;
    BitWidth width;
    bool isUnsigned;
};

class ArrayType : public Type {
public:
    ArrayType(SourceLocation loc, std::shared_ptr<Type> elementType, std::optional<size_t> size = std::nullopt)
        : Type(loc), elementType(elementType), size(size) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    std::shared_ptr<Type> getElementType() const { return elementType; }
    std::optional<size_t> getSize() const { return size; }
    
private:
    std::shared_ptr<Type> elementType;
    std::optional<size_t> size;  // std::nullopt for dynamically sized arrays
};

class PointerType : public Type {
public:
    PointerType(SourceLocation loc, std::shared_ptr<Type> pointeeType)
        : Type(loc), pointeeType(pointeeType) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    std::shared_ptr<Type> getPointeeType() const { return pointeeType; }
    
private:
    std::shared_ptr<Type> pointeeType;
};

class StructType : public Type {
public:
    struct Field {
        std::string name;
        std::shared_ptr<Type> type;
    };
    
    StructType(SourceLocation loc, std::vector<Field> fields)
        : Type(loc), fields(std::move(fields)) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    const std::vector<Field>& getFields() const { return fields; }
    
private:
    std::vector<Field> fields;
};

class FunctionType : public Type {
public:
    FunctionType(SourceLocation loc, 
                std::shared_ptr<Type> returnType, 
                std::vector<std::shared_ptr<Type>> paramTypes)
        : Type(loc), returnType(returnType), paramTypes(std::move(paramTypes)) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    const std::vector<std::shared_ptr<Type>>& getParamTypes() const { return paramTypes; }
    
private:
    std::shared_ptr<Type> returnType;
    std::vector<std::shared_ptr<Type>> paramTypes;
};

class UserDefinedType : public Type {
public:
    UserDefinedType(SourceLocation loc, std::string name)
        : Type(loc), name(std::move(name)) {}
    
    bool isEqual(const Type& other) const override;
    std::string toString() const override;
    
    const std::string& getName() const { return name; }
    
private:
    std::string name;
};

// ----- Expressions -----

class Expression : public ASTNode {
public:
    Expression(SourceLocation loc) : ASTNode(loc) {}
    virtual std::shared_ptr<Type> getType() const = 0;
};

class LiteralExpression : public Expression {
public:
    using LiteralValue = std::variant<
        int64_t,          // Integer literal
        double,           // Float literal
        bool,             // Boolean literal
        char,             // Character literal
        std::string,      // String literal
        std::nullptr_t    // Null literal
    >;
    
    LiteralExpression(SourceLocation loc, LiteralValue value, std::shared_ptr<Type> type)
        : Expression(loc), value(value), type(type) {}
    
    std::shared_ptr<Type> getType() const override { return type; }
    const LiteralValue& getValue() const { return value; }
    
private:
    LiteralValue value;
    std::shared_ptr<Type> type;
};

class ScopeResolutionExpression : public Expression {
public:
    ScopeResolutionExpression(SourceLocation loc, 
                             std::shared_ptr<Expression> scope,
                             std::string identifier)
        : Expression(loc), scope(scope), identifier(std::move(identifier)) {}
    
    std::shared_ptr<Type> getType() const;
    
    std::shared_ptr<Expression> getScope() const { return scope; }
    const std::string& getIdentifier() const { return identifier; }
    
private:
    std::shared_ptr<Expression> scope;
    std::string identifier;
};

class ArrayLiteralExpression : public Expression {
public:
    ArrayLiteralExpression(SourceLocation loc, std::vector<std::shared_ptr<Expression>> elements)
        : Expression(loc), elements(std::move(elements)) {}
    
    std::shared_ptr<Type> getType() const override;
    const std::vector<std::shared_ptr<Expression>>& getElements() const { return elements; }
    
private:
    std::vector<std::shared_ptr<Expression>> elements;
};

class IdentifierExpression : public Expression {
public:
    IdentifierExpression(SourceLocation loc, std::string name)
        : Expression(loc), name(std::move(name)) {}
    
    std::shared_ptr<Type> getType() const override;
    const std::string& getName() const { return name; }
    
private:
    std::string name;
};

class BinaryExpression : public Expression {
public:
    enum class Operator {
        ADD, SUB, MUL, DIV, MOD,
        EQ, NE, LT, LE, GT, GE,
        AND, OR, XOR,
        BIT_AND, BIT_OR, BIT_XOR, BIT_SHL, BIT_SHR,
        ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
        BIT_AND_ASSIGN, BIT_OR_ASSIGN, BIT_XOR_ASSIGN, BIT_SHL_ASSIGN, BIT_SHR_ASSIGN,
        CUSTOM
    };
    
    BinaryExpression(SourceLocation loc, 
                    Operator op, 
                    std::shared_ptr<Expression> left,
                    std::shared_ptr<Expression> right,
                    std::string customOp = "")
        : Expression(loc), op(op), left(left), right(right), customOp(customOp) {}
    
    std::shared_ptr<Type> getType() const override;
    
    Operator getOperator() const { return op; }
    const std::string& getCustomOperator() const { return customOp; }
    std::shared_ptr<Expression> getLeft() const { return left; }
    std::shared_ptr<Expression> getRight() const { return right; }
    
private:
    Operator op;
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    std::string customOp;  // Only used when op is CUSTOM
};

class UnaryExpression : public Expression {
public:
    enum class Operator {
        NEG, NOT, BIT_NOT,
        PRE_INC, PRE_DEC,
        POST_INC, POST_DEC,
        ADDRESS_OF, DEREFERENCE
    };
    
    UnaryExpression(SourceLocation loc, Operator op, std::shared_ptr<Expression> operand)
        : Expression(loc), op(op), operand(operand) {}
    
    std::shared_ptr<Type> getType() const override;
    
    Operator getOperator() const { return op; }
    std::shared_ptr<Expression> getOperand() const { return operand; }
    
private:
    Operator op;
    std::shared_ptr<Expression> operand;
};

class CallExpression : public Expression {
public:
    CallExpression(SourceLocation loc, 
                  std::shared_ptr<Expression> callee,
                  std::vector<std::shared_ptr<Expression>> args)
        : Expression(loc), callee(callee), args(std::move(args)) {}
    
    std::shared_ptr<Type> getType() const override;
    
    std::shared_ptr<Expression> getCallee() const { return callee; }
    const std::vector<std::shared_ptr<Expression>>& getArgs() const { return args; }
    
private:
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> args;
};

class MemberAccessExpression : public Expression {
public:
    MemberAccessExpression(SourceLocation loc, 
                          std::shared_ptr<Expression> object,
                          std::string member,
                          bool isArrow)
        : Expression(loc), object(object), member(std::move(member)), isArrow(isArrow) {}
    
    std::shared_ptr<Type> getType() const override;
    
    std::shared_ptr<Expression> getObject() const { return object; }
    const std::string& getMember() const { return member; }
    bool getIsArrow() const { return isArrow; }
    
private:
    std::shared_ptr<Expression> object;
    std::string member;
    bool isArrow;  // true for '->', false for '.'
};

class IndexExpression : public Expression {
public:
    IndexExpression(SourceLocation loc, 
                   std::shared_ptr<Expression> array,
                   std::shared_ptr<Expression> index)
        : Expression(loc), array(array), index(index) {}
    
    std::shared_ptr<Type> getType() const override;
    
    std::shared_ptr<Expression> getArray() const { return array; }
    std::shared_ptr<Expression> getIndex() const { return index; }
    
private:
    std::shared_ptr<Expression> array;
    std::shared_ptr<Expression> index;
};

class CastExpression : public Expression {
public:
    CastExpression(SourceLocation loc, 
                  std::shared_ptr<Type> targetType,
                  std::shared_ptr<Expression> expr)
        : Expression(loc), targetType(targetType), expr(expr) {}
    
    std::shared_ptr<Type> getType() const override { return targetType; }
    
    std::shared_ptr<Expression> getExpr() const { return expr; }
    
private:
    std::shared_ptr<Type> targetType;
    std::shared_ptr<Expression> expr;
};

class InjectableStringExpression : public Expression {
public:
    InjectableStringExpression(SourceLocation loc,
                              std::string format,
                              std::vector<std::shared_ptr<Expression>> args)
        : Expression(loc), format(std::move(format)), args(std::move(args)) {}
    
    std::shared_ptr<Type> getType() const override;
    
    const std::string& getFormat() const { return format; }
    const std::vector<std::shared_ptr<Expression>>& getArgs() const { return args; }
    
private:
    std::string format;
    std::vector<std::shared_ptr<Expression>> args;
};

// ----- Statements -----

class Statement : public ASTNode {
public:
    Statement(SourceLocation loc) : ASTNode(loc) {}
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(SourceLocation loc, std::shared_ptr<Expression> expr)
        : Statement(loc), expr(expr) {}
    
    std::shared_ptr<Expression> getExpr() const { return expr; }
    
private:
    std::shared_ptr<Expression> expr;
};

class BlockStatement : public Statement {
public:
    BlockStatement(SourceLocation loc, std::vector<std::shared_ptr<Statement>> statements)
        : Statement(loc), statements(std::move(statements)) {}
    
    const std::vector<std::shared_ptr<Statement>>& getStatements() const { return statements; }
    
private:
    std::vector<std::shared_ptr<Statement>> statements;
};

class IfStatement : public Statement {
public:
    IfStatement(SourceLocation loc, 
               std::shared_ptr<Expression> condition,
               std::shared_ptr<Statement> thenBranch,
               std::shared_ptr<Statement> elseBranch = nullptr)
        : Statement(loc), condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
    
    std::shared_ptr<Expression> getCondition() const { return condition; }
    std::shared_ptr<Statement> getThenBranch() const { return thenBranch; }
    std::shared_ptr<Statement> getElseBranch() const { return elseBranch; }
    
private:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> thenBranch;
    std::shared_ptr<Statement> elseBranch;
};

class WhileStatement : public Statement {
public:
    WhileStatement(SourceLocation loc, 
                  std::shared_ptr<Expression> condition,
                  std::shared_ptr<Statement> body)
        : Statement(loc), condition(condition), body(body) {}
    
    std::shared_ptr<Expression> getCondition() const { return condition; }
    std::shared_ptr<Statement> getBody() const { return body; }
    
private:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> body;
};

class ForStatement : public Statement {
public:
    ForStatement(SourceLocation loc, 
                std::shared_ptr<Statement> init,
                std::shared_ptr<Expression> condition,
                std::shared_ptr<Expression> update,
                std::shared_ptr<Statement> body)
        : Statement(loc), init(init), condition(condition), update(update), body(body) {}
    
    std::shared_ptr<Statement> getInit() const { return init; }
    std::shared_ptr<Expression> getCondition() const { return condition; }
    std::shared_ptr<Expression> getUpdate() const { return update; }
    std::shared_ptr<Statement> getBody() const { return body; }
    
private:
    std::shared_ptr<Statement> init;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> update;
    std::shared_ptr<Statement> body;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(SourceLocation loc, std::shared_ptr<Expression> value = nullptr)
        : Statement(loc), value(value) {}
    
    std::shared_ptr<Expression> getValue() const { return value; }
    
private:
    std::shared_ptr<Expression> value;
};

class BreakStatement : public Statement {
public:
    BreakStatement(SourceLocation loc) : Statement(loc) {}
};

class ContinueStatement : public Statement {
public:
    ContinueStatement(SourceLocation loc) : Statement(loc) {}
};

class ThrowStatement : public Statement {
public:
    ThrowStatement(SourceLocation loc, 
                  std::shared_ptr<Expression> exception,
                  std::shared_ptr<Statement> handler = nullptr)
        : Statement(loc), exception(exception), handler(handler) {}
    
    std::shared_ptr<Expression> getException() const { return exception; }
    std::shared_ptr<Statement> getHandler() const { return handler; }
    
private:
    std::shared_ptr<Expression> exception;
    std::shared_ptr<Statement> handler;
};

class TryCatchStatement : public Statement {
public:
    struct CatchClause {
        std::string exceptionVar;
        std::shared_ptr<Type> exceptionType;
        std::shared_ptr<Statement> body;
    };
    
    TryCatchStatement(SourceLocation loc,
                     std::shared_ptr<Statement> tryBlock,
                     std::vector<CatchClause> catchClauses)
        : Statement(loc), tryBlock(tryBlock), catchClauses(std::move(catchClauses)) {}
    
    std::shared_ptr<Statement> getTryBlock() const { return tryBlock; }
    const std::vector<CatchClause>& getCatchClauses() const { return catchClauses; }
    
private:
    std::shared_ptr<Statement> tryBlock;
    std::vector<CatchClause> catchClauses;
};

class ASMStatement : public Statement {
public:
    ASMStatement(SourceLocation loc, std::string asmCode)
        : Statement(loc), asmCode(std::move(asmCode)) {}
    
    const std::string& getASMCode() const { return asmCode; }
    
private:
    std::string asmCode;
};

// ----- Declarations -----

class Declaration : public ASTNode {
public:
    Declaration(SourceLocation loc, std::string name) 
        : ASTNode(loc), name(std::move(name)) {}
    
    const std::string& getName() const { return name; }
    
private:
    std::string name;
};

class ImportDeclaration : public Declaration {
public:
    ImportDeclaration(SourceLocation loc, std::string path)
        : Declaration(loc, "import"), path(std::move(path)) {}
    
    const std::string& getPath() const { return path; }
    
private:
    std::string path;
};

class BuiltinCallExpression : public Expression {
public:
    enum class BuiltinType {
        PRINT,
        INPUT,
        OPEN,
        SOCKET
    };
    
    BuiltinCallExpression(SourceLocation loc, 
                         BuiltinType builtinType,
                         std::vector<std::shared_ptr<Expression>> args)
        : Expression(loc), builtinType(builtinType), args(std::move(args)) {}
    
    std::shared_ptr<Type> getType() const override;
    
    BuiltinType getBuiltinType() const { return builtinType; }
    const std::vector<std::shared_ptr<Expression>>& getArgs() const { return args; }
    
private:
    BuiltinType builtinType;
    std::vector<std::shared_ptr<Expression>> args;
};

class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(SourceLocation loc,
                       std::string name,
                       std::shared_ptr<Type> type,
                       std::shared_ptr<Expression> initializer = nullptr)
        : Declaration(loc, std::move(name)), type(type), initializer(initializer) {}
    
    std::shared_ptr<Type> getType() const { return type; }
    std::shared_ptr<Expression> getInitializer() const { return initializer; }
    
private:
    std::shared_ptr<Type> type;
    std::shared_ptr<Expression> initializer;
};

class Parameter {
public:
    Parameter(std::string name, std::shared_ptr<Type> type, bool isPointer = false)
        : name(std::move(name)), type(type), isPointer(isPointer) {}
    
    const std::string& getName() const { return name; }
    std::shared_ptr<Type> getType() const { return type; }
    bool getIsPointer() const { return isPointer; }
    
private:
    std::string name;
    std::shared_ptr<Type> type;
    bool isPointer;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(SourceLocation loc,
                       std::string name,
                       std::shared_ptr<Type> returnType,
                       std::vector<Parameter> params,
                       std::shared_ptr<BlockStatement> body = nullptr)
        : Declaration(loc, std::move(name)), returnType(returnType), 
          params(std::move(params)), body(body) {}
    
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    const std::vector<Parameter>& getParams() const { return params; }
    std::shared_ptr<BlockStatement> getBody() const { return body; }
    
private:
    std::shared_ptr<Type> returnType;
    std::vector<Parameter> params;
    std::shared_ptr<BlockStatement> body;
};

class StructDeclaration : public Declaration {
public:
    StructDeclaration(SourceLocation loc,
                     std::string name,
                     std::vector<VariableDeclaration> fields)
        : Declaration(loc, std::move(name)), fields(std::move(fields)) {}
    
    const std::vector<VariableDeclaration>& getFields() const { return fields; }
    
private:
    std::vector<VariableDeclaration> fields;
};

class OperatorDeclaration : public Declaration {
public:
    OperatorDeclaration(SourceLocation loc,
                       std::string op,
                       std::vector<Parameter> params,
                       std::shared_ptr<Type> returnType,
                       std::shared_ptr<BlockStatement> body)
        : Declaration(loc, "operator" + op), op(std::move(op)), params(std::move(params)),
          returnType(returnType), body(body) {}
    
    const std::string& getOp() const { return op; }
    const std::vector<Parameter>& getParams() const { return params; }
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    std::shared_ptr<BlockStatement> getBody() const { return body; }
    
private:
    std::string op;
    std::vector<Parameter> params;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<BlockStatement> body;
};

class ObjectDeclaration : public Declaration {
public:
    ObjectDeclaration(SourceLocation loc,
                     std::string name,
                     std::vector<std::shared_ptr<Declaration>> members)
        : Declaration(loc, std::move(name)), members(std::move(members)) {}
    
    const std::vector<std::shared_ptr<Declaration>>& getMembers() const { return members; }
    
private:
    std::vector<std::shared_ptr<Declaration>> members;
};

class ClassDeclaration : public Declaration {
public:
    ClassDeclaration(SourceLocation loc,
                    std::string name,
                    std::vector<std::shared_ptr<Declaration>> members)
        : Declaration(loc, std::move(name)), members(std::move(members)) {}
    
    const std::vector<std::shared_ptr<Declaration>>& getMembers() const { return members; }
    
private:
    std::vector<std::shared_ptr<Declaration>> members;
};

class NamespaceDeclaration : public Declaration {
public:
    NamespaceDeclaration(SourceLocation loc,
                        std::string name,
                        std::vector<std::shared_ptr<ClassDeclaration>> classes)
        : Declaration(loc, std::move(name)), classes(std::move(classes)) {}
    
    const std::vector<std::shared_ptr<ClassDeclaration>>& getClasses() const { return classes; }
    
private:
    std::vector<std::shared_ptr<ClassDeclaration>> classes;
};

// Program root
class Program : public ASTNode {
public:
    Program(SourceLocation loc, std::vector<std::shared_ptr<Declaration>> declarations)
        : ASTNode(loc), declarations(std::move(declarations)) {}
    
    const std::vector<std::shared_ptr<Declaration>>& getDeclarations() const { return declarations; }
    
private:
    std::vector<std::shared_ptr<Declaration>> declarations;
};

} // namespace flux

#endif // FLUX_AST_H
