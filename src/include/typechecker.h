#ifndef FLUX_TYPE_CHECKER_H
#define FLUX_TYPE_CHECKER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast.h"

namespace flux {

// Forward declarations
class Program;

/**
 * TypeChecker class for static type checking
 */
class TypeChecker {
private:
    // Type environment
    std::unordered_map<std::string, std::shared_ptr<Type>> typeEnvironment;
    
    // Current function return type (for checking return statements)
    std::shared_ptr<Type> currentFunctionReturnType;
    
    // Check types for compatibility
    bool areTypesCompatible(
        const std::shared_ptr<Type>& expected,
        const std::shared_ptr<Type>& actual);
    
    // Get common type for binary operations
    std::shared_ptr<Type> getCommonType(
        const std::shared_ptr<Type>& left,
        const std::shared_ptr<Type>& right);
    
public:
    TypeChecker();
    ~TypeChecker();
    
    // Initialize built-in types
    void initialize();
    
    // Type check a program
    bool checkProgram(const std::shared_ptr<Program>& program);
    
    // Define a type
    void defineType(const std::string& name, std::shared_ptr<Type> type);
    
    // Get a type by name
    std::shared_ptr<Type> getType(const std::string& name);
    
    // Check if a type exists
    bool hasType(const std::string& name);
    
    // Reset the type checker
    void reset();
    
    // Set current function return type
    void setCurrentFunctionReturnType(std::shared_ptr<Type> type);
    
    // Get current function return type
    std::shared_ptr<Type> getCurrentFunctionReturnType() const;
    
    // Create a primitive type
    std::shared_ptr<PrimitiveType> createPrimitiveType(
        Type::Kind kind,
        int bitWidth = 0,
        bool isUnsigned = false);
    
    // Create a pointer type
    std::shared_ptr<PointerType> createPointerType(std::shared_ptr<Type> pointeeType);
    
    // Create a function type
    std::shared_ptr<FunctionType> createFunctionType(
        std::shared_ptr<Type> returnType,
        const std::vector<FunctionParam>& params);
};

} // namespace flux

#endif // FLUX_TYPE_CHECKER_H
