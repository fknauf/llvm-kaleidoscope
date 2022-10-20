#include "kaleidoscope/codegen.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/parser.hpp"
#include "kaleidoscope/optimizer.hpp"

#include <fstream>
#include <iostream>

using kaleidoscope::CodeGenerationError;
using kaleidoscope::CodeGenerator;
using kaleidoscope::Error;
using kaleidoscope::Lexer;
using kaleidoscope::ParseError;
using kaleidoscope::Parser;

namespace
{
    class CodeGenerationHandler
    {
    private:
        template <typename T>
        void HandleParse(Parser &p, T (Parser::*parseFunction)())
        {
            try
            {
                auto ast = (p.*parseFunction)();
                codegen_(ast);
            }
            catch (Error const &e)
            {
                std::cerr << e.what() << std::endl;
                // Skip token for error recovery.
                p.getNextToken();
            }
        }

    public:
        CodeGenerationHandler(Parser &p)
            : codegen_(p)
        {
        }

        void HandleDefinition(Parser &p)
        {
            HandleParse(p, &Parser::ParseDefinition);
        }

        void HandleExtern(Parser &p)
        {
            HandleParse(p, &Parser::ParseExtern);
        }

        void HandleTopLevelExpression(Parser &p)
        {
            HandleParse(p, &Parser::ParseTopLevelExpr);
        }

        llvm::orc::ThreadSafeModule stealFinalModule()
        {
            return codegen_.finalizeModule();
        }

    private:
        CodeGenerator codegen_;
    };

    /// top ::= definition | external | expression | ';'
    static void MainLoop(Parser &p)
    {
        CodeGenerationHandler codegen(p);

        while (true)
        {
            switch (p.getCurrentToken().getType())
            {
            case kaleidoscope::tok_eof:
            {
                auto finalModule = codegen.stealFinalModule();

                std::cerr << "------\nBEFORE\n------\n";
                finalModule.getModuleUnlocked()->print(llvm::errs(), nullptr);

                kaleidoscope::optimizeModule(*finalModule.getModuleUnlocked());

                std::cerr << "\n-----\nAFTER\n-----\n";
                finalModule.getModuleUnlocked()->print(llvm::errs(), nullptr);
                return;
            }
            case kaleidoscope::tok_def:
                codegen.HandleDefinition(p);
                break;
            case kaleidoscope::tok_extern:
                codegen.HandleExtern(p);
                break;
            case kaleidoscope::tok_char: // ignore top-level semicolons.
                if (p.getCurrentToken().getCharValue() == ';')
                {
                    p.getNextToken();
                    break;
                }
                // Fall-through
            default:
                codegen.HandleTopLevelExpression(p);
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
        std::ifstream in(argv[1]);
        MainParse(in);
    }
    else
    {
        MainParse(std::cin);
    }
}
