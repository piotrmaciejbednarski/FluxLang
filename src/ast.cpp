#include "include/ast.h"
#include <sstream>

namespace flux {

// Type implementations
std::string PrimitiveType::toString() const {
    std::string result;
    
    switch (kind) {
        case Kind::VOID: result = "void"; break;
        case Kind::BOOL: result = "bool"; break;
        case Kind::INT: 
            if (isUnsigned) result = "unsigned ";
            result += "int";
            if (bitWidth > 0) {
                result += "{" + std::to_string(bitWidth) + "}";
            }
            break;
        case Kind::FLOAT: 
            result = "float";
            if (bitWidth > 0) {
                result += "{" + std::to_string(bitWidth) + "}";
            }
            break;
        case Kind::STRING: result = "string"; break;
        case Kind::NULLPTR: result = "nullptr"; break;
        default: result = "unknown"; break;
    }
    
    return result;
}

bool PrimitiveType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != kind) return false;
    
    auto otherPrim = static_cast<const PrimitiveType*>(other);
    
    // For numeric types, check bit width and signedness
    if (kind == Kind::INT || kind == Kind::FLOAT) {
        return bitWidth == otherPrim->bitWidth && 
               isUnsigned == otherPrim->isUnsigned;
    }
    
    // For other types, just checking the kind is sufficient
    return true;
}

std::string PointerType::toString() const {
    return pointeeType->toString() + "*";
}

bool PointerType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::POINTER) return false;
    
    auto otherPtr = static_cast<const PointerType*>(other);
    return pointeeType->isEquivalentTo(otherPtr->pointeeType.get());
}

std::string StructType::toString() const {
    return "struct " + name;
}

bool StructType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::STRUCT) return false;
    
    auto otherStruct = static_cast<const StructType*>(other);
    return name == otherStruct->name;
}

std::string FunctionType::toString() const {
    std::stringstream ss;
    ss << returnType->toString() << " (";
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << parameters[i].type->toString();
    }
    
    ss << ")";
    return ss.str();
}

bool FunctionType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::FUNCTION) return false;
    
    auto otherFunc = static_cast<const FunctionType*>(other);
    
    // Check return type
    if (!returnType->isEquivalentTo(otherFunc->returnType.get())) {
        return false;
    }
    
    // Check parameter count
    if (parameters.size() != otherFunc->parameters.size()) {
        return false;
    }
    
    // Check each parameter type
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (!parameters[i].type->isEquivalentTo(otherFunc->parameters[i].type.get())) {
            return false;
        }
    }
    
    return true;
}

std::string ClassType::toString() const {
    return "class " + name;
}

bool ClassType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::CLASS) return false;
    
    auto otherClass = static_cast<const ClassType*>(other);
    return name == otherClass->name;
}

std::string ObjectType::toString() const {
    return "object " + name;
}

bool ObjectType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::OBJECT) return false;
    
    auto otherObj = static_cast<const ObjectType*>(other);
    return name == otherObj->name;
}

std::string UnionType::toString() const {
    return "union " + name;
}

bool UnionType::isEquivalentTo(const Type* other) const {
    if (other->getKind() != Kind::UNION) return false;
    
    auto otherUnion = static_cast<const UnionType*>(other);
    return name == otherUnion->name;
}

// Expression implementations
std::shared_ptr<Value> LiteralExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> VariableExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> BinaryExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> UnaryExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> CallExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> IndexExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> MemberExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> ArrowExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

std::shared_ptr<Value> ArrayLiteralExpression::evaluate() {
    // Implementation will be added later
    return nullptr;
}

// Statement implementations
void ExpressionStatement::execute() {
    // Implementation will be added later
}

void BlockStatement::execute() {
    // Implementation will be added later
}

void VariableDeclaration::execute() {
    // Implementation will be added later
}

void IfStatement::execute() {
    // Implementation will be added later
}

void WhileStatement::execute() {
    // Implementation will be added later
}

void ForStatement::execute() {
    // Implementation will be added later
}

void ReturnStatement::execute() {
    // Implementation will be added later
}

void BreakStatement::execute() {
    // Implementation will be added later
}

void ContinueStatement::execute() {
    // Implementation will be added later
}

void ThrowStatement::execute() {
    // Implementation will be added later
}

void TryCatchStatement::execute() {
    // Implementation will be added later
}

void AsmStatement::execute() {
    // Implementation will be added later
}

// Initialize the global memory arena
MemoryArena astArena;

} // namespace flux
