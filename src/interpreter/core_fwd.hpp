#ifndef FLUX_CORE_FWD_HPP
#define FLUX_CORE_FWD_HPP

#include <memory>
#include <vector>
#include <string>

namespace flux {

// Forward declarations
class Expression;
class Statement;
class Type;
class Value;
class Object;
class Function;
class ArrayObject;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using TypePtr = std::unique_ptr<Type>;
using ObjectPtr = std::shared_ptr<Object>;
using FunctionPtr = std::shared_ptr<Function>;

using Integer = int64_t;
using Float = double;
using String = std::string;
using Boolean = bool;

// Statement types enum
enum class StatementType {
    Block,
    Expression,
    When,
    Object,
    Class,
    Namespace,
    Operator,
    Variable,
    Function,
    Return
};

// Expression types enum
enum class ExpressionType {
    Binary,
    Unary,
    Call,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BooleanLiteral
};

} // namespace flux

#endif
