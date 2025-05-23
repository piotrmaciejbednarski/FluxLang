#pragma once

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <variant>

#include "../common/source.h"
#include "../common/arena.h"

namespace flux {
namespace type {

// Forward declarations
class Type;
class BuiltinType;
class DataType;
class ArrayType;
class PointerType;
class FunctionType;
class ObjectType;
class StructType;
class EnumType;
class TemplateType;

// Type kinds for runtime type identification
enum class TypeKind {
    // Built-in types
    VOID,           // void
    BANG_VOID,      // !void (unknown return type)
    
    // Data types (raw bit storage)
    DATA,           // data{N} - variable bit width
    
    // Built-in derived types
    INTEGER,        // signed data{32} int
    UNSIGNED_INTEGER, // unsigned data{32} uint
    FLOAT,          // signed data{64} float
    UNSIGNED_FLOAT, // unsigned data{64} ufloat
    STRING,         // signed data{8}[] string
    BOOLEAN,        // signed data{1} bool
    
    // Composite types
    ARRAY,          // T[] or T[N]
    POINTER,        // T*
    FUNCTION,       // (T1, T2, ...) -> R
    
    // User-defined types
    OBJECT,         // object
    STRUCT,         // struct
    ENUM,           // enum
    
    // Template types
    TEMPLATE,       // template<T> ...
    TEMPLATE_PARAMETER, // T in template<T>
    
    // Special types
    ERROR,          // Error type for type checking failures
    UNKNOWN         // Unknown type (for inference)
};

// Base class for all types in the Flux type system
class Type {
public:
    explicit Type(TypeKind kind) : kind_(kind) {}
    virtual ~Type() = default;
    
    // Get the type kind
    TypeKind kind() const { return kind_; }
    
    // Get human-readable name of the type
    virtual std::string name() const = 0;
    
    // Get the size of this type in bits (0 if unknown/variable)
    virtual size_t sizeInBits() const = 0;
    
    // Get the alignment requirement in bits
    virtual size_t alignmentInBits() const = 0;
    
    // Check if this type is equal to another type
    virtual bool equals(const Type& other) const = 0;
    
    // Check if this type is assignable from another type
    virtual bool isAssignableFrom(const Type& other) const = 0;
    
    // Check if this type can be implicitly converted to another type
    virtual bool isImplicitlyConvertibleTo(const Type& other) const = 0;
    
    // Check if this is a const type
    virtual bool isConst() const { return false; }
    
    // Check if this is a volatile type
    virtual bool isVolatile() const { return false; }
    
    // Check if this is a signed type
    virtual bool isSigned() const { return false; }
    
    // Check if this is a primitive type (data, int, float, etc.)
    virtual bool isPrimitive() const { return false; }
    
    // Check if this is a pointer type
    virtual bool isPointer() const { return false; }
    
    // Check if this is an array type
    virtual bool isArray() const { return false; }
    
    // Check if this is a function type
    virtual bool isFunction() const { return false; }
    
    // Check if this is an object type
    virtual bool isObject() const { return false; }
    
    // Check if this is a struct type
    virtual bool isStruct() const { return false; }
    
    // Check if this is an enum type
    virtual bool isEnum() const { return false; }
    
    // Check if this is a template type
    virtual bool isTemplate() const { return false; }
    
    // Clone this type
    virtual std::unique_ptr<Type> clone() const = 0;

protected:
    TypeKind kind_;
};

// Built-in type (void, !void, etc.)
class BuiltinType : public Type {
public:
    BuiltinType(TypeKind kind, std::string_view name) 
        : Type(kind), name_(name) {}
    
    std::string name() const override { return std::string(name_); }
    
    size_t sizeInBits() const override {
        switch (kind_) {
            case TypeKind::VOID:
            case TypeKind::BANG_VOID:
                return 0;
            default:
                return 0;
        }
    }
    
    size_t alignmentInBits() const override { return sizeInBits(); }
    
    bool equals(const Type& other) const override {
        return other.kind() == kind_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other);
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return equals(other);
    }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<BuiltinType>(kind_, name_);
    }

private:
    std::string_view name_;
};

// Data type (raw bit storage)
class DataType : public Type {
public:
    DataType(int64_t bits, bool isSigned, bool isVolatile = false, size_t alignment = 0)
        : Type(TypeKind::DATA), bits_(bits), isSigned_(isSigned), 
          isVolatile_(isVolatile), alignment_(alignment) {}
    
    std::string name() const override {
        std::string result = isSigned_ ? "signed " : "unsigned ";
        result += "data{" + std::to_string(bits_) + "}";
        if (isVolatile_) result = "volatile " + result;
        return result;
    }
    
    size_t sizeInBits() const override { return static_cast<size_t>(bits_); }
    
    size_t alignmentInBits() const override { 
        return alignment_ > 0 ? alignment_ : sizeInBits(); 
    }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::DATA) return false;
        auto& otherData = static_cast<const DataType&>(other);
        return bits_ == otherData.bits_ && 
               isSigned_ == otherData.isSigned_ &&
               isVolatile_ == otherData.isVolatile_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        if (equals(other)) return true;
        
        // Can assign from compatible data types
        if (other.kind() == TypeKind::DATA) {
            auto& otherData = static_cast<const DataType&>(other);
            // Allow assignment if target can hold the value
            return bits_ >= otherData.bits_ && isSigned_ == otherData.isSigned_;
        }
        
        return false;
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        if (equals(other)) return true;
        
        // Implicit conversion to larger or same-size compatible types
        if (other.kind() == TypeKind::DATA) {
            auto& otherData = static_cast<const DataType&>(other);
            return bits_ <= otherData.bits_ && isSigned_ == otherData.isSigned_;
        }
        
        return false;
    }
    
    bool isVolatile() const override { return isVolatile_; }
    bool isSigned() const override { return isSigned_; }
    bool isPrimitive() const override { return true; }
    
    int64_t bits() const { return bits_; }
    size_t customAlignment() const { return alignment_; }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<DataType>(bits_, isSigned_, isVolatile_, alignment_);
    }

private:
    int64_t bits_;
    bool isSigned_;
    bool isVolatile_;
    size_t alignment_;
};

// Array type (T[] or T[N])
class ArrayType : public Type {
public:
    ArrayType(std::shared_ptr<Type> elementType, std::optional<size_t> size = std::nullopt)
        : Type(TypeKind::ARRAY), elementType_(std::move(elementType)), size_(size) {}
    
    std::string name() const override {
        std::string result = elementType_->name();
        if (size_.has_value()) {
            result += "[" + std::to_string(*size_) + "]";
        } else {
            result += "[]";
        }
        return result;
    }
    
    size_t sizeInBits() const override {
        if (!size_.has_value()) return 0; // Dynamic array
        return elementType_->sizeInBits() * (*size_);
    }
    
    size_t alignmentInBits() const override {
        return elementType_->alignmentInBits();
    }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::ARRAY) return false;
        auto& otherArray = static_cast<const ArrayType&>(other);
        return elementType_->equals(*otherArray.elementType_) && 
               size_ == otherArray.size_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        if (equals(other)) return true;
        
        // Can assign arrays of compatible element types
        if (other.kind() == TypeKind::ARRAY) {
            auto& otherArray = static_cast<const ArrayType&>(other);
            return elementType_->isAssignableFrom(*otherArray.elementType_);
        }
        
        return false;
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        // Arrays generally don't implicitly convert, except to compatible arrays
        return isAssignableFrom(other);
    }
    
    bool isArray() const override { return true; }
    
    std::shared_ptr<Type> elementType() const { return elementType_; }
    std::optional<size_t> size() const { return size_; }
    bool isDynamic() const { return !size_.has_value(); }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<ArrayType>(elementType_->clone(), size_);
    }

private:
    std::shared_ptr<Type> elementType_;
    std::optional<size_t> size_;
};

// Pointer type (T*)
class PointerType : public Type {
public:
    PointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false)
        : Type(TypeKind::POINTER), pointeeType_(std::move(pointeeType)), 
          isConst_(isConst), isVolatile_(isVolatile) {}
    
    std::string name() const override {
        std::string result = pointeeType_->name() + "*";
        if (isConst_) result = "const " + result;
        if (isVolatile_) result = "volatile " + result;
        return result;
    }
    
    size_t sizeInBits() const override { return 64; } // 64-bit pointers
    size_t alignmentInBits() const override { return 64; }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::POINTER) return false;
        auto& otherPtr = static_cast<const PointerType&>(other);
        return pointeeType_->equals(*otherPtr.pointeeType_) &&
               isConst_ == otherPtr.isConst_ &&
               isVolatile_ == otherPtr.isVolatile_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        if (equals(other)) return true;
        
        // Pointer assignment rules
        if (other.kind() == TypeKind::POINTER) {
            auto& otherPtr = static_cast<const PointerType&>(other);
            
            // Can assign if pointee types are compatible
            if (pointeeType_->isAssignableFrom(*otherPtr.pointeeType_)) {
                // const/volatile rules: can assign to more restrictive
                return (!isConst_ || otherPtr.isConst_) && 
                       (!isVolatile_ || otherPtr.isVolatile_);
            }
        }
        
        return false;
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return isAssignableFrom(other);
    }
    
    bool isConst() const override { return isConst_; }
    bool isVolatile() const override { return isVolatile_; }
    bool isPointer() const override { return true; }
    
    std::shared_ptr<Type> pointeeType() const { return pointeeType_; }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<PointerType>(pointeeType_->clone(), isConst_, isVolatile_);
    }

private:
    std::shared_ptr<Type> pointeeType_;
    bool isConst_;
    bool isVolatile_;
};

// Function type ((T1, T2, ...) -> R)
class FunctionType : public Type {
public:
    FunctionType(std::vector<std::shared_ptr<Type>> parameterTypes, 
                 std::shared_ptr<Type> returnType)
        : Type(TypeKind::FUNCTION), parameterTypes_(std::move(parameterTypes)), 
          returnType_(std::move(returnType)) {}
    
    std::string name() const override {
        std::string result = "(";
        for (size_t i = 0; i < parameterTypes_.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameterTypes_[i]->name();
        }
        result += ") -> " + returnType_->name();
        return result;
    }
    
    size_t sizeInBits() const override { return 64; } // Function pointer size
    size_t alignmentInBits() const override { return 64; }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::FUNCTION) return false;
        auto& otherFunc = static_cast<const FunctionType&>(other);
        
        if (parameterTypes_.size() != otherFunc.parameterTypes_.size()) return false;
        if (!returnType_->equals(*otherFunc.returnType_)) return false;
        
        for (size_t i = 0; i < parameterTypes_.size(); ++i) {
            if (!parameterTypes_[i]->equals(*otherFunc.parameterTypes_[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other);
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return equals(other);
    }
    
    bool isFunction() const override { return true; }
    
    const std::vector<std::shared_ptr<Type>>& parameterTypes() const { return parameterTypes_; }
    std::shared_ptr<Type> returnType() const { return returnType_; }
    size_t parameterCount() const { return parameterTypes_.size(); }
    
    std::unique_ptr<Type> clone() const override {
        std::vector<std::shared_ptr<Type>> clonedParams;
        for (const auto& param : parameterTypes_) {
            clonedParams.push_back(param->clone());
        }
        return std::make_unique<FunctionType>(std::move(clonedParams), returnType_->clone());
    }

private:
    std::vector<std::shared_ptr<Type>> parameterTypes_;
    std::shared_ptr<Type> returnType_;
};

// Object type (user-defined object)
class ObjectType : public Type {
public:
    struct Member {
        std::string_view name;
        std::shared_ptr<Type> type;
        bool isConst;
        bool isVolatile;
        
        Member(std::string_view name, std::shared_ptr<Type> type, 
               bool isConst = false, bool isVolatile = false)
            : name(name), type(std::move(type)), isConst(isConst), isVolatile(isVolatile) {}
    };
    
    ObjectType(std::string_view name) : Type(TypeKind::OBJECT), name_(name) {}
    
    std::string name() const override { return std::string(name_); }
    
    size_t sizeInBits() const override {
        size_t total = 0;
        for (const auto& member : members_) {
            total += member.type->sizeInBits();
        }
        return total;
    }
    
    size_t alignmentInBits() const override {
        size_t maxAlign = 8; // Minimum alignment
        for (const auto& member : members_) {
            maxAlign = std::max(maxAlign, member.type->alignmentInBits());
        }
        return maxAlign;
    }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::OBJECT) return false;
        auto& otherObj = static_cast<const ObjectType&>(other);
        return name_ == otherObj.name_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other);
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return equals(other);
    }
    
    bool isObject() const override { return true; }
    
    void addMember(const Member& member) { members_.push_back(member); }
    const std::vector<Member>& members() const { return members_; }
    
    Member* findMember(std::string_view memberName) {
        for (auto& member : members_) {
            if (member.name == memberName) return &member;
        }
        return nullptr;
    }
    
    std::unique_ptr<Type> clone() const override {
        auto result = std::make_unique<ObjectType>(name_);
        result->members_ = members_;
        return result;
    }

private:
    std::string_view name_;
    std::vector<Member> members_;
};

// Struct type (user-defined struct)
class StructType : public Type {
public:
    struct Field {
        std::string_view name;
        std::shared_ptr<Type> type;
        size_t alignment;
        bool isVolatile;
        
        Field(std::string_view name, std::shared_ptr<Type> type, 
              size_t alignment = 0, bool isVolatile = false)
            : name(name), type(std::move(type)), alignment(alignment), isVolatile(isVolatile) {}
    };
    
    StructType(std::string_view name, bool isPacked = false, size_t alignment = 0)
        : Type(TypeKind::STRUCT), name_(name), isPacked_(isPacked), alignment_(alignment) {}
    
    std::string name() const override { return std::string(name_); }
    
    size_t sizeInBits() const override {
        if (isPacked_) {
            // Packed: no padding between fields
            size_t total = 0;
            for (const auto& field : fields_) {
                total += field.type->sizeInBits();
            }
            return total;
        } else {
            // Regular struct with alignment padding
            size_t total = 0;
            for (const auto& field : fields_) {
                size_t fieldAlign = field.alignment > 0 ? field.alignment : field.type->alignmentInBits();
                total = ((total + fieldAlign - 1) / fieldAlign) * fieldAlign; // Align
                total += field.type->sizeInBits();
            }
            return total;
        }
    }
    
    size_t alignmentInBits() const override {
        if (alignment_ > 0) return alignment_;
        
        size_t maxAlign = 8; // Minimum alignment
        for (const auto& field : fields_) {
            size_t fieldAlign = field.alignment > 0 ? field.alignment : field.type->alignmentInBits();
            maxAlign = std::max(maxAlign, fieldAlign);
        }
        return maxAlign;
    }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::STRUCT) return false;
        auto& otherStruct = static_cast<const StructType&>(other);
        return name_ == otherStruct.name_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other);
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return equals(other);
    }
    
    bool isStruct() const override { return true; }
    
    void addField(const Field& field) { fields_.push_back(field); }
    const std::vector<Field>& fields() const { return fields_; }
    
    Field* findField(std::string_view fieldName) {
        for (auto& field : fields_) {
            if (field.name == fieldName) return &field;
        }
        return nullptr;
    }
    
    bool isPacked() const { return isPacked_; }
    size_t customAlignment() const { return alignment_; }
    
    std::unique_ptr<Type> clone() const override {
        auto result = std::make_unique<StructType>(name_, isPacked_, alignment_);
        result->fields_ = fields_;
        return result;
    }

private:
    std::string_view name_;
    std::vector<Field> fields_;
    bool isPacked_;
    size_t alignment_;
};

// Enum type
class EnumType : public Type {
public:
    struct Enumerator {
        std::string_view name;
        int64_t value;
        
        Enumerator(std::string_view name, int64_t value)
            : name(name), value(value) {}
    };
    
    EnumType(std::string_view name) : Type(TypeKind::ENUM), name_(name) {}
    
    std::string name() const override { return std::string(name_); }
    
    size_t sizeInBits() const override { return 32; } // Default enum size
    size_t alignmentInBits() const override { return 32; }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::ENUM) return false;
        auto& otherEnum = static_cast<const EnumType&>(other);
        return name_ == otherEnum.name_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other);
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        // Enums can be implicitly converted to integers
        if (other.kind() == TypeKind::INTEGER || other.kind() == TypeKind::DATA) {
            return true;
        }
        return equals(other);
    }
    
    bool isEnum() const override { return true; }
    bool isPrimitive() const override { return true; }
    
    void addEnumerator(const Enumerator& enumerator) { enumerators_.push_back(enumerator); }
    const std::vector<Enumerator>& enumerators() const { return enumerators_; }
    
    Enumerator* findEnumerator(std::string_view name) {
        for (auto& enumerator : enumerators_) {
            if (enumerator.name == name) return &enumerator;
        }
        return nullptr;
    }
    
    std::unique_ptr<Type> clone() const override {
        auto result = std::make_unique<EnumType>(name_);
        result->enumerators_ = enumerators_;
        return result;
    }

private:
    std::string_view name_;
    std::vector<Enumerator> enumerators_;
};

// Template parameter type
class TemplateParameterType : public Type {
public:
    TemplateParameterType(std::string_view name) 
        : Type(TypeKind::TEMPLATE_PARAMETER), name_(name) {}
    
    std::string name() const override { return std::string(name_); }
    
    size_t sizeInBits() const override { return 0; } // Unknown until instantiated
    size_t alignmentInBits() const override { return 0; }
    
    bool equals(const Type& other) const override {
        if (other.kind() != TypeKind::TEMPLATE_PARAMETER) return false;
        auto& otherTemplate = static_cast<const TemplateParameterType&>(other);
        return name_ == otherTemplate.name_;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return equals(other); // Template parameters only match themselves
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return equals(other);
    }
    
    bool isTemplate() const override { return true; }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<TemplateParameterType>(name_);
    }

private:
    std::string_view name_;
};

// Error type (for error recovery in type checking)
class ErrorType : public Type {
public:
    ErrorType() : Type(TypeKind::ERROR) {}
    
    std::string name() const override { return "<error>"; }
    
    size_t sizeInBits() const override { return 0; }
    size_t alignmentInBits() const override { return 0; }
    
    bool equals(const Type& other) const override {
        return other.kind() == TypeKind::ERROR;
    }
    
    bool isAssignableFrom(const Type& other) const override {
        return true; // Error type accepts anything for error recovery
    }
    
    bool isImplicitlyConvertibleTo(const Type& other) const override {
        return true; // Error type converts to anything for error recovery
    }
    
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<ErrorType>();
    }
};

// Type utility functions
namespace TypeUtils {
    // Check if a type is numeric (integer, float, data)
    inline bool isNumeric(const Type& type) {
        return type.kind() == TypeKind::INTEGER ||
               type.kind() == TypeKind::UNSIGNED_INTEGER ||
               type.kind() == TypeKind::FLOAT ||
               type.kind() == TypeKind::UNSIGNED_FLOAT ||
               type.kind() == TypeKind::DATA;
    }
    
    // Check if a type is integral (integer, data with integer semantics)
    inline bool isIntegral(const Type& type) {
        return type.kind() == TypeKind::INTEGER ||
               type.kind() == TypeKind::UNSIGNED_INTEGER ||
               (type.kind() == TypeKind::DATA && !isFloatingPoint(type));
    }
    
    // Check if a type is floating point
    inline bool isFloatingPoint(const Type& type) {
        if (type.kind() == TypeKind::FLOAT || type.kind() == TypeKind::UNSIGNED_FLOAT) {
            return true;
        }
        // Check if it's a data type with floating point semantics
        // This would require additional context or metadata
        return false;
    }
    
    // Get the common type for binary operations
    std::shared_ptr<Type> getCommonType(const Type& left, const Type& right);
    
    // Check if a type can be used in arithmetic operations
    inline bool isArithmetic(const Type& type) {
        return isNumeric(type);
    }
    
    // Check if a type can be used in comparison operations
    inline bool isComparable(const Type& type) {
        return isNumeric(type) || type.kind() == TypeKind::POINTER || 
               type.kind() == TypeKind::ENUM || type.kind() == TypeKind::BOOLEAN;
    }
    
    // Check if a type supports bitwise operations
    inline bool supportsBitwise(const Type& type) {
        return isIntegral(type) || type.kind() == TypeKind::BOOLEAN;
    }
}

} // namespace type
} // namespace flux