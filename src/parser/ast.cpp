#include "ast.h"

namespace flux {
namespace parser {

// Program
void Program::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Declarations
void ImportDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UsingDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void NamespaceDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ObjectDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StructDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TemplateDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void OperatorDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VariableDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TemplateParameter::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TemplateParameterList::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void Parameter::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ParameterList::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Types
void BuiltinType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DataType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void NamedType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void QualifiedType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PointerType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArrayType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionPointerType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TemplateInstantiationType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Statements
void ExpressionStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CompoundStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WhileStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ForStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SwitchStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CaseStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BreakStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ContinueStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssemblyStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TryStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ThrowStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssertStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Expressions
void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssignmentExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ConditionalExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CallExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void MemberExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SubscriptExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CastExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SizeofExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void TypeofExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IntegerLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FloatLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StringLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CharacterLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DataLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArrayLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DictionaryLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IStringLiteral::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ArrayComprehension::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AnonymousFunction::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

} // namespace parser
} // namespace flux