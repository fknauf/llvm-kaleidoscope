#ifndef INCLUDED_KALEIDOSCOPE_CODEGEN_HPP
#define INCLUDED_KALEIDOSCOPE_CODEGEN_HPP

#include "ast.hpp"

#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <map>
#include <stdexcept>

namespace kaleidoscope
{
    class CodeGenerationError : public std::runtime_error
    {
    public:
        CodeGenerationError(std::string const &errMsg);
    };

    class CodeGenerator
    {
    public:
        CodeGenerator();

        llvm::Value *operator()(NumberExprAST const &expr);
        llvm::Value *operator()(VariableExprAST const &expr);
        llvm::Value *operator()(BinaryExprAST const &expr);
        llvm::Value *operator()(CallExprAST const &expr);

        llvm::Function *operator()(PrototypeAST const &expr);
        llvm::Function *operator()(FunctionAST const &expr);

    private:
        llvm::LLVMContext TheContext;
        llvm::IRBuilder<> Builder;
        llvm::Module TheModule;
        std::map<std::string, llvm::Value *> NamedValues;
    };
}

#endif
