#include "interpreter.hpp"
#include <sstream>
#include <cmath>

namespace flux {

Value Interpreter::interpret(const std::vector<std::unique_ptr<Statement>>& statements) {
    Value lastValue;
    try {
        for (const auto& statement : statements) {
            execute(*statement);
        }
    } catch (const ReturnValue& returnValue) {
        return returnValue.getValue();
    }
    return lastValue;
}

void Interpreter::execute(const Statement& stmt) {
    switch (stmt.getType()) {
        case StatementType::Block:
            visitBlockStatement(static_cast<const BlockStatement&>(stmt));
            break;
        case StatementType::Expression:
            visitExpressionStatement(dynamic_cast<const ExpressionStatement&>(stmt));
            break;
        case StatementType::When:
            visitWhenStatement(dynamic_cast<const WhenStatement&>(stmt));
            break;
        case StatementType::Object:
            visitObjectDefinition(dynamic_cast<const ObjectDefinition&>(stmt));
            break;
        case StatementType::Class:
            visitClassDefinition(static_cast<const ClassDefinition&>(stmt));
            break;
        case StatementType::Namespace:
            visitNamespaceDefinition(static_cast<const NamespaceDefinition&>(stmt));
            break;
        case StatementType::Operator:
            visitOperatorDefinition(static_cast<const OperatorDefinition&>(stmt));
            break;
        case StatementType::Variable:
            visitVariableDeclaration(static_cast<const VariableDeclaration&>(stmt));
            break;
        case StatementType::Function:
            visitFunctionDeclaration(static_cast<const FunctionDeclaration&>(stmt));
            break;
        case StatementType::Return:
            visitReturnStatement(static_cast<const ReturnStatement&>(stmt));
            break;
    }
}

void Interpreter::visitObjectDefinition(const ObjectDefinition& stmt) {
    auto object = std::make_shared<CustomObject>(std::string(stmt.name.lexeme()));
    
    auto objectEnv = std::make_shared<Environment>(environment_);
    EnvironmentGuard guard(*this, objectEnv);
    
    for (const auto& method : stmt.methods) {
        if (auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(method.get())) {
            if (auto func = environment_->get(std::string(funcDecl->name.lexeme())).asFunction()) {
                defineMethod(object, std::string(funcDecl->name.lexeme()), func);
            }
        }
    }
    
    for (const auto& field : stmt.members) {
        Value initialValue = field.initializer ? 
            evaluate(*field.initializer) : Value();
        defineField(object, std::string(field.name.lexeme()), initialValue);
    }
    
    environment_->define(std::string(stmt.name.lexeme()), Value(object));
}

void Interpreter::visitClassDefinition(const ClassDefinition& stmt) {
    auto classObj = std::make_shared<CustomObject>(std::string(stmt.name.lexeme()));
    
    auto classEnv = std::make_shared<Environment>(environment_);
    EnvironmentGuard guard(*this, classEnv);
    
    for (const auto& method : stmt.methods) {
        if (auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(method.get())) {
            if (auto func = environment_->get(std::string(funcDecl->name.lexeme())).asFunction()) {
                defineMethod(classObj, std::string(funcDecl->name.lexeme()), func);
            }
        }
    }
    
    for (const auto& field : stmt.members) {
        Value initialValue = field.initializer ? 
            evaluate(*field.initializer) : Value();
        defineField(classObj, std::string(field.name.lexeme()), initialValue);
    }
    
    environment_->define(std::string(stmt.name.lexeme()), Value(classObj));
}

void Interpreter::visitNamespaceDefinition(const NamespaceDefinition& stmt) {
    auto namespaceEnv = std::make_shared<Environment>(environment_);
    EnvironmentGuard guard(*this, namespaceEnv);
    
    for (const auto& classDef : stmt.classes) {
        visitClassDefinition(classDef);
    }
    
    auto namespaceObj = std::make_shared<CustomObject>("namespace");
    for (const auto& [name, value] : namespaceEnv->getValues()) {
        defineField(namespaceObj, name, value);
    }
    
    environment_->define(std::string(stmt.name.lexeme()), Value(namespaceObj));
}

void Interpreter::visitOperatorDefinition(const OperatorDefinition& stmt) {
    // Move the operator token to avoid capturing by reference
    Token opToken = stmt.op;
    // Move the body to avoid issues with unique_ptr
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    for (const auto& s : stmt.body) {
        bodyStmts.push_back(s->clone()); // Assume we added clone() to Statement
    }
    
    auto func = std::make_shared<Function>(
        [this, opToken, bodyStmts = std::move(bodyStmts)]
        (std::vector<Value> args) mutable -> Value {
            if (args.size() != 2) {
                throw RuntimeError("Operator requires two arguments", opToken);
            }
            
            auto operatorEnv = std::make_shared<Environment>(environment_);
            EnvironmentGuard guard(*this, operatorEnv);
            
            for (const auto& statement : bodyStmts) {
                execute(*statement);
            }
            
            return Value();
        }
    );
    
    std::string opKey = "operator" + std::string(opToken.lexeme());
    environment_->define(opKey, Value(func));
}

Value Interpreter::visitIdentifier(const Identifier& expr) {
    return environment_->get(std::string(expr.name.lexeme()));
}

void Interpreter::defineVariable(const std::string& name, const Value& value) {
    environment_->define(name, value);
}

void Interpreter::assignVariable(const std::string& name, const Value& value) {
    environment_->assign(name, value);
}

Value Interpreter::getVariable(const std::string& name) const {
    return environment_->get(name);
}

// Implement the checkType template function
template<typename T>
T Interpreter::checkType(const Value& value, const std::string& type) const {
    if (!value.is<T>()) {
        throw RuntimeError("Expected " + type, Token(TokenType::ERROR, "", 0, 0));
    }
    return value.as<T>();
}

} // namespace flux
