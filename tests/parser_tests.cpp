/**
 * @file parser_tests.cpp
 * @brief Test suite for the Flux parser
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include "../src/lexer/lexer.hpp"
#include "../src/lexer/token.hpp"
#include "../src/parser/parser.hpp"
#include "../src/parser/ast.hpp"

using namespace flux;

// Custom RTTI-safe dynamic cast
template<typename Derived, typename Base>
std::shared_ptr<Derived> safe_dynamic_pointer_cast(const std::shared_ptr<Base>& base) {
    if (base && base->isA(typeid(Derived))) {
        return std::dynamic_pointer_cast<Derived>(base);
    }
    return nullptr;
}

// Helper function to parse a source string
std::unique_ptr<Program> parseSource(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    Parser parser(tokens);
    return parser.parse();
}

// Test expression parsing
void testExpressionParsing() {
    std::cout << "Running expression parsing test...\n";
    
    // Test a simple expression
    std::string source = "1 + 2 * 3;";
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be an expression statement
    auto stmt = std::dynamic_pointer_cast<ExpressionStmt>(program->statements[0]);
    assert(stmt != nullptr);
    
    // The expression should be a binary expression
    auto expr = std::dynamic_pointer_cast<BinaryExpr>(stmt->expression);
    assert(expr != nullptr);
    
    // The operator should be +
    assert(expr->op.type == TokenType::PLUS);
    
    // The left operand should be an integer literal with value 1
    auto left = std::dynamic_pointer_cast<IntegerLiteral>(expr->left);
    assert(left != nullptr);
    assert(left->value == 1);
    
    // The right operand should be a binary expression with * operator
    auto right = std::dynamic_pointer_cast<BinaryExpr>(expr->right);
    assert(right != nullptr);
    assert(right->op.type == TokenType::STAR);
    
    // The right expression's operands should be integer literals with values 2 and 3
    auto rightLeft = std::dynamic_pointer_cast<IntegerLiteral>(right->left);
    assert(rightLeft != nullptr);
    assert(rightLeft->value == 2);
    
    auto rightRight = std::dynamic_pointer_cast<IntegerLiteral>(right->right);
    assert(rightRight != nullptr);
    assert(rightRight->value == 3);
    
    std::cout << "Expression parsing test passed.\n";
}

// Test variable declaration parsing
void testVariableDeclaration() {
    std::cout << "Running variable declaration test...\n";
    
    // Test a simple variable declaration
    std::string source = "int x = 5;";
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a variable declaration
    auto varDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(program->statements[0]);
    assert(varDecl != nullptr);
    
    // The type should be int
    auto type = std::dynamic_pointer_cast<PrimitiveType>(varDecl->type);
    assert(type != nullptr);
    assert(type->kind == PrimitiveType::PrimitiveKind::INT);
    
    // The name should be x
    assert(varDecl->name.lexeme == "x");
    
    // The initializer should be an integer literal with value 5
    auto initializer = std::dynamic_pointer_cast<IntegerLiteral>(varDecl->initializer);
    assert(initializer != nullptr);
    assert(initializer->value == 5);
    
    // Test a variable declaration with no initializer
    source = "float y;";
    program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a variable declaration
    varDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(program->statements[0]);
    assert(varDecl != nullptr);
    
    // The type should be float
    type = std::dynamic_pointer_cast<PrimitiveType>(varDecl->type);
    assert(type != nullptr);
    assert(type->kind == PrimitiveType::PrimitiveKind::FLOAT);
    
    // The name should be y
    assert(varDecl->name.lexeme == "y");
    
    // The initializer should be null
    assert(varDecl->initializer == nullptr);
    
    std::cout << "Variable declaration test passed.\n";
}

// Test function declaration parsing
void testFunctionDeclaration() {
    std::cout << "Running function declaration test...\n";
    
    // Test a simple function declaration
    std::string source = R"(
        int{32} add(int{32} a, int{32} b) {
            return a + b;
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a function declaration
    auto funcDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(program->statements[0]);
    assert(funcDecl != nullptr);
    
    // The return type should be int{32}
    auto returnType = std::dynamic_pointer_cast<PrimitiveType>(funcDecl->returnType);
    assert(returnType != nullptr);
    assert(returnType->kind == PrimitiveType::PrimitiveKind::INT);
    assert(returnType->bitWidth == 32);
    
    // The name should be add
    assert(funcDecl->name.lexeme == "add");
    
    // Should have two parameters
    assert(funcDecl->parameters.size() == 2);
    
    // First parameter should be int{32} a
    auto param1Type = std::dynamic_pointer_cast<PrimitiveType>(funcDecl->parameters[0].type);
    assert(param1Type != nullptr);
    assert(param1Type->kind == PrimitiveType::PrimitiveKind::INT);
    assert(param1Type->bitWidth == 32);
    assert(funcDecl->parameters[0].name.lexeme == "a");
    
    // Second parameter should be int{32} b
    auto param2Type = std::dynamic_pointer_cast<PrimitiveType>(funcDecl->parameters[1].type);
    assert(param2Type != nullptr);
    assert(param2Type->kind == PrimitiveType::PrimitiveKind::INT);
    assert(param2Type->bitWidth == 32);
    assert(funcDecl->parameters[1].name.lexeme == "b");
    
    // The body should be a block statement
    auto body = safe_dynamic_pointer_cast<BlockStmt>(funcDecl->body);
    assert(body != nullptr);
    
    // The block should have one statement (return)
    assert(body->statements.size() == 1);
    
    // The statement should be a return statement
    auto returnStmt = std::dynamic_pointer_cast<ReturnStmt>(body->statements[0]);
    assert(returnStmt != nullptr);
    
    // The return value should be a binary expression (a + b)
    auto returnExpr = std::dynamic_pointer_cast<BinaryExpr>(returnStmt->value);
    assert(returnExpr != nullptr);
    assert(returnExpr->op.type == TokenType::PLUS);
    
    // Left operand should be a variable reference to a
    auto leftVar = std::dynamic_pointer_cast<VariableExpr>(returnExpr->left);
    assert(leftVar != nullptr);
    assert(leftVar->name.lexeme == "a");
    
    // Right operand should be a variable reference to b
    auto rightVar = std::dynamic_pointer_cast<VariableExpr>(returnExpr->right);
    assert(rightVar != nullptr);
    assert(rightVar->name.lexeme == "b");
    
    std::cout << "Function declaration test passed.\n";
}

// Test if statement parsing
void testIfStatement() {
    std::cout << "Running if statement test...\n";
    
    // Test a simple if statement
    std::string source = R"(
        if (x > 0) {
            y = 1;
        } else {
            y = -1;
        }
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be an if statement
    auto ifStmt = std::dynamic_pointer_cast<IfStmt>(program->statements[0]);
    assert(ifStmt != nullptr);
    
    // The condition should be a binary expression (x > 0)
    auto condition = std::dynamic_pointer_cast<BinaryExpr>(ifStmt->condition);
    assert(condition != nullptr);
    assert(condition->op.type == TokenType::GREATER);
    
    // Left operand should be a variable reference to x
    auto leftVar = std::dynamic_pointer_cast<VariableExpr>(condition->left);
    assert(leftVar != nullptr);
    assert(leftVar->name.lexeme == "x");
    
    // Right operand should be an integer literal with value 0
    auto rightLit = std::dynamic_pointer_cast<IntegerLiteral>(condition->right);
    assert(rightLit != nullptr);
    assert(rightLit->value == 0);
    
    // Then branch should be a block statement
    auto thenBlock = safe_dynamic_pointer_cast<BlockStmt>(ifStmt->thenBranch);
    assert(thenBlock != nullptr);
    
    // The block should have one statement (y = 1)
    assert(thenBlock->statements.size() == 1);
    
    // The statement should be an expression statement (assignment)
    auto thenExpr = std::dynamic_pointer_cast<ExpressionStmt>(thenBlock->statements[0]);
    assert(thenExpr != nullptr);
    
    // The expression should be an assignment
    auto thenAssign = std::dynamic_pointer_cast<AssignExpr>(thenExpr->expression);
    assert(thenAssign != nullptr);
    assert(thenAssign->name.lexeme == "y");
    
    // The value should be an integer literal with value 1
    auto thenValue = std::dynamic_pointer_cast<IntegerLiteral>(thenAssign->value);
    assert(thenValue != nullptr);
    assert(thenValue->value == 1);
    
    // Else branch should be a block statement
    auto elseBlock = safe_dynamic_pointer_cast<BlockStmt>(ifStmt->elseBranch);
    assert(elseBlock != nullptr);
    
    // The block should have one statement (y = -1)
    assert(elseBlock->statements.size() == 1);
    
    // The statement should be an expression statement (assignment)
    auto elseExpr = std::dynamic_pointer_cast<ExpressionStmt>(elseBlock->statements[0]);
    assert(elseExpr != nullptr);
    
    // The expression should be an assignment
    auto elseAssign = std::dynamic_pointer_cast<AssignExpr>(elseExpr->expression);
    assert(elseAssign != nullptr);
    assert(elseAssign->name.lexeme == "y");
    
    // The value should be a unary expression (-1)
    auto elseUnary = std::dynamic_pointer_cast<UnaryExpr>(elseAssign->value);
    assert(elseUnary != nullptr);
    assert(elseUnary->op.type == TokenType::MINUS);
    
    // The value should be an integer literal with value 1
    auto elseValue = std::dynamic_pointer_cast<IntegerLiteral>(elseUnary->right);
    assert(elseValue != nullptr);
    assert(elseValue->value == 1);
    
    std::cout << "If statement test passed.\n";
}

// Test when statement parsing
void testWhenStatement() {
    std::cout << "Running when statement test...\n";
    
    // Test a simple when statement
    std::string source = R"(
        when (event_ready) volatile {
            process_event();
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a when statement
    auto whenStmt = std::dynamic_pointer_cast<WhenStmt>(program->statements[0]);
    assert(whenStmt != nullptr);
    
    // The condition should be a variable reference to event_ready
    auto condition = std::dynamic_pointer_cast<VariableExpr>(whenStmt->condition);
    assert(condition != nullptr);
    assert(condition->name.lexeme == "event_ready");
    
    // Should be volatile
    assert(whenStmt->isVolatile);
    
    // Should not be async
    assert(!whenStmt->isAsync);
    
    // Body should be a block statement
    auto body = safe_dynamic_pointer_cast<BlockStmt>(whenStmt->body);
    assert(body != nullptr);
    
    // The block should have one statement (process_event())
    assert(body->statements.size() == 1);
    
    // The statement should be an expression statement (function call)
    auto expr = std::dynamic_pointer_cast<ExpressionStmt>(body->statements[0]);
    assert(expr != nullptr);
    
    // The expression should be a call expression
    auto call = std::dynamic_pointer_cast<CallExpr>(expr->expression);
    assert(call != nullptr);
    
    // The callee should be a variable reference to process_event
    auto callee = std::dynamic_pointer_cast<VariableExpr>(call->callee);
    assert(callee != nullptr);
    assert(callee->name.lexeme == "process_event");
    
    // Should have no arguments
    assert(call->arguments.size() == 0);
    
    std::cout << "When statement test passed.\n";
}

// Test async when statement parsing
void testAsyncWhenStatement() {
    std::cout << "Running async when statement test...\n";
    
    // Test an async when statement
    std::string source = R"(
        async when (data_available) {
            process_data();
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a when statement
    auto whenStmt = std::dynamic_pointer_cast<WhenStmt>(program->statements[0]);
    assert(whenStmt != nullptr);
    
    // Should be async
    assert(whenStmt->isAsync);
    
    // Should not be volatile
    assert(!whenStmt->isVolatile);
    
    std::cout << "Async when statement test passed.\n";
}

// Test class declaration parsing
void testClassDeclaration() {
    std::cout << "Running class declaration test...\n";
    
    // Test a simple class declaration
    std::string source = R"(
        class Point {
            int{32} x;
            int{32} y;
            
            Point{} constructor(int{32} x, int{32} y) {
                this->x = x;
                this->y = y;
            };
            
            float{64} distance() {
                return sqrt(x * x + y * y);
            };
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a class declaration
    auto classDecl = std::dynamic_pointer_cast<ClassDeclarationStmt>(program->statements[0]);
    assert(classDecl != nullptr);
    
    // The name should be Point
    assert(classDecl->name.lexeme == "Point");
    
    // Should have 4 members (2 variables and 2 methods)
    assert(classDecl->members.size() == 4);
    
    // First member should be a variable declaration for x
    auto xDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(classDecl->members[0]);
    assert(xDecl != nullptr);
    assert(xDecl->name.lexeme == "x");
    
    // Second member should be a variable declaration for y
    auto yDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(classDecl->members[1]);
    assert(yDecl != nullptr);
    assert(yDecl->name.lexeme == "y");
    
    // Third member should be a function declaration for constructor
    auto constructorDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(classDecl->members[2]);
    assert(constructorDecl != nullptr);
    assert(constructorDecl->name.lexeme == "constructor");
    
    // Fourth member should be a function declaration for distance
    auto distanceDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(classDecl->members[3]);
    assert(distanceDecl != nullptr);
    assert(distanceDecl->name.lexeme == "distance");
    
    std::cout << "Class declaration test passed.\n";
}

// Test object declaration parsing
void testObjectDeclaration() {
    std::cout << "Running object declaration test...\n";
    
    // Test a simple object declaration
    std::string source = R"(
        object Counter {
            int value = 0;
            
            void increment() {
                value = value + 1;
            };
            
            void decrement() {
                value = value - 1;
            };
            
            int getValue() {
                return value;
            };
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be an object declaration
    auto objDecl = std::dynamic_pointer_cast<ObjectDeclarationStmt>(program->statements[0]);
    assert(objDecl != nullptr);
    
    // The name should be Counter
    assert(objDecl->name.lexeme == "Counter");
    
    // Should have 4 members (1 variable and 3 methods)
    assert(objDecl->members.size() == 4);
    
    // First member should be a variable declaration for value
    auto valueDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(objDecl->members[0]);
    assert(valueDecl != nullptr);
    assert(valueDecl->name.lexeme == "value");
    
    // Second member should be a function declaration for increment
    auto incrementDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(objDecl->members[1]);
    assert(incrementDecl != nullptr);
    assert(incrementDecl->name.lexeme == "increment");
    
    // Third member should be a function declaration for decrement
    auto decrementDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(objDecl->members[2]);
    assert(decrementDecl != nullptr);
    assert(decrementDecl->name.lexeme == "decrement");
    
    // Fourth member should be a function declaration for getValue
    auto getValueDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(objDecl->members[3]);
    assert(getValueDecl != nullptr);
    assert(getValueDecl->name.lexeme == "getValue");
    
    std::cout << "Object declaration test passed.\n";
}

// Test namespace declaration parsing
void testNamespaceDeclaration() {
    std::cout << "Running namespace declaration test...\n";
    
    // Test a simple namespace declaration
    std::string source = R"(
        namespace Math {
            class Vector2 {
                float x;
                float y;
            };
            
            class Vector3 {
                float x;
                float y;
                float z;
            };
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a namespace declaration
    auto nsDecl = std::dynamic_pointer_cast<NamespaceDeclarationStmt>(program->statements[0]);
    assert(nsDecl != nullptr);
    
    // The name should be Math
    assert(nsDecl->name.lexeme == "Math");
    
    // Should have 2 declarations (Vector2 and Vector3)
    assert(nsDecl->declarations.size() == 2);
    
    // First declaration should be a class declaration for Vector2
    auto vector2Decl = std::dynamic_pointer_cast<ClassDeclarationStmt>(nsDecl->declarations[0]);
    assert(vector2Decl != nullptr);
    assert(vector2Decl->name.lexeme == "Vector2");
    
    // Second declaration should be a class declaration for Vector3
    auto vector3Decl = std::dynamic_pointer_cast<ClassDeclarationStmt>(nsDecl->declarations[1]);
    assert(vector3Decl != nullptr);
    assert(vector3Decl->name.lexeme == "Vector3");
    
    std::cout << "Namespace declaration test passed.\n";
}

// Test operator declaration parsing
void testOperatorDeclaration() {
    std::cout << "Running operator declaration test...\n";
    
    // Test a simple operator declaration
    std::string source = R"(
        operator(Vector2, Vector2)[+] {
            return Vector2(left.x + right.x, left.y + right.y);
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be an operator declaration
    auto opDecl = std::dynamic_pointer_cast<OperatorDeclarationStmt>(program->statements[0]);
    assert(opDecl != nullptr);
    
    // The left type should be Vector2
    auto leftType = std::dynamic_pointer_cast<ClassType>(opDecl->leftType);
    assert(leftType != nullptr);
    assert(leftType->name == "Vector2");
    
    // The right type should be Vector2
    auto rightType = std::dynamic_pointer_cast<ClassType>(opDecl->rightType);
    assert(rightType != nullptr);
    assert(rightType->name == "Vector2");
    
    // The operator should be +
    assert(opDecl->op.lexeme == "+");
    
    std::cout << "Operator declaration test passed.\n";
}

// Test asm statement parsing
void testAsmStatement() {
    std::cout << "Running asm statement test...\n";
    
    // Test a simple asm statement
    std::string source = R"(
        asm {
            mov eax, 1
            mov ebx, 0
            int 0x80
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be an asm statement
    auto asmStmt = std::dynamic_pointer_cast<AsmStmt>(program->statements[0]);
    assert(asmStmt != nullptr);
    
    // The asm code should contain the assembly instructions
    assert(asmStmt->asmCode.find("mov eax, 1") != std::string::npos);
    
    std::cout << "Asm statement test passed.\n";
}

// Test lock statement parsing
void testLockStatement() {
    std::cout << "Running lock statement test...\n";
    
    // Test lock behavior statements
    std::string source = R"(
        lock myFunc ::scope1 ::scope2 {
            // Lock behavior implementation
        };
        
        __lock myFunc ::scope1 ::scope2 {
            // Pre-lock behavior implementation
        };
        
        lock__ myFunc ::scope1 ::scope2 {
            // Post-lock behavior implementation
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have three statements
    assert(program->statements.size() == 3);
    
    // First statement should be a lock statement
    auto lockStmt = std::dynamic_pointer_cast<LockStmt>(program->statements[0]);
    assert(lockStmt != nullptr);
    assert(lockStmt->type == LockStmt::LockType::LOCK);
    
    // Second statement should be a pre-lock statement
    auto preLockStmt = std::dynamic_pointer_cast<LockStmt>(program->statements[1]);
    assert(preLockStmt != nullptr);
    assert(preLockStmt->type == LockStmt::LockType::PRE_LOCK);
    
    // Third statement should be a post-lock statement
    auto postLockStmt = std::dynamic_pointer_cast<LockStmt>(program->statements[2]);
    assert(postLockStmt != nullptr);
    assert(postLockStmt->type == LockStmt::LockType::POST_LOCK);
    
    std::cout << "Lock statement test passed.\n";
}

// Test struct declaration parsing
void testStructDeclaration() {
    std::cout << "Running struct declaration test...\n";
    
    // Test a simple struct declaration
    std::string source = R"(
        struct {
            int{32} x;
            int{32} y;
        } Point;
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have one statement
    assert(program->statements.size() == 1);
    
    // The statement should be a struct declaration
    auto structDecl = std::dynamic_pointer_cast<StructDeclarationStmt>(program->statements[0]);
    assert(structDecl != nullptr);
    
    // The name should be Point
    assert(structDecl->name.lexeme == "Point");
    
    // Should have 2 fields
    assert(structDecl->fields.size() == 2);
    
    // First field should be int{32} x
    auto xField = structDecl->fields[0];
    auto xType = std::dynamic_pointer_cast<PrimitiveType>(xField.type);
    assert(xType != nullptr);
    assert(xType->kind == PrimitiveType::PrimitiveKind::INT);
    assert(xType->bitWidth == 32);
    assert(xField.name.lexeme == "x");
    
    // Second field should be int{32} y
    auto yField = structDecl->fields[1];
    auto yType = std::dynamic_pointer_cast<PrimitiveType>(yField.type);
    assert(yType != nullptr);
    assert(yType->kind == PrimitiveType::PrimitiveKind::INT);
    assert(yType->bitWidth == 32);
    assert(yField.name.lexeme == "y");
    
    std::cout << "Struct declaration test passed.\n";
}

// Test complex program parsing
void testComplexProgram() {
    std::cout << "Running complex program test...\n";
    
    // Test a more complex program with multiple declarations
    std::string source = R"(
        // Vector2 class definition
        class Vector2 {
            float{32} x;
            float{32} y;
            
            Vector2{} constructor(float{32} x, float{32} y) {
                this->x = x;
                this->y = y;
            };
            
            float{32} magnitude() {
                return sqrt(x * x + y * y);
            };
        };
        
        // Vector2 addition operator
        operator(Vector2, Vector2)[+] {
            return Vector2(left.x + right.x, left.y + right.y);
        };
        
        // Main function
        int{32} main() volatile {
            Vector2 v1 = Vector2(3.0, 4.0);
            Vector2 v2 = Vector2(1.0, 2.0);
            Vector2 v3 = v1 + v2;
            
            print(i"Magnitude of v3: {m}":{v3.magnitude();});
            
            return 0;
        };
    )";
    
    std::unique_ptr<Program> program = parseSource(source);
    
    // Should have three top-level declarations
    assert(program->statements.size() == 3);
    
    // First declaration should be a class (Vector2)
    auto classDecl = std::dynamic_pointer_cast<ClassDeclarationStmt>(program->statements[0]);
    assert(classDecl != nullptr);
    assert(classDecl->name.lexeme == "Vector2");
    
    // Second declaration should be an operator (+)
    auto opDecl = std::dynamic_pointer_cast<OperatorDeclarationStmt>(program->statements[1]);
    assert(opDecl != nullptr);
    assert(opDecl->op.lexeme == "+");
    
    // Third declaration should be a function (main)
    auto funcDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(program->statements[2]);
    assert(funcDecl != nullptr);
    assert(funcDecl->name.lexeme == "main");
    
    std::cout << "Complex program test passed.\n";
}

// Main function to run all tests
int main() {
    testExpressionParsing();
    testVariableDeclaration();
    testFunctionDeclaration();
    testIfStatement();
    testWhenStatement();
    testAsyncWhenStatement();
    testClassDeclaration();
    testObjectDeclaration();
    testNamespaceDeclaration();
    testOperatorDeclaration();
    testAsmStatement();
    testLockStatement();
    testStructDeclaration();
    testComplexProgram();
    
    std::cout << "All parser tests passed!\n";
    return 0;
}
