#include "jit.hpp"

#include "api_functions.hpp"

#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

namespace kaleidoscope
{
    // Alles, was den JIT verwendet, verwendet auch die API-Funktionen.
    // Wir brauchen das, um den Linker auszutricksen, der sonst die api_functions.o
    // beim Link ausnehmen könnte, weil die Funktionen darin unbenutzt aussehen.
    __attribute__((used)) auto api_anchor = &putchard;

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
