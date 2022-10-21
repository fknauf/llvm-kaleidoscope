#include "debug.hpp"

namespace kaleidoscope
{
    namespace
    {
        std::string moduleFileName(llvm::Module const &module)
        {
            if (module.getName().str() == "")
            {
                return "fib.ks";
            }
            else
            {
                return module.getName().str() + ".ks";
            }
        }
    }

    DebugInfo::DebugInfo(llvm::Module &module, bool disableDebug)
        : disableDebug_(disableDebug)
    {
        if (!disableDebug)
        {
            module.addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);

            builder_ = std::make_unique<llvm::DIBuilder>(module);
            file_ = builder_->createFile(moduleFileName(module), ".");
            compileUnit_ = builder_->createCompileUnit(llvm::dwarf::DW_LANG_C, file_, "Kaleidoscope compiler", false, "", 0);
            doubleType_ = builder_->createBasicType("double", 64, llvm::dwarf::DW_ATE_float);
            LexicalBlocks.push_back(compileUnit_);
        }
    }

    void DebugInfo::finalize()
    {
        if (!disableDebug_)
        {
            builder_->finalize();
        }
    }

    llvm::DISubroutineType *DebugInfo::CreateFunctionType(unsigned NumArgs)
    {
        if (!disableDebug_)
        {
            llvm::SmallVector<llvm::Metadata *, 8> EltTys(NumArgs + 1, doubleType_);
            return builder_->createSubroutineType(builder_->getOrCreateTypeArray(EltTys));
        }
        else
        {
            return nullptr;
        }
    }

    void DebugInfo::exitScope()
    {
        if (!disableDebug_)
        {
            LexicalBlocks.pop_back();
        }
    }

    void DebugInfo::emitNullLocation(llvm::IRBuilder<> &irBuilder)
    {
        if (!disableDebug_)
        {
            irBuilder.SetCurrentDebugLocation(llvm::DebugLoc());
        }
    }

    void DebugInfo::emitLocation(llvm::IRBuilder<> &irBuilder, SourceLocation const &srcLoc)
    {
        if (!disableDebug_)
        {
            assert(!LexicalBlocks.empty());

            auto scope = LexicalBlocks.back();
            auto debugLoc = llvm::DILocation::get(scope->getContext(),
                                                  srcLoc.line(),
                                                  srcLoc.column(),
                                                  scope);
            irBuilder.SetCurrentDebugLocation(debugLoc);
        }
    }

    void DebugInfo::enterFunction(llvm::IRBuilder<> &irBuilder, llvm::Function *F, PrototypeAST const &proto)
    {
        if (!disableDebug_)
        {
            auto loc = proto.getLocation();

            assert(!file_->isTemporary());

            auto SP = builder_->createFunction(file_,
                                               proto.getName(),
                                               llvm::StringRef(),
                                               file_,
                                               loc.line(),
                                               CreateFunctionType(F->arg_size()),
                                               loc.line(),
                                               llvm::DINode::FlagPrototyped,
                                               llvm::DISubprogram::SPFlagDefinition);

            F->setSubprogram(SP);
            LexicalBlocks.push_back(SP);
            emitNullLocation(irBuilder);
        }
    }

    void DebugInfo::declareParameter(llvm::IRBuilder<> &irBuilder, llvm::AllocaInst *alloca, std::string const &name, int argIdx, SourceLocation const &loc)
    {
        if (!disableDebug_)
        {
            assert(!LexicalBlocks.empty());

            auto SP = LexicalBlocks.back();
            auto D = builder_->createParameterVariable(SP, name, argIdx, file_, loc.line(), doubleType_, true);
            builder_->insertDeclare(alloca, D, builder_->createExpression(), llvm::DILocation::get(SP->getContext(), loc.line(), 0, SP), irBuilder.GetInsertBlock());
        }
    }
}