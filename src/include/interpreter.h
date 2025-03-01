#ifndef FLUX_INTERPRETER_H
#define FLUX_INTERPRETER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <functional>
#include <iostream>
#include <stdexcept>
#include "ast.h" // Include ast.h to get access to ScopeResolutionExpression

namespace flux {

// Forward declarations
class Program;
class Expression;
class Statement;
class Declaration;
class NamespaceDeclaration;
class ClassDeclaration;
class StructDeclaration;
class ObjectDeclaration;
class FunctionDeclaration;
class VariableDeclaration;
class CallExpression;
class MemberAccessExpression;
class IndexExpression;
class InjectableStringExpression;
class UnaryExpression;
class BinaryExpression;
class ScopeResolutionExpression; // Forward declaration

class Environment;

// Runtime error handling
class InterpreterError : public std::runtime_error {
public:
    InterpreterError(const std::string& message);
};

class Interpreter; // Forward declaration to resolve circular dependency

// Value type for interpreter
class Value {
public:
    using ValueType = std::variant<
        std::monostate,  // void/null
        int64_t,         // integer
        double,          // float
        bool,            // boolean
        char,            // character
        std::string,     // string
        std::vector<Value>,  // array
        std::shared_ptr<struct Callable>,  // function/callable
        std::unordered_map<std::string, Value>  // object/struct
    >;
    
    Value() : value(std::monostate{}) {}
    
    template<typename T>
    Value(T val) : value(val) {}
    
    template<typename T>
    const T& as() const {
        return std::get<T>(value);
    }

    template<typename T>
    T& as() {
        return std::get<T>(value);
    }
    
    template<typename T>
    bool is() const {
        return std::holds_alternative<T>(value);
    }
    
    bool isVoid() const { return is<std::monostate>(); }
    bool isInteger() const { return is<int64_t>(); }
    bool isFloat() const { return is<double>(); }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isBoolean() const { return is<bool>(); }
    bool isCharacter() const { return is<char>(); }
    bool isString() const { return is<std::string>(); }
    bool isArray() const { return is<std::vector<Value>>(); }
    bool isCallable() const { return is<std::shared_ptr<Callable>>(); }
    bool isObject() const { return is<std::unordered_map<std::string, Value>>(); }
    
    std::string toString() const;
    
    ValueType value;
};

// Interface for callable objects (functions, operators, etc.)
struct Callable {
    virtual ~Callable() = default;
    virtual Value call(Interpreter& interpreter, const std::vector<Value>& args) = 0;
    virtual int arity() const = 0;
};

// Function implementation
class Function : public Callable {
public:
    Function(std::shared_ptr<FunctionDeclaration> declaration, std::shared_ptr<Environment> closure);
    
    Value call(Interpreter& interpreter, const std::vector<Value>& args) override;
    int arity() const override;
    
private:
    std::shared_ptr<FunctionDeclaration> declaration;
    std::shared_ptr<Environment> closure;
};

// Built-in function implementation
class BuiltinFunction : public Callable {
public:
    using NativeFunction = std::function<Value(Interpreter&, const std::vector<Value>&)>;
    
    BuiltinFunction(std::string name, 
                    int arity, 
                    NativeFunction function);
    
    Value call(Interpreter& interpreter, const std::vector<Value>& args) override;
    int arity() const override;
    
private:
    std::string name;
    int functionArity;
    NativeFunction function;
};

// Environment for storing variables
class Environment {
public:
    Environment();
    Environment(std::shared_ptr<Environment> enclosing);
    
    void define(const std::string& name, const Value& value);
    Value get(const std::string& name);
    void assign(const std::string& name, const Value& value);
    
    void printSymbols();
    
    std::shared_ptr<Environment> getEnclosing() const { return enclosing; }
    void setEnclosing(std::shared_ptr<Environment>);
    void updateEnclosing(std::shared_ptr<Environment>);
    
    // Add this method to support case-insensitive lookups
    std::unordered_map<std::string, Value> getAllSymbols() const;
    
private:
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
};

// Main interpreter class
class Interpreter {
public:
    struct ReturnValue {
        Value value;
    };

    Interpreter();
    
    // Execute a program
    Value execute(std::shared_ptr<Program> program);
    
    // Evaluate expressions
    Value evaluate(std::shared_ptr<Expression> expr);
    
    // Execute statements
    void execute(std::shared_ptr<Statement> stmt);
    
    // Execute block (should be public to support function calls)
    void executeBlock(std::shared_ptr<Statement> stmt, std::shared_ptr<Environment> env);
    
    // Define built-in functions
    void defineBuiltins();
    
    // Print global symbols for debugging
    void printGlobalSymbols();
    
    // Utility methods
    bool isTruthy(const Value& value);
    bool isEqual(const Value& a, const Value& b);
    
    // Environment access
    std::shared_ptr<Environment> getGlobalEnvironment() const { return globals; }
    std::shared_ptr<Environment> getCurrentEnvironment() const { return environment; }
    void pushEnvironment(std::shared_ptr<Environment> newEnv);
    void popEnvironment();
    
    // Built-in function implementations
    Value print(const std::vector<Value>& args);
    Value input(const std::vector<Value>& args);
    
    // Specific declaration execution methods
    void executeDeclaration(std::shared_ptr<Declaration> decl);
    
private:
    // Expression handlers
    Value evaluateLiteral(std::shared_ptr<Expression> expr);
    Value evaluateIdentifier(std::shared_ptr<Expression> expr);
    Value evaluateArray(std::shared_ptr<Expression> expr);
    Value evaluateUnary(std::shared_ptr<Expression> expr);
    Value evaluateBinary(std::shared_ptr<Expression> expr);
    Value evaluateCall(std::shared_ptr<Expression> expr);
    Value evaluateMemberAccess(std::shared_ptr<Expression> expr);
    Value evaluateIndex(std::shared_ptr<Expression> expr);
    Value evaluateInjectableString(std::shared_ptr<Expression> expr);
    Value evaluateScopeResolution(std::shared_ptr<Expression> expr);
    Value evaluateObjectInstantiation(std::shared_ptr<Expression> expr);
    
    // Statement handlers
    void executeExpression(std::shared_ptr<Statement> stmt);
    void executeIf(std::shared_ptr<Statement> stmt);
    void executeWhile(std::shared_ptr<Statement> stmt);
    void executeFor(std::shared_ptr<Statement> stmt);
    void executeReturn(std::shared_ptr<Statement> stmt);
    void executeVariableDeclaration(std::shared_ptr<Statement> stmt);
    void executeFunctionDeclaration(std::shared_ptr<Statement> stmt);
    void executeNamespaceDeclaration(std::shared_ptr<NamespaceDeclaration> decl);
    void executeClassDeclaration(std::shared_ptr<ClassDeclaration> decl);
    void executeStructDeclaration(std::shared_ptr<StructDeclaration> decl);
    void executeObjectDeclaration(std::shared_ptr<ObjectDeclaration> decl);
    
    // Helper methods
    std::string buildQualifiedName(std::shared_ptr<ScopeResolutionExpression> scopeExpr);
    std::string extractQualifiedName(std::shared_ptr<ScopeResolutionExpression> expr);
    
    // Environment management
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
};

} // namespace flux

#endif // FLUX_INTERPRETER_H
