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

class CodeGenerationHandler
{
private:
    template <typename T>
    void HandleParse(Parser &p, T (Parser::*parseFunction)())
    {
        try
        {
            auto ast = (p.*parseFunction)();
            auto ir = codegen_(ast);
        }
        catch (Error const &e)
        {
            std::cerr << e.what() << std::endl;
            // Skip token for error recovery.
            p.getNextToken();
        }
    }

public:
    CodeGenerationHandler(llvm::LLVMContext &context)
        : codegen_(context) {}

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

    std::unique_ptr<llvm::Module> stealFinalModule()
    {
        return codegen_.stealModule();
    }

private:
    CodeGenerator codegen_;
};

namespace
{
    /// top ::= definition | external | expression | ';'
    static void MainLoop(llvm::LLVMContext &llvmContext, Parser &p)
    {
        CodeGenerationHandler codegen(llvmContext);

        while (true)
        {
            switch (p.getCurrentToken().getType())
            {
            case kaleidoscope::tok_eof:
            {
                std::unique_ptr<llvm::Module> finalModule = codegen.stealFinalModule();

                std::cerr << "------\nBEFORE\n------\n";
                finalModule->print(llvm::errs(), nullptr);

                kaleidoscope::optimizeModule(*finalModule);

                std::cerr << "\n-----\nAFTER\n-----\n";
                finalModule->print(llvm::errs(), nullptr);
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

        llvm::LLVMContext llvmContext;

        Lexer lexer(in);
        Parser parser(lexer);

        parser.getNextToken();

        MainLoop(llvmContext, parser);
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
