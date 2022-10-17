#include "optimizer.hpp"

#include <llvm/Passes/PassBuilder.h>

namespace kaleidoscope
{
    void optimizeModule(llvm::Module &module)
    {
        llvm::PassBuilder builder;

        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        builder.registerModuleAnalyses(mam);
        builder.registerCGSCCAnalyses(cgam);
        builder.registerFunctionAnalyses(fam);
        builder.registerLoopAnalyses(lam);
        builder.crossRegisterProxies(lam, fam, cgam, mam);

        llvm::ModulePassManager mpm = builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

        mpm.run(module, mam);
    }
}