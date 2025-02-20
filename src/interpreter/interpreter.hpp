#ifndef FLUX_INTERPRETER_HPP
#define FLUX_INTERPRETER_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "environment.hpp"
#include "value.hpp"
#include "../parser/statements.hpp"
#include "../parser/expressions.hpp"
#include "../parser/ast.hpp"

class ExpressionStatement;
class FunctionDeclaration;
class ReturnStatement;

namespace flux {

class Interpreter {
public:
    // Constructor initializes with global environment
    Interpreter() : environment_(Environment::createGlobalEnvironment()) {}

    // Main interpretation methods
    Value interpret(const std::vector<std::unique_ptr<Statement>>& statements);
    Value evaluate(const Expression& expr);
    void execute(const Statement& stmt);

    // Statement execution methods
    void visitBlockStatement(const BlockStatement& stmt);
    void visitExpressionStatement(const ExpressionStatement& stmt);
    void visitWhenStatement(const WhenStatement& stmt);
    void visitObjectDefinition(const ObjectDefinition& stmt);
    void visitClassDefinition(const ClassDefinition& stmt);
    void visitNamespaceDefinition(const NamespaceDefinition& stmt);
    void visitOperatorDefinition(const OperatorDefinition& stmt);
    void visitVariableDeclaration(const VariableDeclaration& stmt);
    void visitFunctionDeclaration(const FunctionDeclaration& stmt);
    void visitReturnStatement(const ReturnStatement& stmt);

    // Expression evaluation methods
    Value visitBinaryExpression(const BinaryExpression& expr);
    Value visitUnaryExpression(const UnaryExpression& expr);
    Value visitCallExpression(const CallExpression& expr);
    Value visitIdentifier(const Identifier& expr);
    Value visitIntegerLiteral(const IntegerLiteral& expr);
    Value visitFloatLiteral(const FloatLiteral& expr);
    Value visitStringLiteral(const StringLiteral& expr);
    Value visitBooleanLiteral(const BooleanLiteral& expr);

    // Environment management
    void defineVariable(const std::string& name, const Value& value);
    void assignVariable(const std::string& name, const Value& value);
    Value getVariable(const std::string& name) const;
    std::shared_ptr<Environment> getCurrentEnvironment() const { return environment_; }
    void setCurrentEnvironment(std::shared_ptr<Environment> env) { environment_ = std::move(env); }

    // Error handling
    class RuntimeError : public std::runtime_error {
    public:
        RuntimeError(const std::string& message, const Token& token)
            : std::runtime_error(message), token_(token) {}

        const Token& getToken() const { return token_; }

    private:
        Token token_;
    };

    // Special execution modes
    class ReturnValue {
    public:
        explicit ReturnValue(Value value) : value_(std::move(value)) {}
        const Value& getValue() const { return value_; }
    private:
        Value value_;
    };

    // When-block management
    struct WhenContext {
        std::vector<std::unique_ptr<Statement>> body;
        bool isVolatile;
        std::shared_ptr<Environment> environment;
    };

    void registerWhenBlock(std::unique_ptr<WhenContext> context);
    void checkWhenConditions();
    void cleanupVolatileBlocks();

private:
    std::shared_ptr<Environment> environment_;
    std::vector<std::unique_ptr<WhenContext>> whenBlocks_;
    bool inWhenBlock_ = false;

    // Helper methods
    Value evaluateBinaryOp(const BinaryExpression& expr);
    Value evaluateUnaryOp(const UnaryExpression& expr);
    Value evaluateOperatorOverload(const Token& op, const Value& left, const Value& right);
    std::vector<Value> evaluateArguments(const std::vector<std::unique_ptr<Expression>>& arguments);
    bool isTruthy(const Value& value) const;
    void checkNumberOperand(const Token& op, const Value& operand) const;
    void checkNumberOperands(const Token& op, const Value& left, const Value& right) const;

    // Type checking and conversion
    template<typename T>
    T checkType(const Value& value, const std::string& type) const {
        if (!value.is<T>()) {
            throw RuntimeError("Expected " + type, Token(TokenType::ERROR, "", 0, 0));
        }
        return value.as<T>();
    }

    // Operator implementation helpers
    Value add(const Value& left, const Value& right, const Token& op);
    Value subtract(const Value& left, const Value& right, const Token& op);
    Value multiply(const Value& left, const Value& right, const Token& op);
    Value divide(const Value& left, const Value& right, const Token& op);
    Value modulo(const Value& left, const Value& right, const Token& op);

    // Object system helpers
    ObjectPtr createObject(const std::string& type);
    void defineMethod(ObjectPtr object, const std::string& name, FunctionPtr method);
    void defineField(ObjectPtr object, const std::string& name, const Value& value);

    // When-block helpers
    void executeWhenBlock(const WhenContext& context);
    void cleanupWhenBlock(const WhenContext& context);

    // Environment management helpers
    class EnvironmentGuard {
    public:
        EnvironmentGuard(Interpreter& interpreter, std::shared_ptr<Environment> newEnv)
            : interpreter_(interpreter)
            , oldEnv_(interpreter.environment_) {
            interpreter_.environment_ = std::move(newEnv);
        }

        ~EnvironmentGuard() {
            interpreter_.environment_ = oldEnv_;
        }

    private:
        Interpreter& interpreter_;
        std::shared_ptr<Environment> oldEnv_;
    };
};

} // namespace flux

#endif // FLUX_INTERPRETER_HPP
