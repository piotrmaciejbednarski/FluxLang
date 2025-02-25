#include "include/interpreter.h"
#include "include/builtins.h"
#include <iostream>
#include <sstream>

namespace flux {

Interpreter::Interpreter() {
    environment = std::make_shared<Environment>();
    typeChecker = std::make_shared<TypeChecker>();
}

Interpreter::~Interpreter() {}

void Interpreter::initialize() {
    // Register built-in functions and types
    builtins::registerBuiltins(environment);
    typeChecker->initialize();
}

void Interpreter::reset() {
    environment = std::make_shared<Environment>();
    typeChecker = std::make_shared<TypeChecker>();
    initialize();
    while (!contextStack.empty()) {
        contextStack.pop();
    }
}

std::shared_ptr<RuntimeValue> Interpreter::interpret(const std::shared_ptr<Program>& program) {
    if (!program) {
        throw RuntimeError("Cannot interpret null program");
    }

    // Reset context stack
    while (!contextStack.empty()) {
        contextStack.pop();
    }

    // Perform type checking first
    if (!typeChecker->checkProgram(program)) {
        throw RuntimeError("Type checking failed");
    }

    // Evaluate the program
    return evaluateProgram(program);
}

std::shared_ptr<RuntimeValue> Interpreter::interpretExpression(const std::shared_ptr<Expression>& expr) {
    if (!expr) {
        throw RuntimeError("Cannot interpret null expression");
    }

    return evaluateExpression(expr);
}

std::shared_ptr<RuntimeValue> Interpreter::evaluate(std::string_view source) {
    // This would require parsing the source into an AST
    // For now, a placeholder
    throw RuntimeError("Full source evaluation not yet implemented");
}

void Interpreter::registerNativeFunction(
    const std::string& name,
    NativeFunction::NativeFn function) {
    environment->define(name, RuntimeValue::makeNativeFunction(
        std::make_unique<NativeFunction>(name, function)
    ));
}

void Interpreter::pushContext() {
    contextStack.push(Context());
}

void Interpreter::popContext() {
    if (!contextStack.empty()) {
        contextStack.pop();
    }
}

Interpreter::Context& Interpreter::currentContext() {
    if (contextStack.empty()) {
        pushContext();
    }
    return contextStack.top();
}

void Interpreter::reportRuntimeError(const std::string& message, const AstLocation& location) {
    // Use Flux's error reporting system
    errorReporter.reportError(
        ErrorType::RUNTIME_ERROR, 
        message, 
        location.toSourceLocation()
    );
}

std::shared_ptr<RuntimeValue> Interpreter::evaluatePrintStatement(const std::shared_ptr<PrintStatement>& printStmt) {
    // Get the expression
    auto expr = printStmt->getExpression();
    
    if (auto literalExpr = std::dynamic_pointer_cast<LiteralExpression>(expr)) {
        const auto& value = literalExpr->getValue();
        
        if (std::holds_alternative<std::string>(value)) {
            std::string str = std::get<std::string>(value);
            
            // Remove quotes if present
            if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
                str = str.substr(1, str.length() - 2);
            }

            // Also try stderr which is usually not captured
            std::cerr << str << std::endl;
            
            // And try regular cout (which might be captured)
            std::cout << str << std::endl;
        }
    }
    
    // Fall back to normal evaluation if not a string literal
    auto value = evaluateExpression(expr);
    
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateProgram(const std::shared_ptr<Program>& program) {
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    
    //std::cout << "Debug: Program has " << program->getDeclarations().size() << " declarations" << std::endl;
    
        // DEBUG
        for (const auto& declaration : program->getDeclarations()) {
        //std::cout << "Debug: Processing declaration type: ";
        if (std::dynamic_pointer_cast<Statement>(declaration)) {
            //std::cout << "Statement";
            if (std::dynamic_pointer_cast<PrintStatement>(declaration)) {
                //std::cout << " (PrintStatement)";
            }
        } else {
            //std::cout << "Other";
        }
        std::cout << std::endl;
        }
    
    for (const auto& declaration : program->getDeclarations()) {
        if (auto nsDecl = std::dynamic_pointer_cast<NamespaceDeclaration>(declaration)) {
            lastValue = evaluateNamespace(nsDecl);
        }
        else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(declaration)) {
            lastValue = evaluateClass(classDecl);
        }
        else if (auto structDecl = std::dynamic_pointer_cast<StructDeclaration>(declaration)) {
            lastValue = evaluateStruct(structDecl);
        }
        else if (auto objectDecl = std::dynamic_pointer_cast<ObjectDeclaration>(declaration)) {
            lastValue = evaluateObject(objectDecl);
        }
        else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(declaration)) {
            lastValue = evaluateFunction(funcDecl);
        }
        else if (auto importDecl = std::dynamic_pointer_cast<ImportDeclaration>(declaration)) {
            lastValue = evaluateImport(importDecl);
        }
        else if (auto typedefDecl = std::dynamic_pointer_cast<TypedefDeclaration>(declaration)) {
            lastValue = evaluateTypedef(typedefDecl);
        }
        else if (auto unionDecl = std::dynamic_pointer_cast<UnionDeclaration>(declaration)) {
            lastValue = evaluateUnion(unionDecl);
        }
        else if (auto stmt = std::dynamic_pointer_cast<Statement>(declaration)) {
            //std::cout << "Debug: Evaluating statement" << std::endl;
            lastValue = evaluateStatement(stmt);
        }
    }
    
    return lastValue;
}

// Placeholder methods for other evaluation functions
std::shared_ptr<RuntimeValue> Interpreter::evaluateNamespace(const std::shared_ptr<NamespaceDeclaration>& ns) {
    // Evaluate classes within the namespace
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    for (const auto& classDecl : ns->getClasses()) {
        lastValue = evaluateClass(classDecl);
    }
    return lastValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateClass(const std::shared_ptr<ClassDeclaration>& classDecl) {
    // Register class type in type checker
    typeChecker->defineType(classDecl->getName(), 
        std::make_shared<ClassType>(classDecl->getName()));
    
    // Evaluate any objects or methods within the class
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    
    // TODO: Implement full class evaluation logic
    return lastValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateStruct(const std::shared_ptr<StructDeclaration>& structDecl) {
    // Register struct type in type checker
    auto structType = std::make_shared<StructType>(structDecl->getName());
    for (const auto& field : structDecl->getFields()) {
        structType->addField(field);
    }
    
    typeChecker->defineType(structDecl->getName(), structType);
    
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateObject(const std::shared_ptr<ObjectDeclaration>& objectDecl) {
    // Register object type in type checker
    auto objectType = std::make_shared<ObjectType>(objectDecl->getName());
    for (const auto& field : objectDecl->getFields()) {
        objectType->addField(field);
    }
    
    typeChecker->defineType(objectDecl->getName(), objectType);
    
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateFunction(const std::shared_ptr<FunctionDeclaration>& funcDecl) {
    // Create a function runtime value and register in the environment
    auto function = std::make_unique<Function>(funcDecl, environment);
    auto functionValue = RuntimeValue::makeFunction(std::move(function));
    
    environment->define(funcDecl->getName(), functionValue);
    
    return functionValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateImport(const std::shared_ptr<ImportDeclaration>& importDecl) {
    // Placeholder for import logic
    // In a real implementation, this would load and parse the imported file
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateTypedef(const std::shared_ptr<TypedefDeclaration>& typedefDecl) {
    // Register the typedef in the type checker
    typeChecker->defineType(typedefDecl->getName(), typedefDecl->getType());
    
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateUnion(const std::shared_ptr<UnionDeclaration>& unionDecl) {
    // Register union type in type checker
    auto unionType = std::make_shared<UnionType>(unionDecl->getName());
    for (const auto& variant : unionDecl->getVariants()) {
        unionType->addVariant(variant);
    }
    
    typeChecker->defineType(unionDecl->getName(), unionType);
    
    return RuntimeValue::makeNil();
}

// Statement evaluation methods
std::shared_ptr<RuntimeValue> Interpreter::evaluateStatement(const std::shared_ptr<Statement>& stmt) {
    if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        return evaluateExpressionStatement(exprStmt);
    }
    else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        return evaluateBlockStatement(blockStmt);
    }
    else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        return evaluateVariableDeclaration(varDecl);
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        return evaluateIfStatement(ifStmt);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        return evaluateWhileStatement(whileStmt);
    }
    else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        return evaluateForStatement(forStmt);
    }
    else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        return evaluateReturnStatement(returnStmt);
    }
    else if (auto breakStmt = std::dynamic_pointer_cast<BreakStatement>(stmt)) {
        return evaluateBreakStatement(breakStmt);
    }
    else if (auto continueStmt = std::dynamic_pointer_cast<ContinueStatement>(stmt)) {
        return evaluateContinueStatement(continueStmt);
    }
    else if (auto throwStmt = std::dynamic_pointer_cast<ThrowStatement>(stmt)) {
        return evaluateThrowStatement(throwStmt);
    }
    else if (auto tryCatchStmt = std::dynamic_pointer_cast<TryCatchStatement>(stmt)) {
        return evaluateTryCatchStatement(tryCatchStmt);
    }
    else if (auto asmStmt = std::dynamic_pointer_cast<AsmStatement>(stmt)) {
        return evaluateAsmStatement(asmStmt);
    }
    else if (auto printStmt = std::dynamic_pointer_cast<PrintStatement>(stmt)) {
        return evaluatePrintStatement(printStmt);
    }
    
    // Default case
    throw RuntimeError("Unknown statement type");
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateExpressionStatement(const std::shared_ptr<ExpressionStatement>& exprStmt) {
    return evaluateExpression(exprStmt->getExpression());
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateBlockStatement(const std::shared_ptr<BlockStatement>& blockStmt) {
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    
    // Create a new scope for the block
    pushContext();
    
    for (const auto& stmt : blockStmt->getStatements()) {
        lastValue = evaluateStatement(stmt);
        
        // Handle return, break, continue values
        if (lastValue->isReturnValue() || 
            lastValue->isBreak() || 
            lastValue->isContinue() ||
            lastValue->isException()) {
            break;
        }
    }
    
    popContext();
    
    return lastValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateVariableDeclaration(const std::shared_ptr<VariableDeclaration>& varDecl) {
    std::shared_ptr<RuntimeValue> initialValue;
    
    if (varDecl->getInitializer()) {
        initialValue = evaluateExpression(varDecl->getInitializer());
    } else {
        // Create a default value based on the type
        initialValue = builtins::createDefaultValue(varDecl->getType());
    }
    
    // Define in the current environment
    environment->define(varDecl->getName(), initialValue);
    
    return initialValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateIfStatement(const std::shared_ptr<IfStatement>& ifStmt) {
    auto condition = evaluateExpression(ifStmt->getCondition());
    
    if (condition->isTruthy()) {
        return evaluateStatement(ifStmt->getThenBranch());
    }
    else if (ifStmt->getElseBranch()) {
        return evaluateStatement(ifStmt->getElseBranch());
    }
    
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateWhileStatement(const std::shared_ptr<WhileStatement>& whileStmt) {
    currentContext().inLoop = true;
    
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    
    while (true) {
        auto condition = evaluateExpression(whileStmt->getCondition());
        
        if (!condition->isTruthy()) {
            break;
        }
        
        lastValue = evaluateStatement(whileStmt->getBody());
        
        if (lastValue->isBreak()) {
            break;
        }
        
        if (lastValue->isContinue()) {
            continue;
        }
        
        if (lastValue->isReturnValue() || lastValue->isException()) {
            break;
        }
    }
    
    currentContext().inLoop = false;
    return lastValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateForStatement(const std::shared_ptr<ForStatement>& forStmt) {
    currentContext().inLoop = true;
    
    std::shared_ptr<RuntimeValue> lastValue = RuntimeValue::makeNil();
    
    // Initializer (optional)
    if (forStmt->getInitializer()) {
        evaluateStatement(forStmt->getInitializer());
    }
    
    while (true) {
        // Condition (optional)
        if (forStmt->getCondition()) {
            auto condition = evaluateExpression(forStmt->getCondition());
            if (!condition->isTruthy()) {
                break;
            }
        }
        
        // Execute body
        lastValue = evaluateStatement(forStmt->getBody());
        
        if (lastValue->isBreak()) {
            break;
        }
        
        if (lastValue->isContinue()) {
            // Continue to increment, skipping rest of body
        }
        
        if (lastValue->isReturnValue() || lastValue->isException()) {
            break;
        }
        
        // Increment (optional)
        if (forStmt->getIncrement()) {
            evaluateExpression(forStmt->getIncrement());
        }
    }
    
    currentContext().inLoop = false;
    return lastValue;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateReturnStatement(const std::shared_ptr<ReturnStatement>& returnStmt) {
    if (returnStmt->getValue()) {
        auto returnValue = evaluateExpression(returnStmt->getValue());
        return RuntimeValue::makeReturn(returnValue);
    }
    
    return RuntimeValue::makeReturn(RuntimeValue::makeNil());
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateBreakStatement(const std::shared_ptr<BreakStatement>& breakStmt) {
    if (!currentContext().inLoop) {
        throw RuntimeError("Break statement outside of loop");
    }
    
    return RuntimeValue::makeBreak();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateContinueStatement(const std::shared_ptr<ContinueStatement>& continueStmt) {
    if (!currentContext().inLoop) {
        throw RuntimeError("Continue statement outside of loop");
    }
    
    return RuntimeValue::makeContinue();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateThrowStatement(const std::shared_ptr<ThrowStatement>& throwStmt) {
    auto exception = evaluateExpression(throwStmt->getException());
    
    // If a handler is provided, execute it
    if (throwStmt->getHandler()) {
        currentContext().inTryCatch = true;
        auto handlerResult = evaluateStatement(throwStmt->getHandler());
        currentContext().inTryCatch = false;
        
        // TODO: Implement detailed exception handling logic
        return RuntimeValue::makeException(exception);
    }
    
    return RuntimeValue::makeException(exception);
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateTryCatchStatement(const std::shared_ptr<TryCatchStatement>& tryCatchStmt) {
    currentContext().inTryCatch = true;
    
    try {
        // Execute try block
        auto tryResult = evaluateStatement(tryCatchStmt->getTryBlock());
        
        // If an exception occurred, execute catch block
        if (tryResult->isException()) {
            // Create a new scope for the catch block
            auto catchEnv = environment->createChild();
            
            // Define the exception variable in the catch block's scope
            catchEnv->define(
                tryCatchStmt->getExceptionVar(), 
                tryResult->asException()
            );
            
            // Temporarily switch environment
            auto prevEnv = environment;
            environment = catchEnv;
            
            auto catchResult = evaluateStatement(tryCatchStmt->getCatchBlock());
            
            // Restore previous environment
            environment = prevEnv;
            
            return catchResult;
        }
        
        return tryResult;
    }
    catch (...) {
        // Fallback error handling
        return RuntimeValue::makeNil();
    }
    
    currentContext().inTryCatch = false;
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateAsmStatement(const std::shared_ptr<AsmStatement>& asmStmt) {
    // In a real implementation, this would interface with the system's assembly interpreter
    // For now, just a placeholder
    std::cout << "ASM: " << asmStmt->getAsmCode() << std::endl;
    return RuntimeValue::makeNil();
}

// Expression evaluation methods
std::shared_ptr<RuntimeValue> Interpreter::evaluateExpression(const std::shared_ptr<Expression>& expr) {
    if (auto literal = std::dynamic_pointer_cast<LiteralExpression>(expr)) {
        return evaluateLiteral(literal);
    }
    else if (auto variable = std::dynamic_pointer_cast<VariableExpression>(expr)) {
        return evaluateVariable(variable);
    }
    else if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        return evaluateBinary(binary);
    }
    else if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
        return evaluateUnary(unary);
    }
    else if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        return evaluateCall(call);
    }
    else if (auto index = std::dynamic_pointer_cast<IndexExpression>(expr)) {
        return evaluateIndex(index);
    }
    else if (auto member = std::dynamic_pointer_cast<MemberExpression>(expr)) {
        return evaluateMember(member);
    }
    else if (auto arrow = std::dynamic_pointer_cast<ArrowExpression>(expr)) {
        return evaluateArrow(arrow);
    }
    else if (auto arrayLiteral = std::dynamic_pointer_cast<ArrayLiteralExpression>(expr)) {
        return evaluateArrayLiteral(arrayLiteral);
    }
    
    throw RuntimeError("Unknown expression type");
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateLiteral(const std::shared_ptr<LiteralExpression>& literal) {
    //std::cout << "Debug: Evaluating literal expression" << std::endl;
    
    // Convert the literal's value to a runtime value
    const auto& value = literal->getValue();
    
    if (std::holds_alternative<bool>(value)) {
        return RuntimeValue::makeBool(std::get<bool>(value));
    }
    else if (std::holds_alternative<int>(value)) {
        return RuntimeValue::makeInt(std::get<int>(value));
    }
    else if (std::holds_alternative<double>(value)) {
        return RuntimeValue::makeFloat(std::get<double>(value));
    }
    else if (std::holds_alternative<std::string>(value)) {
        std::string str = std::get<std::string>(value);
        //std::cout << "Debug: String literal value is: \"" << str << "\"" << std::endl;
        
        // Remove the enclosing quotes if present
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            str = str.substr(1, str.size() - 2);
            //std::cout << "Debug: After removing quotes: \"" << str << "\"" << std::endl;
        }
        std::cout << str;
        
        return RuntimeValue::makeString(str);
    }
    
    // Default case
    //std::cout << "Debug: Unknown literal type or no variant match" << std::endl;
    return RuntimeValue::makeNil();
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateVariable(const std::shared_ptr<VariableExpression>& variable) {
    return environment->get(variable->getName());
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateBinary(const std::shared_ptr<BinaryExpression>& binary) {
    auto left = evaluateExpression(binary->getLeft());
    auto right = evaluateExpression(binary->getRight());
    
    switch (binary->getOperator()) {
        // Arithmetic operations
        case BinaryOp::ADD:
        case BinaryOp::SUB:
        case BinaryOp::MUL:
        case BinaryOp::DIV:
        case BinaryOp::MOD:
        case BinaryOp::BITAND:
        case BinaryOp::BITOR:
        case BinaryOp::BITXOR:
        case BinaryOp::SHIFTLEFT:
        case BinaryOp::SHIFTRIGHT:
        case BinaryOp::EXPONENT:
            return builtins::performArithmetic(left, right, binary->getOperator());
        
        // Comparison operations
        case BinaryOp::LT:
        case BinaryOp::LE:
        case BinaryOp::GT:
        case BinaryOp::GE:
        case BinaryOp::EQ:
        case BinaryOp::NE:
            return builtins::performComparison(left, right, binary->getOperator());
        
        // Logical operations
        case BinaryOp::LOGICAL_AND:
        case BinaryOp::LOGICAL_OR:
            return builtins::performLogical(left, right, binary->getOperator());
        
        default:
            throw RuntimeError("Unsupported binary operation");
    }
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateUnary(const std::shared_ptr<UnaryExpression>& unary) {
    auto operand = evaluateExpression(unary->getOperand());
    
    switch (unary->getOperator()) {
        case UnaryOp::NEGATE:
            // Negate numeric values
            if (operand->isInt()) {
                return RuntimeValue::makeInt(-operand->asInt());
            }
            if (operand->isFloat()) {
                return RuntimeValue::makeFloat(-operand->asFloat());
            }
            throw RuntimeError("Cannot negate non-numeric value");
        
        case UnaryOp::NOT:
            // Logical NOT
            return RuntimeValue::makeBool(!operand->isTruthy());
        
        case UnaryOp::BITNOT:
            // Bitwise NOT for integers
            if (operand->isInt()) {
                return RuntimeValue::makeInt(~operand->asInt());
            }
            throw RuntimeError("Bitwise NOT only works on integers");
        
        case UnaryOp::DEREFERENCE:
            // Pointer dereference
            if (operand->isPointer()) {
                return operand->asPointer() ? operand->asPointer() : RuntimeValue::makeNil();
            }
            throw RuntimeError("Cannot dereference non-pointer value");
        
        case UnaryOp::ADDRESS_OF:
            // Get address of a value (create a pointer)
            return RuntimeValue::makePointer(operand);
        
        case UnaryOp::INCREMENT:
        case UnaryOp::DECREMENT:
            // TODO: Implement in-place increment/decrement
            throw RuntimeError("Increment/decrement not yet implemented");
        
        default:
            throw RuntimeError("Unsupported unary operation");
    }
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateCall(const std::shared_ptr<CallExpression>& call) {
    // Evaluate the callee and arguments
    auto callee = evaluateExpression(call->getCallee());
    
    std::vector<std::shared_ptr<RuntimeValue>> arguments;
    for (const auto& arg : call->getArguments()) {
        arguments.push_back(evaluateExpression(arg));
    }
    
    return callFunction(callee, arguments);
}

std::shared_ptr<RuntimeValue> Interpreter::callFunction(
    const std::shared_ptr<RuntimeValue>& callee,
    const std::vector<std::shared_ptr<RuntimeValue>>& arguments) {
    
    if (callee->isNativeFunction()) {
        // Call native function directly
        return callee->asNativeFunction()->call(arguments, environment);
    }
    else if (callee->isFunction()) {
        // Get the function object
        Function* func = callee->asFunction();
        
        // Create a new environment for function call
        auto functionEnv = std::make_shared<Environment>(func->getClosure());
        
        // Bind parameters
        const auto& parameters = func->getParameters();
        
        if (parameters.size() != arguments.size()) {
            throw RuntimeError("Incorrect number of arguments");
        }
        
        for (size_t i = 0; i < parameters.size(); ++i) {
            functionEnv->define(parameters[i].name, arguments[i]);
        }
        
        // Temporarily switch environment
        auto prevEnv = environment;
        environment = functionEnv;
        
        // Execute function body
        std::shared_ptr<RuntimeValue> result = RuntimeValue::makeNil();
        
        try {
            result = evaluateStatement(func->getBody());
            
            // If result is a return value, extract the actual value
            if (result->isReturnValue()) {
                result = result->asReturnValue();
            }
        }
        catch (...) {
            // Restore previous environment in case of exception
            environment = prevEnv;
            throw;
        }
        
        // Restore previous environment
        environment = prevEnv;
        
        return result;
    }
    
    throw RuntimeError("Cannot call non-function value");
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateIndex(const std::shared_ptr<IndexExpression>& index) {
    auto array = evaluateExpression(index->getArray());
    auto indexValue = evaluateExpression(index->getIndex());
    
    if (!array->isArray()) {
        throw RuntimeError("Cannot index non-array value");
    }
    
    if (!indexValue->isInt()) {
        throw RuntimeError("Array index must be an integer");
    }
    
    int32_t idx = indexValue->asInt();
    const auto& elements = array->asArray();
    
    if (idx < 0 || idx >= static_cast<int32_t>(elements.size())) {
        throw RuntimeError("Array index out of bounds");
    }
    
    return elements[idx];
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateMember(const std::shared_ptr<MemberExpression>& member) {
    auto object = evaluateExpression(member->getObject());
    
    if (object->isStruct() || object->isObject()) {
        const auto& fields = object->isStruct() ? object->asStruct() : object->asObject();
        
        auto it = fields.find(member->getMemberName());
        if (it != fields.end()) {
            return it->second;
        }
        
        throw RuntimeError("Member '" + member->getMemberName() + "' not found");
    }
    
    throw RuntimeError("Cannot access member of non-struct/non-object value");
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateArrow(const std::shared_ptr<ArrowExpression>& arrow) {
    auto pointer = evaluateExpression(arrow->getPointer());
    
    if (!pointer->isPointer()) {
        throw RuntimeError("Arrow operator requires a pointer");
    }
    
    auto pointedObject = pointer->asPointer();
    
    if (pointedObject->isStruct() || pointedObject->isObject()) {
        const auto& fields = pointedObject->isStruct() ? pointedObject->asStruct() : pointedObject->asObject();
        
        auto it = fields.find(arrow->getMemberName());
        if (it != fields.end()) {
            return it->second;
        }
        
        throw RuntimeError("Member '" + arrow->getMemberName() + "' not found");
    }
    
    throw RuntimeError("Cannot access member through pointer to non-struct/non-object");
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateArrayLiteral(const std::shared_ptr<ArrayLiteralExpression>& arrayLiteral) {
    std::vector<std::shared_ptr<RuntimeValue>> elements;
    
    for (const auto& element : arrayLiteral->getElements()) {
        elements.push_back(evaluateExpression(element));
    }
    
    return RuntimeValue::makeArray(elements);
}

std::shared_ptr<RuntimeValue> Interpreter::evaluateStringInterpolation(const std::shared_ptr<Expression>& interpolationExpr) {
    if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(interpolationExpr)) {
        if (binaryExpr->getOperator() != BinaryOp::EQ) {
            throw RuntimeError("Invalid string interpolation syntax");
        }
        
        auto formatValue = evaluateExpression(binaryExpr->getLeft());
        if (!formatValue->isString()) {
            throw RuntimeError("First part of interpolation must be a string");
        }
        std::string formatString = formatValue->asString();
        
        std::vector<std::shared_ptr<RuntimeValue>> values;
        
        if (auto arrayLiteral = std::dynamic_pointer_cast<ArrayLiteralExpression>(binaryExpr->getRight())) {
            for (const auto& expr : arrayLiteral->getElements()) {
                values.push_back(evaluateExpression(expr));
            }
        }
        else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(binaryExpr->getRight())) {
            std::vector<std::shared_ptr<RuntimeValue>> blockValues;
            
            for (const auto& stmt : blockStmt->getStatements()) {
                if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
                    blockValues.push_back(evaluateExpression(exprStmt->getExpression()));
                }
            }
            
            values = std::move(blockValues);
        }
        else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(binaryExpr->getRight())) {
            values.push_back(evaluateExpression(exprStmt->getExpression()));
        }
        else {
            throw RuntimeError("Invalid interpolation value specification");
        }
        
        std::string interpolatedString = builtins::interpolateString(formatString, values);
        return RuntimeValue::makeString(interpolatedString);
    }
    
    throw RuntimeError("Invalid string interpolation expression");
}

std::shared_ptr<RuntimeValue> Interpreter::typeToRuntimeValue(const std::shared_ptr<Type>& type) {
    // Convert AST type to runtime value representation
    return builtins::createDefaultValue(type);
}

std::shared_ptr<RuntimeValue> Interpreter::allocateMemory(size_t size) {
    // Create an array of NIL values as memory
    std::vector<std::shared_ptr<RuntimeValue>> memory(size, RuntimeValue::makeNil());
    return RuntimeValue::makeArray(memory);
}

void Interpreter::deallocateMemory(const std::shared_ptr<RuntimeValue>& pointer) {
    // In this simple implementation, memory is automatically managed
    // TODO: Implement garbage collection here
    return;
}

} // namespace flux
