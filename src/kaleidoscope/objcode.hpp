#ifndef INCLUDED_KALEIDOSCOPE_OBJCODE_HPP
#define INCLUDED_KALEIDOSCOPE_OBJCODE_HPP

#include "error.hpp"

#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>

namespace kaleidoscope
{
    class ObjCodeError : public Error
    {
    public:
        ObjCodeError(std::string const &errMsg);
    };

    class ObjCodeWriter
    {
    public:
        ObjCodeWriter(llvm::TargetOptions options = {},
                      llvm::Optional<llvm::Reloc::Model> relocationModel = {},
                      std::string const &cpu = "generic",
                      std::string const &features = "");

        ObjCodeWriter(std::string const &targetTriple,
                      llvm::TargetOptions options = {},
                      llvm::Optional<llvm::Reloc::Model> relocationModel = {},
                      std::string const &cpu = "generic",
                      std::string const &features = "");

        auto getDataLayout() const { return targetMachine_->createDataLayout(); }

        void writeModuleToStream(llvm::raw_pwrite_stream &dest,
                                 llvm::Module &module,
                                 llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile);

    private:
        std::unique_ptr<llvm::TargetMachine> targetMachine_;
    };
}

#endif
