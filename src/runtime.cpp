#include "include/runtime.h"
#include <sstream>
#include <cmath>

namespace flux {

// ===== RuntimeValue Implementation =====

// Static factory methods for creating RuntimeValue instances
std::shared_ptr<RuntimeValue> RuntimeValue::makeNil() {
    return std::make_shared<RuntimeValue>(Type::NIL);
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeBool(bool value) {
    auto result = std::make_shared<RuntimeValue>(Type::BOOL);
    result->data = value;
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeInt(int32_t value, int bitWidth, bool isUnsigned) {
    auto result = std::make_shared<RuntimeValue>(Type::INT);
    result->data = value;
    result->bitWidth = bitWidth;
    result->isUnsigned = isUnsigned;
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeFloat(float value, int bitWidth) {
    auto result = std::make_shared<RuntimeValue>(Type::FLOAT);
    result->data = value;
    result->bitWidth = bitWidth;
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeString(const std::string& value) {
    auto result = std::make_shared<RuntimeValue>(Type::STRING);
    result->data = value;
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeArray(std::vector<std::shared_ptr<RuntimeValue>> elements) {
    auto result = std::make_shared<RuntimeValue>(Type::ARRAY);
    result->data = std::move(elements);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makePointer(std::shared_ptr<RuntimeValue> target) {
    auto result = std::make_shared<RuntimeValue>(Type::POINTER);
    result->data = std::move(target);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeFunction(std::unique_ptr<Function> func) {
    auto result = std::make_shared<RuntimeValue>(Type::FUNCTION);
    result->data = std::move(func);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeNativeFunction(std::unique_ptr<NativeFunction> func) {
    auto result = std::make_shared<RuntimeValue>(Type::NATIVE_FUNCTION);
    result->data = std::move(func);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeStruct(std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> fields) {
    auto result = std::make_shared<RuntimeValue>(Type::STRUCT);
    result->data = std::move(fields);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeObject(std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> fields) {
    auto result = std::make_shared<RuntimeValue>(Type::OBJECT);
    result->data = std::move(fields);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeClass(std::shared_ptr<ClassDeclaration> classDecl) {
    auto result = std::make_shared<RuntimeValue>(Type::CLASS);
    result->data = std::move(classDecl);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeReturn(std::shared_ptr<RuntimeValue> value) {
    auto result = std::make_shared<RuntimeValue>(Type::RETURN_VALUE);
    result->data = std::move(value);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeException(std::shared_ptr<RuntimeValue> value) {
    auto result = std::make_shared<RuntimeValue>(Type::EXCEPTION);
    result->data = std::move(value);
    return result;
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeBreak() {
    return std::make_shared<RuntimeValue>(Type::BREAK);
}

std::shared_ptr<RuntimeValue> RuntimeValue::makeContinue() {
    return std::make_shared<RuntimeValue>(Type::CONTINUE);
}

// Value getters with type checking
bool RuntimeValue::asBool() const {
    if (type != Type::BOOL) throw RuntimeError("Value is not a boolean");
    return std::get<bool>(data);
}

int32_t RuntimeValue::asInt() const {
    if (type != Type::INT) throw RuntimeError("Value is not an integer");
    return std::get<int32_t>(data);
}

float RuntimeValue::asFloat() const {
    if (type != Type::FLOAT) throw RuntimeError("Value is not a float");
    return std::get<float>(data);
}

const std::string& RuntimeValue::asString() const {
    if (type != Type::STRING) {
        std::cerr << "Error: Value type is " << static_cast<int>(type) << ", not STRING" << std::endl;
        throw RuntimeError("Value is not a string");
    }
    
    try {
        return std::get<std::string>(data);
    } catch (const std::bad_variant_access& e) {
        std::cerr << "Error accessing string data: " << e.what() << std::endl;
        throw RuntimeError("Failed to access string data");
    }
}

const std::vector<std::shared_ptr<RuntimeValue>>& RuntimeValue::asArray() const {
    if (type != Type::ARRAY) throw RuntimeError("Value is not an array");
    return std::get<std::vector<std::shared_ptr<RuntimeValue>>>(data);
}

std::vector<std::shared_ptr<RuntimeValue>>& RuntimeValue::asArray() {
    if (type != Type::ARRAY) throw RuntimeError("Value is not an array");
    return std::get<std::vector<std::shared_ptr<RuntimeValue>>>(data);
}

std::shared_ptr<RuntimeValue> RuntimeValue::asPointer() const {
    if (type != Type::POINTER && type != Type::RETURN_VALUE && type != Type::EXCEPTION) 
        throw RuntimeError("Value is not a pointer, return value, or exception");
    return std::get<std::shared_ptr<RuntimeValue>>(data);
}

Function* RuntimeValue::asFunction() const {
    if (type != Type::FUNCTION) throw RuntimeError("Value is not a function");
    return std::get<std::unique_ptr<Function>>(data).get();
}

NativeFunction* RuntimeValue::asNativeFunction() const {
    if (type != Type::NATIVE_FUNCTION) throw RuntimeError("Value is not a native function");
    return std::get<std::unique_ptr<NativeFunction>>(data).get();
}

const std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& RuntimeValue::asStruct() const {
    if (type != Type::STRUCT) throw RuntimeError("Value is not a struct");
    return std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data);
}

std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& RuntimeValue::asStruct() {
    if (type != Type::STRUCT) throw RuntimeError("Value is not a struct");
    return std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data);
}

const std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& RuntimeValue::asObject() const {
    if (type != Type::OBJECT) throw RuntimeError("Value is not an object");
    return std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data);
}

std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>& RuntimeValue::asObject() {
    if (type != Type::OBJECT) throw RuntimeError("Value is not an object");
    return std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data);
}

std::shared_ptr<ClassDeclaration> RuntimeValue::asClass() const {
    if (type != Type::CLASS) throw RuntimeError("Value is not a class");
    return std::get<std::shared_ptr<ClassDeclaration>>(data);
}

std::shared_ptr<RuntimeValue> RuntimeValue::asReturnValue() const {
    if (type != Type::RETURN_VALUE) throw RuntimeError("Value is not a return value");
    return std::get<std::shared_ptr<RuntimeValue>>(data);
}

std::shared_ptr<RuntimeValue> RuntimeValue::asException() const {
    if (type != Type::EXCEPTION) throw RuntimeError("Value is not an exception");
    return std::get<std::shared_ptr<RuntimeValue>>(data);
}

// Utility methods

std::string RuntimeValue::toString() const {
    std::stringstream ss;
    
    switch (type) {
        case Type::NIL:
            return "nil";
            
        case Type::BOOL:
            return asBool() ? "true" : "false";
            
        case Type::INT:
            if (isUnsigned) {
                ss << "unsigned ";
            }
            ss << "int{" << bitWidth << "}:" << asInt();
            return ss.str();
            
        case Type::FLOAT:
            ss << "float{" << bitWidth << "}:" << asFloat();
            return ss.str();
            
        case Type::STRING:
            try {
                const auto& s = std::get<std::string>(data);
                return s;
            } catch (const std::bad_variant_access&) {
                return "<invalid string>";
            }
            
        case Type::ARRAY: {
            ss << "[";
            const auto& array = asArray();
            for (size_t i = 0; i < array.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << array[i]->toString();
            }
            ss << "]";
            return ss.str();
        }
        
        case Type::POINTER:
            ss << "@" << asPointer()->toString();
            return ss.str();
            
        case Type::FUNCTION:
            return "<function>";
            
        case Type::NATIVE_FUNCTION:
            return "<native function>";
            
        case Type::STRUCT:
            return "<struct>";
            
        case Type::OBJECT:
            return "<object>";
            
        case Type::CLASS:
            return "<class " + asClass()->getName() + ">";
            
        case Type::RETURN_VALUE:
            return "<return " + asReturnValue()->toString() + ">";
            
        case Type::EXCEPTION:
            return "<exception " + asException()->toString() + ">";
            
        case Type::BREAK:
            return "<break>";
            
        case Type::CONTINUE:
            return "<continue>";
            
        default:
            return "<unknown type>";
    }
}

bool RuntimeValue::isTruthy() const {
    switch (type) {
        case Type::NIL:
            return false;
            
        case Type::BOOL:
            return asBool();
            
        case Type::INT:
            return asInt() != 0;
            
        case Type::FLOAT:
            return asFloat() != 0.0f;
            
        case Type::STRING:
            return !asString().empty();
            
        case Type::ARRAY:
            return !asArray().empty();
            
        case Type::POINTER:
            return asPointer() != nullptr;
            
        case Type::STRUCT:
        case Type::OBJECT:
            return !std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data).empty();
            
        case Type::FUNCTION:
        case Type::NATIVE_FUNCTION:
        case Type::CLASS:
            return true;
            
        case Type::RETURN_VALUE:
            return asReturnValue()->isTruthy();
            
        case Type::EXCEPTION:
            return asException()->isTruthy();
            
        case Type::BREAK:
        case Type::CONTINUE:
            return true;
            
        default:
            return false;
    }
}

// Continuing runtime.cpp implementation

bool RuntimeValue::equals(const std::shared_ptr<RuntimeValue>& other) const {
    if (!other || type != other->type) {
        return false;
    }
    
    switch (type) {
        case Type::NIL:
            return true; // All nil values are equal
            
        case Type::BOOL:
            return asBool() == other->asBool();
            
        case Type::INT:
            return asInt() == other->asInt();
            
        case Type::FLOAT:
            return asFloat() == other->asFloat();
            
        case Type::STRING:
            return asString() == other->asString();
            
        case Type::ARRAY: {
            const auto& a = asArray();
            const auto& b = other->asArray();
            
            if (a.size() != b.size()) return false;
            
            for (size_t i = 0; i < a.size(); ++i) {
                if (!a[i]->equals(b[i])) return false;
            }
            
            return true;
        }
        
        case Type::POINTER:
            return asPointer()->equals(other->asPointer());
            
        case Type::STRUCT:
        case Type::OBJECT: {
            const auto& a = std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(data);
            const auto& b = std::get<std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>>(other->data);
            
            if (a.size() != b.size()) return false;
            
            for (const auto& [key, value] : a) {
                auto it = b.find(key);
                if (it == b.end() || !value->equals(it->second)) return false;
            }
            
            return true;
        }
        
        case Type::FUNCTION:
        case Type::NATIVE_FUNCTION:
            // Functions are equal only if they are the same instance
            return this == other.get();
            
        case Type::CLASS:
            return asClass() == other->asClass();
            
        case Type::RETURN_VALUE:
            return asReturnValue()->equals(other->asReturnValue());
            
        case Type::EXCEPTION:
            return asException()->equals(other->asException());
            
        case Type::BREAK:
        case Type::CONTINUE:
            return true; // All break/continue values are equal
            
        default:
            return false;
    }
}

// ===== Function Implementation =====

const std::string& Function::getName() const {
    return declaration->getName();
}

const std::vector<Parameter>& Function::getParameters() const {
    return declaration->getParameters();
}

const std::shared_ptr<BlockStatement>& Function::getBody() const {
    return declaration->getBody();
}

}
