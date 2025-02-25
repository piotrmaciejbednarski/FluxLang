#ifndef FLUX_INTERPRETER_H
#define FLUX_INTERPRETER_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stack>
#include "ast.h"
#include "error.h"
#include "runtime.h"
#include "builtins.h"
#include "typechecker.h"

namespace flux {

/**
 * Interpreter class for evaluating Flux programs
 */
class Interpreter {
private:
    // Global environment
    std::shared_ptr<Environment> environment;
    
    // Type checker
    std::shared_ptr<TypeChecker> typeChecker;
    
    // Current execution context
    struct Context {
        bool inLoop = false;       // Inside a loop (for break/continue)
        bool inFunction = false;   // Inside a function (for return)
        bool inTryCatch = false;   // Inside a try-catch block (for exceptions)
    };
    
    std::stack<Context> contextStack;
    
    // Visitor methods for AST nodes
    std::shared_ptr<RuntimeValue> evaluateProgram(const std::shared_ptr<Program>& program);
    std::shared_ptr<RuntimeValue> evaluateNamespace(const std::shared_ptr<NamespaceDeclaration>& ns);
    std::shared_ptr<RuntimeValue> evaluateClass(const std::shared_ptr<ClassDeclaration>& classDecl);
    std::shared_ptr<RuntimeValue> evaluateStruct(const std::shared_ptr<StructDeclaration>& structDecl);
    std::shared_ptr<RuntimeValue> evaluateObject(const std::shared_ptr<ObjectDeclaration>& objectDecl);
    std::shared_ptr<RuntimeValue> evaluateFunction(const std::shared_ptr<FunctionDeclaration>& funcDecl);
    std::shared_ptr<RuntimeValue> evaluateImport(const std::shared_ptr<ImportDeclaration>& importDecl);
    std::shared_ptr<RuntimeValue> evaluateTypedef(const std::shared_ptr<TypedefDeclaration>& typedefDecl);
    std::shared_ptr<RuntimeValue> evaluateUnion(const std::shared_ptr<UnionDeclaration>& unionDecl);
    
    // Statement evaluation
    std::shared_ptr<RuntimeValue> evaluateStatement(const std::shared_ptr<Statement>& stmt);
    std::shared_ptr<RuntimeValue> evaluateExpressionStatement(const std::shared_ptr<ExpressionStatement>& exprStmt);
    std::shared_ptr<RuntimeValue> evaluateBlockStatement(const std::shared_ptr<BlockStatement>& blockStmt);
    std::shared_ptr<RuntimeValue> evaluateVariableDeclaration(const std::shared_ptr<VariableDeclaration>& varDecl);
    std::shared_ptr<RuntimeValue> evaluateIfStatement(const std::shared_ptr<IfStatement>& ifStmt);
    std::shared_ptr<RuntimeValue> evaluateWhileStatement(const std::shared_ptr<WhileStatement>& whileStmt);
    std::shared_ptr<RuntimeValue> evaluateForStatement(const std::shared_ptr<ForStatement>& forStmt);
    std::shared_ptr<RuntimeValue> evaluateReturnStatement(const std::shared_ptr<ReturnStatement>& returnStmt);
    std::shared_ptr<RuntimeValue> evaluateBreakStatement(const std::shared_ptr<BreakStatement>& breakStmt);
    std::shared_ptr<RuntimeValue> evaluateContinueStatement(const std::shared_ptr<ContinueStatement>& continueStmt);
    std::shared_ptr<RuntimeValue> evaluateThrowStatement(const std::shared_ptr<ThrowStatement>& throwStmt);
    std::shared_ptr<RuntimeValue> evaluateTryCatchStatement(const std::shared_ptr<TryCatchStatement>& tryCatchStmt);
    std::shared_ptr<RuntimeValue> evaluateAsmStatement(const std::shared_ptr<AsmStatement>& asmStmt);
    std::shared_ptr<RuntimeValue> evaluatePrintStatement(const std::shared_ptr<PrintStatement>& printStmt);
    
    // Expression evaluation
    std::shared_ptr<RuntimeValue> evaluateExpression(const std::shared_ptr<Expression>& expr);
    std::shared_ptr<RuntimeValue> evaluateLiteral(const std::shared_ptr<LiteralExpression>& literal);
    std::shared_ptr<RuntimeValue> evaluateVariable(const std::shared_ptr<VariableExpression>& variable);
    std::shared_ptr<RuntimeValue> evaluateBinary(const std::shared_ptr<BinaryExpression>& binary);
    std::shared_ptr<RuntimeValue> evaluateUnary(const std::shared_ptr<UnaryExpression>& unary);
    std::shared_ptr<RuntimeValue> evaluateCall(const std::shared_ptr<CallExpression>& call);
    std::shared_ptr<RuntimeValue> evaluateIndex(const std::shared_ptr<IndexExpression>& index);
    std::shared_ptr<RuntimeValue> evaluateMember(const std::shared_ptr<MemberExpression>& member);
    std::shared_ptr<RuntimeValue> evaluateArrow(const std::shared_ptr<ArrowExpression>& arrow);
    std::shared_ptr<RuntimeValue> evaluateArrayLiteral(const std::shared_ptr<ArrayLiteralExpression>& arrayLiteral);
    std::shared_ptr<RuntimeValue> evaluateStringInterpolation(const std::shared_ptr<Expression>& interpolationExpr);
    
    // Helper methods
    std::shared_ptr<RuntimeValue> callFunction(
        const std::shared_ptr<RuntimeValue>& callee,
        const std::vector<std::shared_ptr<RuntimeValue>>& arguments);
    
    std::shared_ptr<RuntimeValue> createObjectInstance(
        const std::shared_ptr<ObjectType>& objectType);
    
    std::shared_ptr<RuntimeValue> createStructInstance(
        const std::shared_ptr<StructType>& structType);
    
    // Convert AST type to runtime value
    std::shared_ptr<RuntimeValue> typeToRuntimeValue(const std::shared_ptr<Type>& type);
    
    // Memory management helpers
    std::shared_ptr<RuntimeValue> allocateMemory(size_t size);
    void deallocateMemory(const std::shared_ptr<RuntimeValue>& pointer);
    
    // String interpolation
    std::string interpolateString(
        const std::string& format,
        const std::vector<std::shared_ptr<RuntimeValue>>& values);
    
    // Context management
    void pushContext();
    void popContext();
    Context& currentContext();
    
    // Error handling
    void reportRuntimeError(const std::string& message, const AstLocation& location);
    
public:
    Interpreter();
    ~Interpreter();
    
    // Initialize the interpreter with built-ins
    void initialize();
    
    // Reset the interpreter state
    void reset();
    
    // Evaluate a program
    std::shared_ptr<RuntimeValue> interpret(const std::shared_ptr<Program>& program);
    
    // Evaluate a single expression
    std::shared_ptr<RuntimeValue> interpretExpression(const std::shared_ptr<Expression>& expr);
    
    // Evaluate a string of Flux code
    std::shared_ptr<RuntimeValue> evaluate(std::string_view source);
    
    // Native function registration
    void registerNativeFunction(
        const std::string& name,
        NativeFunction::NativeFn function);
    
    // Environment access
    std::shared_ptr<Environment> getEnvironment() const { return environment; }
    
    // Type checker access
    std::shared_ptr<TypeChecker> getTypeChecker() const { return typeChecker; }
};

} // namespace flux

#endif // FLUX_INTERPRETER_H
