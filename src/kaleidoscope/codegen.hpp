#ifndef INCLUDED_KALEIDOSCOPE_CODEGEN_HPP
#define INCLUDED_KALEIDOSCOPE_CODEGEN_HPP

#include "ast.hpp"
#include "debug.hpp"
#include "error.hpp"
#include "symbols.hpp"
#include "parser.hpp"

#include <llvm/IR/DIBuilder.h>
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
        CodeGenerator(Parser &p, llvm::DataLayout dataLayout = llvm::DataLayout(""), std::string const &moduleName = "module", bool disableDebug = true);

        llvm::Value *operator()(ExprAST const &expr);
        llvm::Value *operator()(NumberExprAST const &expr);
        llvm::Value *operator()(VariableExprAST const &expr);
        llvm::Value *operator()(UnaryExprAST const &expr);
        llvm::Value *operator()(BinaryExprAST const &expr);
        llvm::Value *operator()(CallExprAST const &expr);
        llvm::Value *operator()(IfExprAST const &expr);
        llvm::Value *operator()(ForExprAST const &expr);
        llvm::Value *operator()(VarExprAST const &expr);

        llvm::Function *operator()(PrototypeAST const &expr);
        llvm::Function *operator()(FunctionAST const &expr);

        llvm::orc::ThreadSafeModule finalizeModule(std::string const &newModuleName = "module");

        void registerExtern(PrototypeAST ast);

    private:
        llvm::Value *generateOptional(ExprAST const *ast, double defaultValue);

        llvm::Function *getFunction(std::string const &name, std::string const &errmsg_format);
        llvm::Value *getConstant(double value) const;
        llvm::Value *getBoolCondition(llvm::Value *condValue, llvm::Twine const &name);

        llvm::AllocaInst *createScopedVariable(llvm::Function *F, std::string const &varName);

        Parser &TheParser;
        llvm::DataLayout dataLayout;
        std::unique_ptr<llvm::LLVMContext> TheContext;
        std::unique_ptr<llvm::IRBuilder<>> TheBuilder;
        std::unique_ptr<llvm::Module> TheModule;

        bool disableDebug_;
        std::unique_ptr<DebugInfo> debugInfo_;

        SymbolTable globalSymbols_;
        SymbolTable *activeScope_;
        std::map<std::string, PrototypeAST> FunctionProtos;
    };
}

#endif
