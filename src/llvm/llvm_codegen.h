#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/Target/TargetMachine.h>

#include "../parser/ast.h"
#include "../common/arena.h"

namespace flux {
namespace codegen {

class LLVMCodeGenerator {
public:
    // Constructor
    LLVMCodeGenerator(common::Arena& arena);

    // Generate LLVM IR for an entire program
    std::unique_ptr<llvm::Module> generateProgram(const parser::Program& program);

private:
    // Context and builder for LLVM IR generation
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;

    // Reference to the memory arena
    common::Arena& arena_;

    // Visitor methods for different AST node types
    llvm::Value* generateDeclaration(const parser::Decl* decl);
    llvm::Value* generateStatement(const parser::Stmt* stmt);
    llvm::Value* generateExpression(const parser::Expr* expr);
    llvm::Type* generateType(const parser::TypeExpr* typeExpr);

    // Helper methods for specific code generation tasks
    llvm::Function* createMainFunction();
    void setupBuiltinFunctions();
};

// Factory function to create the code generator
std::unique_ptr<LLVMCodeGenerator> createCodeGenerator(common::Arena& arena);

} // namespace codegen
} // namespace flux