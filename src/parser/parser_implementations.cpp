/**
 * @file parser_implementations.cpp
 * @brief Implementations for virtual methods in parser-related classes
 */

#include "parser/statements.hpp"
#include "parser/expressions.hpp"
#include "parser/ast.hpp"
#include <sstream>

namespace flux {
    // Expression toString() implementations
    std::string IntegerLiteral::toString() const {
        std::stringstream ss;
        ss << "IntegerLiteral(" << value << ")";
        return ss.str();
    }

    std::string FloatLiteral::toString() const {
        std::stringstream ss;
        ss << "FloatLiteral(" << value << ")";
        return ss.str();
    }

    std::string BooleanLiteral::toString() const {
        return "BooleanLiteral(" + std::string(value ? "true" : "false") + ")";
    }

    std::string CharLiteral::toString() const {
        std::stringstream ss;
        ss << "CharLiteral('" << value << "')";
        return ss.str();
    }

    std::string StringLiteral::toString() const {
        std::stringstream ss;
        ss << "StringLiteral(\"" << value << "\")";
        return ss.str();
    }

    std::string NullLiteral::toString() const {
        return "NullLiteral(nullptr)";
    }

    std::string ArrayLiteral::toString() const {
        std::stringstream ss;
        ss << "ArrayLiteral(size=" << elements.size() << ")";
        return ss.str();
    }

    std::string CharArrayLiteral::toString() const {
        std::stringstream ss;
        ss << "CharArrayLiteral(size=" << chars.size() << ")";
        return ss.str();
    }

    std::string BinaryExpr::toString() const {
        std::stringstream ss;
        ss << "BinaryExpr(" << op.lexeme << ")";
        return ss.str();
    }

    std::string UnaryExpr::toString() const {
        std::stringstream ss;
        ss << "UnaryExpr(" << op.lexeme << ")";
        return ss.str();
    }

    std::string GroupingExpr::toString() const {
        return "GroupingExpr()";
    }

    std::string VariableExpr::toString() const {
        return "VariableExpr(" + name.lexeme + ")";
    }

    std::string AssignExpr::toString() const {
        return "AssignExpr(" + name.lexeme + ")";
    }

    std::string LogicalExpr::toString() const {
        std::stringstream ss;
        ss << "LogicalExpr(" << op.lexeme << ")";
        return ss.str();
    }

    std::string CallExpr::toString() const {
        return "CallExpr()";
    }

    std::string ArrayAccessExpr::toString() const {
        return "ArrayAccessExpr()";
    }

    std::string MemberAccessExpr::toString() const {
        std::stringstream ss;
        ss << "MemberAccessExpr(" << op.lexeme << " " << member.lexeme << ")";
        return ss.str();
    }

    std::string InterpolatedStringExpr::toString() const {
        return "InterpolatedStringExpr()";
    }

    std::string TypeCastExpr::toString() const {
        return "TypeCastExpr()";
    }

    std::string AddressOfExpr::toString() const {
        return "AddressOfExpr()";
    }

    std::string DereferenceExpr::toString() const {
        return "DereferenceExpr()";
    }

    // Type toString() implementations
    std::string PrimitiveType::toString() const {
        std::string kindName;
        switch (kind) {
            case PrimitiveKind::INT: kindName = "int"; break;
            case PrimitiveKind::FLOAT: kindName = "float"; break;
            case PrimitiveKind::CHAR: kindName = "char"; break;
            case PrimitiveKind::BOOL: kindName = "bool"; break;
            case PrimitiveKind::VOID: kindName = "void"; break;
            case PrimitiveKind::STRING: kindName = "string"; break;
            default: kindName = "unknown"; break;
        }
        
        std::stringstream ss;
        ss << kindName;
        if (bitWidth > 0) {
            ss << "{" << bitWidth << "}";
        }
        return ss.str();
    }

    std::string ArrayType::toString() const {
        std::stringstream ss;
        ss << elementType->toString() << "[" << (size ? std::to_string(*size) : "") << "]";
        return ss.str();
    }

    std::string PointerType::toString() const {
        return pointeeType->toString() + "*";
    }

    std::string ClassType::toString() const {
        return name;
    }

    // Statement toString() implementations
    std::string ExpressionStmt::toString() const {
        return "ExpressionStmt()";
    }

    std::string BlockStmt::toString() const {
        std::stringstream ss;
        ss << "BlockStmt(size=" << statements.size() 
           << ", volatile=" << (isVolatile ? "true" : "false") << ")";
        return ss.str();
    }

    std::string VarDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "VarDeclarationStmt(" << name.lexeme 
           << ", volatile=" << (isVolatile ? "true" : "false") << ")";
        return ss.str();
    }

    std::string IfStmt::toString() const {
        return "IfStmt()";
    }

    std::string WhileStmt::toString() const {
        return "WhileStmt()";
    }

    std::string ForStmt::toString() const {
        return "ForStmt()";
    }

    std::string WhenStmt::toString() const {
        std::stringstream ss;
        ss << "WhenStmt(volatile=" << (isVolatile ? "true" : "false") 
           << ", async=" << (isAsync ? "true" : "false") << ")";
        return ss.str();
    }

    std::string FunctionDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "FunctionDeclarationStmt(" << name.lexeme 
           << ", volatile=" << (isVolatile ? "true" : "false")
           << ", async=" << (isAsync ? "true" : "false") << ")";
        return ss.str();
    }

    std::string ClassDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "ClassDeclarationStmt(" << name.lexeme 
           << ", members=" << members.size() << ")";
        return ss.str();
    }

    std::string ObjectDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "ObjectDeclarationStmt(" << name.lexeme 
           << ", members=" << members.size() << ")";
        return ss.str();
    }

    std::string NamespaceDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "NamespaceDeclarationStmt(" << name.lexeme 
           << ", declarations=" << declarations.size() << ")";
        return ss.str();
    }

    std::string StructDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "StructDeclarationStmt(" << name.lexeme 
           << ", fields=" << fields.size() << ")";
        return ss.str();
    }

    std::string OperatorDeclarationStmt::toString() const {
        std::stringstream ss;
        ss << "OperatorDeclarationStmt(op=" << op.lexeme << ")";
        return ss.str();
    }

    std::string ReturnStmt::toString() const {
        return "ReturnStmt()";
    }

    std::string AsmStmt::toString() const {
        std::stringstream ss;
        ss << "AsmStmt(code=" << asmCode << ")";
        return ss.str();
    }

    std::string BreakStmt::toString() const {
        return "BreakStmt()";
    }

    std::string ContinueStmt::toString() const {
        return "ContinueStmt()";
    }

	std::string LockStmt::toString() const {
	    std::stringstream ss;
	    
	    // Convert lock type to string
	    std::string typeStr;
	    switch (type) {
		case LockType::LOCK: typeStr = "LOCK"; break;
		case LockType::PRE_LOCK: typeStr = "PRE_LOCK"; break;
		case LockType::POST_LOCK: typeStr = "POST_LOCK"; break;
	    }
	    
	    ss << "LockStmt(type=" << typeStr 
	       << ", scopes=" << scopes.size() 
	       << ", has_body=" << (body ? "true" : "false") << ")";
	    
	    return ss.str();
	}
}
