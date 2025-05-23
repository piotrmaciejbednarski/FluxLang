#include "type_checker.h"
#include "../output/writer.h"
#include <algorithm>
#include <sstream>

namespace flux {
namespace type {

// TypeChecker implementation
TypeChecker::TypeChecker(common::Arena& arena) 
    : arena_(arena), currentScope_(nullptr) {
    initializeBuiltinTypes();
    pushScope(); // Global scope
}

void TypeChecker::initializeBuiltinTypes() {
    // Create built-in types
    voidType_ = createBuiltinType(TypeKind::VOID, "void");
    bangVoidType_ = createBuiltinType(TypeKind::BANG_VOID, "!void");
    
    // Create standard data types (as defined in language spec)
    intType_ = createDataType(32, true);   // signed data{32} int
    floatType_ = createDataType(64, true); // signed data{64} float (but treated as float)
    stringType_ = createArrayType(createDataType(8, true)); // signed data{8}[] string
    boolType_ = createDataType(1, true);   // signed data{1} bool
    
    // Register built-in types
    namedTypes_["void"] = voidType_;
    namedTypes_["!void"] = bangVoidType_;
    namedTypes_["int"] = intType_;
    namedTypes_["float"] = floatType_;
    namedTypes_["string"] = stringType_;
    namedTypes_["bool"] = boolType_;
}

bool TypeChecker::checkProgram(const parser::Program& program) {
    // First pass: collect all declarations
    for (const auto& decl : program.declarations) {
        decl->accept(*this);
    }
    
    // Check for main function
    Symbol* mainSymbol = lookupSymbol("main");
    if (!mainSymbol || !mainSymbol->isFunction) {
        error(common::ErrorCode::UNDEFINED_IDENTIFIER, 
              "Program must have a main() function", 
              common::SourceRange());
    }
    
    return !hasErrors();
}

// Scope management
void TypeChecker::pushScope() {
    auto scope = std::make_unique<Scope>(currentScope_);
    currentScope_ = scope.get();
    scopes_.push_back(std::move(scope));
}

void TypeChecker::popScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
        currentScope_ = scopes_.empty() ? nullptr : scopes_.back().get();
    }
}

// Symbol management
void TypeChecker::declareSymbol(const Symbol& symbol) {
    if (currentScope_->hasLocal(symbol.name)) {
        error(common::ErrorCode::UNDEFINED_IDENTIFIER,
              "Symbol '" + std::string(symbol.name) + "' is already declared in this scope",
              symbol.declarationLocation);
        return;
    }
    
    currentScope_->addSymbol(symbol);
}

Symbol* TypeChecker::lookupSymbol(std::string_view name) {
    return currentScope_ ? currentScope_->find(name) : nullptr;
}

// Type creation helpers
std::shared_ptr<Type> TypeChecker::createBuiltinType(TypeKind kind, std::string_view name) {
    auto type = std::make_shared<BuiltinType>(kind, name);
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createDataType(int64_t bits, bool isSigned, bool isVolatile) {
    auto type = std::make_shared<DataType>(bits, isSigned, isVolatile);
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createArrayType(std::shared_ptr<Type> elementType, std::optional<size_t> size) {
    auto type = std::make_shared<ArrayType>(std::move(elementType), size);
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createPointerType(std::shared_ptr<Type> pointeeType, bool isConst, bool isVolatile) {
    auto type = std::make_shared<PointerType>(std::move(pointeeType), isConst, isVolatile);
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createFunctionType(std::vector<std::shared_ptr<Type>> paramTypes, std::shared_ptr<Type> returnType) {
    auto type = std::make_shared<FunctionType>(std::move(paramTypes), std::move(returnType));
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createObjectType(std::string_view name, const parser::ObjectDecl& decl) {
    auto type = std::make_shared<ObjectType>(name);
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createStructType(std::string_view name, const parser::StructDecl& decl) {
    auto type = std::make_shared<StructType>(name, decl.isPacked, decl.alignment.value_or(0));
    
    // Add fields
    for (const auto& field : decl.fields) {
        auto fieldType = field.type->accept(*this);
        if (fieldType) {
            type->addField(StructType::Field(field.name, fieldType, 
                                           field.alignment.value_or(0), field.isVolatile));
        }
    }
    
    managedTypes_.push_back(type);
    return type;
}

std::shared_ptr<Type> TypeChecker::createEnumType(std::string_view name, const parser::EnumDecl& decl) {
    auto type = std::make_shared<EnumType>(name);
    
    // Add enumerators
    int64_t currentValue = 0;
    for (const auto& member : decl.members) {
        if (member.value) {
            // TODO: Evaluate constant expression for explicit value
            // For now, just use sequential values
        }
        type->addEnumerator(EnumType::Enumerator(member.name, currentValue++));
    }
    
    managedTypes_.push_back(type);
    return type;
}

// Type compatibility checking
bool TypeChecker::isAssignable(const Type& target, const Type& source) {
    return target.isAssignableFrom(source);
}

bool TypeChecker::isCompatible(const Type& left, const Type& right) {
    return left.equals(right) || left.isAssignableFrom(right) || right.isAssignableFrom(left);
}

bool TypeChecker::isImplicitlyConvertible(const Type& from, const Type& to) {
    return from.isImplicitlyConvertibleTo(to);
}

std::shared_ptr<Type> TypeChecker::getCommonType(const Type& left, const Type& right) {
    if (left.equals(right)) {
        return left.clone();
    }
    
    // Handle numeric promotions
    if (TypeUtils::isNumeric(left) && TypeUtils::isNumeric(right)) {
        // Float takes precedence over integer
        if (left.kind() == TypeKind::FLOAT || right.kind() == TypeKind::FLOAT) {
            return floatType_;
        }
        
        // Larger integer types take precedence
        if (left.kind() == TypeKind::DATA && right.kind() == TypeKind::DATA) {
            auto& leftData = static_cast<const DataType&>(left);
            auto& rightData = static_cast<const DataType&>(right);
            
            if (leftData.bits() >= rightData.bits()) {
                return left.clone();
            } else {
                return right.clone();
            }
        }
        
        return intType_;
    }
    
    // For non-numeric types, no common type
    return std::make_shared<ErrorType>();
}

// Expression visitors
std::shared_ptr<Type> TypeChecker::visitLiteralExpr(const parser::LiteralExpr& expr) {
    if (auto* intVal = std::get_if<int64_t>(&expr.value)) {
        return intType_;
    } else if (auto* floatVal = std::get_if<double>(&expr.value)) {
        return floatType_;
    } else if (auto* stringVal = std::get_if<std::string>(&expr.value)) {
        return stringType_;
    } else if (auto* boolVal = std::get_if<bool>(&expr.value)) {
        return boolType_;
    }
    
    return std::make_shared<ErrorType>();
}

std::shared_ptr<Type> TypeChecker::visitVariableExpr(const parser::VariableExpr& expr) {
    Symbol* symbol = lookupSymbol(expr.name);
    if (!symbol) {
        undefinedSymbolError(expr.name, expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return symbol->type;
}

std::shared_ptr<Type> TypeChecker::visitUnaryExpr(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    switch (expr.op.type()) {
        case lexer::TokenType::PLUS:
        case lexer::TokenType::MINUS:
            return checkUnaryArithmetic(expr);
            
        case lexer::TokenType::EXCLAMATION:
        case lexer::TokenType::KEYWORD_NOT:
            return checkUnaryLogical(expr);
            
        case lexer::TokenType::TILDE:
            return checkUnaryBitwise(expr);
            
        case lexer::TokenType::ASTERISK:
        case lexer::TokenType::AT_REF:
            return checkUnaryPointer(expr);
            
        case lexer::TokenType::PLUS_PLUS:
        case lexer::TokenType::MINUS_MINUS:
            return checkUnaryIncDec(expr);
            
        default:
            typeError("Invalid unary operator", expr.range);
            return std::make_shared<ErrorType>();
    }
}

std::shared_ptr<Type> TypeChecker::visitBinaryExpr(const parser::BinaryExpr& expr) {
    switch (expr.op.type()) {
        case lexer::TokenType::PLUS:
        case lexer::TokenType::MINUS:
        case lexer::TokenType::ASTERISK:
        case lexer::TokenType::SLASH:
        case lexer::TokenType::PERCENT:
        case lexer::TokenType::DOUBLE_ASTERISK:
            return checkArithmeticOperation(expr);
            
        case lexer::TokenType::EQUAL_EQUAL:
        case lexer::TokenType::NOT_EQUAL:
        case lexer::TokenType::LESS:
        case lexer::TokenType::LESS_EQUAL:
        case lexer::TokenType::GREATER:
        case lexer::TokenType::GREATER_EQUAL:
            return checkComparisonOperation(expr);
            
        case lexer::TokenType::KEYWORD_AND:
        case lexer::TokenType::AMPERSAND_AMPERSAND:
        case lexer::TokenType::KEYWORD_OR:
        case lexer::TokenType::PIPE_PIPE:
            return checkLogicalOperation(expr);
            
        case lexer::TokenType::AMPERSAND:
        case lexer::TokenType::PIPE:
        case lexer::TokenType::CARET:
        case lexer::TokenType::LESS_LESS:
        case lexer::TokenType::GREATER_GREATER:
            return checkBitwiseOperation(expr);
            
        case lexer::TokenType::EQUAL:
        case lexer::TokenType::PLUS_EQUAL:
        case lexer::TokenType::MINUS_EQUAL:
        case lexer::TokenType::ASTERISK_EQUAL:
        case lexer::TokenType::SLASH_EQUAL:
        case lexer::TokenType::PERCENT_EQUAL:
            return checkAssignmentOperation(expr);
            
        default:
            typeError("Invalid binary operator", expr.range);
            return std::make_shared<ErrorType>();
    }
}

std::shared_ptr<Type> TypeChecker::visitGroupExpr(const parser::GroupExpr& expr) {
    return expr.expression->accept(*this);
}

std::shared_ptr<Type> TypeChecker::visitCallExpr(const parser::CallExpr& expr) {
    auto calleeType = expr.callee->accept(*this);
    if (!calleeType) return std::make_shared<ErrorType>();
    
    if (calleeType->isFunction()) {
        return checkFunctionCall(expr, *calleeType);
    } else if (calleeType->isObject()) {
        return checkObjectConstruction(expr, *calleeType);
    } else {
        typeError("Expression is not callable", expr.range);
        return std::make_shared<ErrorType>();
    }
}

std::shared_ptr<Type> TypeChecker::visitGetExpr(const parser::GetExpr& expr) {
    return checkPropertyAccess(expr);
}

std::shared_ptr<Type> TypeChecker::visitSetExpr(const parser::SetExpr& expr) {
    return checkPropertyAssignment(expr);
}

std::shared_ptr<Type> TypeChecker::visitArrayExpr(const parser::ArrayExpr& expr) {
    if (expr.elements.empty()) {
        // Empty array - need context to determine element type
        return createArrayType(std::make_shared<ErrorType>(), 0);
    }
    
    // Get the type of the first element
    auto elementType = expr.elements[0]->accept(*this);
    if (!elementType) return std::make_shared<ErrorType>();
    
    // Check that all elements have compatible types
    for (size_t i = 1; i < expr.elements.size(); ++i) {
        auto elemType = expr.elements[i]->accept(*this);
        if (!elemType || !isCompatible(*elementType, *elemType)) {
            typeError("Array elements must have compatible types", expr.range);
            return std::make_shared<ErrorType>();
        }
    }
    
    return createArrayType(elementType, expr.elements.size());
}

std::shared_ptr<Type> TypeChecker::visitSubscriptExpr(const parser::SubscriptExpr& expr) {
    return checkArrayAccess(expr);
}

std::shared_ptr<Type> TypeChecker::visitTernaryExpr(const parser::TernaryExpr& expr) {
    auto conditionType = expr.condition->accept(*this);
    auto thenType = expr.thenExpr->accept(*this);
    auto elseType = expr.elseExpr->accept(*this);
    
    if (!conditionType || !thenType || !elseType) {
        return std::make_shared<ErrorType>();
    }
    
    // Condition must be boolean-compatible
    if (!isImplicitlyConvertible(*conditionType, *boolType_)) {
        typeError("Condition must be boolean", expr.condition->range);
    }
    
    // Then and else branches must have compatible types
    auto commonType = getCommonType(*thenType, *elseType);
    if (commonType->kind() == TypeKind::ERROR) {
        typeError("Ternary branches must have compatible types", expr.range);
    }
    
    return commonType;
}

std::shared_ptr<Type> TypeChecker::visitIStringExpr(const parser::IStringExpr& expr) {
    // Check that all expressions in the i-string are valid
    for (const auto& exprPart : expr.exprParts) {
        auto exprType = exprPart->accept(*this);
        if (!exprType) return std::make_shared<ErrorType>();
        
        // All expressions should be convertible to string
        if (!isImplicitlyConvertible(*exprType, *stringType_)) {
            typeError("Injectable string expressions must be convertible to string", 
                     exprPart->range);
        }
    }
    
    return stringType_;
}

std::shared_ptr<Type> TypeChecker::visitCastExpr(const parser::CastExpr& expr) {
    auto targetType = expr.targetType->accept(*this);
    auto sourceType = expr.expression->accept(*this);
    
    if (!targetType || !sourceType) {
        return std::make_shared<ErrorType>();
    }
    
    // TODO: Add more sophisticated cast checking
    // For now, allow all casts (Flux supports extensive casting)
    
    return targetType;
}

std::shared_ptr<Type> TypeChecker::visitAssignExpr(const parser::AssignExpr& expr) {
    auto targetType = expr.target->accept(*this);
    auto valueType = expr.value->accept(*this);
    
    if (!targetType || !valueType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!isAssignable(*targetType, *valueType)) {
        incompatibleTypeError(*targetType, *valueType, expr.range);
    }
    
    return targetType;
}

std::shared_ptr<Type> TypeChecker::visitSizeOfExpr(const parser::SizeOfExpr& expr) {
    auto targetType = expr.targetType->accept(*this);
    if (!targetType) return std::make_shared<ErrorType>();
    
    return intType_;
}

std::shared_ptr<Type> TypeChecker::visitTypeOfExpr(const parser::TypeOfExpr& expr) {
    auto exprType = expr.expression->accept(*this);
    if (!exprType) return std::make_shared<ErrorType>();
    
    // typeof returns a type, but we don't have a type type
    // For now, return the type itself
    return exprType;
}

std::shared_ptr<Type> TypeChecker::visitOpExpr(const parser::OpExpr& expr) {
    // Look up custom operator
    std::string opName = "operator(" + std::string(expr.operatorName) + ")";
    Symbol* opSymbol = lookupSymbol(opName);
    
    if (!opSymbol || !opSymbol->isFunction) {
        undefinedSymbolError(expr.operatorName, expr.range);
        return std::make_shared<ErrorType>();
    }
    
    // Check argument types
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    // TODO: Check operator signature compatibility
    
    return opSymbol->returnType;
}

std::shared_ptr<Type> TypeChecker::visitAddressOfExpr(const parser::AddressOfExpr& expr) {
    // address<value> expression
    auto valueType = expr.addressValue->accept(*this);
    if (!valueType) return std::make_shared<ErrorType>();
    
    // The value should be an integer (address)
    if (!TypeUtils::isIntegral(*valueType)) {
        typeError("Address value must be an integer", expr.range);
    }
    
    // Return a void pointer for now
    return createPointerType(voidType_);
}

// Binary operation type checking helpers
std::shared_ptr<Type> TypeChecker::checkArithmeticOperation(const parser::BinaryExpr& expr) {
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!TypeUtils::isArithmetic(*leftType) || !TypeUtils::isArithmetic(*rightType)) {
        typeError("Arithmetic operations require numeric types", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return getCommonType(*leftType, *rightType);
}

std::shared_ptr<Type> TypeChecker::checkComparisonOperation(const parser::BinaryExpr& expr) {
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!TypeUtils::isComparable(*leftType) || !TypeUtils::isComparable(*rightType)) {
        typeError("Comparison operations require comparable types", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    if (!isCompatible(*leftType, *rightType)) {
        typeError("Cannot compare incompatible types", expr.range);
    }
    
    return boolType_;
}

std::shared_ptr<Type> TypeChecker::checkLogicalOperation(const parser::BinaryExpr& expr) {
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    // Both operands should be boolean-convertible
    if (!isImplicitlyConvertible(*leftType, *boolType_)) {
        typeError("Left operand of logical operation must be boolean", expr.left->range);
    }
    
    if (!isImplicitlyConvertible(*rightType, *boolType_)) {
        typeError("Right operand of logical operation must be boolean", expr.right->range);
    }
    
    return boolType_;
}

std::shared_ptr<Type> TypeChecker::checkBitwiseOperation(const parser::BinaryExpr& expr) {
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!TypeUtils::supportsBitwise(*leftType) || !TypeUtils::supportsBitwise(*rightType)) {
        typeError("Bitwise operations require integral types", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return getCommonType(*leftType, *rightType);
}

std::shared_ptr<Type> TypeChecker::checkAssignmentOperation(const parser::BinaryExpr& expr) {
    auto leftType = expr.left->accept(*this);
    auto rightType = expr.right->accept(*this);
    
    if (!leftType || !rightType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!isAssignable(*leftType, *rightType)) {
        incompatibleTypeError(*leftType, *rightType, expr.range);
    }
    
    return leftType;
}

// Statement visitors (simplified implementations)
void TypeChecker::visitExprStmt(const parser::ExprStmt& stmt) {
    stmt.expression->accept(*this);
}

void TypeChecker::visitBlockStmt(const parser::BlockStmt& stmt) {
    pushScope();
    for (const auto& statement : stmt.statements) {
        statement->accept(*this);
    }
    popScope();
}

void TypeChecker::visitVarStmt(const parser::VarStmt& stmt) {
    std::shared_ptr<Type> varType;
    
    if (stmt.type) {
        varType = stmt.type->accept(*this);
    } else if (stmt.initializer) {
        varType = stmt.initializer->accept(*this);
    } else {
        error("Variable must have either explicit type or initializer", stmt.range);
        return;
    }
    
    if (!varType) return;
    
    Symbol symbol(stmt.name.lexeme(), varType, stmt.range);
    declareSymbol(symbol);
}

void TypeChecker::visitReturnStmt(const parser::ReturnStmt& stmt) {
    if (!context_.currentFunctionReturnType) {
        error("Return statement outside function", stmt.range);
        return;
    }
    
    if (stmt.value) {
        auto valueType = stmt.value->accept(*this);
        if (valueType && !isAssignable(*context_.currentFunctionReturnType, *valueType)) {
            incompatibleTypeError(*context_.currentFunctionReturnType, *valueType, stmt.range);
        }
    } else {
        // Return without value - function should return void
        if (!context_.currentFunctionReturnType->equals(*voidType_)) {
            typeError("Function must return a value", stmt.range);
        }
    }
}

// Declaration visitors (simplified implementations)
void TypeChecker::visitFunctionDecl(const parser::FunctionDecl& decl) {
    // Create parameter types
    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& param : decl.parameters) {
        if (param.type) {
            auto paramType = param.type->accept(*this);
            if (paramType) {
                paramTypes.push_back(paramType);
            }
        } else {
            error("Function parameters must have explicit types", decl.range);
            return;
        }
    }
    
    // Get return type
    std::shared_ptr<Type> returnType = voidType_;
    if (decl.returnType) {
        returnType = decl.returnType->accept(*this);
        if (!returnType) return;
    }
    
    // Create function type
    auto funcType = createFunctionType(paramTypes, returnType);
    
    // Create symbol
    Symbol symbol(decl.name, funcType, decl.range);
    symbol.isFunction = true;
    symbol.parameterTypes = paramTypes;
    symbol.returnType = returnType;
    
    declareSymbol(symbol);
    
    // Type check function body if present
    if (decl.body && !decl.isPrototype) {
        pushScope();
        
        // Add parameters to scope
        for (size_t i = 0; i < decl.parameters.size(); ++i) {
            Symbol paramSymbol(decl.parameters[i].name, paramTypes[i], decl.range);
            declareSymbol(paramSymbol);
        }
        
        // Set function context
        context_.currentFunctionReturnType = returnType;
        
        // Check body
        decl.body->accept(*this);
        
        // Restore context
        context_.currentFunctionReturnType = nullptr;
        
        popScope();
    }
}

void TypeChecker::visitVarDecl(const parser::VarDecl& decl) {
    std::shared_ptr<Type> varType;
    
    if (decl.type) {
        varType = decl.type->accept(*this);
    } else if (decl.initializer) {
        varType = decl.initializer->accept(*this);
    } else {
        error("Variable must have either explicit type or initializer", decl.range);
        return;
    }
    
    if (!varType) return;
    
    Symbol symbol(decl.name, varType, decl.range, decl.isConst, decl.isVolatile);
    declareSymbol(symbol);
}

void TypeChecker::visitObjectDecl(const parser::ObjectDecl& decl) {
    auto objectType = createObjectType(decl.name, decl);
    
    Symbol symbol(decl.name, objectType, decl.range);
    declareSymbol(symbol);
    
    // Register the type
    namedTypes_[std::string(decl.name)] = objectType;
}

void TypeChecker::visitStructDecl(const parser::StructDecl& decl) {
    auto structType = createStructType(decl.name, decl);
    
    Symbol symbol(decl.name, structType, decl.range);
    declareSymbol(symbol);
    
    // Register the type
    namedTypes_[std::string(decl.name)] = structType;
}

void TypeChecker::visitEnumDecl(const parser::EnumDecl& decl) {
    auto enumType = createEnumType(decl.name, decl);
    
    Symbol symbol(decl.name, enumType, decl.range);
    declareSymbol(symbol);
    
    // Register the type
    namedTypes_[std::string(decl.name)] = enumType;
}

// Type expression visitors
std::shared_ptr<Type> TypeChecker::visitNamedTypeExpr(const parser::NamedTypeExpr& type) {
    auto it = namedTypes_.find(std::string(type.name));
    if (it != namedTypes_.end()) {
        return it->second;
    }
    
    // Check template parameters
    if (context_.inTemplate) {
        auto templateType = resolveTemplateType(type.name);
        if (templateType) return templateType;
    }
    
    error(common::ErrorCode::UNDEFINED_TYPE, 
          "Undefined type '" + std::string(type.name) + "'", type.range);
    return std::make_shared<ErrorType>();
}

std::shared_ptr<Type> TypeChecker::visitArrayTypeExpr(const parser::ArrayTypeExpr& type) {
    auto elementType = type.elementType->accept(*this);
    if (!elementType) return std::make_shared<ErrorType>();
    
    std::optional<size_t> size;
    if (type.sizeExpr) {
        // TODO: Evaluate constant expression for array size
        // For now, assume dynamic array
    }
    
    return createArrayType(elementType, size);
}

std::shared_ptr<Type> TypeChecker::visitPointerTypeExpr(const parser::PointerTypeExpr& type) {
    auto pointeeType = type.pointeeType->accept(*this);
    if (!pointeeType) return std::make_shared<ErrorType>();
    
    return createPointerType(pointeeType, type.isConst, type.isVolatile);
}

std::shared_ptr<Type> TypeChecker::visitFunctionTypeExpr(const parser::FunctionTypeExpr& type) {
    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& paramType : type.parameterTypes) {
        auto param = paramType->accept(*this);
        if (!param) return std::make_shared<ErrorType>();
        paramTypes.push_back(param);
    }
    
    auto returnType = type.returnType->accept(*this);
    if (!returnType) return std::make_shared<ErrorType>();
    
    return createFunctionType(paramTypes, returnType);
}

std::shared_ptr<Type> TypeChecker::visitDataTypeExpr(const parser::DataTypeExpr& type) {
    return createDataType(type.bits, type.isSigned, type.isVolatile);
}

// Error reporting helpers
void TypeChecker::error(common::ErrorCode code, std::string_view message, const common::SourceRange& location) {
    errors_.addError(code, message, makeSourceLocation(location));
}

void TypeChecker::error(std::string_view message, const common::SourceRange& location) {
    error(common::ErrorCode::TYPE_MISMATCH, message, location);
}

void TypeChecker::typeError(std::string_view message, const common::SourceRange& location) {
    error(common::ErrorCode::TYPE_MISMATCH, message, location);
}

void TypeChecker::incompatibleTypeError(const Type& expected, const Type& actual, const common::SourceRange& location) {
    std::string message = "Type mismatch: expected '" + expected.name() + 
                         "', got '" + actual.name() + "'";
    typeError(message, location);
}

void TypeChecker::undefinedSymbolError(std::string_view name, const common::SourceRange& location) {
    error(common::ErrorCode::UNDEFINED_IDENTIFIER, 
          "Undefined symbol '" + std::string(name) + "'", location);
}

output::SourceLocation TypeChecker::makeSourceLocation(const common::SourceRange& range) const {
    // For now, create a basic source location
    // In a complete implementation, this would extract line content from the source
    return output::SourceLocation("", range.start.line, range.start.column, "", 0, 0);
}

// Remaining helper method implementations

std::shared_ptr<Type> TypeChecker::checkUnaryArithmetic(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    if (!TypeUtils::isArithmetic(*operandType)) {
        typeError("Unary arithmetic operations require numeric types", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return operandType;
}

std::shared_ptr<Type> TypeChecker::checkUnaryLogical(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    if (!isImplicitlyConvertible(*operandType, *boolType_)) {
        typeError("Logical NOT requires boolean-convertible type", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return boolType_;
}

std::shared_ptr<Type> TypeChecker::checkUnaryBitwise(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    if (!TypeUtils::supportsBitwise(*operandType)) {
        typeError("Bitwise NOT requires integral type", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return operandType;
}

std::shared_ptr<Type> TypeChecker::checkUnaryPointer(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    if (expr.op.type() == lexer::TokenType::ASTERISK) {
        // Dereference operator
        if (!operandType->isPointer()) {
            typeError("Cannot dereference non-pointer type", expr.range);
            return std::make_shared<ErrorType>();
        }
        
        auto ptrType = static_cast<const PointerType*>(operandType.get());
        return ptrType->pointeeType();
    } else if (expr.op.type() == lexer::TokenType::AT_REF) {
        // Address-of operator
        return createPointerType(operandType);
    }
    
    return std::make_shared<ErrorType>();
}

std::shared_ptr<Type> TypeChecker::checkUnaryIncDec(const parser::UnaryExpr& expr) {
    auto operandType = expr.right->accept(*this);
    if (!operandType) return std::make_shared<ErrorType>();
    
    if (!TypeUtils::isArithmetic(*operandType) && !operandType->isPointer()) {
        typeError("Increment/decrement requires arithmetic or pointer type", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    return operandType;
}

std::shared_ptr<Type> TypeChecker::checkFunctionCall(const parser::CallExpr& expr, const Type& calleeType) {
    auto& funcType = static_cast<const FunctionType&>(calleeType);
    
    // Check argument count
    if (expr.arguments.size() != funcType.parameterCount()) {
        typeError("Function called with wrong number of arguments", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    // Check argument types
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        auto argType = expr.arguments[i]->accept(*this);
        if (!argType) continue;
        
        const auto& paramType = funcType.parameterTypes()[i];
        if (!isAssignable(*paramType, *argType)) {
            std::string message = "Argument " + std::to_string(i + 1) + 
                                " type mismatch: expected '" + paramType->name() + 
                                "', got '" + argType->name() + "'";
            typeError(message, expr.arguments[i]->range);
        }
    }
    
    return funcType.returnType();
}

std::shared_ptr<Type> TypeChecker::checkObjectConstruction(const parser::CallExpr& expr, const Type& objectType) {
    // Object construction: ObjectType{} obj(args);
    // For now, just return the object type
    // TODO: Check constructor arguments
    return objectType.clone();
}

std::shared_ptr<Type> TypeChecker::checkArrayAccess(const parser::SubscriptExpr& expr) {
    auto arrayType = expr.array->accept(*this);
    auto indexType = expr.index->accept(*this);
    
    if (!arrayType || !indexType) {
        return std::make_shared<ErrorType>();
    }
    
    if (!arrayType->isArray()) {
        typeError("Cannot index non-array type", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    if (!TypeUtils::isIntegral(*indexType)) {
        typeError("Array index must be integral type", expr.index->range);
        return std::make_shared<ErrorType>();
    }
    
    auto arrType = static_cast<const ArrayType*>(arrayType.get());
    return arrType->elementType();
}

std::shared_ptr<Type> TypeChecker::checkPropertyAccess(const parser::GetExpr& expr) {
    auto objectType = expr.object->accept(*this);
    if (!objectType) return std::make_shared<ErrorType>();
    
    if (objectType->isObject()) {
        auto objType = static_cast<const ObjectType*>(objectType.get());
        auto member = objType->findMember(expr.name.lexeme());
        if (!member) {
            undefinedSymbolError(expr.name.lexeme(), expr.range);
            return std::make_shared<ErrorType>();
        }
        return member->type;
    } else if (objectType->isStruct()) {
        auto structType = static_cast<const StructType*>(objectType.get());
        auto field = structType->findField(expr.name.lexeme());
        if (!field) {
            undefinedSymbolError(expr.name.lexeme(), expr.range);
            return std::make_shared<ErrorType>();
        }
        return field->type;
    } else {
        typeError("Cannot access member of non-object/struct type", expr.range);
        return std::make_shared<ErrorType>();
    }
}

std::shared_ptr<Type> TypeChecker::checkPropertyAssignment(const parser::SetExpr& expr) {
    auto objectType = expr.object->accept(*this);
    auto valueType = expr.value->accept(*this);
    
    if (!objectType || !valueType) {
        return std::make_shared<ErrorType>();
    }
    
    std::shared_ptr<Type> memberType;
    
    if (objectType->isObject()) {
        auto objType = static_cast<const ObjectType*>(objectType.get());
        auto member = objType->findMember(expr.name.lexeme());
        if (!member) {
            undefinedSymbolError(expr.name.lexeme(), expr.range);
            return std::make_shared<ErrorType>();
        }
        
        if (member->isConst) {
            typeError("Cannot assign to const member", expr.range);
        }
        
        memberType = member->type;
    } else if (objectType->isStruct()) {
        auto structType = static_cast<const StructType*>(objectType.get());
        auto field = structType->findField(expr.name.lexeme());
        if (!field) {
            undefinedSymbolError(expr.name.lexeme(), expr.range);
            return std::make_shared<ErrorType>();
        }
        
        memberType = field->type;
    } else {
        typeError("Cannot access member of non-object/struct type", expr.range);
        return std::make_shared<ErrorType>();
    }
    
    if (!isAssignable(*memberType, *valueType)) {
        incompatibleTypeError(*memberType, *valueType, expr.range);
    }
    
    return memberType;
}

// Template handling methods
void TypeChecker::enterTemplate(const std::vector<std::string_view>& templateParams) {
    context_.inTemplate = true;
    for (const auto& param : templateParams) {
        auto templateType = std::make_shared<TemplateParameterType>(param);
        context_.templateBindings[param] = templateType;
    }
}

void TypeChecker::exitTemplate() {
    context_.inTemplate = false;
    context_.templateBindings.clear();
}

std::shared_ptr<Type> TypeChecker::resolveTemplateType(std::string_view name) {
    auto it = context_.templateBindings.find(name);
    if (it != context_.templateBindings.end()) {
        return it->second;
    }
    return nullptr;
}

// Remaining statement visitors (basic implementations)
void TypeChecker::visitIfStmt(const parser::IfStmt& stmt) {
    auto conditionType = stmt.condition->accept(*this);
    if (conditionType && !isImplicitlyConvertible(*conditionType, *boolType_)) {
        typeError("If condition must be boolean", stmt.condition->range);
    }
    
    stmt.thenBranch->accept(*this);
    if (stmt.elseBranch) {
        stmt.elseBranch->accept(*this);
    }
}

void TypeChecker::visitWhileStmt(const parser::WhileStmt& stmt) {
    auto conditionType = stmt.condition->accept(*this);
    if (conditionType && !isImplicitlyConvertible(*conditionType, *boolType_)) {
        typeError("While condition must be boolean", stmt.condition->range);
    }
    
    bool wasInLoop = context_.inLoop;
    context_.inLoop = true;
    stmt.body->accept(*this);
    context_.inLoop = wasInLoop;
}

void TypeChecker::visitForStmt(const parser::ForStmt& stmt) {
    pushScope();
    
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }
    
    if (stmt.condition) {
        auto conditionType = stmt.condition->accept(*this);
        if (conditionType && !isImplicitlyConvertible(*conditionType, *boolType_)) {
            typeError("For condition must be boolean", stmt.condition->range);
        }
    }
    
    if (stmt.increment) {
        stmt.increment->accept(*this);
    }
    
    bool wasInLoop = context_.inLoop;
    context_.inLoop = true;
    stmt.body->accept(*this);
    context_.inLoop = wasInLoop;
    
    popScope();
}

void TypeChecker::visitBreakStmt(const parser::BreakStmt& stmt) {
    if (!context_.inLoop) {
        error("Break statement outside loop", stmt.range);
    }
}

void TypeChecker::visitContinueStmt(const parser::ContinueStmt& stmt) {
    if (!context_.inLoop) {
        error("Continue statement outside loop", stmt.range);
    }
}

void TypeChecker::visitTryStmt(const parser::TryStmt& stmt) {
    bool wasInTry = context_.inTryBlock;
    context_.inTryBlock = true;
    
    stmt.tryBlock->accept(*this);
    
    for (const auto& catchClause : stmt.catchClauses) {
        pushScope();
        
        if (catchClause.exceptionType) {
            auto exceptionType = catchClause.exceptionType->accept(*this);
            // TODO: Add exception variable to scope
        }
        
        catchClause.handler->accept(*this);
        popScope();
    }
    
    context_.inTryBlock = wasInTry;
}

void TypeChecker::visitSwitchStmt(const parser::SwitchStmt& stmt) {
    auto valueType = stmt.value->accept(*this);
    
    for (const auto& caseClause : stmt.cases) {
        auto patternType = caseClause.pattern->accept(*this);
        if (valueType && patternType && !isCompatible(*valueType, *patternType)) {
            typeError("Case pattern type incompatible with switch value", 
                     caseClause.pattern->range);
        }
        
        caseClause.body->accept(*this);
    }
    
    if (stmt.defaultCase) {
        stmt.defaultCase->accept(*this);
    }
}

void TypeChecker::visitThrowStmt(const parser::ThrowStmt& stmt) {
    if (stmt.message) {
        auto messageType = stmt.message->accept(*this);
        if (messageType && !isImplicitlyConvertible(*messageType, *stringType_)) {
            typeError("Throw message must be string-convertible", stmt.message->range);
        }
    }
    
    if (stmt.body) {
        stmt.body->accept(*this);
    }
}

void TypeChecker::visitDeclStmt(const parser::DeclStmt& stmt) {
    stmt.declaration->accept(*this);
}

// Remaining declaration visitors (basic implementations)
void TypeChecker::visitClassDecl(const parser::ClassDecl& decl) {
    // For now, treat classes similar to objects
    // TODO: Implement proper class semantics
    visitObjectDecl(parser::ObjectDecl(decl.name, {}, {}, decl.range));
}

void TypeChecker::visitNamespaceDecl(const parser::NamespaceDecl& decl) {
    pushScope();
    
    for (const auto& declaration : decl.declarations) {
        declaration->accept(*this);
    }
    
    popScope();
}

void TypeChecker::visitImportDecl(const parser::ImportDecl& decl) {
    // TODO: Implement import handling
    // For now, just record that we've seen the import
}

void TypeChecker::visitOperatorDecl(const parser::OperatorDecl& decl) {
    // Create parameter types
    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& param : decl.parameters) {
        auto paramType = param.type->accept(*this);
        if (paramType) {
            paramTypes.push_back(paramType);
        }
    }
    
    // Get return type
    auto returnType = decl.returnType->accept(*this);
    if (!returnType) return;
    
    // Create function type for the operator
    auto funcType = createFunctionType(paramTypes, returnType);
    
    // Create symbol with operator name
    std::string opName = "operator(" + std::string(decl.op) + ")";
    Symbol symbol(opName, funcType, decl.range);
    symbol.isFunction = true;
    symbol.parameterTypes = paramTypes;
    symbol.returnType = returnType;
    
    declareSymbol(symbol);
}

void TypeChecker::visitUsingDecl(const parser::UsingDecl& decl) {
    // TODO: Implement using declaration handling
}

void TypeChecker::visitTypeDecl(const parser::TypeDecl& decl) {
    auto underlyingType = decl.underlyingType->accept(*this);
    if (!underlyingType) return;
    
    // Create type alias
    namedTypes_[std::string(decl.name)] = underlyingType;
    
    Symbol symbol(decl.name, underlyingType, decl.range);
    declareSymbol(symbol);
}

void TypeChecker::visitDataDecl(const parser::DataDecl& decl) {
    auto dataType = createDataType(decl.bits, decl.isSigned, decl.isVolatile);
    
    // Register the type
    namedTypes_[std::string(decl.name)] = dataType;
    
    Symbol symbol(decl.name, dataType, decl.range);
    declareSymbol(symbol);
}

void TypeChecker::visitTemplateDecl(const parser::TemplateDecl& decl) {
    // Extract template parameter names
    std::vector<std::string_view> templateParams;
    for (const auto& param : decl.parameters) {
        templateParams.push_back(param.name);
    }
    
    // Enter template context
    enterTemplate(templateParams);
    
    // Type check the templated declaration
    decl.declaration->accept(*this);
    
    // Exit template context
    exitTemplate();
}

void TypeChecker::visitAsmDecl(const parser::AsmDecl& decl) {
    // Assembly blocks don't need type checking
    // Just verify we're not in an invalid context
}

void TypeChecker::visitSectionDecl(const parser::SectionDecl& decl) {
    // Type check the contained declaration
    decl.declaration->accept(*this);
    
    // TODO: Validate section attributes and address expressions
    if (decl.addressExpr) {
        auto addrType = decl.addressExpr->accept(*this);
        if (addrType && !TypeUtils::isIntegral(*addrType)) {
            typeError("Section address must be integral", decl.addressExpr->range);
        }
    }
}

} // namespace type
} // namespace flux