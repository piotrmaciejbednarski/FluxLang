#ifndef FLUX_BUILTINS_H
#define FLUX_BUILTINS_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include "runtime.h"

namespace flux {

// Forward declarations
class Environment;

/**
 * Builtins namespace for Flux built-in functions and utilities
 */
namespace builtins {

// Print to standard output
std::shared_ptr<RuntimeValue> print(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Get user input
std::shared_ptr<RuntimeValue> input(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Open a file
std::shared_ptr<RuntimeValue> open(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Create a socket connection
std::shared_ptr<RuntimeValue> socket(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Get length of strings and arrays
std::shared_ptr<RuntimeValue> length(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Memory allocation
std::shared_ptr<RuntimeValue> memalloc(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Math functions
std::shared_ptr<RuntimeValue> sin(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> cos(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> tan(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> cot(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> sec(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> cosec(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> quad_eq(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

std::shared_ptr<RuntimeValue> sqrt(
    const std::vector<std::shared_ptr<RuntimeValue>>& args,
    std::shared_ptr<Environment> env);

// Utility functions

// Register all built-in functions to an environment
void registerBuiltins(std::shared_ptr<Environment> env);

// Convert between different numeric types
std::shared_ptr<RuntimeValue> convertNumericValue(
    const std::shared_ptr<RuntimeValue>& value,
    RuntimeValue::Type targetType,
    int targetBitWidth = 32,
    bool targetUnsigned = false);

// Create default value for a given flux type
std::shared_ptr<RuntimeValue> createDefaultValue(const std::shared_ptr<Type>& type);

// String interpolation
std::string interpolateString(
    const std::string& format,
    const std::vector<std::shared_ptr<RuntimeValue>>& values);

// Type conversion utilities
RuntimeValue::Type fluxTypeToRuntimeType(const std::shared_ptr<Type>& type);

// Arithmetic operations
std::shared_ptr<RuntimeValue> performArithmetic(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op);

// Comparison operations
std::shared_ptr<RuntimeValue> performComparison(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op);

// Logical operations
std::shared_ptr<RuntimeValue> performLogical(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right,
    BinaryOp op);

// String concatenation
std::shared_ptr<RuntimeValue> concatStrings(
    const std::shared_ptr<RuntimeValue>& left,
    const std::shared_ptr<RuntimeValue>& right);

} // namespace builtins

} // namespace flux

#endif // FLUX_BUILTINS_H
