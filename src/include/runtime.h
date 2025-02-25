#ifndef FLUX_RUNTIME_H
#define FLUX_RUNTIME_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <functional>
#include "ast.h"
#include "error.h"

namespace flux {

// Forward declarations
class RuntimeValue;
class Environment;
class Function;
class NativeFunction;

// Runtime error class
class RuntimeError : public std::runtime_error {
private:
    SourceLocation location;
    
public:
    RuntimeError(const std::string& message, const SourceLocation& location = SourceLocation())
        : std::runtime_error(message), location(location) {}
    
    const SourceLocation& getLocation() const { return location; }
};

// Runtime value class that represents all Flux values
class RuntimeValue {
public:
    enum class Type {
        NIL,
        BOOL,
        INT,       // int{32} by default
        FLOAT,     // float{32} by default
        STRING,    // string type
        ARRAY,     // array type
        POINTER,   // pointer type
        FUNCTION,  // function value
        NATIVE_FUNCTION, // built-in function
        STRUCT,    // struct instance
        OBJECT,    // object instance
        CLASS,     // class definition
        RETURN_VALUE,    // Return value wrapper
        EXCEPTION,       // Exception wrapper
        BREAK,           // Break statement marker
        CONTINUE         // Continue statement marker
    };

private:
    Type type;
    int bitWidth = 32;    // For numeric types
    bool isUnsigned = false; // For integer types
    std::shared_ptr<flux::Type> fluxType; // AST type information
    
    // Value storage using variant
    std::variant<
        bool,                                   // BOOL
        int32_t,                                // INT
        float,                                  // FLOAT
        std::string,                            // STRING
        std::vector<std::shared_ptr<RuntimeValue>>, // ARRAY
        std::shared_ptr<RuntimeValue>,          // POINTER, RETURN_VALUE, EXCEPTION
        std::unique_ptr<Function>,              // FUNCTION
        std::unique_ptr<NativeFunction>,        // NATIVE_FUNCTION
        std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>, // STRUCT, OBJECT
        std::shared_ptr<ClassDeclaration>       // CLASS
    > data;
    
public:
    // Constructor with basic type
    explicit RuntimeValue(Type type) : type(type) {}
    
    // Type-specific constructors
    static std::shared_ptr<RuntimeValue> makeNil();
    static std::shared_ptr<RuntimeValue> makeBool(bool value);
    static std::shared_ptr<RuntimeValue> makeInt(int32_t value, int bitWidth = 32, bool isUnsigned = false);
    static std::shared_ptr<RuntimeValue> makeFloat(float value, int bitWidth = 32);
    static std::shared_ptr<RuntimeValue> makeString(const std::string& value);
    static std::shared_ptr<RuntimeValue> makeArray(std::vector<std::shared_ptr<RuntimeValue>> elements);
    static std::shared_ptr<RuntimeValue> makePointer(std::shared_ptr<RuntimeValue> target);
    static std::shared_ptr<RuntimeValue> makeFunction(std::unique_ptr<Function> func);
    static std::shared_ptr<RuntimeValue> makeNativeFunction(std::unique_ptr<NativeFunction> func);
    static std::shared_ptr<RuntimeValue> makeStruct(std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> fields);
    static std::shared_ptr<RuntimeValue> makeObject(std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> fields);
    static std::shared_ptr<RuntimeValue> makeClass(std::shared_ptr<ClassDeclaration> classDecl);
    static std::shared_ptr<RuntimeValue> makeReturn(std::shared_ptr<RuntimeValue> value);
    static std::shared_ptr<RuntimeValue> makeException(std::shared_ptr<RuntimeValue> value);
    static std::shared_ptr<RuntimeValue> makeBreak();
    static std::shared_ptr<RuntimeValue> makeContinue();
    
    // Type checking methods
    Type getType() const { return type; }
    int getBitWidth() const { return bitWidth; }
    bool getIsUnsigned() const { return isUnsigned; }
    std::shared_ptr<flux::Type> getFluxType() const { return fluxType; }
    void setFluxType(std::shared_ptr<flux::Type> type) { fluxType = type; }
    
    bool isNil() const { return type == Type::NIL; }
    bool isBool() const { return type == Type::BOOL; }
    bool isInt() const { return type == Type::INT; }
    bool isFloat() const { return type == Type::FLOAT; }
    bool isNumeric() const { return isInt() || isFloat(); }
    bool isString() const { return type == Type::STRING; }
    bool isArray() const { return type == Type::ARRAY; }
    bool isPointer() const { return type == Type::POINTER; }
    bool isFunction() const { return type == Type::FUNCTION; }
    bool isNativeFunction() const { return type == Type::NATIVE_FUNCTION; }
    bool isStruct() const { return type == Type::STRUCT; }
    bool isObject() const { return type == Type::OBJECT; }
    bool isClass() const { return type == Type::CLASS; }
    bool isReturnValue() const { return type == Type::RETURN_VALUE; }
    bool isException() const { return type == Type::EXCEPTION; }
    bool isBreak() const { return type == Type::BREAK; }
    bool isContinue() const { return type == Type::CONTINUE; }
    
    // Value getters with type checking
    bool asBool() const;
    int32_t asInt() const;
    float asFloat() const;
    const std::string& asString() const;
    const std::vector<std::shared_ptr<RuntimeValue>>& asArray() const;
    std::vector<std::shared_ptr<RuntimeValue>>& asArray();
    std::shared_ptr<RuntimeValue> asPointer() const;
    Function* asFunction() const;
    NativeFunction* asNativeFunction() const;
    const std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& asStruct() const;
    std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& asStruct();
    const std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& asObject() const;
    std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& asObject();
    std::shared_ptr<ClassDeclaration> asClass() const;
    std::shared_ptr<RuntimeValue> asReturnValue() const;
    std::shared_ptr<RuntimeValue> asException() const;
    
    // Utility methods
    std::string toString() const;
    bool isTruthy() const;
    bool equals(const std::shared_ptr<RuntimeValue>& other) const;
};

// Function class for Flux functions
class Function {
private:
    std::shared_ptr<FunctionDeclaration> declaration;
    std::shared_ptr<Environment> closure;
    
public:
    Function(std::shared_ptr<FunctionDeclaration> declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(closure) {}
    
    const std::string& getName() const;
    const std::vector<Parameter>& getParameters() const;
    const std::shared_ptr<BlockStatement>& getBody() const;
    std::shared_ptr<Environment> getClosure() const { return closure; }
};

// Native function class for built-in functions
class NativeFunction {
public:
    using NativeFn = std::function<std::shared_ptr<RuntimeValue>(
        std::vector<std::shared_ptr<RuntimeValue>>, std::shared_ptr<Environment>
    )>;
    
private:
    std::string name;
    NativeFn function;
    
public:
    NativeFunction(std::string name, NativeFn function)
        : name(std::move(name)), function(std::move(function)) {}
    
    const std::string& getName() const { return name; }
    std::shared_ptr<RuntimeValue> call(
        std::vector<std::shared_ptr<RuntimeValue>> args, 
        std::shared_ptr<Environment> env) const {
        return function(std::move(args), env);
    }
};

// Environment class for variable storage and scope management
class Environment : public std::enable_shared_from_this<Environment> {
private:
    std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> values;
    std::shared_ptr<Environment> enclosing;
    
public:
    Environment() : enclosing(nullptr) {}
    explicit Environment(std::shared_ptr<Environment> enclosing) 
        : enclosing(std::move(enclosing)) {}
    
    // Define a new variable in the current scope
    void define(const std::string& name, std::shared_ptr<RuntimeValue> value) {
        values[name] = std::move(value);
    }
    
    // Get a variable's value, looking in enclosing scopes if necessary
    std::shared_ptr<RuntimeValue> get(const std::string& name) {
        auto it = values.find(name);
        if (it != values.end()) {
            return it->second;
        }
        
        if (enclosing) {
            return enclosing->get(name);
        }
        
        throw RuntimeError("Undefined variable '" + name + "'.");
    }
    
    // Assign a value to an existing variable
    void assign(const std::string& name, std::shared_ptr<RuntimeValue> value) {
        auto it = values.find(name);
        if (it != values.end()) {
            it->second = std::move(value);
            return;
        }
        
        if (enclosing) {
            enclosing->assign(name, std::move(value));
            return;
        }
        
        throw RuntimeError("Undefined variable '" + name + "'.");
    }
    
    // Get a reference to a variable's value
    std::shared_ptr<RuntimeValue>& getReference(const std::string& name) {
        auto it = values.find(name);
        if (it != values.end()) {
            return it->second;
        }
        
        if (enclosing) {
            return enclosing->getReference(name);
        }
        
        throw RuntimeError("Undefined variable '" + name + "'.");
    }
    
    // Check if a variable exists in this environment or enclosing ones
    bool exists(const std::string& name) const {
        if (values.find(name) != values.end()) {
            return true;
        }
        
        if (enclosing) {
            return enclosing->exists(name);
        }
        
        return false;
    }
    
    // Create a new child environment
    std::shared_ptr<Environment> createChild() {
        return std::make_shared<Environment>(shared_from_this());
    }
    
    // Get the global environment (root)
    std::shared_ptr<Environment> getGlobal() {
        if (!enclosing) {
            return shared_from_this();
        }
        return enclosing->getGlobal();
    }
};

} // namespace flux

#endif // FLUX_RUNTIME_H
