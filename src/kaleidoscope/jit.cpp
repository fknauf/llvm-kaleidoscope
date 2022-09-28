#include "jit.hpp"

#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

namespace kaleidoscope
{
    KaleidoscopeJIT::KaleidoscopeJIT(std::unique_ptr<llvm::orc::ExecutionSession> ES,
                                     llvm::orc::JITTargetMachineBuilder JTMB,
                                     llvm::DataLayout DL)
        : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
          ObjectLayer(*this->ES,
                      []()
                      { return std::make_unique<llvm::SectionMemoryManager>(); }),
          CompileLayer(*this->ES, ObjectLayer,
                       std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(JTMB))),
          MainJD(this->ES->createBareJITDylib("<main>"))
    {
        MainJD.addGenerator(
            cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                DL.getGlobalPrefix())));
        if (JTMB.getTargetTriple().isOSBinFormatCOFF())
        {
            ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    KaleidoscopeJIT::~KaleidoscopeJIT()
    {
        if (auto Err = ES->endSession())
            ES->reportError(std::move(Err));
    }

    llvm::Expected<std::unique_ptr<KaleidoscopeJIT>> KaleidoscopeJIT::Create()
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        auto EPC = llvm::orc::SelfExecutorProcessControl::Create();
        if (!EPC)
            return EPC.takeError();

        auto ES = std::make_unique<llvm::orc ::ExecutionSession>(std::move(*EPC));

        llvm::orc::JITTargetMachineBuilder JTMB(
            ES->getExecutorProcessControl().getTargetTriple());

        auto DL = JTMB.getDefaultDataLayoutForTarget();
        if (!DL)
            return DL.takeError();

        return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(JTMB),
                                                 std::move(*DL));
    }

    llvm::Error KaleidoscopeJIT::addModule(llvm::orc::ThreadSafeModule TSM, llvm::orc::ResourceTrackerSP RT)
    {
        if (!RT)
            RT = MainJD.getDefaultResourceTracker();
        return CompileLayer.add(RT, std::move(TSM));
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> KaleidoscopeJIT::lookup(llvm::StringRef Name)
    {
        return ES->lookup({&MainJD}, Mangle(Name.str()));
    }
}

#include <iostream>

extern "C" double putchard(double X)
{
    std::cerr << static_cast<char>(X) << std::flush;
    return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" double printd(double X)
{
    std::cerr << X << std::endl;
    return 0;
}
