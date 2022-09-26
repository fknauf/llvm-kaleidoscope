#ifndef INCLUDED_KALEIDOSCOPE_CODEGEN_HPP
#define INCLUDED_KALEIDOSCOPE_CODEGEN_HPP

#include "ast.hpp"
#include "error.hpp"

#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>

#include <map>
#include <memory>
#include <stdexcept>

namespace kaleidoscope
{
    class CodeGenerationError : public Error
    {
    public:
        CodeGenerationError(std::string const &errMsg);
    };

    class CodeGenerator
    {
    public:
        CodeGenerator(llvm::DataLayout dataLayout = llvm::DataLayout(""));

        llvm::Value *operator()(NumberExprAST const &expr);
        llvm::Value *operator()(VariableExprAST const &expr);
        llvm::Value *operator()(BinaryExprAST const &expr);
        llvm::Value *operator()(CallExprAST const &expr);

        llvm::Function *operator()(PrototypeAST const &expr);
        llvm::Function *operator()(FunctionAST const &expr);

        llvm::Module const &getModule() const
        {
            return *TheModule;
        }

        llvm::orc::ThreadSafeModule stealModule();

        void registerExtern(PrototypeAST ast);

    private:
        llvm::Function *getFunction(std::string const &name, std::string const &errmsg_format);

        llvm::DataLayout dataLayout;
        std::unique_ptr<llvm::LLVMContext> TheContext;
        std::unique_ptr<llvm::IRBuilder<>> Builder;
        std::unique_ptr<llvm::Module> TheModule;
        std::map<std::string, llvm::Value *> NamedValues;
        std::map<std::string, PrototypeAST> FunctionProtos;
    };
}

#endif
