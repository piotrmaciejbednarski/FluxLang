#include "ast.h"

namespace flux {
namespace parser {

// Expression clone methods

std::unique_ptr<Expr> LiteralExpr::clone() const {
    return std::make_unique<LiteralExpr>(token, value);
}

std::unique_ptr<Expr> VariableExpr::clone() const {
    return std::make_unique<VariableExpr>(token);
}

std::unique_ptr<Expr> UnaryExpr::clone() const {
    return std::make_unique<UnaryExpr>(op, right->clone(), prefix, range);
}

std::unique_ptr<Expr> BinaryExpr::clone() const {
    return std::make_unique<BinaryExpr>(left->clone(), op, right->clone(), range);
}

std::unique_ptr<Expr> GroupExpr::clone() const {
    return std::make_unique<GroupExpr>(expression->clone(), range);
}

std::unique_ptr<Expr> CallExpr::clone() const {
    std::vector<std::unique_ptr<Expr>> clonedArgs;
    for (const auto& arg : arguments) {
        clonedArgs.push_back(arg->clone());
    }
    return std::make_unique<CallExpr>(callee->clone(), paren, std::move(clonedArgs), range);
}

std::unique_ptr<Expr> GetExpr::clone() const {
    return std::make_unique<GetExpr>(object->clone(), name, range);
}

std::unique_ptr<Expr> SetExpr::clone() const {
    return std::make_unique<SetExpr>(object->clone(), name, value->clone(), range);
}

std::unique_ptr<Expr> ArrayExpr::clone() const {
    std::vector<std::unique_ptr<Expr>> clonedElements;
    for (const auto& element : elements) {
        clonedElements.push_back(element->clone());
    }
    return std::make_unique<ArrayExpr>(std::move(clonedElements), range);
}

std::unique_ptr<Expr> SubscriptExpr::clone() const {
    return std::make_unique<SubscriptExpr>(array->clone(), index->clone(), range);
}

std::unique_ptr<Expr> TernaryExpr::clone() const {
    return std::make_unique<TernaryExpr>(
        condition->clone(), thenExpr->clone(), elseExpr->clone(), range);
}

std::unique_ptr<Expr> IStringExpr::clone() const {
    std::vector<std::string_view> clonedTextParts = textParts;
    std::vector<std::unique_ptr<Expr>> clonedExprParts;
    for (const auto& expr : exprParts) {
        clonedExprParts.push_back(expr->clone());
    }
    return std::make_unique<IStringExpr>(std::move(clonedTextParts), std::move(clonedExprParts), range);
}

std::unique_ptr<Expr> CastExpr::clone() const {
    return std::make_unique<CastExpr>(
        static_cast<std::unique_ptr<TypeExpr>>(targetType->clone()),
        expression->clone(), range);
}

std::unique_ptr<Expr> AssignExpr::clone() const {
    return std::make_unique<AssignExpr>(target->clone(), op, value->clone(), range);
}

// Statement clone methods

std::unique_ptr<Stmt> ExprStmt::clone() const {
    return std::make_unique<ExprStmt>(expression->clone(), range);
}

std::unique_ptr<Stmt> BlockStmt::clone() const {
    std::vector<std::unique_ptr<Stmt>> clonedStmts;
    for (const auto& stmt : statements) {
        clonedStmts.push_back(stmt->clone());
    }
    return std::make_unique<BlockStmt>(std::move(clonedStmts), range);
}

std::unique_ptr<Stmt> VarStmt::clone() const {
    return std::make_unique<VarStmt>(
        name, 
        type ? static_cast<std::unique_ptr<TypeExpr>>(type->clone()) : nullptr,
        initializer ? initializer->clone() : nullptr, 
        range);
}

std::unique_ptr<Stmt> IfStmt::clone() const {
    return std::make_unique<IfStmt>(
        condition->clone(), 
        thenBranch->clone(), 
        elseBranch ? elseBranch->clone() : nullptr, 
        range);
}

std::unique_ptr<Stmt> WhileStmt::clone() const {
    return std::make_unique<WhileStmt>(condition->clone(), body->clone(), range);
}

std::unique_ptr<Stmt> ForStmt::clone() const {
    return std::make_unique<ForStmt>(
        initializer ? initializer->clone() : nullptr,
        condition ? condition->clone() : nullptr,
        increment ? increment->clone() : nullptr,
        body->clone(),
        range);
}

std::unique_ptr<Stmt> ReturnStmt::clone() const {
    return std::make_unique<ReturnStmt>(
        keyword, value ? value->clone() : nullptr, range);
}

std::unique_ptr<Stmt> BreakStmt::clone() const {
    return std::make_unique<BreakStmt>(keyword, range);
}

std::unique_ptr<Stmt> ContinueStmt::clone() const {
    return std::make_unique<ContinueStmt>(keyword, range);
}

std::unique_ptr<Stmt> TryStmt::clone() const {
    std::unique_ptr<Stmt> clonedTryBlock = tryBlock->clone();
    std::vector<TryStmt::CatchClause> clonedCatchClauses;
    
    for (const auto& catchClause : catchClauses) {
        clonedCatchClauses.emplace_back(
            catchClause.exceptionType ? 
                static_cast<std::unique_ptr<TypeExpr>>(catchClause.exceptionType->clone()) : nullptr,
            catchClause.handler->clone());
    }
    
    return std::make_unique<TryStmt>(
        std::move(clonedTryBlock), std::move(clonedCatchClauses), range);
}

std::unique_ptr<Stmt> MatchStmt::clone() const {
    std::unique_ptr<Expr> clonedValue = value->clone();
    std::vector<MatchStmt::CaseClause> clonedCases;
    
    for (const auto& caseClause : cases) {
        clonedCases.emplace_back(caseClause.pattern->clone(), caseClause.body->clone());
    }
    
    return std::make_unique<MatchStmt>(
        std::move(clonedValue), 
        std::move(clonedCases),
        defaultCase ? defaultCase->clone() : nullptr,
        range);
}

// Declaration clone methods

std::unique_ptr<Decl> FunctionDecl::clone() const {
    std::vector<FunctionDecl::Parameter> clonedParameters;
    
    for (const auto& param : parameters) {
        clonedParameters.emplace_back(
            param.name,
            param.type ? static_cast<std::unique_ptr<TypeExpr>>(param.type->clone()) : nullptr);
    }
    
    return std::make_unique<FunctionDecl>(
        name,
        std::move(clonedParameters),
        returnType ? static_cast<std::unique_ptr<TypeExpr>>(returnType->clone()) : nullptr,
        body->clone(),
        range);
}

std::unique_ptr<Decl> VarDecl::clone() const {
    return std::make_unique<VarDecl>(
        name,
        type ? static_cast<std::unique_ptr<TypeExpr>>(type->clone()) : nullptr,
        initializer ? initializer->clone() : nullptr,
        isConst,
        range);
}

std::unique_ptr<Decl> ClassDecl::clone() const {
    std::vector<std::string_view> clonedBaseClasses = baseClasses;
    std::vector<std::string_view> clonedExclusions = exclusions;
    std::vector<std::unique_ptr<Decl>> clonedMembers;
    
    for (const auto& member : members) {
        clonedMembers.push_back(member->clone());
    }
    
    return std::make_unique<ClassDecl>(
        name,
        std::move(clonedBaseClasses),
        std::move(clonedExclusions),
        std::move(clonedMembers),
        range);
}

std::unique_ptr<Decl> ObjectDecl::clone() const {
    std::vector<std::string_view> clonedBaseObjects = baseObjects;
    std::vector<std::unique_ptr<Decl>> clonedMembers;
    
    for (const auto& member : members) {
        clonedMembers.push_back(member->clone());
    }
    
    return std::make_unique<ObjectDecl>(
        name,
        std::move(clonedBaseObjects),
        std::move(clonedMembers),
        range);
}

std::unique_ptr<Decl> StructDecl::clone() const {
    std::vector<StructDecl::Field> clonedFields;
    
    for (const auto& field : fields) {
        clonedFields.emplace_back(
            field.name,
            static_cast<std::unique_ptr<TypeExpr>>(field.type->clone()));
    }
    
    return std::make_unique<StructDecl>(
        name,
        std::move(clonedFields),
        range);
}

std::unique_ptr<Decl> NamespaceDecl::clone() const {
    std::vector<std::unique_ptr<Decl>> clonedDeclarations;
    
    for (const auto& decl : declarations) {
        clonedDeclarations.push_back(decl->clone());
    }
    
    return std::make_unique<NamespaceDecl>(
        name,
        std::move(clonedDeclarations),
        range);
}

std::unique_ptr<Decl> ImportDecl::clone() const {
    return std::make_unique<ImportDecl>(path, alias, range);
}

std::unique_ptr<Decl> OperatorDecl::clone() const {
    std::vector<OperatorDecl::Parameter> clonedParameters;
    
    for (const auto& param : parameters) {
        clonedParameters.emplace_back(
            param.name,
            static_cast<std::unique_ptr<TypeExpr>>(param.type->clone()));
    }
    
    return std::make_unique<OperatorDecl>(
        op,
        std::move(clonedParameters),
        static_cast<std::unique_ptr<TypeExpr>>(returnType->clone()),
        body->clone(),
        range);
}

std::unique_ptr<Decl> UsingDecl::clone() const {
    std::vector<std::string_view> clonedPath = path;
    return std::make_unique<UsingDecl>(std::move(clonedPath), range);
}

std::unique_ptr<Decl> TypeDecl::clone() const {
    return std::make_unique<TypeDecl>(
        name,
        static_cast<std::unique_ptr<TypeExpr>>(underlyingType->clone()),
        range);
}

std::unique_ptr<Decl> DataDecl::clone() const {
    return std::make_unique<DataDecl>(name, bits, isSigned, range);
}

std::unique_ptr<Decl> EnumDecl::clone() const {
    std::vector<EnumDecl::Member> clonedMembers;
    
    for (const auto& member : members) {
        clonedMembers.emplace_back(
            member.name,
            member.value ? member.value->clone() : nullptr);
    }
    
    return std::make_unique<EnumDecl>(
        name,
        std::move(clonedMembers),
        range);
}

std::unique_ptr<Decl> TemplateDecl::clone() const {
    std::vector<TemplateDecl::Parameter> clonedParameters;
    
    for (const auto& param : parameters) {
        if (param.kind == TemplateDecl::Parameter::Kind::TYPE) {
            clonedParameters.emplace_back(param.name, param.kind);
        } else {
            clonedParameters.emplace_back(
                param.name,
                param.kind,
                static_cast<std::unique_ptr<TypeExpr>>(param.type->clone()));
        }
    }
    
    return std::make_unique<TemplateDecl>(
        std::move(clonedParameters),
        declaration->clone(),
        range);
}

std::unique_ptr<Decl> AsmDecl::clone() const {
    return std::make_unique<AsmDecl>(code, range);
}

// Type expression clone methods

std::unique_ptr<TypeExpr> NamedTypeExpr::clone() const {
    return std::make_unique<NamedTypeExpr>(name, range);
}

std::unique_ptr<TypeExpr> ArrayTypeExpr::clone() const {
    return std::make_unique<ArrayTypeExpr>(
        static_cast<std::unique_ptr<TypeExpr>>(elementType->clone()),
        sizeExpr ? sizeExpr->clone() : nullptr,
        range);
}

std::unique_ptr<TypeExpr> PointerTypeExpr::clone() const {
    return std::make_unique<PointerTypeExpr>(
        static_cast<std::unique_ptr<TypeExpr>>(pointeeType->clone()),
        range);
}

std::unique_ptr<TypeExpr> FunctionTypeExpr::clone() const {
    std::vector<std::unique_ptr<TypeExpr>> clonedParameterTypes;
    
    for (const auto& paramType : parameterTypes) {
        clonedParameterTypes.push_back(
            static_cast<std::unique_ptr<TypeExpr>>(paramType->clone()));
    }
    
    return std::make_unique<FunctionTypeExpr>(
        std::move(clonedParameterTypes),
        static_cast<std::unique_ptr<TypeExpr>>(returnType->clone()),
        range);
}

std::unique_ptr<TypeExpr> DataTypeExpr::clone() const {
    return std::make_unique<DataTypeExpr>(bits, isSigned, range);
}

std::unique_ptr<Expr> SizeOfExpr::clone() const {
    return std::make_unique<SizeOfExpr>(
        std::unique_ptr<TypeExpr>(dynamic_cast<TypeExpr*>(targetType->clone().release())),
        range);
}

std::unique_ptr<Expr> TypeOfExpr::clone() const {
    return std::make_unique<TypeOfExpr>(expression->clone(), range);
}

std::unique_ptr<Expr> OpExpr::clone() const {
    return std::make_unique<OpExpr>(
        left->clone(),
        operatorName,
        right->clone(),
        range);
}

} // namespace parser
} // namespace flux