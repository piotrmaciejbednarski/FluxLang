#pragma once

#include "../common/source.h"
#include "../common/arena.h"
#include "../lexer/token.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <optional>

namespace flux {
namespace parser {

// Forward declarations
class ASTVisitor;
class ASTNode;
class Declaration;
class Statement;
class Expression;
class Type;

// Base AST node class
class ASTNode {
public:
    ASTNode(const common::SourceRange& range) : source_range_(range) {}
    virtual ~ASTNode() = default;
    
    // Get source location information
    const common::SourceRange& getSourceRange() const { return source_range_; }
    
    // Visitor pattern
    virtual void accept(ASTVisitor& visitor) = 0;
    
    // RTTI helper
    template<typename T>
    T* as() { return dynamic_cast<T*>(this); }
    
    template<typename T>
    const T* as() const { return dynamic_cast<const T*>(this); }

protected:
    common::SourceRange source_range_;
};

// Program - root of the AST
class Program : public ASTNode {
public:
    Program(const common::SourceRange& range) : ASTNode(range) {}
    
    void addDeclaration(Declaration* decl) { declarations_.push_back(decl); }
    const std::vector<Declaration*>& getDeclarations() const { return declarations_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<Declaration*> declarations_;
};

// Base class for all declarations
class Declaration : public ASTNode {
public:
    Declaration(const common::SourceRange& range) : ASTNode(range) {}
    virtual ~Declaration() = default;
};

// Import statement
class ImportDeclaration : public Declaration {
public:
    ImportDeclaration(const common::SourceRange& range, std::string_view module_path,
                     std::optional<std::string_view> alias = std::nullopt)
        : Declaration(range), module_path_(module_path), alias_(alias) {}
    
    std::string_view getModulePath() const { return module_path_; }
    std::optional<std::string_view> getAlias() const { return alias_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view module_path_;
    std::optional<std::string_view> alias_;
};

// Using statement
class UsingDeclaration : public Declaration {
public:
    UsingDeclaration(const common::SourceRange& range) : Declaration(range) {}
    
    void addName(std::string_view name) { names_.push_back(name); }
    const std::vector<std::string_view>& getNames() const { return names_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::string_view> names_;
};

// Template parameters
class TemplateParameter : public ASTNode {
public:
    TemplateParameter(const common::SourceRange& range, std::string_view name)
        : ASTNode(range), name_(name) {}
    
    std::string_view getName() const { return name_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
};

class TemplateParameterList : public ASTNode {
public:
    TemplateParameterList(const common::SourceRange& range) : ASTNode(range) {}
    
    void addParameter(TemplateParameter* param) { parameters_.push_back(param); }
    const std::vector<TemplateParameter*>& getParameters() const { return parameters_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<TemplateParameter*> parameters_;
};

// Forward declaration
class ObjectDeclaration : public Declaration {
public:
    ObjectDeclaration(const common::SourceRange& range, std::string_view name,
                     TemplateParameterList* template_params = nullptr,
                     Type* parent_type = nullptr, bool is_forward = false)
        : Declaration(range), name_(name), template_params_(template_params),
          parent_type_(parent_type), is_forward_(is_forward) {}
    
    std::string_view getName() const { return name_; }
    TemplateParameterList* getTemplateParameters() const { return template_params_; }
    Type* getParentType() const { return parent_type_; }
    bool isForwardDeclaration() const { return is_forward_; }
    
    void addMember(Declaration* member) { members_.push_back(member); }
    const std::vector<Declaration*>& getMembers() const { return members_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
    TemplateParameterList* template_params_;
    Type* parent_type_;
    bool is_forward_;
    std::vector<Declaration*> members_;
};

// Struct declaration
class StructDeclaration : public Declaration {
public:
    StructDeclaration(const common::SourceRange& range, std::string_view name,
                     TemplateParameterList* template_params = nullptr)
        : Declaration(range), name_(name), template_params_(template_params) {}
    
    std::string_view getName() const { return name_; }
    TemplateParameterList* getTemplateParameters() const { return template_params_; }
    
    void addMember(Declaration* member) { members_.push_back(member); }
    const std::vector<Declaration*>& getMembers() const { return members_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
    TemplateParameterList* template_params_;
    std::vector<Declaration*> members_;
};

// Function parameter
class Parameter : public ASTNode {
public:
    Parameter(const common::SourceRange& range, Type* type, std::string_view name)
        : ASTNode(range), type_(type), name_(name) {}
    
    Type* getType() const { return type_; }
    std::string_view getName() const { return name_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* type_;
    std::string_view name_;
};

class ParameterList : public ASTNode {
public:
    ParameterList(const common::SourceRange& range) : ASTNode(range) {}
    
    void addParameter(Parameter* param) { parameters_.push_back(param); }
    const std::vector<Parameter*>& getParameters() const { return parameters_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<Parameter*> parameters_;
};

// Function declaration
class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(const common::SourceRange& range, std::string_view name,
                       ParameterList* params, Type* return_type, Statement* body = nullptr)
        : Declaration(range), name_(name), parameters_(params), 
          return_type_(return_type), body_(body) {}
    
    std::string_view getName() const { return name_; }
    ParameterList* getParameters() const { return parameters_; }
    Type* getReturnType() const { return return_type_; }
    Statement* getBody() const { return body_; }
    void setBody(Statement* body) { body_ = body; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
    ParameterList* parameters_;
    Type* return_type_;
    Statement* body_;
};

// Template declaration
class TemplateDeclaration : public Declaration {
public:
    TemplateDeclaration(const common::SourceRange& range, TemplateParameterList* params,
                       Declaration* declaration)
        : Declaration(range), template_params_(params), declaration_(declaration) {}
    
    TemplateParameterList* getTemplateParameters() const { return template_params_; }
    Declaration* getDeclaration() const { return declaration_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    TemplateParameterList* template_params_;
    Declaration* declaration_;
};

// Operator declaration
class OperatorDeclaration : public Declaration {
public:
    OperatorDeclaration(const common::SourceRange& range, ParameterList* params,
                       std::string_view operator_symbol, Type* return_type, Statement* body)
        : Declaration(range), parameters_(params), operator_symbol_(operator_symbol),
          return_type_(return_type), body_(body) {}
    
    ParameterList* getParameters() const { return parameters_; }
    std::string_view getOperatorSymbol() const { return operator_symbol_; }
    Type* getReturnType() const { return return_type_; }
    Statement* getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    ParameterList* parameters_;
    std::string_view operator_symbol_;
    Type* return_type_;
    Statement* body_;
};

// Variable declaration
class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(const common::SourceRange& range, Type* type, std::string_view name,
                       Expression* initializer = nullptr)
        : Declaration(range), type_(type), name_(name), initializer_(initializer) {}
    
    Type* getType() const { return type_; }
    std::string_view getName() const { return name_; }
    Expression* getInitializer() const { return initializer_; }
    void setInitializer(Expression* init) { initializer_ = init; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* type_;
    std::string_view name_;
    Expression* initializer_;
};

// Namespace declaration
class NamespaceDeclaration : public Declaration {
public:
    NamespaceDeclaration(const common::SourceRange& range, std::string_view name)
        : Declaration(range), name_(name) {}
    
    std::string_view getName() const { return name_; }
    
    void addMember(Declaration* member) { members_.push_back(member); }
    const std::vector<Declaration*>& getMembers() const { return members_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
    std::vector<Declaration*> members_;
};

// Base class for all types
class Type : public ASTNode {
public:
    Type(const common::SourceRange& range) : ASTNode(range) {}
    virtual ~Type() = default;
};

// Built-in types
class BuiltinType : public Type {
public:
    enum Kind {
        VOID,
        AUTO,
        DATA
    };
    
    BuiltinType(const common::SourceRange& range, Kind kind) : Type(range), kind_(kind) {}
    
    Kind getKind() const { return kind_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Kind kind_;
};

// Data type with bit width
class DataType : public Type {
public:
    DataType(const common::SourceRange& range, int bit_width, bool is_signed = false)
        : Type(range), bit_width_(bit_width), is_signed_(is_signed) {}
    
    int getBitWidth() const { return bit_width_; }
    bool isSigned() const { return is_signed_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    int bit_width_;
    bool is_signed_;
};

// Named type (identifier)
class NamedType : public Type {
public:
    NamedType(const common::SourceRange& range, std::string_view name)
        : Type(range), name_(name) {}
    
    std::string_view getName() const { return name_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
};

// Qualified type (namespace::type)
class QualifiedType : public Type {
public:
    QualifiedType(const common::SourceRange& range) : Type(range) {}
    
    void addQualifier(std::string_view qualifier) { qualifiers_.push_back(qualifier); }
    const std::vector<std::string_view>& getQualifiers() const { return qualifiers_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::string_view> qualifiers_;
};

// Pointer type
class PointerType : public Type {
public:
    PointerType(const common::SourceRange& range, Type* pointee_type, bool is_const = false,
               bool is_volatile = false)
        : Type(range), pointee_type_(pointee_type), is_const_(is_const), is_volatile_(is_volatile) {}
    
    Type* getPointeeType() const { return pointee_type_; }
    bool isConst() const { return is_const_; }
    bool isVolatile() const { return is_volatile_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* pointee_type_;
    bool is_const_;
    bool is_volatile_;
};

// Array type
class ArrayType : public Type {
public:
    ArrayType(const common::SourceRange& range, Type* element_type, Expression* size = nullptr)
        : Type(range), element_type_(element_type), size_(size) {}
    
    Type* getElementType() const { return element_type_; }
    Expression* getSize() const { return size_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* element_type_;
    Expression* size_;
};

// Function pointer type
class FunctionPointerType : public Type {
public:
    FunctionPointerType(const common::SourceRange& range, Type* return_type)
        : Type(range), return_type_(return_type) {}
    
    Type* getReturnType() const { return return_type_; }
    
    void addParameterType(Type* param_type) { parameter_types_.push_back(param_type); }
    const std::vector<Type*>& getParameterTypes() const { return parameter_types_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* return_type_;
    std::vector<Type*> parameter_types_;
};

// Template instantiation type
class TemplateInstantiationType : public Type {
public:
    TemplateInstantiationType(const common::SourceRange& range, std::string_view template_name)
        : Type(range), template_name_(template_name) {}
    
    std::string_view getTemplateName() const { return template_name_; }
    
    void addArgument(Type* arg) { arguments_.push_back(arg); }
    const std::vector<Type*>& getArguments() const { return arguments_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view template_name_;
    std::vector<Type*> arguments_;
};

// Base class for all statements
class Statement : public ASTNode {
public:
    Statement(const common::SourceRange& range) : ASTNode(range) {}
    virtual ~Statement() = default;
};

// Expression statement
class ExpressionStatement : public Statement {
public:
    ExpressionStatement(const common::SourceRange& range, Expression* expression = nullptr)
        : Statement(range), expression_(expression) {}
    
    Expression* getExpression() const { return expression_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* expression_;
};

// Compound statement (block)
class CompoundStatement : public Statement {
public:
    CompoundStatement(const common::SourceRange& range) : Statement(range) {}
    
    void addStatement(Statement* stmt) { statements_.push_back(stmt); }
    const std::vector<Statement*>& getStatements() const { return statements_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<Statement*> statements_;
};

// If statement
class IfStatement : public Statement {
public:
    IfStatement(const common::SourceRange& range, Expression* condition, Statement* then_stmt,
               Statement* else_stmt = nullptr)
        : Statement(range), condition_(condition), then_stmt_(then_stmt), else_stmt_(else_stmt) {}
    
    Expression* getCondition() const { return condition_; }
    Statement* getThenStatement() const { return then_stmt_; }
    Statement* getElseStatement() const { return else_stmt_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* condition_;
    Statement* then_stmt_;
    Statement* else_stmt_;
};

// While statement
class WhileStatement : public Statement {
public:
    WhileStatement(const common::SourceRange& range, Expression* condition, Statement* body)
        : Statement(range), condition_(condition), body_(body) {}
    
    Expression* getCondition() const { return condition_; }
    Statement* getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* condition_;
    Statement* body_;
};

// For statement
class ForStatement : public Statement {
public:
    enum Kind {
        TRADITIONAL,  // for (init; condition; increment)
        RANGE_BASED   // for (var in range)
    };
    
    // Traditional for loop constructor
    ForStatement(const common::SourceRange& range, Statement* init,
                Expression* condition, Expression* increment, Statement* body)
        : Statement(range), kind_(TRADITIONAL), init_(init), condition_(condition),
          increment_(increment), body_(body), range_expression_(nullptr) {}
    
    // Range-based for loop constructor
    ForStatement(const common::SourceRange& range, std::string_view variable,
                Expression* range_expr, Statement* body, 
                std::optional<std::string_view> index_var = std::nullopt)
        : Statement(range), kind_(RANGE_BASED), variable_(variable), index_variable_(index_var),
          range_expression_(range_expr), body_(body), init_(nullptr), condition_(nullptr), increment_(nullptr) {}
    
    Kind getKind() const { return kind_; }
    
    // Traditional for loop
    Statement* getInit() const { return init_; }
    Expression* getCondition() const { return condition_; }
    Expression* getIncrement() const { return increment_; }
    
    // Range-based for loop
    std::optional<std::string_view> getVariable() const { return variable_; }
    std::optional<std::string_view> getIndexVariable() const { return index_variable_; }
    Expression* getRangeExpression() const { return range_expression_; }
    
    Statement* getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Kind kind_;
    Statement* body_;
    
    // Traditional for loop members
    Statement* init_;
    Expression* condition_;
    Expression* increment_;
    
    // Range-based for loop members
    std::optional<std::string_view> variable_;
    std::optional<std::string_view> index_variable_;
    Expression* range_expression_;
};

// Case statement for switch
class CaseStatement : public Statement {
public:
    CaseStatement(const common::SourceRange& range, Expression* value, Statement* body,
                 bool is_default = false)
        : Statement(range), value_(value), body_(body), is_default_(is_default) {}
    
    Expression* getValue() const { return value_; }
    Statement* getBody() const { return body_; }
    bool isDefault() const { return is_default_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* value_;
    Statement* body_;
    bool is_default_;
};

// Switch statement
class SwitchStatement : public Statement {
public:
    SwitchStatement(const common::SourceRange& range, Expression* expression)
        : Statement(range), expression_(expression) {}
    
    Expression* getExpression() const { return expression_; }
    
    void addCase(CaseStatement* case_stmt) { cases_.push_back(case_stmt); }
    const std::vector<CaseStatement*>& getCases() const { return cases_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* expression_;
    std::vector<CaseStatement*> cases_;
};

// Return statement
class ReturnStatement : public Statement {
public:
    ReturnStatement(const common::SourceRange& range, Expression* value = nullptr)
        : Statement(range), value_(value) {}
    
    Expression* getValue() const { return value_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* value_;
};

// Break statement
class BreakStatement : public Statement {
public:
    BreakStatement(const common::SourceRange& range) : Statement(range) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Continue statement
class ContinueStatement : public Statement {
public:
    ContinueStatement(const common::SourceRange& range) : Statement(range) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Assembly statement
class AssemblyStatement : public Statement {
public:
    AssemblyStatement(const common::SourceRange& range, std::string_view assembly_code)
        : Statement(range), assembly_code_(assembly_code) {}
    
    std::string_view getAssemblyCode() const { return assembly_code_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view assembly_code_;
};

// Try-catch statement
class TryStatement : public Statement {
public:
    TryStatement(const common::SourceRange& range, Statement* try_block, Statement* catch_block)
        : Statement(range), try_block_(try_block), catch_block_(catch_block) {}
    
    Statement* getTryBlock() const { return try_block_; }
    Statement* getCatchBlock() const { return catch_block_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Statement* try_block_;
    Statement* catch_block_;
};

// Throw statement
class ThrowStatement : public Statement {
public:
    ThrowStatement(const common::SourceRange& range, Expression* expression)
        : Statement(range), expression_(expression) {}
    
    Expression* getExpression() const { return expression_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* expression_;
};

// Assert statement
class AssertStatement : public Statement {
public:
    AssertStatement(const common::SourceRange& range, Expression* condition)
        : Statement(range), condition_(condition) {}
    
    Expression* getCondition() const { return condition_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* condition_;
};

// Base class for all expressions
class Expression : public ASTNode {
public:
    Expression(const common::SourceRange& range) : ASTNode(range) {}
    virtual ~Expression() = default;
};

// Binary expression
class BinaryExpression : public Expression {
public:
    BinaryExpression(const common::SourceRange& range, lexer::TokenType operator_type,
                    Expression* left, Expression* right)
        : Expression(range), operator_type_(operator_type), left_(left), right_(right) {}
    
    lexer::TokenType getOperator() const { return operator_type_; }
    Expression* getLeft() const { return left_; }
    Expression* getRight() const { return right_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    lexer::TokenType operator_type_;
    Expression* left_;
    Expression* right_;
};

// Unary expression
class UnaryExpression : public Expression {
public:
    enum Kind {
        PREFIX,
        POSTFIX
    };
    
    UnaryExpression(const common::SourceRange& range, lexer::TokenType operator_type,
                   Expression* operand, Kind kind = PREFIX)
        : Expression(range), operator_type_(operator_type), operand_(operand), kind_(kind) {}
    
    lexer::TokenType getOperator() const { return operator_type_; }
    Expression* getOperand() const { return operand_; }
    Kind getKind() const { return kind_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    lexer::TokenType operator_type_;
    Expression* operand_;
    Kind kind_;
};

// Assignment expression
class AssignmentExpression : public Expression {
public:
    AssignmentExpression(const common::SourceRange& range, lexer::TokenType operator_type,
                        Expression* left, Expression* right)
        : Expression(range), operator_type_(operator_type), left_(left), right_(right) {}
    
    lexer::TokenType getOperator() const { return operator_type_; }
    Expression* getLeft() const { return left_; }
    Expression* getRight() const { return right_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    lexer::TokenType operator_type_;
    Expression* left_;
    Expression* right_;
};

// Conditional expression (ternary)
class ConditionalExpression : public Expression {
public:
    ConditionalExpression(const common::SourceRange& range, Expression* condition,
                         Expression* true_expr, Expression* false_expr)
        : Expression(range), condition_(condition), true_expr_(true_expr), false_expr_(false_expr) {}
    
    Expression* getCondition() const { return condition_; }
    Expression* getTrueExpression() const { return true_expr_; }
    Expression* getFalseExpression() const { return false_expr_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* condition_;
    Expression* true_expr_;
    Expression* false_expr_;
};

// Function call expression
class CallExpression : public Expression {
public:
    CallExpression(const common::SourceRange& range, Expression* function)
        : Expression(range), function_(function) {}
    
    Expression* getFunction() const { return function_; }
    
    void addArgument(Expression* arg) { arguments_.push_back(arg); }
    const std::vector<Expression*>& getArguments() const { return arguments_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* function_;
    std::vector<Expression*> arguments_;
};

// Member access expression
class MemberExpression : public Expression {
public:
    MemberExpression(const common::SourceRange& range, Expression* object, std::string_view member)
        : Expression(range), object_(object), member_(member) {}
    
    Expression* getObject() const { return object_; }
    std::string_view getMember() const { return member_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* object_;
    std::string_view member_;
};

// Array subscript expression
class SubscriptExpression : public Expression {
public:
    SubscriptExpression(const common::SourceRange& range, Expression* array, Expression* index)
        : Expression(range), array_(array), index_(index) {}
    
    Expression* getArray() const { return array_; }
    Expression* getIndex() const { return index_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* array_;
    Expression* index_;
};

// Cast expression
class CastExpression : public Expression {
public:
    CastExpression(const common::SourceRange& range, Type* target_type, Expression* expression)
        : Expression(range), target_type_(target_type), expression_(expression) {}
    
    Type* getTargetType() const { return target_type_; }
    Expression* getExpression() const { return expression_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* target_type_;
    Expression* expression_;
};

// Sizeof expression
class SizeofExpression : public Expression {
public:
    SizeofExpression(const common::SourceRange& range, Type* type)
        : Expression(range), type_(type) {}
    
    Type* getType() const { return type_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Type* type_;
};

// Typeof expression
class TypeofExpression : public Expression {
public:
    TypeofExpression(const common::SourceRange& range, Expression* expression)
        : Expression(range), expression_(expression) {}
    
    Expression* getExpression() const { return expression_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* expression_;
};

// Identifier expression
class IdentifierExpression : public Expression {
public:
    IdentifierExpression(const common::SourceRange& range, std::string_view name)
        : Expression(range), name_(name) {}
    
    std::string_view getName() const { return name_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view name_;
};

// Literal expressions
class IntegerLiteral : public Expression {
public:
    IntegerLiteral(const common::SourceRange& range, long long value)
        : Expression(range), value_(value) {}
    
    long long getValue() const { return value_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    long long value_;
};

class FloatLiteral : public Expression {
public:
    FloatLiteral(const common::SourceRange& range, double value)
        : Expression(range), value_(value) {}
    
    double getValue() const { return value_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    double value_;
};

class StringLiteral : public Expression {
public:
    StringLiteral(const common::SourceRange& range, std::string_view value)
        : Expression(range), value_(value) {}
    
    std::string_view getValue() const { return value_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view value_;
};

class CharacterLiteral : public Expression {
public:
    CharacterLiteral(const common::SourceRange& range, char value)
        : Expression(range), value_(value) {}
    
    char getValue() const { return value_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    char value_;
};

class DataLiteral : public Expression {
public:
    DataLiteral(const common::SourceRange& range, std::string_view bit_data)
        : Expression(range), bit_data_(bit_data) {}
    
    std::string_view getBitData() const { return bit_data_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view bit_data_;
};

// Array literal
class ArrayLiteral : public Expression {
public:
    ArrayLiteral(const common::SourceRange& range) : Expression(range) {}
    
    void addElement(Expression* element) { elements_.push_back(element); }
    const std::vector<Expression*>& getElements() const { return elements_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<Expression*> elements_;
};

// Dictionary literal
class DictionaryLiteral : public Expression {
public:
    struct KeyValuePair {
        Expression* key;
        Expression* value;
        
        KeyValuePair(Expression* k, Expression* v) : key(k), value(v) {}
    };
    
    DictionaryLiteral(const common::SourceRange& range) : Expression(range) {}
    
    void addPair(Expression* key, Expression* value) { 
        pairs_.emplace_back(key, value);
    }
    const std::vector<KeyValuePair>& getPairs() const { return pairs_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::vector<KeyValuePair> pairs_;
};

// i-string literal
class IStringLiteral : public Expression {
public:
    IStringLiteral(const common::SourceRange& range, std::string_view format)
        : Expression(range), format_(format) {}
    
    std::string_view getFormat() const { return format_; }
    
    void addExpression(Expression* expr) { expressions_.push_back(expr); }
    const std::vector<Expression*>& getExpressions() const { return expressions_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    std::string_view format_;
    std::vector<Expression*> expressions_;
};

// Array comprehension
class ArrayComprehension : public Expression {
public:
    ArrayComprehension(const common::SourceRange& range, Expression* element_expr,
                      std::string_view variable, Expression* iterable, 
                      Expression* condition = nullptr)
        : Expression(range), element_expr_(element_expr), variable_(variable),
          iterable_(iterable), condition_(condition) {}
    
    Expression* getElementExpression() const { return element_expr_; }
    std::string_view getVariable() const { return variable_; }
    Expression* getIterable() const { return iterable_; }
    Expression* getCondition() const { return condition_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Expression* element_expr_;
    std::string_view variable_;
    Expression* iterable_;
    Expression* condition_;
};

// Anonymous function/block
class AnonymousFunction : public Expression {
public:
    AnonymousFunction(const common::SourceRange& range, Statement* body)
        : Expression(range), body_(body) {}
    
    Statement* getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;

private:
    Statement* body_;
};

// Visitor interface for traversing the AST
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Program
    virtual void visit(Program& node) = 0;
    
    // Declarations
    virtual void visit(ImportDeclaration& node) = 0;
    virtual void visit(UsingDeclaration& node) = 0;
    virtual void visit(NamespaceDeclaration& node) = 0;
    virtual void visit(ObjectDeclaration& node) = 0;
    virtual void visit(StructDeclaration& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(TemplateDeclaration& node) = 0;
    virtual void visit(OperatorDeclaration& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(TemplateParameter& node) = 0;
    virtual void visit(TemplateParameterList& node) = 0;
    virtual void visit(Parameter& node) = 0;
    virtual void visit(ParameterList& node) = 0;
    
    // Types
    virtual void visit(BuiltinType& node) = 0;
    virtual void visit(DataType& node) = 0;
    virtual void visit(NamedType& node) = 0;
    virtual void visit(QualifiedType& node) = 0;
    virtual void visit(PointerType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(FunctionPointerType& node) = 0;
    virtual void visit(TemplateInstantiationType& node) = 0;
    
    // Statements
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(CompoundStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(CaseStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(AssemblyStatement& node) = 0;
    virtual void visit(TryStatement& node) = 0;
    virtual void visit(ThrowStatement& node) = 0;
    virtual void visit(AssertStatement& node) = 0;
    
    // Expressions
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ConditionalExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(MemberExpression& node) = 0;
    virtual void visit(SubscriptExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(SizeofExpression& node) = 0;
    virtual void visit(TypeofExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(FloatLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(CharacterLiteral& node) = 0;
    virtual void visit(DataLiteral& node) = 0;
    virtual void visit(ArrayLiteral& node) = 0;
    virtual void visit(DictionaryLiteral& node) = 0;
    virtual void visit(IStringLiteral& node) = 0;
    virtual void visit(ArrayComprehension& node) = 0;
    virtual void visit(AnonymousFunction& node) = 0;
};

} // namespace parser
} // namespace flux