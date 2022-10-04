#include "kaleidoscope/codegen.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/parser.hpp"
#include "kaleidoscope/optimizer.hpp"
#include "kaleidoscope/jit.hpp"

#include <llvm/Support/Error.h>

#include <fstream>
#include <iostream>

using kaleidoscope::CodeGenerationError;
using kaleidoscope::CodeGenerator;
using kaleidoscope::Error;
using kaleidoscope::KaleidoscopeJIT;
using kaleidoscope::Lexer;
using kaleidoscope::ParseError;
using kaleidoscope::Parser;

namespace
{
    class JITHandler
    {
    private:
        template <typename T, typename F>
        void HandleParse(Parser &p,
                         T (Parser::*parseFunction)(),
                         F &&irHandler)
        {
            try
            {
                auto ast = (p.*parseFunction)();
                auto ir = codegen_(ast);

                irHandler(ast, ir);
            }
            catch (Error const &e)
            {
                std::cerr << e.what() << std::endl;
                // Skip token for error recovery.
                p.getNextToken();
            }
        }

    public:
        JITHandler(Parser &p)
            : jitCompiler_(ExitOnErr(KaleidoscopeJIT::Create())),
              codegen_(p, jitCompiler_->getDataLayout())
        {
        }

        void HandleDefinition(Parser &p)
        {
            HandleParse(
                p, &Parser::ParseDefinition, [this](auto &, auto &)
                { auto H = jitCompiler_->addModule(codegen_.stealModule()); });
        }

        void HandleExtern(Parser &p)
        {
            HandleParse(p, &Parser::ParseExtern, [this](auto &ast, auto &)
                        { codegen_.registerExtern(ast); });
        }

        void HandleTopLevelExpression(Parser &p)
        {
            HandleParse(p, &Parser::ParseTopLevelExpr, [this](auto &, auto &)
                        {
            auto RT = jitCompiler_->getMainJITDylib().createResourceTracker();
            auto H = jitCompiler_->addModule(codegen_.stealModule(), RT);

            auto exprSymbol = ExitOnErr(jitCompiler_->lookup("__anon_expr"));
            auto FP = reinterpret_cast<double (*)()>(exprSymbol.getAddress());

            auto result = FP();

            std::cerr << "Evaluated to " << result << std::endl;

            ExitOnErr(RT->remove()); });
        }

        auto stealFinalModule()
        {
            return codegen_.stealModule();
        }

    private:
        llvm::ExitOnError ExitOnErr;
        std::unique_ptr<KaleidoscopeJIT> jitCompiler_;
        CodeGenerator codegen_;
    };

    /// top ::= definition | external | expression | ';'
    static void MainLoop(Parser &p)
    {
        JITHandler handler(p);

        while (true)
        {
            switch (p.getCurrentToken().getType())
            {
            case kaleidoscope::tok_eof:
            {
                auto finalModule = handler.stealFinalModule();

                std::cerr << "------\nBEFORE\n------\n";
                finalModule.getModuleUnlocked()->print(llvm::errs(), nullptr);

                kaleidoscope::optimizeModule(*finalModule.getModuleUnlocked());

                std::cerr << "\n-----\nAFTER\n-----\n";
                finalModule.getModuleUnlocked()->print(llvm::errs(), nullptr);
                return;
            }
            case kaleidoscope::tok_def:
                handler.HandleDefinition(p);
                break;
            case kaleidoscope::tok_extern:
                handler.HandleExtern(p);
                break;
            case kaleidoscope::tok_char: // ignore top-level semicolons.
                if (p.getCurrentToken().getCharValue() == ';')
                {
                    p.getNextToken();
                    break;
                }
                // Fall-through
            default:
                handler.HandleTopLevelExpression(p);
                break;
            }
        }
    }

    void MainParse(std::istream &in)
    {
        using kaleidoscope::Lexer;
        using kaleidoscope::Parser;

        Lexer lexer(in);
        Parser parser(lexer);

        parser.getNextToken();

        MainLoop(parser);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::ifstream in(argv[i]);
            MainParse(in);
        }
    }
    else
    {
        MainParse(std::cin);
    }
}
