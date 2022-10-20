#ifndef INCLUDED_KALEIDOSCOPE_DEBUG_HPP
#define INCLUDED_KALEIDOSCOPE_DEBUG_HPP

#include "ast.hpp"
#include "sourcelocation.hpp"

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <vector>

namespace kaleidoscope
{
    class DebugInfo
    {
    public:
        DebugInfo(llvm::Module &module);

        void enterFunction(llvm::IRBuilder<> &irBuilder, llvm::Function *F, PrototypeAST const &proto);
        void exitScope();

        void declareParameter(llvm::IRBuilder<> &irBuilder, llvm::AllocaInst *alloca, std::string const &name, int argIdx, SourceLocation const &loc);

        void emitNullLocation(llvm::IRBuilder<> &irBuilder);
        void emitLocation(llvm::IRBuilder<> &irBuilder, SourceLocation const &srcLoc);

        void finalize();

    private:
        llvm::DISubroutineType *CreateFunctionType(unsigned NumArgs);

        std::unique_ptr<llvm::DIBuilder> builder_;
        llvm::DIFile *file_;
        llvm::DICompileUnit *compileUnit_;
        llvm::DIType *doubleType_;
        std::vector<llvm::DIScope *> LexicalBlocks;
    };

    class DebugScope
    {
    public:
        DebugScope(DebugInfo &debugInfo, llvm::IRBuilder<> &irBuilder, llvm::Function *F, PrototypeAST const &proto)
            : debugInfo_(debugInfo)
        {
            debugInfo.enterFunction(irBuilder, F, proto);
        }
        ~DebugScope() { debugInfo_.exitScope(); }

    private:
        DebugInfo &debugInfo_;
    };
}

#endif
