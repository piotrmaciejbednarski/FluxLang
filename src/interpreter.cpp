#include "include/interpreter.h"
#include "include/ast.h"
#include <sstream>
#include <iostream>

namespace flux {

// InterpreterError implementation
InterpreterError::InterpreterError(const std::string& message)
    : std::runtime_error(message) {}

// Value implementation
std::string Value::toString() const {
    if (isVoid()) return "void";
    if (isInteger()) return std::to_string(as<int64_t>());
    if (isFloat()) return std::to_string(as<double>());
    if (isBoolean()) return as<bool>() ? "true" : "false";
    if (isCharacter()) return std::string(1, as<char>());
    if (isString()) return as<std::string>();
    
    if (isArray()) {
        const auto& array = as<std::vector<Value>>();
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < array.size(); ++i) {
            oss << array[i].toString();
            if (i < array.size() - 1) oss << ", ";
        }
        oss << "]";
        return oss.str();
    }
    
    if (isObject()) {
        const auto& obj = as<std::unordered_map<std::string, Value>>();
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [key, value] : obj) {
            if (!first) oss << ", ";
            oss << key << ": " << value.toString();
            first = false;
        }
        oss << "}";
        return oss.str();
    }
    
    if (isCallable()) return "<function>";
    
    return "<unknown>";
}

// Function implementation
Function::Function(std::shared_ptr<FunctionDeclaration> declaration, std::shared_ptr<Environment> closure)
    : declaration(declaration), closure(closure) {
}

Value Function::call(Interpreter& interpreter, const std::vector<Value>& args) {    
    // Create a new environment for function execution
    auto environment = std::make_shared<Environment>(closure);
    
    // Bind parameters to arguments
    const auto& params = declaration->getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        environment->define(params[i].getName(), args[i]);
    }
    
    try {
        // Execute function body
        interpreter.executeBlock(declaration->getBody(), environment);
        return Value(); // Return void by default
    } catch (const Interpreter::ReturnValue& returnValue) {
        // Handle return statement
        return returnValue.value;
    }
}

int Function::arity() const {
    return static_cast<int>(declaration->getParams().size());
}

// BuiltinFunction implementation
BuiltinFunction::BuiltinFunction(
    std::string name, 
    int arity, 
    std::function<Value(Interpreter&, const std::vector<Value>&)> function
) : name(std::move(name)), 
    functionArity(arity), 
    function(std::move(function)) {}

Value BuiltinFunction::call(Interpreter& interpreter, const std::vector<Value>& args) {
    return function(interpreter, args);
}

int BuiltinFunction::arity() const {
    return functionArity;
}

// Environment implementation
Environment::Environment() : enclosing(nullptr) {}

Environment::Environment(std::shared_ptr<Environment> enclosingEnv) 
    : enclosing(enclosingEnv) {}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

Value Environment::get(const std::string& name) {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    
    if (enclosing) {
        return enclosing->get(name);
    }
    
    throw InterpreterError("Undefined variable '" + name + "'.");
}

void Environment::assign(const std::string& name, const Value& value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
    
    throw InterpreterError("Undefined variable '" + name + "'.");
}

void Environment::printSymbols() {
    std::cout << "Environment Symbols:" << std::endl;
    for (const auto& [name, value] : values) {
        std::cout << name << ": " << value.toString() << std::endl;
    }
    
    if (enclosing) {
        std::cout << "--- Enclosing Environment ---" << std::endl;
        enclosing->printSymbols();
    }
}

// Interpreter implementation
Interpreter::Interpreter() {
    globals = std::make_shared<Environment>();
    environment = globals;
    
    defineBuiltins();
}

void Interpreter::defineBuiltins() {
    // Define print function
    auto printFunction = std::make_shared<BuiltinFunction>(
        "print", 
        -1, // Variable arity
        [this](Interpreter&, const std::vector<Value>& args) -> Value {
            return this->print(args);
        }
    );
    globals->define("print", Value(printFunction));
    
    // Define input function
    auto inputFunction = std::make_shared<BuiltinFunction>(
        "input", 
        -1, // Variable arity
        [this](Interpreter&, const std::vector<Value>& args) -> Value {
            return this->input(args);
        }
    );
    globals->define("input", Value(inputFunction));
}

void Interpreter::printGlobalSymbols() {
    std::cout << "Global Symbols:" << std::endl;
    globals->printSymbols();
}

Value Interpreter::print(const std::vector<Value>& args) {
    if (args.empty()) {
        throw InterpreterError("print requires at least one argument");
    }

    std::string output;
    for (size_t i = 0; i < args.size(); ++i) {
        output += args[i].toString();
        if (i < args.size() - 1) output += " ";
    }
    std::cout << output << std::endl;
    
    // Return the last argument
    return args.back();
}

Value Interpreter::input(const std::vector<Value>& args) {
    // Print prompt if provided
    if (!args.empty()) {
        std::cout << args[0].toString();
    }
    
    std::string input;
    std::getline(std::cin, input);
    return Value(input);
}

bool Interpreter::isTruthy(const Value& value) {
    if (value.isVoid()) return false;
    if (value.isBoolean()) return value.as<bool>();
    if (value.isInteger()) return value.as<int64_t>() != 0;
    if (value.isFloat()) return value.as<double>() != 0.0;
    if (value.isString()) return !value.as<std::string>().empty();
    if (value.isArray()) return !value.as<std::vector<Value>>().empty();
    return true;
}

bool Interpreter::isEqual(const Value& a, const Value& b) {
    // Handle null/void equality
    if (a.isVoid() && b.isVoid()) return true;
    if (a.isVoid() || b.isVoid()) return false;
    
    // Handle type-specific equality
    if (a.isInteger() && b.isInteger()) {
        return a.as<int64_t>() == b.as<int64_t>();
    }
    
    if (a.isFloat() && b.isFloat()) {
        return a.as<double>() == b.as<double>();
    }
    
    // Handle mixed numeric types
    if (a.isNumber() && b.isNumber()) {
        double aVal = a.isFloat() ? a.as<double>() : static_cast<double>(a.as<int64_t>());
        double bVal = b.isFloat() ? b.as<double>() : static_cast<double>(b.as<int64_t>());
        return aVal == bVal;
    }
    
    if (a.isBoolean() && b.isBoolean()) {
        return a.as<bool>() == b.as<bool>();
    }
    
    if (a.isCharacter() && b.isCharacter()) {
        return a.as<char>() == b.as<char>();
    }
    
    if (a.isString() && b.isString()) {
        return a.as<std::string>() == b.as<std::string>();
    }
    
    // Different types are not equal
    return false;
}

Value Interpreter::execute(std::shared_ptr<Program> program) {
    try {
        const auto& declarations = program->getDeclarations();
        
        std::cout << "Total Declarations: " << declarations.size() << std::endl;
        
        // Extensive logging for declarations
        std::cout << "=== Detailed Declaration Logging ===" << std::endl;
        for (const auto& decl : declarations) {
            if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(decl)) {
                std::cout << "Function Declaration Found:" << std::endl;
                std::cout << "  Name: " << funcDecl->getName() << std::endl;
                std::cout << "  Return Type: " << funcDecl->getReturnType()->toString() << std::endl;
                std::cout << "  Parameters: " << funcDecl->getParams().size() << std::endl;
                std::cout << "  Has Body: " << (funcDecl->getBody() != nullptr) << std::endl;
            } else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(decl)) {
                std::cout << "Class Declaration Found:" << std::endl;
                std::cout << "  Name: " << classDecl->getName() << std::endl;
                std::cout << "  Members: " << classDecl->getMembers().size() << std::endl;
            } else {
                std::cout << "Other Declaration: " << decl->getName() << std::endl;
            }
        }
        
        // First pass: Process namespace, class and function declarations
        for (const auto& decl : declarations) {
            if (auto namespaceDecl = std::dynamic_pointer_cast<NamespaceDeclaration>(decl)) {
                // Register namespace in globals
                executeNamespaceDeclaration(namespaceDecl);
            } else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(decl)) {
                // Register class in globals
                executeClassDeclaration(classDecl);
            } else if (auto objDecl = std::dynamic_pointer_cast<ObjectDeclaration>(decl)) {
                // Register object in globals
                executeObjectDeclaration(objDecl);
            } else if (auto structDecl = std::dynamic_pointer_cast<StructDeclaration>(decl)) {
                // Register struct in globals
                executeStructDeclaration(structDecl);
            } else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(decl)) {
                // Create function object
                auto function = std::make_shared<Function>(funcDecl, environment);
                
                // Register function in global scope
                globals->define(funcDecl->getName(), Value(function));
            }
        }
        
        // Look for main function and execute it
        try {
            // Print all global symbols for comprehensive debugging
            std::cout << "\n=== Global Symbols Before Execution ===" << std::endl;
            printGlobalSymbols();
            std::cout << "=====================================" << std::endl << std::endl;
            
            // Try to get the main function
            Value mainFunc = globals->get("main");
            
            if (mainFunc.isCallable()) {                
                // Empty vector for main() arguments
                std::vector<Value> args;
                
                try {
                    Value result = mainFunc.as<std::shared_ptr<Callable>>()->call(*this, args);
                    if (result.toString() != "0") {
                        std::cout << "\nMain function returned error: " << result.toString() << std::endl;
                        return result;
                    }
                    std::cout << "\nMain function returned success: " << result.toString() << std::endl;
                    return result;
                }
                catch (const ReturnValue& returnValue) {
                    std::cout << "\nMain function executed with return value: " << returnValue.value.toString() << std::endl;
                    return returnValue.value;
                }
                catch (const std::exception& e) {
                    std::cerr << "\nException while executing main function: " << e.what() << std::endl;
                    return Value(-1);
                }
            } else {
                std::cout << "\nMain function is NOT callable" << std::endl;
                return Value(-1); // Return -1 if main is not callable
            }
        } catch (const InterpreterError& e) {
            std::cerr << "Error locating main function: " << e.what() << std::endl;
            std::cerr << "Available global symbols:" << std::endl;
            printGlobalSymbols();
            return Value(-1); // Return error code
        }
    } catch (const InterpreterError& error) {
        std::cerr << "Runtime Error: " << error.what() << std::endl;
        return Value(-1); // Return error code
    }
}

void Interpreter::execute(std::shared_ptr<Statement> stmt) {
    if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        evaluate(exprStmt->getExpr());
    } else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        executeBlock(stmt, std::make_shared<Environment>(environment));
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        executeIf(stmt);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        executeWhile(stmt);
    } else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        executeFor(stmt);
    } else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        executeReturn(stmt);
    }
}

void Interpreter::executeDeclaration(std::shared_ptr<Declaration> decl) {
    // Handle function declarations
    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(decl)) {
        // Create function object
        auto function = std::make_shared<Function>(funcDecl, environment);
        
        // Define function in current environment
        environment->define(funcDecl->getName(), Value(function));
    }
    // Handle variable declarations
    else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(decl)) {
        // Evaluate initializer if present
        Value value;
        if (varDecl->getInitializer()) {
            value = evaluate(varDecl->getInitializer());
        }
        
        // Define variable in current environment
        environment->define(varDecl->getName(), value);
    }
    // Handle namespace declarations
    else if (auto namespaceDecl = std::dynamic_pointer_cast<NamespaceDeclaration>(decl)) {
        executeNamespaceDeclaration(namespaceDecl);
    }
    // Handle class declarations
    else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(decl)) {
        executeClassDeclaration(classDecl);
    }
    // Handle struct declarations
    else if (auto structDecl = std::dynamic_pointer_cast<StructDeclaration>(decl)) {
        executeStructDeclaration(structDecl);
    }
    // Handle object declarations
    else if (auto objDecl = std::dynamic_pointer_cast<ObjectDeclaration>(decl)) {
        executeObjectDeclaration(objDecl);
    }
    // Handle import declarations
    else if (auto importDecl = std::dynamic_pointer_cast<ImportDeclaration>(decl)) {
        // TODO: Implement import handling
        std::cout << "Import of " << importDecl->getPath() << " not yet implemented" << std::endl;
    }
}

void Interpreter::executeBlock(std::shared_ptr<Statement> blockStmt, std::shared_ptr<Environment> env) {
    // Ensure the statement is a BlockStatement
    auto block = std::dynamic_pointer_cast<BlockStatement>(blockStmt);
    if (!block) {
        std::cerr << "executeBlock called with non-block statement" << std::endl;
        return;
    }

    // Save the current environment
    auto previous = environment;
    
    try {
        // Set the new environment
        environment = env;
        
        // Execute all statements in the block
        for (const auto& statement : block->getStatements()) {
            if (!statement) {
                std::cerr << "Null statement in block" << std::endl;
                continue;
            }
            execute(statement);
        }
    } catch (const ReturnValue& returnValue) {
        // Restore environment before propagating return
        environment = previous;
        throw;  // Re-throw to handle function returns
    } catch (const std::exception& e) {
        // Restore the previous environment even if an exception occurs
        std::cerr << "Exception during block execution: " << e.what() << std::endl;
        environment = previous;
        throw;
    } catch (...) {
        // Restore the previous environment even if an exception occurs
        std::cerr << "Unknown exception during block execution" << std::endl;
        environment = previous;
        throw;
    }
    
    // Restore the previous environment
    environment = previous;
}

void Interpreter::executeExpression(std::shared_ptr<Statement> stmt) {
    auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt);
    if (exprStmt) {
        evaluate(exprStmt->getExpr());
    }
}

void Interpreter::executeIf(std::shared_ptr<Statement> stmt) {
    auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt);
    if (!ifStmt) return;

    Value condition = evaluate(ifStmt->getCondition());
    
    if (isTruthy(condition)) {
        execute(ifStmt->getThenBranch());
    } else if (ifStmt->getElseBranch()) {
        execute(ifStmt->getElseBranch());
    }
}

void Interpreter::executeWhile(std::shared_ptr<Statement> stmt) {
    auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt);
    if (!whileStmt) return;

    while (true) {
        Value condition = evaluate(whileStmt->getCondition());
        if (!isTruthy(condition)) break;
        
        execute(whileStmt->getBody());
    }
}

void Interpreter::executeFor(std::shared_ptr<Statement> stmt) {
    auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt);
    if (!forStmt) return;

    // Create a new environment for the for loop
    auto loopEnv = std::make_shared<Environment>(environment);
    
    // Save the current environment
    auto previous = environment;
    environment = loopEnv;
    
    try {
        // Execute initializer if present
        if (forStmt->getInit()) {
            execute(forStmt->getInit());
        }
        
        // Execute loop condition and body
        while (true) {
            // Check condition if present
            if (forStmt->getCondition()) {
                Value condition = evaluate(forStmt->getCondition());
                if (!isTruthy(condition)) break;
            }
            
            // Execute loop body
            execute(forStmt->getBody());
            
            // Execute increment if present
            if (forStmt->getUpdate()) {
                evaluate(forStmt->getUpdate());
            }
        }
    } catch (...) {
        // Restore environment even if an exception occurs
        environment = previous;
        throw;
    }
    
    // Restore the previous environment
    environment = previous;
}

void Interpreter::executeReturn(std::shared_ptr<Statement> stmt) {
    auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt);
    if (!returnStmt) return;

    Value value;
    if (returnStmt->getValue()) {
        value = evaluate(returnStmt->getValue());
    }
    
    // Use exception for non-local return
    throw ReturnValue{value};
}

void Interpreter::executeVariableDeclaration(std::shared_ptr<Statement> stmt) {
    // Convert the statement back to a variable declaration
    auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt);
    if (!varDecl) return;

    Value value;
    if (varDecl->getInitializer()) {
        value = evaluate(varDecl->getInitializer());
    }
    
    environment->define(varDecl->getName(), value);
}

void Interpreter::executeFunctionDeclaration(std::shared_ptr<Statement> stmt) {
    // Convert the statement back to a function declaration
    auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(stmt);
    if (!funcDecl) return;

    auto function = std::make_shared<Function>(funcDecl, environment);
    environment->define(funcDecl->getName(), Value(function));
}

Value Interpreter::evaluate(std::shared_ptr<Expression> expr) {
    // Literal expressions
    if (auto literal = std::dynamic_pointer_cast<LiteralExpression>(expr)) {
        return evaluateLiteral(expr);
    }
    
    // Identifier expressions
    if (auto identifier = std::dynamic_pointer_cast<IdentifierExpression>(expr)) {
        return evaluateIdentifier(expr);
    }
    
    // Object instantiation expressions
    if (auto objInst = std::dynamic_pointer_cast<ObjectInstantiationExpression>(expr)) {
        return evaluateObjectInstantiation(expr);
    }
    
    // Array literal expressions
    if (auto arrayLiteral = std::dynamic_pointer_cast<ArrayLiteralExpression>(expr)) {
        return evaluateArray(expr);
    }
    
    // Scope resolution expressions
    if (auto scopeResolution = std::dynamic_pointer_cast<ScopeResolutionExpression>(expr)) {
        return evaluateScopeResolution(expr);
    }
    
    // Unary expressions
    if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
        return evaluateUnary(expr);
    }
    
    // Binary expressions
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        return evaluateBinary(expr);
    }
    
    // Call expressions
    if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        return evaluateCall(expr);
    }
    
    // Member access expressions
    if (auto memberAccess = std::dynamic_pointer_cast<MemberAccessExpression>(expr)) {
        return evaluateMemberAccess(expr);
    }
    
    // Index expressions
    if (auto index = std::dynamic_pointer_cast<IndexExpression>(expr)) {
        return evaluateIndex(expr);
    }
    
    // Injectable string expressions
    if (auto injectableExpr = std::dynamic_pointer_cast<InjectableStringExpression>(expr)) {
        return evaluateInjectableString(expr);
    }
    
    // Builtin call expressions
    if (auto builtinCall = std::dynamic_pointer_cast<BuiltinCallExpression>(expr)) {
        // Evaluate all arguments
        std::vector<Value> args;
        for (const auto& arg : builtinCall->getArgs()) {
            args.push_back(evaluate(arg));
        }
        
        // Handle specific builtin functions
        switch (builtinCall->getBuiltinType()) {
            case BuiltinCallExpression::BuiltinType::PRINT:
                return print(args);
            
            case BuiltinCallExpression::BuiltinType::INPUT:
                return input(args);
                
            case BuiltinCallExpression::BuiltinType::OPEN:
                // Placeholder implementation
                std::cout << "File opening not implemented yet" << std::endl;
                return Value();
                
            case BuiltinCallExpression::BuiltinType::SOCKET:
                // Placeholder implementation
                std::cout << "Socket operations not implemented yet" << std::endl;
                return Value();
                
            default:
                throw InterpreterError("Unknown builtin function");
        }
    }
    
    // Throw an error for unknown expression types
    throw InterpreterError("Unknown expression type");
}

Value Interpreter::evaluateLiteral(std::shared_ptr<Expression> expr) {
    auto literal = std::dynamic_pointer_cast<LiteralExpression>(expr);
    if (!literal) return Value();
    
    const auto& literalValue = literal->getValue();
    
    if (std::holds_alternative<int64_t>(literalValue)) {
        return Value(std::get<int64_t>(literalValue));
    } else if (std::holds_alternative<double>(literalValue)) {
        return Value(std::get<double>(literalValue));
    } else if (std::holds_alternative<bool>(literalValue)) {
        return Value(std::get<bool>(literalValue));
    } else if (std::holds_alternative<char>(literalValue)) {
        return Value(std::get<char>(literalValue));
    } else if (std::holds_alternative<std::string>(literalValue)) {
        return Value(std::get<std::string>(literalValue));
    } else if (std::holds_alternative<std::nullptr_t>(literalValue)) {
        return Value(); // Void/null
    }
    
    throw InterpreterError("Unsupported literal type");
}

Value Interpreter::evaluateIdentifier(std::shared_ptr<Expression> expr) {
    auto identifier = std::dynamic_pointer_cast<IdentifierExpression>(expr);
    if (!identifier) return Value();
    
    try {
        return environment->get(identifier->getName());
    } catch (const InterpreterError& e) {
        throw InterpreterError("Undefined variable '" + identifier->getName() + "'");
    }
}

Value Interpreter::evaluateArray(std::shared_ptr<Expression> expr) {
    auto arrayLiteral = std::dynamic_pointer_cast<ArrayLiteralExpression>(expr);
    if (!arrayLiteral) return Value();
    
    std::vector<Value> elements;
    for (const auto& element : arrayLiteral->getElements()) {
        elements.push_back(evaluate(element));
    }
    
    return Value(elements);
}

Value Interpreter::evaluateUnary(std::shared_ptr<Expression> expr) {
    auto unary = std::dynamic_pointer_cast<UnaryExpression>(expr);
    if (!unary) return Value();
    
    Value operand = evaluate(unary->getOperand());
    
    switch (unary->getOperator()) {
        case UnaryExpression::Operator::NEG:
            if (operand.isInteger()) {
                return Value(-operand.as<int64_t>());
            } else if (operand.isFloat()) {
                return Value(-operand.as<double>());
            }
            throw InterpreterError("Operand must be a number");
            
        case UnaryExpression::Operator::NOT:
            return Value(!isTruthy(operand));
            
        case UnaryExpression::Operator::BIT_NOT:
            if (operand.isInteger()) {
                return Value(~operand.as<int64_t>());
            }
            throw InterpreterError("Operand must be an integer");
            
        default:
            throw InterpreterError("Unknown unary operator");
    }
}

Value Interpreter::evaluateBinary(std::shared_ptr<Expression> expr) {
    auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr);
    if (!binary) return Value();

    // Handle assignment separately
    if (binary->getOperator() == BinaryExpression::Operator::ASSIGN) {
        // For assignment, the left side should be an identifier
        auto identifier = std::dynamic_pointer_cast<IdentifierExpression>(binary->getLeft());
        if (!identifier) {
            throw InterpreterError("Invalid assignment target");
        }
        
        // Evaluate the right side
        Value value = evaluate(binary->getRight());
        
        // Assign the value to the variable
        environment->assign(identifier->getName(), value);
        
        return value;
    }

    Value left = evaluate(binary->getLeft());
    Value right = evaluate(binary->getRight());
    
    switch (binary->getOperator()) {
        case BinaryExpression::Operator::ADD:
            // String concatenation
            if (left.isString() && right.isString()) {
                return Value(left.as<std::string>() + right.as<std::string>());
            }
            // Numeric addition
            if (left.isNumber() && right.isNumber()) {
                if (left.isFloat() || right.isFloat()) {
                    double leftVal = left.isFloat() ? left.as<double>() : 
                                     static_cast<double>(left.as<int64_t>());
                    double rightVal = right.isFloat() ? right.as<double>() : 
                                      static_cast<double>(right.as<int64_t>());
                    return Value(leftVal + rightVal);
                }
                // Both are integers
                return Value(left.as<int64_t>() + right.as<int64_t>());
            }
            throw InterpreterError("Invalid operands for addition");
            
        case BinaryExpression::Operator::EQ:
            return Value(isEqual(left, right));
            
        // Add more cases for other binary operators as needed
        
        default:
            throw InterpreterError("Unsupported binary operator");
    }
}

Value Interpreter::evaluateCall(std::shared_ptr<Expression> expr) {
    auto call = std::dynamic_pointer_cast<CallExpression>(expr);
    if (!call) return Value();

    // Special handling for method calls (object.method())
    if (auto memberAccess = std::dynamic_pointer_cast<MemberAccessExpression>(call->getCallee())) {
        // Evaluate the object
        Value objectValue = evaluate(memberAccess->getObject());
        
        // Get the method name
        const std::string& methodName = memberAccess->getMember();
        
        // If it's an object, look for the method
        if (objectValue.isObject()) {
            const auto& objectMap = objectValue.as<std::unordered_map<std::string, Value>>();
            auto it = objectMap.find(methodName);
            
            if (it != objectMap.end() && it->second.isCallable()) {
                // Evaluate arguments
                std::vector<Value> arguments;
                for (const auto& arg : call->getArgs()) {
                    arguments.push_back(evaluate(arg));
                }
                
                // Get the method
                auto method = it->second.as<std::shared_ptr<Callable>>();
                
                // Call the method with the arguments
                return method->call(*this, arguments);
            }
            
            throw InterpreterError("Object has no method '" + methodName + "'");
        }
        
        throw InterpreterError("Cannot call method on non-object value");
    }
    
    // Normal function call handling
    Value callee = evaluate(call->getCallee());
    
    // Check if it's a callable object
    if (!callee.isCallable()) {
        throw InterpreterError("Can only call functions and methods");
    }
    
    // Evaluate arguments
    std::vector<Value> arguments;
    for (const auto& arg : call->getArgs()) {
        arguments.push_back(evaluate(arg));
    }
    
    // Get the callable function
    auto function = callee.as<std::shared_ptr<Callable>>();
    
    // Check arity (if applicable)
    if (function->arity() >= 0 && static_cast<int>(arguments.size()) != function->arity()) {
        // Special case for print, which is variadic
        if (function->arity() == -1 && arguments.empty()) {
            throw InterpreterError("print requires at least one argument");
        }
    }
    
    // Call the function and return its result
    return function->call(*this, arguments);
}

Value Interpreter::evaluateMemberAccess(std::shared_ptr<Expression> expr) {
    auto memberAccess = std::dynamic_pointer_cast<MemberAccessExpression>(expr);
    if (!memberAccess) return Value();

    // Evaluate the object expression
    Value object = evaluate(memberAccess->getObject());
    
    // Get the member name
    const std::string& memberName = memberAccess->getMember();
    
    // Check if the object is an actual object (map of key/values)
    if (object.isObject()) {
        // Access the member from the object
        const auto& objectMap = object.as<std::unordered_map<std::string, Value>>();
        auto it = objectMap.find(memberName);
        
        if (it != objectMap.end()) {
            return it->second;
        }
        
        throw InterpreterError("Undefined member '" + memberName + "'");
    }
    
    // For arrow operator (pointer dereferencing)
    if (memberAccess->getIsArrow()) {
        // For pointers, we would dereference first
        throw InterpreterError("Pointer dereferencing not implemented");
    }
    
    throw InterpreterError("Cannot access member '" + memberName + "' on non-object value");
}

Value Interpreter::evaluateIndex(std::shared_ptr<Expression> expr) {
    auto index = std::dynamic_pointer_cast<IndexExpression>(expr);
    if (!index) return Value();

    // Evaluate the array and index expressions
    Value arrayValue = evaluate(index->getArray());
    Value indexValue = evaluate(index->getIndex());
    
    // Check if it's a string (for character access)
    if (arrayValue.isString()) {
        if (!indexValue.isInteger()) {
            throw InterpreterError("String index must be an integer");
        }
        
        int64_t i = indexValue.as<int64_t>();
        const std::string& str = arrayValue.as<std::string>();
        
        if (i < 0 || i >= static_cast<int64_t>(str.length())) {
            throw InterpreterError("String index out of bounds");
        }
        
        // Return the character at the specified index
        return Value(str[i]);
    }
    
    // Check if it's an array
    if (arrayValue.isArray()) {
        if (!indexValue.isInteger()) {
            throw InterpreterError("Array index must be an integer");
        }
        
        int64_t i = indexValue.as<int64_t>();
        const auto& array = arrayValue.as<std::vector<Value>>();
        
        if (i < 0 || i >= static_cast<int64_t>(array.size())) {
            throw InterpreterError("Array index out of bounds");
        }
        
        // Return the element at the specified index
        return array[i];
    }
    
    // Check if it's an object (for property access by string key)
    if (arrayValue.isObject()) {
        if (indexValue.isString()) {
            const std::string& key = indexValue.as<std::string>();
            const auto& object = arrayValue.as<std::unordered_map<std::string, Value>>();
            
            auto it = object.find(key);
            if (it != object.end()) {
                return it->second;
            }
            
            throw InterpreterError("Object does not have property '" + key + "'");
        }
        
        throw InterpreterError("Object property access requires string key");
    }
    
    throw InterpreterError("Cannot use index operator on non-indexable value");
}

Value Interpreter::evaluateScopeResolution(std::shared_ptr<Expression> expr) {
    auto scopeResolution = std::dynamic_pointer_cast<ScopeResolutionExpression>(expr);
    if (!scopeResolution) return Value();

    // Get scope and identifier
    std::shared_ptr<Expression> scopeExpr = scopeResolution->getScope();
    const std::string& identifier = scopeResolution->getIdentifier();
    
    // First, handle simple namespace::identifier case
    if (auto identExpr = std::dynamic_pointer_cast<IdentifierExpression>(scopeExpr)) {
        const std::string& scope = identExpr->getName();
        
        // Look up the scope in the global environment
        try {
            Value scopeValue = globals->get(scope);
            
            if (scopeValue.isObject()) {
                // Look for identifier in the object/namespace
                const auto& scopeMap = scopeValue.as<std::unordered_map<std::string, Value>>();
                auto it = scopeMap.find(identifier);
                
                if (it != scopeMap.end()) {
                    return it->second;
                }
            }
            
            // Try direct qualified name lookup as fallback
            std::string qualifiedName = scope + "::" + identifier;
            try {
                return globals->get(qualifiedName);
            } catch (const InterpreterError& e) {
                // Continue to nested scope resolution
            }
        } catch (const InterpreterError& e) {
            // Continue to nested scope resolution
        }
    }
    
    // Handle nested scope resolution (A::B::C)
    Value scopeValue = evaluate(scopeExpr);
    
    // If the scope expression itself is a scope resolution
    if (auto nestedScopeExpr = std::dynamic_pointer_cast<ScopeResolutionExpression>(scopeExpr)) {
        // Get the string representation of the scope path
        std::string scopePath = "";
        
        // Build scope path by traversing nested scopes
        std::shared_ptr<Expression> current = scopeExpr;
        while (auto currentScope = std::dynamic_pointer_cast<ScopeResolutionExpression>(current)) {
            if (auto rightIdent = std::dynamic_pointer_cast<IdentifierExpression>(currentScope->getScope())) {
                if (!scopePath.empty()) {
                    scopePath = rightIdent->getName() + "::" + scopePath;
                } else {
                    scopePath = rightIdent->getName() + "::" + currentScope->getIdentifier();
                }
                current = currentScope->getScope();
            } else {
                // Handle more complex scope expressions
                break;
            }
        }
        
        if (auto baseIdent = std::dynamic_pointer_cast<IdentifierExpression>(current)) {
            if (!scopePath.empty()) {
                scopePath = baseIdent->getName() + "::" + scopePath;
            } else {
                scopePath = baseIdent->getName();
            }
        }
        
        // Try the combined qualified name
        std::string fullQualifiedName = scopePath + "::" + identifier;
        try {
            return globals->get(fullQualifiedName);
        } catch (const InterpreterError& e) {
            // Fall through to next approach
        }
    }
    
    if (scopeValue.isObject()) {
        // Look for identifier in the object
        const auto& objectMap = scopeValue.as<std::unordered_map<std::string, Value>>();
        auto it = objectMap.find(identifier);
        
        if (it != objectMap.end()) {
            return it->second;
        }
    }
    
    throw InterpreterError("Cannot resolve '" + identifier + "' in scope");
}

void Interpreter::executeNamespaceDeclaration(std::shared_ptr<NamespaceDeclaration> decl) {
    std::string namespaceName = decl->getName();
    
    // Create namespace environment
    std::unordered_map<std::string, Value> namespaceEnv;
    
    // Process namespace members (classes)
    for (const auto& classDecl : decl->getClasses()) {
        std::string className = classDecl->getName();
        std::string qualifiedName = namespaceName + "::" + className;
        
        // Create class environment
        std::unordered_map<std::string, Value> classEnv;
        
        // Process class members
        for (const auto& member : classDecl->getMembers()) {
            if (auto objDecl = std::dynamic_pointer_cast<ObjectDeclaration>(member)) {
                // Process object declaration
                std::string objName = objDecl->getName();
                std::string objQualifiedName = qualifiedName + "::" + objName;
                
                // Create object environment
                std::unordered_map<std::string, Value> objEnv;
                
                // Process object members
                for (const auto& objMember : objDecl->getMembers()) {
                    if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(objMember)) {
                        std::string varName = varDecl->getName();
                        
                        // Evaluate initializer if present
                        Value varValue;
                        if (varDecl->getInitializer()) {
                            varValue = evaluate(varDecl->getInitializer());
                        }
                        
                        // Add to object environment
                        objEnv[varName] = varValue;
                    }
                    else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(objMember)) {
                        std::string funcName = funcDecl->getName();
                        
                        // Create function object
                        auto function = std::make_shared<Function>(funcDecl, environment);
                        
                        // Add to object environment
                        objEnv[funcName] = Value(function);
                    }
                }
                
                // Add object to class environment
                classEnv[objName] = Value(objEnv);
                
                // IMPORTANT: Register the object in the global environment with its qualified name
                // This allows direct access via scope resolution operator (::)
                globals->define(objQualifiedName, Value(objEnv));
            }
            else if (auto structDecl = std::dynamic_pointer_cast<StructDeclaration>(member)) {
                std::string structName = structDecl->getName();
                std::string structQualifiedName = qualifiedName + "::" + structName;
                
                // Create struct template
                std::unordered_map<std::string, Value> structEnv;
                
                // Process fields
                for (const auto& field : structDecl->getFields()) {
                    // Default field value
                    Value defaultValue;
                    
                    // Add field to struct environment
                    structEnv[field.getName()] = defaultValue;
                }
                
                // Add to class environment
                classEnv[structName] = Value(structEnv);
                
                // Also register with qualified name
                globals->define(structQualifiedName, Value(structEnv));
            }
            else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(member)) {
                // This could be an object instance in the class
                std::string varName = varDecl->getName();
                std::string varQualifiedName = qualifiedName + "::" + varName;
                
                // Try to find the object type
                try {
                    auto typePtr = varDecl->getType();
                    std::string typeName = typePtr->toString();
                    
                    Value objectValue;
                    try {
                        objectValue = globals->get(typeName);
                        if (objectValue.isObject()) {
                            // Clone the object for the class instance
                            auto objectInstance = objectValue.as<std::unordered_map<std::string, Value>>();
                            
                            // Add to class environment
                            classEnv[varName] = Value(objectInstance);
                            
                            // Register with qualified name
                            globals->define(varQualifiedName, Value(objectInstance));
                        }
                    } catch (const InterpreterError& e) {
                        // Type not found, create an empty object
                        std::unordered_map<std::string, Value> emptyObj;
                        classEnv[varName] = Value(emptyObj);
                        globals->define(varQualifiedName, Value(emptyObj));
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error getting type name: " << e.what() << std::endl;
                }
            }
        }
        
        // Register class in global environment with qualified name
        globals->define(qualifiedName, Value(classEnv));
        
        // Add to namespace environment
        namespaceEnv[className] = Value(classEnv);
    }
    
    // Register namespace in global environment
    globals->define(namespaceName, Value(namespaceEnv));
    
    std::cout << "Registered namespace '" << namespaceName << "' with " 
              << decl->getClasses().size() << " classes" << std::endl;
}

void Interpreter::executeClassDeclaration(std::shared_ptr<ClassDeclaration> decl) {
    std::string className = decl->getName();
    
    // Create class environment
    std::unordered_map<std::string, Value> classEnv;
    
    // Process class members
    for (const auto& member : decl->getMembers()) {
        if (auto objDecl = std::dynamic_pointer_cast<ObjectDeclaration>(member)) {
            // Process object members
            std::string objName = objDecl->getName();
            std::string qualifiedName = className + "_" + objName;
            
            // Create object value
            std::unordered_map<std::string, Value> objEnv;
            
            // Process object members
            for (const auto& objMember : objDecl->getMembers()) {
                if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(objMember)) {
                    std::string varName = varDecl->getName();
                    std::string fullyQualifiedName = qualifiedName + "_" + varName;
                    
                    // Evaluate initializer if present
                    Value varValue;
                    if (varDecl->getInitializer()) {
                        varValue = evaluate(varDecl->getInitializer());
                    }
                    
                    // Register in global environment
                    globals->define(fullyQualifiedName, varValue);
                    
                    // Add to object environment
                    objEnv[varName] = varValue;
                }
                else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(objMember)) {
                    std::string funcName = funcDecl->getName();
                    std::string fullyQualifiedName = qualifiedName + "_" + funcName;
                    
                    // Create function object
                    auto function = std::make_shared<Function>(funcDecl, environment);
                    
                    // Register in global environment with qualified name
                    globals->define(fullyQualifiedName, Value(function));
                    
                    // Add to object environment
                    objEnv[funcName] = Value(function);
                }
            }
            
            // Register object in global environment
            globals->define(qualifiedName, Value(objEnv));
            
            // Add to class environment
            classEnv[objName] = Value(objEnv);
        }
        else if (auto structDecl = std::dynamic_pointer_cast<StructDeclaration>(member)) {
            // Process struct declaration
            executeStructDeclaration(structDecl);
        }
    }
    
    // Register class in global environment
    globals->define(className, Value(classEnv));
}

void Interpreter::executeStructDeclaration(std::shared_ptr<StructDeclaration> decl) {
    std::string structName = decl->getName();
    
    // Create struct template (fields without values)
    std::unordered_map<std::string, Value> structTemplate;
    
    // Process fields
    for (const auto& field : decl->getFields()) {
        // Default field value
        Value defaultValue;
        
        // Add field to struct template
        structTemplate[field.getName()] = defaultValue;
    }
    
    // Register struct template in global environment
    globals->define(structName, Value(structTemplate));
}

void Interpreter::executeObjectDeclaration(std::shared_ptr<ObjectDeclaration> decl) {
    std::string objName = decl->getName();
    
    // Create object environment
    std::unordered_map<std::string, Value> objEnv;
    
    // Process object members
    for (const auto& member : decl->getMembers()) {
        if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(member)) {
            std::string varName = varDecl->getName();
            
            // Evaluate initializer if present
            Value varValue;
            if (varDecl->getInitializer()) {
                varValue = evaluate(varDecl->getInitializer());
            }
            
            // Add to object environment
            objEnv[varName] = varValue;
        }
        else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(member)) {
            std::string funcName = funcDecl->getName();
            
            // Create function object
            auto function = std::make_shared<Function>(funcDecl, environment);
            
            // Add to object environment
            objEnv[funcName] = Value(function);
        }
    }
    
    // Register object in environment
    environment->define(objName, Value(objEnv));
}

Value Interpreter::evaluateInjectableString(std::shared_ptr<Expression> expr) {
    auto injectableExpr = std::dynamic_pointer_cast<InjectableStringExpression>(expr);
    if (!injectableExpr) {
        throw InterpreterError("Invalid injectable string expression");
    }

    // Get the format string and arguments
    const std::string& format = injectableExpr->getFormat();
    const auto& args = injectableExpr->getArgs();

    // Create a result string
    std::string result;
    
    // Track current position in the format string
    size_t pos = 0;
    
    // Iterate through the format string
    while (pos < format.length()) {
        // Find the next '{' placeholder
        size_t openBrace = format.find('{', pos);
        
        if (openBrace == std::string::npos) {
            // No more placeholders, append remaining string
            result += format.substr(pos);
            break;
        }
        
        // Append text before the placeholder
        result += format.substr(pos, openBrace - pos);
        
        // Find closing '}'
        size_t closeBrace = format.find('}', openBrace);
        
        if (closeBrace == std::string::npos) {
            throw InterpreterError("Malformed injectable string: unmatched '{'");
        }
        
        // Extract placeholder content
        std::string placeholder = format.substr(openBrace + 1, closeBrace - openBrace - 1);
        
        // Try to find the corresponding argument
        bool found = false;
        for (size_t i = 0; i < args.size(); ++i) {
            // Evaluate the argument
            Value argValue = evaluate(args[i]);
            
            // Currently support direct identifier or index-based matching
            if (auto identExpr = std::dynamic_pointer_cast<IdentifierExpression>(args[i])) {
                if (identExpr->getName() == placeholder) {
                    result += argValue.toString();
                    found = true;
                    break;
                }
            } else {
                // If no identifier match, use index-based matching
                if (std::to_string(i) == placeholder) {
                    result += argValue.toString();
                    found = true;
                    break;
                }
            }
        }
        
        // If no matching argument found, leave placeholder as-is
        if (!found) {
            result += "{" + placeholder + "}";
        }
        
        // Move position to after the placeholder
        pos = closeBrace + 1;
    }
    
    // Return as a string value
    return Value(result);
}

Value Interpreter::evaluateObjectInstantiation(std::shared_ptr<Expression> expr) {
    auto objInst = std::dynamic_pointer_cast<ObjectInstantiationExpression>(expr);
    if (!objInst) return Value();
    
    std::string objectType = objInst->getObjectType();
    std::string objectName = objInst->getObjectName();
    
    // Look up the object type in the environment
    Value objectTypeValue;
    try {
        objectTypeValue = environment->get(objectType);
    } catch (const InterpreterError& e) {
        throw InterpreterError("Unknown object type '" + objectType + "'");
    }
    
    // Clone the object (if it's an object)
    if (objectTypeValue.isObject()) {
        auto objectTemplate = objectTypeValue.as<std::unordered_map<std::string, Value>>();
        
        // Create a new instance of the object by copying the template
        auto objectInstance = objectTemplate;
        
        // Define the new object in the environment
        environment->define(objectName, Value(objectInstance));
        
        std::cout << "Created instance of '" << objectType << "' named '" << objectName << "'" << std::endl;
        
        // Return the object instance
        return Value(objectInstance);
    }
    
    throw InterpreterError("Cannot create instance of non-object type '" + objectType + "'");
}

} // namespace flux
