#ifndef INCLUDED_KALEIDOSCOPE_JIT_HPP
#define INCLUDED_KALEIDOSCOPE_JIT_HPP

#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>

namespace kaleidoscope
{
    class KaleidoscopeJIT
    {
    private:
        std::unique_ptr<llvm::orc::ExecutionSession> ES;

        llvm::DataLayout DL;
        llvm::orc::MangleAndInterner Mangle;

        llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
        llvm::orc::IRCompileLayer CompileLayer;

        llvm::orc::JITDylib &MainJD;

    public:
        KaleidoscopeJIT(std::unique_ptr<llvm::orc::ExecutionSession> ES,
                        llvm::orc::JITTargetMachineBuilder JTMB,
                        llvm::DataLayout DL);

        ~KaleidoscopeJIT();

        static llvm::Expected<std::unique_ptr<KaleidoscopeJIT>> Create();

        const llvm::DataLayout &getDataLayout() const { return DL; }
        llvm::orc::JITDylib &getMainJITDylib() { return MainJD; }

        llvm::Error addModule(llvm::orc::ThreadSafeModule TSM, llvm::orc::ResourceTrackerSP RT = nullptr);

        llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef Name);
    };
}

#endif
