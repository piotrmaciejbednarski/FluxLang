// Implementation of virtual methods for AST classes

#include "include/ast.h"
#include <sstream>

namespace flux {

class SymbolResolver {
public:
    // Placeholder for symbol table and type resolution
    static std::shared_ptr<Type> resolveIdentifier(const std::string& name) {
        // In a real implementation, this would:
        // 1. Look up the identifier in current scope
        // 2. Check namespaces, imported modules
        // 3. Return the resolved type or nullptr

        // Temporary implementation
        if (name == "int") {
            return std::make_shared<PrimitiveType>(
                SourceLocation{}, 
                PrimitiveTypeKind::INT, 
                BitWidth::_32, 
                false
            );
        }
        
        // Default fallback
        return std::make_shared<UserDefinedType>(
            SourceLocation{}, 
            "unknown"
        );
    }
};

// PrimitiveType implementation
bool PrimitiveType::isEqual(const Type& other) const {
    // Try to cast to PrimitiveType
    const PrimitiveType* otherPrim = dynamic_cast<const PrimitiveType*>(&other);
    if (!otherPrim) return false;
    
    return kind == otherPrim->kind && 
           width == otherPrim->width && 
           isUnsigned == otherPrim->isUnsigned;
}

std::string PrimitiveType::toString() const {
    std::stringstream ss;
    
    // Add unsigned specifier if needed
    if (isUnsigned) ss << "unsigned ";
    
    // Add type name
    switch (kind) {
        case PrimitiveTypeKind::INT: ss << "int"; break;
        case PrimitiveTypeKind::FLOAT: ss << "float"; break;
        case PrimitiveTypeKind::CHAR: ss << "char"; break;
        case PrimitiveTypeKind::VOID: ss << "void"; break;
        case PrimitiveTypeKind::BOOL: ss << "bool"; break;
    }
    
    // Add bit width
    switch (width) {
        case BitWidth::_8: ss << "{8}"; break;
        case BitWidth::_16: ss << "{16}"; break;
        case BitWidth::_32: ss << "{32}"; break;
        case BitWidth::_64: ss << "{64}"; break;
        case BitWidth::_128: ss << "{128}"; break;
        default: break;
    }
    
    return ss.str();
}

// ArrayType implementation
std::shared_ptr<Type> ArrayLiteralExpression::getType() const {
    if (elements.empty()) {
        // Create an empty array of void type
        return std::make_shared<ArrayType>(
            getLocation(),
            std::make_shared<PrimitiveType>(
                getLocation(), 
                PrimitiveTypeKind::VOID, 
                BitWidth::DEFAULT, 
                false
            ), 
            0
        );
    }
    
    // Use the type of the first element to create the array type
    return std::make_shared<ArrayType>(
        getLocation(), 
        elements[0]->getType(), 
        elements.size()
    );
}

bool ArrayType::isEqual(const Type& other) const {
    const ArrayType* otherArray = dynamic_cast<const ArrayType*>(&other);
    if (!otherArray) return false;

    if (!elementType->isEqual(*otherArray->elementType)) return false;

    return size == otherArray->size;
}

std::string ArrayType::toString() const {
    std::stringstream ss;
    ss << elementType->toString() << "[";
    
    if (size) {
        ss << *size;
    }
    
    ss << "]";
    return ss.str();
}

// PointerType implementation
bool PointerType::isEqual(const Type& other) const {
    const PointerType* otherPtr = dynamic_cast<const PointerType*>(&other);
    if (!otherPtr) return false;
    
    return pointeeType->isEqual(*otherPtr->pointeeType);
}

std::string PointerType::toString() const {
    return pointeeType->toString() + "*";
}

// UserDefinedType implementation
bool UserDefinedType::isEqual(const Type& other) const {
    const UserDefinedType* otherUser = dynamic_cast<const UserDefinedType*>(&other);
    if (!otherUser) return false;
    
    return name == otherUser->name;
}

std::string UserDefinedType::toString() const {
    return name;
}

// StructType implementation
bool StructType::isEqual(const Type& other) const {
    const StructType* otherStruct = dynamic_cast<const StructType*>(&other);
    if (!otherStruct) return false;
    
    // Compare number of fields
    if (fields.size() != otherStruct->fields.size()) return false;
    
    // Compare each field's type and name
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].name != otherStruct->fields[i].name) return false;
        if (!fields[i].type->isEqual(*otherStruct->fields[i].type)) return false;
    }
    
    return true;
}

std::string StructType::toString() const {
    std::stringstream ss;
    ss << "struct { ";
    
    for (const auto& field : fields) {
        ss << field.type->toString() << " " << field.name << "; ";
    }
    
    ss << "}";
    return ss.str();
}

// FunctionType implementation
bool FunctionType::isEqual(const Type& other) const {
    const FunctionType* otherFunc = dynamic_cast<const FunctionType*>(&other);
    if (!otherFunc) return false;
    
    // Compare return type
    if (!returnType->isEqual(*otherFunc->returnType)) return false;
    
    // Compare parameter types
    if (paramTypes.size() != otherFunc->paramTypes.size()) return false;
    
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        if (!paramTypes[i]->isEqual(*otherFunc->paramTypes[i])) return false;
    }
    
    return true;
}

std::string FunctionType::toString() const {
    std::stringstream ss;
    ss << returnType->toString() << "(";
    
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        ss << paramTypes[i]->toString();
        if (i < paramTypes.size() - 1) ss << ", ";
    }
    
    ss << ")";
    return ss.str();
}

// Expression type implementations
std::shared_ptr<Type> IdentifierExpression::getType() const {
    // Use symbol resolver to find the type of the identifier
    return SymbolResolver::resolveIdentifier(name);
}

std::shared_ptr<Type> BinaryExpression::getType() const {
    auto leftType = left->getType();
    auto rightType = right->getType();
    
    // Type promotion and inference logic
    switch (op) {
        case Operator::ADD:
        case Operator::SUB:
        case Operator::MUL:
        case Operator::DIV:
        case Operator::MOD: {
            // Numeric type promotion
            if (auto leftPrim = std::dynamic_pointer_cast<PrimitiveType>(leftType)) {
                if (leftPrim->getKind() == PrimitiveTypeKind::FLOAT) {
                    return leftType;  // Prefer float if one operand is float
                }
            }
            
            if (auto rightPrim = std::dynamic_pointer_cast<PrimitiveType>(rightType)) {
                if (rightPrim->getKind() == PrimitiveTypeKind::FLOAT) {
                    return rightType;
                }
            }
            
            // Default to integer type with maximum bit width
            return std::make_shared<PrimitiveType>(
                getLocation(), 
                PrimitiveTypeKind::INT, 
                BitWidth::_64, 
                false
            );
        }
        
        case Operator::EQ:
        case Operator::NE:
        case Operator::LT:
        case Operator::LE:
        case Operator::GT:
        case Operator::GE: {
            // Comparison always returns bool
            return std::make_shared<PrimitiveType>(
                getLocation(), 
                PrimitiveTypeKind::BOOL, 
                BitWidth::DEFAULT, 
                false
            );
        }
        
        case Operator::AND:
        case Operator::OR: {
            // Logical operations return bool
            return std::make_shared<PrimitiveType>(
                getLocation(), 
                PrimitiveTypeKind::BOOL, 
                BitWidth::DEFAULT, 
                false
            );
        }
        
        case Operator::BIT_AND:
        case Operator::BIT_OR:
        case Operator::BIT_XOR: {
            // Bitwise operations return the type of the left operand
            return leftType;
        }
        
        case Operator::ASSIGN:
        case Operator::ADD_ASSIGN:
        case Operator::SUB_ASSIGN:
        case Operator::MUL_ASSIGN:
        case Operator::DIV_ASSIGN:
        case Operator::MOD_ASSIGN: {
            // Assignment returns the type of the left side
            return leftType;
        }
        
        default:
            return std::make_shared<UserDefinedType>(getLocation(), "unknown_binary_op");
    }
}

std::shared_ptr<Type> UnaryExpression::getType() const {
    // Similar to BinaryExpression, return operand type
    return operand->getType();
}

std::shared_ptr<Type> CallExpression::getType() const {
    // Try to resolve the type of the callee (function/method)
    auto calleeType = callee->getType();
    
    // If the callee is a function type, return its return type
    if (auto funcType = std::dynamic_pointer_cast<FunctionType>(calleeType)) {
        return funcType->getReturnType();
    }
    
    // For method calls or other callable types, try to infer return type
    // This is a simplified approach and would need more sophisticated type inference
    return std::make_shared<UserDefinedType>(getLocation(), "call_result");
}

std::shared_ptr<Type> MemberAccessExpression::getType() const {
    auto objectType = object->getType();
    
    // Handle different type scenarios
    if (auto structType = std::dynamic_pointer_cast<StructType>(objectType)) {
        // Search for member in struct fields
        for (const auto& field : structType->getFields()) {
            if (field.name == member) {
                return field.type;
            }
        }
    }
    
    if (auto userType = std::dynamic_pointer_cast<UserDefinedType>(objectType)) {
        // TODO: Implement more sophisticated member resolution
        // This would involve looking up the type definition and finding the member
        return std::make_shared<UserDefinedType>(getLocation(), "member_type");
    }
    
    // Fallback for unresolved member access
    return std::make_shared<UserDefinedType>(getLocation(), "unknown_member");
}

std::shared_ptr<Type> IndexExpression::getType() const {
    auto arrayType = std::dynamic_pointer_cast<ArrayType>(array->getType());
    
    if (arrayType) {
        return arrayType->getElementType();
    }
    
    // Fallback
    return std::make_shared<UserDefinedType>(getLocation(), "unknown");
}

std::shared_ptr<Type> InjectableStringExpression::getType() const {
    SourceLocation loc = getLocation();  // Get the location using getLocation()
    return std::make_shared<UserDefinedType>(
        loc,  // Pass the location as an argument
        "string"
    );
}

} // namespace flux
