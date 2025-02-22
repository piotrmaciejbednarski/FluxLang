#ifndef FLUX_AST_H
#define FLUX_AST_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include "error.h"

namespace flux {

// Forward declarations
class MemoryArena;
class Type;
class Value;

// Location in source code for error reporting
struct AstLocation {
    std::string_view filename;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;
    
    AstLocation() : filename(""), startLine(0), startColumn(0), endLine(0), endColumn(0) {}
    
    AstLocation(std::string_view file, int sl, int sc, int el, int ec)
        : filename(file), startLine(sl), startColumn(sc), endLine(el), endColumn(ec) {}
    
    SourceLocation toSourceLocation() const {
        return SourceLocation(filename, startLine, startColumn);
    }
};

// Base class for all AST nodes
class AstNode {
protected:
    AstLocation location;
    
public:
    AstNode() = default;
    explicit AstNode(AstLocation location) : location(std::move(location)) {}
    virtual ~AstNode() = default;
    
    const AstLocation& getLocation() const { return location; }
    void setLocation(const AstLocation& loc) { location = loc; }
};

// Type system classes
class Type {
public:
	enum class Kind {
	    VOID,
	    BOOL,
	    INT,
	    FLOAT,
	    STRING,
	    STRUCT,
	    CLASS,
	    OBJECT,
	    FUNCTION,
	    POINTER,
	    UNION,
	    NULLPTR
	};
    
    virtual ~Type() = default;
    virtual Kind getKind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool isEquivalentTo(const Type* other) const = 0;
};

// Primitive type (int, float, etc.)
class PrimitiveType : public Type {
private:
    Kind kind;
    int bitWidth; // For int{32}, float{64}, etc.
    bool isUnsigned; // For unsigned int
    
public:
    PrimitiveType(Kind kind, int bitWidth = 0, bool isUnsigned = false) 
        : kind(kind), bitWidth(bitWidth), isUnsigned(isUnsigned) {}
    
    Kind getKind() const override { return kind; }
    int getBitWidth() const { return bitWidth; }
    bool getIsUnsigned() const { return isUnsigned; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Pointer type
class PointerType : public Type {
private:
    std::shared_ptr<Type> pointeeType;
    
public:
    explicit PointerType(std::shared_ptr<Type> pointee) : pointeeType(std::move(pointee)) {}
    
    Kind getKind() const override { return Kind::POINTER; }
    const std::shared_ptr<Type>& getPointeeType() const { return pointeeType; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Structure field
struct StructField {
    std::string name;
    std::shared_ptr<Type> type;
    
    StructField(std::string name, std::shared_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {}
};

// Struct type
class StructType : public Type {
private:
    std::string name;
    std::vector<StructField> fields;
    
public:
    explicit StructType(std::string name) : name(std::move(name)) {}
    
    Kind getKind() const override { return Kind::STRUCT; }
    const std::string& getName() const { return name; }
    
    void addField(const StructField& field) { fields.push_back(field); }
    const std::vector<StructField>& getFields() const { return fields; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Function parameter
struct FunctionParam {
    std::string name;
    std::shared_ptr<Type> type;
    
    FunctionParam(std::string name, std::shared_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {}
};

// Function type
class FunctionType : public Type {
private:
    std::shared_ptr<Type> returnType;
    std::vector<FunctionParam> parameters;
    
public:
    explicit FunctionType(std::shared_ptr<Type> returnType)
        : returnType(std::move(returnType)) {}
    
    Kind getKind() const override { return Kind::FUNCTION; }
    const std::shared_ptr<Type>& getReturnType() const { return returnType; }
    
    void addParameter(const FunctionParam& param) { parameters.push_back(param); }
    const std::vector<FunctionParam>& getParameters() const { return parameters; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Class type
class ClassType : public Type {
private:
    std::string name;
    std::vector<StructField> fields;
    std::vector<std::shared_ptr<FunctionType>> methods;
    
public:
    explicit ClassType(std::string name) : name(std::move(name)) {}
    
    Kind getKind() const override { return Kind::CLASS; }
    const std::string& getName() const { return name; }
    
    void addField(const StructField& field) { fields.push_back(field); }
    const std::vector<StructField>& getFields() const { return fields; }
    
    void addMethod(std::shared_ptr<FunctionType> method) { methods.push_back(std::move(method)); }
    const std::vector<std::shared_ptr<FunctionType>>& getMethods() const { return methods; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Object type
class ObjectType : public Type {
private:
    std::string name;
    std::vector<StructField> fields;
    std::vector<std::shared_ptr<FunctionType>> methods;
    
public:
    explicit ObjectType(std::string name) : name(std::move(name)) {}
    
    Kind getKind() const override { return Kind::OBJECT; }
    const std::string& getName() const { return name; }
    
    void addField(const StructField& field) { fields.push_back(field); }
    const std::vector<StructField>& getFields() const { return fields; }
    
    void addMethod(std::shared_ptr<FunctionType> method) { methods.push_back(std::move(method)); }
    const std::vector<std::shared_ptr<FunctionType>>& getMethods() const { return methods; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Union type
class UnionType : public Type {
private:
    std::string name;
    std::vector<StructField> variants;
    
public:
    explicit UnionType(std::string name) : name(std::move(name)) {}
    
    Kind getKind() const override { return Kind::UNION; }
    const std::string& getName() const { return name; }
    
    void addVariant(const StructField& variant) { variants.push_back(variant); }
    const std::vector<StructField>& getVariants() const { return variants; }
    
    std::string toString() const override;
    bool isEquivalentTo(const Type* other) const override;
};

// Expression base class
class Expression : public AstNode {
public:
    using AstNode::AstNode;
    virtual std::shared_ptr<Type> getType() const = 0;
    virtual std::shared_ptr<Value> evaluate() = 0;
};

// Literal expression (numbers, booleans, strings)
class LiteralExpression : public Expression {
public:
    using LiteralValue = std::variant<bool, int, double, std::string>;
    
private:
    LiteralValue value;
    std::shared_ptr<Type> type;
    
public:
    LiteralExpression(LiteralValue value, std::shared_ptr<Type> type, AstLocation location)
        : Expression(location), value(std::move(value)), type(std::move(type)) {}
    
    const LiteralValue& getValue() const { return value; }
    std::shared_ptr<Type> getType() const override { return type; }
    std::shared_ptr<Value> evaluate() override;
};

// Variable reference
class VariableExpression : public Expression {
private:
    std::string name;
    std::shared_ptr<Type> type;
    
public:
    VariableExpression(std::string name, AstLocation location)
        : Expression(location), name(std::move(name)) {}
    
    const std::string& getName() const { return name; }
    void setType(std::shared_ptr<Type> t) { type = std::move(t); }
    std::shared_ptr<Type> getType() const override { return type; }
    std::shared_ptr<Value> evaluate() override;
};

// Binary operation
enum class BinaryOp {
    ADD, SUB, MUL, DIV, MOD,
    LOGICAL_AND, LOGICAL_OR,
    EQ, NE, LT, LE, GT, GE,
    AND, OR, BITAND, BITOR, BITXOR,
    SHIFTLEFT, SHIFTRIGHT,
    EXPONENT  // ** operator in Flux
};

class BinaryExpression : public Expression {
private:
    BinaryOp op;
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    std::shared_ptr<Type> resultType;
    
public:
    BinaryExpression(BinaryOp op, std::shared_ptr<Expression> left, 
                    std::shared_ptr<Expression> right, AstLocation location)
        : Expression(location), op(op), left(std::move(left)), right(std::move(right)) {}
    
    BinaryOp getOperator() const { return op; }
    const std::shared_ptr<Expression>& getLeft() const { return left; }
    const std::shared_ptr<Expression>& getRight() const { return right; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Unary operation
enum class UnaryOp {
    NEGATE, NOT, BITNOT, DEREFERENCE, ADDRESS_OF,
    INCREMENT, DECREMENT
};

class UnaryExpression : public Expression {
private:
    UnaryOp op;
    std::shared_ptr<Expression> operand;
    std::shared_ptr<Type> resultType;
    
public:
    UnaryExpression(UnaryOp op, std::shared_ptr<Expression> operand, AstLocation location)
        : Expression(location), op(op), operand(std::move(operand)) {}
    
    UnaryOp getOperator() const { return op; }
    const std::shared_ptr<Expression>& getOperand() const { return operand; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Function call
class CallExpression : public Expression {
private:
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> arguments;
    std::shared_ptr<Type> resultType;
    
public:
    CallExpression(std::shared_ptr<Expression> callee, 
                  std::vector<std::shared_ptr<Expression>> args,
                  AstLocation location)
        : Expression(location), callee(std::move(callee)), arguments(std::move(args)) {}
    
    const std::shared_ptr<Expression>& getCallee() const { return callee; }
    const std::vector<std::shared_ptr<Expression>>& getArguments() const { return arguments; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Array access
class IndexExpression : public Expression {
private:
    std::shared_ptr<Expression> array;
    std::shared_ptr<Expression> index;
    std::shared_ptr<Type> resultType;
    
public:
    IndexExpression(std::shared_ptr<Expression> array, 
                   std::shared_ptr<Expression> index,
                   AstLocation location)
        : Expression(location), array(std::move(array)), index(std::move(index)) {}
    
    const std::shared_ptr<Expression>& getArray() const { return array; }
    const std::shared_ptr<Expression>& getIndex() const { return index; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Member access (struct.field)
class MemberExpression : public Expression {
private:
    std::shared_ptr<Expression> object;
    std::string memberName;
    std::shared_ptr<Type> resultType;
    
public:
    MemberExpression(std::shared_ptr<Expression> object, 
                    std::string memberName,
                    AstLocation location)
        : Expression(location), object(std::move(object)), memberName(std::move(memberName)) {}
    
    const std::shared_ptr<Expression>& getObject() const { return object; }
    const std::string& getMemberName() const { return memberName; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Arrow member access (ptr->field)
class ArrowExpression : public Expression {
private:
    std::shared_ptr<Expression> pointer;
    std::string memberName;
    std::shared_ptr<Type> resultType;
    
public:
    ArrowExpression(std::shared_ptr<Expression> pointer, 
                   std::string memberName,
                   AstLocation location)
        : Expression(location), pointer(std::move(pointer)), memberName(std::move(memberName)) {}
    
    const std::shared_ptr<Expression>& getPointer() const { return pointer; }
    const std::string& getMemberName() const { return memberName; }
    void setResultType(std::shared_ptr<Type> type) { resultType = std::move(type); }
    std::shared_ptr<Type> getType() const override { return resultType; }
    std::shared_ptr<Value> evaluate() override;
};

// Array literal expression
class ArrayLiteralExpression : public Expression {
private:
    std::vector<std::shared_ptr<Expression>> elements;
    std::shared_ptr<Type> type;
    
public:
    ArrayLiteralExpression(std::vector<std::shared_ptr<Expression>> elements, 
                          AstLocation location)
        : Expression(location), elements(std::move(elements)) {}
    
    const std::vector<std::shared_ptr<Expression>>& getElements() const { return elements; }
    void setType(std::shared_ptr<Type> t) { type = std::move(t); }
    std::shared_ptr<Type> getType() const override { return type; }
    std::shared_ptr<Value> evaluate() override;
};

// Statement base class
class Statement : public AstNode {
public:
    using AstNode::AstNode;
    virtual void execute() = 0;
};

// Expression statement
class ExpressionStatement : public Statement {
private:
    std::shared_ptr<Expression> expression;
    
public:
    ExpressionStatement(std::shared_ptr<Expression> expr, AstLocation location)
        : Statement(location), expression(std::move(expr)) {}
    
    const std::shared_ptr<Expression>& getExpression() const { return expression; }
    void execute() override;
};

// Block statement (compound statement)
class BlockStatement : public Statement {
private:
    std::vector<std::shared_ptr<Statement>> statements;
    
public:
    explicit BlockStatement(AstLocation location) : Statement(location) {}
    
    void addStatement(std::shared_ptr<Statement> stmt) { statements.push_back(std::move(stmt)); }
    const std::vector<std::shared_ptr<Statement>>& getStatements() const { return statements; }
    void execute() override;
};

// Variable declaration
// Variable declaration
class VariableDeclaration : public Statement {
private:
    std::string name;
    std::shared_ptr<Type> type;
    std::shared_ptr<Expression> initializer;
    bool isGlobal;
    
public:
    VariableDeclaration(std::string name, std::shared_ptr<Type> type, 
                       std::shared_ptr<Expression> initializer,
                       AstLocation location,
                       bool isGlobal = false)
        : Statement(location), name(std::move(name)), type(std::move(type)), 
          initializer(std::move(initializer)), isGlobal(isGlobal) {}
    
    const std::string& getName() const { return name; }
    const std::shared_ptr<Type>& getType() const { return type; }
    const std::shared_ptr<Expression>& getInitializer() const { return initializer; }
    bool getIsGlobal() const { return isGlobal; }
    void execute() override;
};

// If statement
class IfStatement : public Statement {
private:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> thenBranch;
    std::shared_ptr<Statement> elseBranch; // Optional
    
public:
    IfStatement(std::shared_ptr<Expression> condition,
               std::shared_ptr<Statement> thenBranch,
               std::shared_ptr<Statement> elseBranch,
               AstLocation location)
        : Statement(location), condition(std::move(condition)), 
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    const std::shared_ptr<Expression>& getCondition() const { return condition; }
    const std::shared_ptr<Statement>& getThenBranch() const { return thenBranch; }
    const std::shared_ptr<Statement>& getElseBranch() const { return elseBranch; }
    void execute() override;
};

// While statement
class WhileStatement : public Statement {
private:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> body;
    
public:
    WhileStatement(std::shared_ptr<Expression> condition,
                  std::shared_ptr<Statement> body,
                  AstLocation location)
        : Statement(location), condition(std::move(condition)), body(std::move(body)) {}
    
    const std::shared_ptr<Expression>& getCondition() const { return condition; }
    const std::shared_ptr<Statement>& getBody() const { return body; }
    void execute() override;
};

// For statement
class ForStatement : public Statement {
private:
    std::shared_ptr<Statement> initializer;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    std::shared_ptr<Statement> body;
    
public:
    ForStatement(std::shared_ptr<Statement> initializer,
                std::shared_ptr<Expression> condition,
                std::shared_ptr<Expression> increment,
                std::shared_ptr<Statement> body,
                AstLocation location)
        : Statement(location), initializer(std::move(initializer)), 
          condition(std::move(condition)), increment(std::move(increment)), 
          body(std::move(body)) {}
    
    const std::shared_ptr<Statement>& getInitializer() const { return initializer; }
    const std::shared_ptr<Expression>& getCondition() const { return condition; }
    const std::shared_ptr<Expression>& getIncrement() const { return increment; }
    const std::shared_ptr<Statement>& getBody() const { return body; }
    void execute() override;
};

// Return statement
class ReturnStatement : public Statement {
private:
    std::shared_ptr<Expression> value; // Optional
    
public:
    ReturnStatement(std::shared_ptr<Expression> value, AstLocation location)
        : Statement(location), value(std::move(value)) {}
    
    const std::shared_ptr<Expression>& getValue() const { return value; }
    void execute() override;
};

// Break statement
class BreakStatement : public Statement {
public:
    explicit BreakStatement(AstLocation location) : Statement(location) {}
    void execute() override;
};

// Continue statement
class ContinueStatement : public Statement {
public:
    explicit ContinueStatement(AstLocation location) : Statement(location) {}
    void execute() override;
};

// Throw statement
class ThrowStatement : public Statement {
private:
    std::shared_ptr<Expression> exception;
    std::shared_ptr<Statement> handler; // Optional code block to execute when thrown
    
public:
    ThrowStatement(std::shared_ptr<Expression> exception, 
                  std::shared_ptr<Statement> handler,
                  AstLocation location)
        : Statement(location), exception(std::move(exception)), handler(std::move(handler)) {}
    
    const std::shared_ptr<Expression>& getException() const { return exception; }
    const std::shared_ptr<Statement>& getHandler() const { return handler; }
    void execute() override;
};

// Try-catch statement
class TryCatchStatement : public Statement {
private:
    std::shared_ptr<Statement> tryBlock;
    std::shared_ptr<Statement> catchBlock;
    std::string exceptionVar;
    
public:
    TryCatchStatement(std::shared_ptr<Statement> tryBlock,
                     std::shared_ptr<Statement> catchBlock,
                     std::string exceptionVar,
                     AstLocation location)
        : Statement(location), tryBlock(std::move(tryBlock)), 
          catchBlock(std::move(catchBlock)), exceptionVar(std::move(exceptionVar)) {}
    
    const std::shared_ptr<Statement>& getTryBlock() const { return tryBlock; }
    const std::shared_ptr<Statement>& getCatchBlock() const { return catchBlock; }
    const std::string& getExceptionVar() const { return exceptionVar; }
    void execute() override;
};

// Function parameter
struct Parameter {
    std::string name;
    std::shared_ptr<Type> type;
    
    Parameter(std::string name, std::shared_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {}
};

// Function declaration
class FunctionDeclaration : public AstNode {
private:
    std::string name;
    std::shared_ptr<Type> returnType;
    std::vector<Parameter> parameters;
    std::shared_ptr<BlockStatement> body;
    
public:
    FunctionDeclaration(std::string name, std::shared_ptr<Type> returnType,
                       std::vector<Parameter> parameters,
                       std::shared_ptr<BlockStatement> body,
                       AstLocation location)
        : AstNode(location), name(std::move(name)), returnType(std::move(returnType)),
          parameters(std::move(parameters)), body(std::move(body)) {}

    void setBody(std::shared_ptr<BlockStatement> body) { this->body = std::move(body); }
    
    const std::string& getName() const { return name; }
    const std::shared_ptr<Type>& getReturnType() const { return returnType; }
    const std::vector<Parameter>& getParameters() const { return parameters; }
    const std::shared_ptr<BlockStatement>& getBody() const { return body; }
};

// Struct declaration
class StructDeclaration : public AstNode {
private:
    std::string name;
    std::vector<StructField> fields;
    
public:
    StructDeclaration(std::string name, std::vector<StructField> fields, AstLocation location)
        : AstNode(location), name(std::move(name)), fields(std::move(fields)) {}
    
    const std::string& getName() const { return name; }
    const std::vector<StructField>& getFields() const { return fields; }
};

// Class declaration
class ClassDeclaration : public AstNode {
private:
    std::string name;
    std::vector<StructField> fields;
    std::vector<std::shared_ptr<FunctionDeclaration>> methods;
    std::vector<std::shared_ptr<ObjectType>> objects;
    
public:
    ClassDeclaration(std::string name, AstLocation location)
        : AstNode(location), name(std::move(name)) {}
    
    const std::string& getName() const { return name; }
    
    void addField(const StructField& field) { fields.push_back(field); }
    const std::vector<StructField>& getFields() const { return fields; }
    
    void addMethod(std::shared_ptr<FunctionDeclaration> method) { methods.push_back(std::move(method)); }
    const std::vector<std::shared_ptr<FunctionDeclaration>>& getMethods() const { return methods; }
    
    void addObject(std::shared_ptr<ObjectType> object) { objects.push_back(std::move(object)); }
    const std::vector<std::shared_ptr<ObjectType>>& getObjects() const { return objects; }
};

// Object declaration
class ObjectDeclaration : public AstNode {
private:
    std::string name;
    std::vector<StructField> fields;
    std::vector<std::shared_ptr<FunctionDeclaration>> methods;
    
public:
    ObjectDeclaration(std::string name, AstLocation location)
        : AstNode(location), name(std::move(name)) {}
    
    const std::string& getName() const { return name; }
    
    void addField(const StructField& field) { fields.push_back(field); }
    const std::vector<StructField>& getFields() const { return fields; }
    
    void addMethod(std::shared_ptr<FunctionDeclaration> method) { methods.push_back(std::move(method)); }
    const std::vector<std::shared_ptr<FunctionDeclaration>>& getMethods() const { return methods; }
};

// Namespace declaration
class NamespaceDeclaration : public AstNode {
private:
    std::string name;
    std::vector<std::shared_ptr<ClassDeclaration>> classes;
    
public:
    NamespaceDeclaration(std::string name, AstLocation location)
        : AstNode(location), name(std::move(name)) {}
    
    const std::string& getName() const { return name; }
    
    void addClass(std::shared_ptr<ClassDeclaration> classDecl) { classes.push_back(std::move(classDecl)); }
    const std::vector<std::shared_ptr<ClassDeclaration>>& getClasses() const { return classes; }
};

// Import statement
class ImportDeclaration : public AstNode {
private:
    std::string path;
    
public:
    ImportDeclaration(std::string path, AstLocation location)
        : AstNode(location), path(std::move(path)) {}
    
    const std::string& getPath() const { return path; }
};

// Typedef declaration
class TypedefDeclaration : public AstNode {
private:
    std::string name;
    std::shared_ptr<Type> type;
    
public:
    TypedefDeclaration(std::string name, std::shared_ptr<Type> type, AstLocation location)
        : AstNode(location), name(std::move(name)), type(std::move(type)) {}
    
    const std::string& getName() const { return name; }
    const std::shared_ptr<Type>& getType() const { return type; }
};

// Union declaration
class UnionDeclaration : public AstNode {
private:
    std::string name;
    std::vector<StructField> variants;
    
public:
    UnionDeclaration(std::string name, std::vector<StructField> variants, AstLocation location)
        : AstNode(location), name(std::move(name)), variants(std::move(variants)) {}
    
    const std::string& getName() const { return name; }
    const std::vector<StructField>& getVariants() const { return variants; }
};

// ASM block
class AsmStatement : public Statement {
private:
    std::string asmCode;
    
public:
    AsmStatement(std::string asmCode, AstLocation location)
        : Statement(location), asmCode(std::move(asmCode)) {}
    
    const std::string& getAsmCode() const { return asmCode; }
    void execute() override;
};

// Program - top level container
class Program : public AstNode {
private:
    std::vector<std::shared_ptr<AstNode>> declarations;
    
public:
    explicit Program(AstLocation location) : AstNode(location) {}
    
    void addDeclaration(std::shared_ptr<AstNode> decl) { declarations.push_back(std::move(decl)); }
    const std::vector<std::shared_ptr<AstNode>>& getDeclarations() const { return declarations; }
};

// Memory Arena for efficient AST allocation
class MemoryArena {
private:
    struct Block {
        static constexpr size_t BlockSize = 4096;
        char* memory;
        size_t used;
        Block* next;
        
        Block() : memory(new char[BlockSize]), used(0), next(nullptr) {}
        ~Block() { delete[] memory; }
    };
    
    Block* head;
    
public:
    MemoryArena() : head(new Block()) {}
    ~MemoryArena() {
        Block* current = head;
        while (current) {
            Block* next = current->next;
            delete current;
            current = next;
        }
    }
    
    // Allocate memory from the arena
    void* allocate(size_t size) {
        // Align size to 8 bytes
        size = (size + 7) & ~7;
        
        Block* current = head;
        while (current->used + size > Block::BlockSize) {
            if (current->next == nullptr) {
                current->next = new Block();
            }
            current = current->next;
        }
        
        void* result = current->memory + current->used;
        current->used += size;
        return result;
    }
    
    // Create a new AST node in the arena
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* memory = allocate(sizeof(T));
        return new(memory) T(std::forward<Args>(args)...);
    }
    
    // Reset the arena (free all memory at once)
    void reset() {
        Block* current = head;
        while (current) {
            current->used = 0;
            current = current->next;
        }
    }
    
    // No copy or move
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
};

// Global memory arena for AST allocation
extern MemoryArena astArena;

} // namespace flux

#endif // FLUX_AST_H
