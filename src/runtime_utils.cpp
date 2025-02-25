#include "include/runtime.h"
#include <cmath>
#include <iostream>
#include <sstream>

namespace flux {

// Helper functions for runtime operations

// Convert a Flux type to its corresponding runtime value type
RuntimeValue::Type fluxTypeToRuntimeType(const std::shared_ptr<flux::Type>& type) {
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

// Create a default value for a given Flux type
std::shared_ptr<RuntimeValue> createDefaultValue(const std::shared_ptr<flux::Type>& type) {
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

// Convert between different numeric types
std::shared_ptr<RuntimeValue> convertNumericValue(
    const std::shared_ptr<RuntimeValue>& value,
    RuntimeValue::Type targetType,
    int targetBitWidth,
    bool targetUnsigned
) {
    if (!value->isNumeric()) {
        throw RuntimeError("Cannot convert non-numeric value to numeric type");
    }
    
    if (targetType == RuntimeValue::Type::INT) {
        if (value->isInt()) {
            if (value->getBitWidth() == targetBitWidth && value->getIsUnsigned() == targetUnsigned) {
                return value; // No conversion needed
            }
            
            // Convert from one int type to another
            int32_t intValue = value->asInt();
            
            // Apply bit masking based on target bit width
            if (targetBitWidth < 32) {
                int32_t mask = (1 << targetBitWidth) - 1;
                intValue &= mask;
            }
            
            return RuntimeValue::makeInt(intValue, targetBitWidth, targetUnsigned);
        }
        else if (value->isFloat()) {
            // Convert from float to int
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
            
            // Convert from one float type to another
            float floatValue = value->asFloat();
            return RuntimeValue::makeFloat(floatValue, targetBitWidth);
        }
        else if (value->isInt()) {
            // Convert from int to float
            float floatValue = static_cast<float>(value->asInt());
            return RuntimeValue::makeFloat(floatValue, targetBitWidth);
        }
    }
    
    throw RuntimeError("Unsupported numeric conversion");
}

// Perform arithmetic operations
std::shared_ptr<RuntimeValue> performArithmetic(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op
) {
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

// Perform comparison operations
std::shared_ptr<RuntimeValue> performComparison(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op
) {
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

// Perform logical operations
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

// Concatenate strings
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

// String interpolation helper
std::string interpolateString(
    const std::string& format,
    const std::vector<std::shared_ptr<RuntimeValue>>& values
) {
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

} // namespace flux
