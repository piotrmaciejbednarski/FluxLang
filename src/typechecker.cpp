#include "include/typechecker.h"
#include "include/ast.h"
#include <iostream>

namespace flux {

TypeChecker::TypeChecker() {}

TypeChecker::~TypeChecker() {}

void TypeChecker::initialize() {
    typeEnvironment.clear();
    
    // Register only the precise native types
    defineType("void", std::make_shared<PrimitiveType>(Type::Kind::VOID));
    defineType("bool", std::make_shared<PrimitiveType>(Type::Kind::BOOL));
    defineType("int", std::make_shared<PrimitiveType>(Type::Kind::INT, 32, false));
    defineType("unsigned int", std::make_shared<PrimitiveType>(Type::Kind::INT, 32, true));
    defineType("float", std::make_shared<PrimitiveType>(Type::Kind::FLOAT, 32));
    defineType("string", std::make_shared<PrimitiveType>(Type::Kind::STRING));
}

bool TypeChecker::checkProgram(const std::shared_ptr<Program>& program) {
    reset();
    initialize();

    bool hasErrors = false;

    for (const auto& declaration : program->getDeclarations()) {
        if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(declaration)) {
            // Get the type string
            std::string typeName = varDecl->getType()->toString();
            
            // Explicitly check against known types
            if (typeName != "void" && 
                typeName != "bool" && 
                typeName != "int" && 
                typeName != "unsigned int" && 
                typeName != "float" && 
                typeName != "string" &&
                !typeName.find("int{") == 0 &&  // Allow int with bit width
                !typeName.find("float{") == 0)  // Allow float with bit width
            {
                std::cerr << "Error: Undefined type '" 
                          << typeName 
                          << "' for variable '" 
                          << varDecl->getName() 
                          << "'" << std::endl;
                hasErrors = true;
            }
        }
    }

    return !hasErrors;
}

void TypeChecker::defineType(const std::string& name, std::shared_ptr<Type> type) {
    typeEnvironment[name] = type;
}

std::shared_ptr<Type> TypeChecker::getType(const std::string& name) {
    auto it = typeEnvironment.find(name);
    return it != typeEnvironment.end() ? it->second : nullptr;
}

bool TypeChecker::hasType(const std::string& name) {
    return typeEnvironment.find(name) != typeEnvironment.end();
}

void TypeChecker::reset() {
    typeEnvironment.clear();
}

void TypeChecker::setCurrentFunctionReturnType(std::shared_ptr<Type> type) {
    currentFunctionReturnType = type;
}

std::shared_ptr<Type> TypeChecker::getCurrentFunctionReturnType() const {
    return currentFunctionReturnType;
}

bool TypeChecker::areTypesCompatible(
    const std::shared_ptr<Type>& expected,
    const std::shared_ptr<Type>& actual) {
    
    if (!expected || !actual) return false;

    // Exact type match
    if (expected->isEquivalentTo(actual.get())) return true;

    // Pointer type compatibility
    if (expected->getKind() == Type::Kind::POINTER) {
        if (actual->getKind() == Type::Kind::NULLPTR) return true;
        
        auto expectedPtr = std::dynamic_pointer_cast<PointerType>(expected);
        auto actualPtr = std::dynamic_pointer_cast<PointerType>(actual);
        
        if (actualPtr) {
            return expectedPtr->getPointeeType()->isEquivalentTo(
                actualPtr->getPointeeType().get());
        }
    }

    // Numeric type conversions
    if (expected->getKind() == Type::Kind::INT && actual->getKind() == Type::Kind::INT) {
        auto expectedInt = std::dynamic_pointer_cast<PrimitiveType>(expected);
        auto actualInt = std::dynamic_pointer_cast<PrimitiveType>(actual);
        
        // Can assign smaller integer to larger one
        return expectedInt->getBitWidth() >= actualInt->getBitWidth();
    }
    
    // Float compatibility
    if (expected->getKind() == Type::Kind::FLOAT && actual->getKind() == Type::Kind::FLOAT) {
        auto expectedFloat = std::dynamic_pointer_cast<PrimitiveType>(expected);
        auto actualFloat = std::dynamic_pointer_cast<PrimitiveType>(actual);
        
        return expectedFloat->getBitWidth() >= actualFloat->getBitWidth();
    }
    
    // Void is compatible with everything 
    if (expected->getKind() == Type::Kind::VOID) return true;
    
    return false;
}

std::shared_ptr<Type> TypeChecker::getCommonType(
    const std::shared_ptr<Type>& left,
    const std::shared_ptr<Type>& right) {
    
    if (!left || !right) return nullptr;
    
    if (left->isEquivalentTo(right.get())) {
        return left;
    }
    
    // Handle numeric types
    if (left->getKind() == Type::Kind::INT && right->getKind() == Type::Kind::INT) {
        auto leftInt = std::dynamic_pointer_cast<PrimitiveType>(left);
        auto rightInt = std::dynamic_pointer_cast<PrimitiveType>(right);
        
        int maxBitWidth = std::max(leftInt->getBitWidth(), rightInt->getBitWidth());
        bool isUnsigned = leftInt->getIsUnsigned() || rightInt->getIsUnsigned();
        
        return createPrimitiveType(Type::Kind::INT, maxBitWidth, isUnsigned);
    }
    
    if ((left->getKind() == Type::Kind::INT && right->getKind() == Type::Kind::FLOAT) ||
        (left->getKind() == Type::Kind::FLOAT && right->getKind() == Type::Kind::INT)) {
        auto floatType = (left->getKind() == Type::Kind::FLOAT) ? left : right;
        return floatType;
    }
    
    if (left->getKind() == Type::Kind::FLOAT && right->getKind() == Type::Kind::FLOAT) {
        auto leftFloat = std::dynamic_pointer_cast<PrimitiveType>(left);
        auto rightFloat = std::dynamic_pointer_cast<PrimitiveType>(right);
        
        int maxBitWidth = std::max(leftFloat->getBitWidth(), rightFloat->getBitWidth());
        return createPrimitiveType(Type::Kind::FLOAT, maxBitWidth);
    }
    
    // No common type found
    return nullptr;
}

std::shared_ptr<PrimitiveType> TypeChecker::createPrimitiveType(
    Type::Kind kind,
    int bitWidth,
    bool isUnsigned) {
    
    return std::make_shared<PrimitiveType>(kind, bitWidth, isUnsigned);
}

std::shared_ptr<PointerType> TypeChecker::createPointerType(std::shared_ptr<Type> pointeeType) {
    return std::make_shared<PointerType>(pointeeType);
}

std::shared_ptr<FunctionType> TypeChecker::createFunctionType(
    std::shared_ptr<Type> returnType,
    const std::vector<FunctionParam>& params) {
    
    auto funcType = std::make_shared<FunctionType>(returnType);
    
    for (const auto& param : params) {
        funcType->addParameter(FunctionParam(param.name, param.type));
    }
    
    return funcType;
}

} // namespace flux
