#include "llvm_codegen.h"

#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>

namespace flux {
namespace codegen {

// Initialize LLVM targets and other necessary components
class LLVMInitializer {
public:
    LLVMInitializer() {
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();
    }
};

// Constructor
LLVMCodeGenerator::LLVMCodeGenerator(common::Arena& arena)
    : context_(), module_(std::make_unique<llvm::Module>("FluxModule", context_)), 
      builder_(std::make_unique<llvm::IRBuilder<>>(context_)), arena_(arena) {
    // Perform one-time LLVM initialization
    static LLVMInitializer initializer;
}

// Generate LLVM IR for an entire program
std::unique_ptr<llvm::Module> LLVMCodeGenerator::generateProgram(const parser::Program& program) {
    // Create the main function
    auto* mainFunc = createMainFunction();

    // Generate code for each declaration
    for (const auto& decl : program.declarations) {
        generateDeclaration(decl.get());
    }

    return std::move(module_);
}

// Create the main function
llvm::Function* LLVMCodeGenerator::createMainFunction() {
    // Define the main function type (int main())
    auto* intType = llvm::Type::getInt32TypeInContext(context_);
    auto* mainFuncType = llvm::FunctionType::get(intType, false);
    
    // Create the main function
    auto* mainFunc = llvm::Function::Create(
        mainFuncType, 
        llvm::Function::ExternalLinkage, 
        "main", 
        module_.get()
    );

    // Create a basic block for the main function
    auto* entryBlock = llvm::BasicBlock::Create(context_, "entry", mainFunc);
    builder_->SetInsertPoint(entryBlock);

    // Return 0 by default
    builder_->CreateRet(llvm::ConstantInt::get(intType, 0));

    return mainFunc;
}

// Set up builtin functions
void LLVMCodeGenerator::setupBuiltinFunctions() {
    // TODO: Implement standard library and builtin function support
}

// Generate code for declarations
llvm::Value* LLVMCodeGenerator::generateDeclaration(const parser::Decl* decl) {
    // TODO: Add specific declaration type handling
    // Will include function declarations, variable declarations, etc.
    return nullptr;
}

// Generate code for statements
llvm::Value* LLVMCodeGenerator::generateStatement(const parser::Stmt* stmt) {
    // TODO: Add specific statement type handling
    // Will include if statements, loops, etc.
    return nullptr;
}

// Generate code for expressions
llvm::Value* LLVMCodeGenerator::generateExpression(const parser::Expr* expr) {
    // TODO: Add specific expression type handling
    // Will include literals, variables, function calls, etc.
    return nullptr;
}

// Generate LLVM type from Flux type expression
llvm::Type* LLVMCodeGenerator::generateType(const parser::TypeExpr* typeExpr) {
    // TODO: Add type mapping from Flux types to LLVM types
    return nullptr;
}

// Factory function to create the code generator
std::unique_ptr<LLVMCodeGenerator> createCodeGenerator(common::Arena& arena) {
    return std::make_unique<LLVMCodeGenerator>(arena);
}

} // namespace codegen
} // namespace flux