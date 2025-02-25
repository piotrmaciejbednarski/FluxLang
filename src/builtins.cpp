#include "include/builtins.h"
#include "include/runtime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

namespace flux {
namespace builtins {

// Print to standard output
std::shared_ptr<RuntimeValue> print(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i]->toString();
    }
    std::cout << std::endl;
    
    return RuntimeValue::makeNil();
}

// Get user input
std::shared_ptr<RuntimeValue> input(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    // Print prompt if provided
    if (!args.empty() && args[0]->isString()) {
        std::cout << args[0]->asString();
    }
    
    std::string userInput;
    std::getline(std::cin, userInput);
    
    // Execute handler if provided
    if (args.size() > 1 && args[1]->isFunction()) {
        std::vector<std::shared_ptr<RuntimeValue>> handlerArgs = {
            RuntimeValue::makeString(userInput)
        };
        
        env->define("_", RuntimeValue::makeString(userInput));
        // Instead of using call method directly, we'll run this when we implement the interpreter
        return RuntimeValue::makeString(userInput);
    }
    
    return RuntimeValue::makeString(userInput);
}

// Open a file
std::shared_ptr<RuntimeValue> open(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.size() < 2 || !args[0]->isString() || !args[1]->isString()) {
        throw RuntimeError("open() requires filename and mode arguments");
    }
    
    std::string filename = args[0]->asString();
    std::string mode = args[1]->asString();
    
    // Create a file object with methods
    auto fileObject = RuntimeValue::makeObject(std::unordered_map<std::string, std::shared_ptr<RuntimeValue>>());
    
    // File handle stored in object (non-accessible directly)
    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>();
    
    // Open file with appropriate mode
    std::ios_base::openmode openMode = std::ios_base::in;
    if (mode.find('w') != std::string::npos) {
        openMode |= std::ios_base::out;
    }
    if (mode.find('b') != std::string::npos) {
        openMode |= std::ios_base::binary;
    }
    if (mode.find('a') != std::string::npos) {
        openMode |= std::ios_base::app;
    }
    
    file->open(filename, openMode);
    if (!file->is_open()) {
        throw RuntimeError("Failed to open file: " + filename);
    }
    
    // Add file methods
    auto& fileObjectData = fileObject->asObject();
    
    // read() method
    fileObjectData["read"] = RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("read", 
            [file](const std::vector<std::shared_ptr<RuntimeValue>>& args, std::shared_ptr<Environment> env) {
                std::stringstream buffer;
                buffer << file->rdbuf();
                return RuntimeValue::makeString(buffer.str());
            }
        )
    );
    
    // write() method
    fileObjectData["write"] = RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("write",
            [file](const std::vector<std::shared_ptr<RuntimeValue>>& args, std::shared_ptr<Environment> env) {
                if (args.empty() || !args[0]->isString()) {
                    throw RuntimeError("write() requires a string argument");
                }
                
                *file << args[0]->asString();
                return RuntimeValue::makeNil();
            }
        )
    );
    
    // close() method
    fileObjectData["close"] = RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("close",
            [file](const std::vector<std::shared_ptr<RuntimeValue>>& args, std::shared_ptr<Environment> env) {
                file->close();
                return RuntimeValue::makeNil();
            }
        )
    );
    
    // seek() method
    fileObjectData["seek"] = RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("seek",
            [file](const std::vector<std::shared_ptr<RuntimeValue>>& args, std::shared_ptr<Environment> env) {
                if (args.empty() || !args[0]->isInt()) {
                    throw RuntimeError("seek() requires an integer position argument");
                }
                
                file->seekg(args[0]->asInt());
                file->seekp(args[0]->asInt());
                return RuntimeValue::makeNil();
            }
        )
    );
    
    // Call the block handler if provided
    if (args.size() > 2 && args[2]->isFunction()) {
        std::vector<std::shared_ptr<RuntimeValue>> handlerArgs = { fileObject };
        // Instead of calling directly, we'll return the handler and file for the interpreter to handle
        return fileObject;
    }
    
    return fileObject;
}

// Create a socket connection
std::shared_ptr<RuntimeValue> socket(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    // Socket implementation would require platform-specific code
    // This is a simplified placeholder
    throw RuntimeError("socket() is not implemented in this interpreter");
    
    return RuntimeValue::makeNil();
}

// Get length of strings and arrays
std::shared_ptr<RuntimeValue> length(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty()) {
        throw RuntimeError("length() requires an argument");
    }
    
    if (args[0]->isString()) {
        return RuntimeValue::makeInt(static_cast<int32_t>(args[0]->asString().length()));
    }
    else if (args[0]->isArray()) {
        return RuntimeValue::makeInt(static_cast<int32_t>(args[0]->asArray().size()));
    }
    
    throw RuntimeError("length() argument must be a string or array");
}

// Memory allocation
std::shared_ptr<RuntimeValue> memalloc(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isInt()) {
        throw RuntimeError("memalloc() requires a size argument");
    }
    
    int32_t size = args[0]->asInt();
    if (size <= 0) {
        throw RuntimeError("memalloc() size must be positive");
    }
    
    // Create an array of nil values
    std::vector<std::shared_ptr<RuntimeValue>> memory(size, RuntimeValue::makeNil());
    
    return RuntimeValue::makeArray(memory);
}

// Math functions
std::shared_ptr<RuntimeValue> sin(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("sin() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    return RuntimeValue::makeFloat(static_cast<float>(std::sin(value)));
}

std::shared_ptr<RuntimeValue> cos(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("cos() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    return RuntimeValue::makeFloat(static_cast<float>(std::cos(value)));
}

std::shared_ptr<RuntimeValue> tan(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("tan() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    return RuntimeValue::makeFloat(static_cast<float>(std::tan(value)));
}

std::shared_ptr<RuntimeValue> cot(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("cot() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    double tanValue = std::tan(value);
    if (tanValue == 0.0) {
        throw RuntimeError("cot() undefined for this value");
    }
    
    return RuntimeValue::makeFloat(static_cast<float>(1.0 / tanValue));
}

std::shared_ptr<RuntimeValue> sec(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("sec() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    double cosValue = std::cos(value);
    if (cosValue == 0.0) {
        throw RuntimeError("sec() undefined for this value");
    }
    
    return RuntimeValue::makeFloat(static_cast<float>(1.0 / cosValue));
}

std::shared_ptr<RuntimeValue> cosec(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("cosec() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    double sinValue = std::sin(value);
    if (sinValue == 0.0) {
        throw RuntimeError("cosec() undefined for this value");
    }
    
    return RuntimeValue::makeFloat(static_cast<float>(1.0 / sinValue));
}

std::shared_ptr<RuntimeValue> quad_eq(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.size() < 3 || !args[0]->isNumeric() || !args[1]->isNumeric() || !args[2]->isNumeric()) {
        throw RuntimeError("quad_eq() requires three numeric arguments (a, b, c)");
    }
    
    double a = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    double b = args[1]->isFloat() ? 
        static_cast<double>(args[1]->asFloat()) : 
        static_cast<double>(args[1]->asInt());
    
    double c = args[2]->isFloat() ? 
        static_cast<double>(args[2]->asFloat()) : 
        static_cast<double>(args[2]->asInt());
    
    if (a == 0.0) {
        throw RuntimeError("quad_eq() requires a non-zero 'a' coefficient");
    }
    
    double discriminant = b * b - 4.0 * a * c;
    std::vector<std::shared_ptr<RuntimeValue>> solutions;
    
    if (discriminant > 0.0) {
        double root1 = (-b + std::sqrt(discriminant)) / (2.0 * a);
        double root2 = (-b - std::sqrt(discriminant)) / (2.0 * a);
        
        solutions.push_back(RuntimeValue::makeFloat(static_cast<float>(root1)));
        solutions.push_back(RuntimeValue::makeFloat(static_cast<float>(root2)));
    }
    else if (discriminant == 0.0) {
        double root = -b / (2.0 * a);
        solutions.push_back(RuntimeValue::makeFloat(static_cast<float>(root)));
    }
    else {
        double realPart = -b / (2.0 * a);
        double imagPart = std::sqrt(-discriminant) / (2.0 * a);
        
        // For complex roots, we'd ideally return complex numbers
        // Since we don't have a complex type yet, we'll return strings
        std::stringstream root1;
        root1 << realPart << " + " << imagPart << "i";
        
        std::stringstream root2;
        root2 << realPart << " - " << imagPart << "i";
        
        solutions.push_back(RuntimeValue::makeString(root1.str()));
        solutions.push_back(RuntimeValue::makeString(root2.str()));
    }
    
    return RuntimeValue::makeArray(solutions);
}

std::shared_ptr<RuntimeValue> sqrt(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env) {
    
    if (args.empty() || !args[0]->isNumeric()) {
        throw RuntimeError("sqrt() requires a numeric argument");
    }
    
    double value = args[0]->isFloat() ? 
        static_cast<double>(args[0]->asFloat()) : 
        static_cast<double>(args[0]->asInt());
    
    if (value < 0.0) {
        throw RuntimeError("sqrt() undefined for negative values");
    }
    
    return RuntimeValue::makeFloat(static_cast<float>(std::sqrt(value)));
}

// Register all built-in functions to an environment
void registerBuiltins(std::shared_ptr<Environment> env) {
    env->define("print", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("print", print)
    ));
    
    env->define("input", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("input", input)
    ));
    
    env->define("open", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("open", open)
    ));
    
    env->define("socket", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("socket", socket)
    ));
    
    env->define("length", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("length", length)
    ));
    
    env->define("memalloc", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("memalloc", memalloc)
    ));
    
    // Math functions
    env->define("sin", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("sin", sin)
    ));
    
    env->define("cos", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("cos", cos)
    ));
    
    env->define("tan", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("tan", tan)
    ));
    
    env->define("cot", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("cot", cot)
    ));
    
    env->define("sec", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("sec", sec)
    ));
    
    env->define("cosec", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("cosec", cosec)
    ));
    
    env->define("quad_eq", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("quad_eq", quad_eq)
    ));
    
    env->define("sqrt", RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>("sqrt", sqrt)
    ));
}

// Convert between different numeric types
std::shared_ptr<RuntimeValue> convertNumericValue(
    const std::shared_ptr<RuntimeValue>& value,
    RuntimeValue::Type targetType,
    int targetBitWidth,
    bool targetUnsigned) {
    
    if (!value->isNumeric()) {
        throw RuntimeError("Cannot convert non-numeric value");
    }
    
    if (targetType == RuntimeValue::Type::INT) {
        if (value->isInt()) {
            if (value->getBitWidth() == targetBitWidth && value->getIsUnsigned() == targetUnsigned) {
                return value; // No conversion needed
            }
            
            // Convert int to int with different bit width or signedness
            int32_t intValue = value->asInt();
            
            // Apply bit masking based on target bit width
            if (targetBitWidth < 32) {
                int32_t mask = (1 << targetBitWidth) - 1;
                intValue &= mask;
            }
            
            return RuntimeValue::makeInt(intValue, targetBitWidth, targetUnsigned);
        }
        else if (value->isFloat()) {
            // Convert float to int
            int32_t intValue = static_cast<int32_t>(value->asFloat());
            
            // Apply bit masking based on target bit width
            if (targetBitWidth < 32) {
                int32_t mask = (1 << targetBitWidth) - 1;
                intValue &= mask;
            }
            
            return RuntimeValue::makeInt(intValue, targetBitWidth, targetUnsigned);
        }
    }
    else if (targetType == RuntimeValue::Type::FLOAT) {
        if (value->isFloat()) {
            if (value->getBitWidth() == targetBitWidth) {
                return value; // No conversion needed
            }
            
            // Convert float to float with different bit width
            float floatValue = value->asFloat();
            return RuntimeValue::makeFloat(floatValue, targetBitWidth);
        }
        else if (value->isInt()) {
            // Convert int to float
            float floatValue = static_cast<float>(value->asInt());
            return RuntimeValue::makeFloat(floatValue, targetBitWidth);
        }
    }
    
    throw RuntimeError("Unsupported numeric conversion");
}

// Create default value for a given Flux type
std::shared_ptr<RuntimeValue> createDefaultValue(const std::shared_ptr<Type>& type) {
    if (!type) return RuntimeValue::makeNil();
    
    switch (type->getKind()) {
        case Type::Kind::VOID:
            return RuntimeValue::makeNil();
            
        case Type::Kind::BOOL:
            return RuntimeValue::makeBool(false);
            
        case Type::Kind::INT: {
            auto intType = std::dynamic_pointer_cast<PrimitiveType>(type);
            return RuntimeValue::makeInt(0, intType->getBitWidth(), intType->getIsUnsigned());
        }
            
        case Type::Kind::FLOAT: {
            auto floatType = std::dynamic_pointer_cast<PrimitiveType>(type);
            return RuntimeValue::makeFloat(0.0f, floatType->getBitWidth());
        }
            
        case Type::Kind::STRING:
            return RuntimeValue::makeString("");
            
        case Type::Kind::POINTER:
            return RuntimeValue::makePointer(nullptr);
            
        case Type::Kind::STRUCT: {
            auto structType = std::dynamic_pointer_cast<StructType>(type);
            std::unordered_map<std::string, std::shared_ptr<RuntimeValue>> fields;
            
            for (const auto& field : structType->getFields()) {
                fields[field.name] = createDefaultValue(field.type);
            }
            
            return RuntimeValue::makeStruct(std::move(fields));
        }
            
        case Type::Kind::OBJECT:
        case Type::Kind::CLASS:
            // Would need object instance creation logic
            return RuntimeValue::makeNil();
            
        case Type::Kind::FUNCTION:
            return RuntimeValue::makeNil();
            
        case Type::Kind::UNION:
            // Would need special handling for unions
            return RuntimeValue::makeNil();
            
        case Type::Kind::NULLPTR:
            return RuntimeValue::makePointer(nullptr);
            
        default:
            return RuntimeValue::makeNil();
    }
}

// String interpolation
std::string interpolateString(
    const std::string& format,
    const std::vector<std::shared_ptr<RuntimeValue>>& values) {
    
    std::string result;
    size_t valueIndex = 0;
    
    for (size_t i = 0; i < format.length(); ++i) {
        if (format[i] == '{' && i + 1 < format.length() && format[i + 1] == '}') {
            // Found {} placeholder
            if (valueIndex < values.size()) {
                result += values[valueIndex]->toString();
                valueIndex++;
            } else {
                result += "{}"; // No value available
            }
            i++; // Skip the closing }
        } else {
            result += format[i];
        }
    }
    
    return result;
}

// Type conversion utilities
RuntimeValue::Type fluxTypeToRuntimeType(const std::shared_ptr<Type>& type) {
    if (!type) return RuntimeValue::Type::NIL;
    
    switch (type->getKind()) {
        case Type::Kind::VOID:
            return RuntimeValue::Type::NIL;
        case Type::Kind::BOOL:
            return RuntimeValue::Type::BOOL;
        case Type::Kind::INT:
            return RuntimeValue::Type::INT;
        case Type::Kind::FLOAT:
            return RuntimeValue::Type::FLOAT;
        case Type::Kind::STRING:
            return RuntimeValue::Type::STRING;
        case Type::Kind::POINTER:
            return RuntimeValue::Type::POINTER;
        case Type::Kind::STRUCT:
            return RuntimeValue::Type::STRUCT;
        case Type::Kind::CLASS:
            return RuntimeValue::Type::CLASS;
        case Type::Kind::OBJECT:
            return RuntimeValue::Type::OBJECT;
        case Type::Kind::FUNCTION:
            return RuntimeValue::Type::FUNCTION;
        case Type::Kind::UNION:
            // Unions would need special handling
            return RuntimeValue::Type::STRUCT;
        case Type::Kind::NULLPTR:
            return RuntimeValue::Type::POINTER;
        default:
            return RuntimeValue::Type::NIL;
    }
}

// Arithmetic operations
std::shared_ptr<RuntimeValue> performArithmetic(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op) {
    
    // Special case for string concatenation
    if (op == BinaryOp::ADD && (left->isString() || right->isString())) {
        return concatStrings(left, right);
    }
    
    // Both operands must be numeric
    if (!left->isNumeric() || !right->isNumeric()) {
        throw RuntimeError("Arithmetic operations require numeric operands");
    }
    
    // If either operand is float, result is float
    bool resultIsFloat = left->isFloat() || right->isFloat();
    
    // For mixed int/float operations, convert to float
    if (resultIsFloat) {
        float leftVal = left->isFloat() ? left->asFloat() : static_cast<float>(left->asInt());
        float rightVal = right->isFloat() ? right->asFloat() : static_cast<float>(right->asInt());
        
        // Use the larger bit width
        int resultBitWidth = std::max(left->getBitWidth(), right->getBitWidth());
        
        switch (op) {
            case BinaryOp::ADD:
                return RuntimeValue::makeFloat(leftVal + rightVal, resultBitWidth);
            case BinaryOp::SUB:
                return RuntimeValue::makeFloat(leftVal - rightVal, resultBitWidth);
            case BinaryOp::MUL:
                return RuntimeValue::makeFloat(leftVal * rightVal, resultBitWidth);
            case BinaryOp::DIV:
                if (rightVal == 0.0f) {
                    throw RuntimeError("Division by zero");
                }
                return RuntimeValue::makeFloat(leftVal / rightVal, resultBitWidth);
            case BinaryOp::MOD:
                if (rightVal == 0.0f) {
                    throw RuntimeError("Modulo by zero");
                }
                return RuntimeValue::makeFloat(std::fmod(leftVal, rightVal), resultBitWidth);
            case BinaryOp::EXPONENT:
                return RuntimeValue::makeFloat(std::pow(leftVal, rightVal), resultBitWidth);
            default:
                throw RuntimeError("Unsupported float operation");
        }
    }
    else {
        // Integer operations
        int32_t leftVal = left->asInt();
        int32_t rightVal = right->asInt();
        
        // Use the larger bit width
        int resultBitWidth = std::max(left->getBitWidth(), right->getBitWidth());
        
        // Result is unsigned if either operand is unsigned
        bool resultIsUnsigned = left->getIsUnsigned() || right->getIsUnsigned();
        
        switch (op) {
            case BinaryOp::ADD:
                return RuntimeValue::makeInt(leftVal + rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::SUB:
                return RuntimeValue::makeInt(leftVal - rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::MUL:
                return RuntimeValue::makeInt(leftVal * rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::DIV:
                if (rightVal == 0) {
                    throw RuntimeError("Division by zero");
                }
                return RuntimeValue::makeInt(leftVal / rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::MOD:
                if (rightVal == 0) {
                    throw RuntimeError("Modulo by zero");
                }
                return RuntimeValue::makeInt(leftVal % rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::BITAND:
                return RuntimeValue::makeInt(leftVal & rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::BITOR:
                return RuntimeValue::makeInt(leftVal | rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::BITXOR:
                return RuntimeValue::makeInt(leftVal ^ rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::SHIFTLEFT:
                return RuntimeValue::makeInt(leftVal << rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::SHIFTRIGHT:
                return RuntimeValue::makeInt(leftVal >> rightVal, resultBitWidth, resultIsUnsigned);
            case BinaryOp::EXPONENT:
                return RuntimeValue::makeInt(static_cast<int32_t>(std::pow(leftVal, rightVal)), 
                                           resultBitWidth, resultIsUnsigned);
            default:
                throw RuntimeError("Unsupported integer operation");
        }
    }
}

// Comparison operations
// Continuing builtins.cpp implementation

std::shared_ptr<RuntimeValue> performComparison(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op) {
    
    // Handle equality operations for any type
    if (op == BinaryOp::EQ) {
        return RuntimeValue::makeBool(left->equals(right));
    }
    else if (op == BinaryOp::NE) {
        return RuntimeValue::makeBool(!left->equals(right));
    }
    
    // Other comparisons require numeric operands or strings
    if (left->isNumeric() && right->isNumeric()) {
        // If either operand is float, compare as floats
        if (left->isFloat() || right->isFloat()) {
            float leftVal = left->isFloat() ? left->asFloat() : static_cast<float>(left->asInt());
            float rightVal = right->isFloat() ? right->asFloat() : static_cast<float>(right->asInt());
            
            switch (op) {
                case BinaryOp::LT:
                    return RuntimeValue::makeBool(leftVal < rightVal);
                case BinaryOp::LE:
                    return RuntimeValue::makeBool(leftVal <= rightVal);
                case BinaryOp::GT:
                    return RuntimeValue::makeBool(leftVal > rightVal);
                case BinaryOp::GE:
                    return RuntimeValue::makeBool(leftVal >= rightVal);
                default:
                    throw RuntimeError("Unsupported comparison operation");
            }
        }
        else {
            // Integer comparisons
            int32_t leftVal = left->asInt();
            int32_t rightVal = right->asInt();
            
            switch (op) {
                case BinaryOp::LT:
                    return RuntimeValue::makeBool(leftVal < rightVal);
                case BinaryOp::LE:
                    return RuntimeValue::makeBool(leftVal <= rightVal);
                case BinaryOp::GT:
                    return RuntimeValue::makeBool(leftVal > rightVal);
                case BinaryOp::GE:
                    return RuntimeValue::makeBool(leftVal >= rightVal);
                default:
                    throw RuntimeError("Unsupported comparison operation");
            }
        }
    }
    else if (left->isString() && right->isString()) {
        // String comparisons
        const std::string& leftStr = left->asString();
        const std::string& rightStr = right->asString();
        
        switch (op) {
            case BinaryOp::LT:
                return RuntimeValue::makeBool(leftStr < rightStr);
            case BinaryOp::LE:
                return RuntimeValue::makeBool(leftStr <= rightStr);
            case BinaryOp::GT:
                return RuntimeValue::makeBool(leftStr > rightStr);
            case BinaryOp::GE:
                return RuntimeValue::makeBool(leftStr >= rightStr);
            default:
                throw RuntimeError("Unsupported comparison operation");
        }
    }
    
    throw RuntimeError("Cannot compare values of different types");
}

// String concatenation utility
std::shared_ptr<RuntimeValue> concatStrings(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right
) {
    std::string leftStr;
    std::string rightStr;
    
    // Convert left operand to string
    if (left->isString()) {
        leftStr = left->asString();
    } else {
        leftStr = left->toString();
    }
    
    // Convert right operand to string
    if (right->isString()) {
        rightStr = right->asString();
    } else {
        rightStr = right->toString();
    }
    
    return RuntimeValue::makeString(leftStr + rightStr);
}

std::shared_ptr<RuntimeValue> performLogical(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,  
    BinaryOp op
) {
    switch (op) {
        case BinaryOp::LOGICAL_AND:
            return RuntimeValue::makeBool(left->isTruthy() && right->isTruthy());
            
        case BinaryOp::LOGICAL_OR:
            return RuntimeValue::makeBool(left->isTruthy() || right->isTruthy());
            
        default:
            throw RuntimeError("Unsupported logical operation");
    }
}

} // Builtins namespace
} // Flux namespace
