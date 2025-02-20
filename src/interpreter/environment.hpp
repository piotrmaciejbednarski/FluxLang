#ifndef FLUX_ENVIRONMENT_HPP
#define FLUX_ENVIRONMENT_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include <iostream>
#include "value.hpp"

namespace flux {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    // Constructors for different environment types
    Environment() : enclosing_(nullptr) {}
    explicit Environment(std::shared_ptr<Environment> enclosing) 
        : enclosing_(std::move(enclosing)) {}

    // Variable declaration and definition
    void define(const std::string& name, const Value& value) {
        values_[name] = value;
    }

    // Variable assignment with scope traversal
    void assign(const std::string& name, const Value& value) {
        if (auto found = getEnvironmentWithVariable(name)) {
            found->values_[name] = value;
        } else {
            throw UndefinedVariableError("Undefined variable '" + name + "'");
        }
    }

    // Variable lookup with scope traversal
    Value get(const std::string& name) const {
        if (auto found = getEnvironmentWithVariable(name)) {
            return found->values_.at(name);
        }
        throw UndefinedVariableError("Undefined variable '" + name + "'");
    }

    // Check if a variable exists in the current or enclosing scopes
    bool exists(const std::string& name) const {
        return getEnvironmentWithVariable(name) != nullptr;
    }

    // Delete a variable from its scope
    void remove(const std::string& name) {
        if (auto found = getEnvironmentWithVariable(name)) {
            found->values_.erase(name);
        }
    }

    // Create a new scope
    std::shared_ptr<Environment> createChildScope() {
        return std::make_shared<Environment>(shared_from_this());
    }

    // Get enclosing environment
    std::shared_ptr<Environment> getEnclosing() const {
        return enclosing_;
    }

    // Create a closure environment
    std::shared_ptr<Environment> createClosure() {
        auto closure = std::make_shared<Environment>(shared_from_this());
        closure->isClosure_ = true;
        return closure;
    }

    // Special environments
    static std::shared_ptr<Environment> createGlobalEnvironment();
    static std::shared_ptr<Environment> createModuleEnvironment(std::shared_ptr<Environment> global);

    // Exception classes
    class UndefinedVariableError : public std::runtime_error {
    public:
        explicit UndefinedVariableError(const std::string& message) 
            : std::runtime_error(message) {}
    };

    class ConstantModificationError : public std::runtime_error {
    public:
        explicit ConstantModificationError(const std::string& message) 
            : std::runtime_error(message) {}
    };

private:
    // Member variables
    std::unordered_map<std::string, Value> values_;
    std::shared_ptr<Environment> enclosing_;
    bool isClosure_ = false;

    // Helper to find the environment containing a variable
    const Environment* getEnvironmentWithVariable(const std::string& name) const {
        if (values_.find(name) != values_.end()) {
            return this;
        }
        if (enclosing_) {
            return enclosing_->getEnvironmentWithVariable(name);
        }
        return nullptr;
    }

    Environment* getEnvironmentWithVariable(const std::string& name) {
        return const_cast<Environment*>(
            static_cast<const Environment*>(this)->getEnvironmentWithVariable(name)
        );
    }
    
    const std::unordered_map<std::string, Value>& getValues() const {
    	return values_;
    }

    // Friends
    friend class Interpreter;
};

// Global environment setup
inline std::shared_ptr<Environment> Environment::createGlobalEnvironment() {
    auto global = std::make_shared<Environment>();
    
    // Add built-in functions
    global->define("print", Value(std::make_shared<Function>(
        [](std::vector<Value> args) -> Value {
            for (const auto& arg : args) {
                std::cout << arg.toString() << " ";
            }
            std::cout << std::endl;
            return Value();
        }
    )));

    global->define("to_string", Value(std::make_shared<Function>(
        [](std::vector<Value> args) -> Value {
            if (args.empty()) return Value(std::string(""));
            return Value(args[0].toString());
        }
    )));

    global->define("to_number", Value(std::make_shared<Function>(
        [](std::vector<Value> args) -> Value {
            if (args.empty()) return Value(0);
            if (args[0].isInteger()) return args[0];
            if (args[0].isFloat()) return args[0];
            if (args[0].isString()) {
                try {
                    size_t pos;
                    std::string str = args[0].asString();
			if (str.find('.') != std::string::npos) {
			    return Value(std::stod(str));
			} else {
			    return Value(static_cast<Integer>(std::stoll(str)));
			}
                } catch (...) {
                    return Value(0);
                }
            }
            return Value(0);
        }
    )));

    global->define("array", Value(std::make_shared<Function>(
        [](std::vector<Value> args) -> Value {
            return Value(std::make_shared<ArrayObject>(args));
        }
    )));

    global->define("length", Value(std::make_shared<Function>(
        [](std::vector<Value> args) -> Value {
            if (args.empty()) return Value(0);
            if (args[0].isString()) {
                return Value(static_cast<Integer>(args[0].asString().length()));
            }
            if (args[0].isObject()) {
                auto obj = args[0].asObject();
                if (auto arr = std::dynamic_pointer_cast<ArrayObject>(obj)) {
                    return Value(static_cast<Integer>(arr->size()));
                }
            }
            return Value(0);
        }
    )));

    // Add built-in constants
    global->define("true", Value(true));
    global->define("false", Value(false));
    global->define("null", Value());

    return global;
}

// Module environment setup
inline std::shared_ptr<Environment> Environment::createModuleEnvironment(
    std::shared_ptr<Environment> global) {
    return std::make_shared<Environment>(std::move(global));
}

} // namespace flux

#endif // FLUX_ENVIRONMENT_HPP
