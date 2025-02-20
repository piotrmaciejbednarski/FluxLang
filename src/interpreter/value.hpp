#ifndef FLUX_VALUE_HPP
#define FLUX_VALUE_HPP

#include "core_fwd.hpp"
#include <variant>
#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace flux {

class Object {
public:
    virtual ~Object() = default;
    
    virtual Value get(const std::string& name) const {
        auto it = fields_.find(name);
        if (it != fields_.end()) {
            return it->second;
        }
        throw std::runtime_error("Undefined property: " + name);
    }

    virtual void set(const std::string& name, const Value& value) {
        fields_[name] = value;
    }

    virtual bool has(const std::string& name) const {
        return fields_.find(name) != fields_.end();
    }

    virtual std::string toString() const {
        return "[object " + getTypeName() + "]";
    }

    virtual std::string getTypeName() const = 0;

protected:
    std::unordered_map<std::string, Value> fields_;
};

class Value {
public:
    using ValueVariant = std::variant<
        std::monostate,  // Represents null/none
        Integer,
        Float,
        String,
        Boolean,
        ObjectPtr,
        FunctionPtr
    >;

    // Constructors
    Value() : value_(std::monostate{}) {}
    Value(Integer v) : value_(v) {}
    Value(Float v) : value_(v) {}
    Value(const String& v) : value_(v) {}
    Value(Boolean v) : value_(v) {}
    Value(ObjectPtr v) : value_(std::move(v)) {}
    Value(FunctionPtr v) : value_(std::move(v)) {}
    explicit Value(int32_t v) : value_(static_cast<Integer>(v)) {}
    explicit Value(size_t v) : value_(static_cast<Integer>(static_cast<int64_t>(v))) {}

    // Type checking
    bool isNull() const { return std::holds_alternative<std::monostate>(value_); }
    bool isInteger() const { return std::holds_alternative<Integer>(value_); }
    bool isFloat() const { return std::holds_alternative<Float>(value_); }
    bool isString() const { return std::holds_alternative<String>(value_); }
    bool isBoolean() const { return std::holds_alternative<Boolean>(value_); }
    bool isObject() const { return std::holds_alternative<ObjectPtr>(value_); }
    bool isFunction() const { return std::holds_alternative<FunctionPtr>(value_); }
    bool isNumber() const { return isInteger() || isFloat(); }

    // Value getters
    Integer asInteger() const {
        if (!isInteger()) throw TypeError("Expected integer");
        return std::get<Integer>(value_);
    }

    Float asFloat() const {
        if (!isFloat()) throw TypeError("Expected float");
        return std::get<Float>(value_);
    }

    const String& asString() const {
        if (!isString()) throw TypeError("Expected string");
        return std::get<String>(value_);
    }

    Boolean asBoolean() const {
        if (!isBoolean()) throw TypeError("Expected boolean");
        return std::get<Boolean>(value_);
    }

    ObjectPtr asObject() const {
        if (!isObject()) throw TypeError("Expected object");
        return std::get<ObjectPtr>(value_);
    }

    FunctionPtr asFunction() const {
        if (!isFunction()) throw TypeError("Expected function");
        return std::get<FunctionPtr>(value_);
    }

    template<typename T>
    bool is() const {
        if constexpr (std::is_same_v<T, Integer>) return isInteger();
        if constexpr (std::is_same_v<T, Float>) return isFloat();
        if constexpr (std::is_same_v<T, String>) return isString();
        if constexpr (std::is_same_v<T, Boolean>) return isBoolean();
        return false;
    }

    template<typename T>
    T as() const {
        if constexpr (std::is_same_v<T, Integer>) return asInteger();
        if constexpr (std::is_same_v<T, Float>) return asFloat();
        if constexpr (std::is_same_v<T, String>) return asString();
        if constexpr (std::is_same_v<T, Boolean>) return asBoolean();
        throw TypeError("Invalid type conversion");
    }

    class TypeError : public std::runtime_error {
    public:
        explicit TypeError(const std::string& message) 
            : std::runtime_error(message) {}
    };

private:
    ValueVariant value_;
};

class ArrayObject : public Object {
public:
    explicit ArrayObject(std::vector<Value> values) : values_(std::move(values)) {}
    
    std::string getTypeName() const override { return "Array"; }
    size_t size() const { return values_.size(); }
    
    Value get(const std::string& name) const override {
        if (name == "length") {
            return Value(static_cast<Integer>(values_.size()));
        }
        try {
            size_t index = std::stoul(name);
            if (index < values_.size()) {
                return values_[index];
            }
        } catch (...) {}
        return Object::get(name);
    }

private:
    std::vector<Value> values_;
};

class Function {
public:
    using NativeFunction = std::function<Value(std::vector<Value>)>;

    explicit Function(NativeFunction fn) 
        : native_(std::move(fn)), isNative_(true) {}

    Function(std::vector<std::string> params,
            std::shared_ptr<class Environment> closure,
            std::vector<StmtPtr> body)
        : parameters_(std::move(params))
        , closure_(std::move(closure))
        , body_(std::move(body))
        , isNative_(false) {}

    Value call(std::vector<Value> arguments);
    size_t arity() const { return parameters_.size(); }
    bool isNative() const { return isNative_; }

private:
    std::vector<std::string> parameters_;
    std::shared_ptr<class Environment> closure_;
    std::vector<StmtPtr> body_;
    NativeFunction native_;
    bool isNative_;
};

} // namespace flux

#endif
