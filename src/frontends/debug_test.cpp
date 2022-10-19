#include "kaleidoscope/codegen.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/objcode.hpp"
#include "kaleidoscope/parser.hpp"
#include "kaleidoscope/jit.hpp"

#include <llvm/Support/Error.h>

#include <fstream>
#include <iostream>

using kaleidoscope::CodeGenerationError;
using kaleidoscope::CodeGenerator;
using kaleidoscope::Error;
using kaleidoscope::KaleidoscopeJIT;
using kaleidoscope::Lexer;
using kaleidoscope::ObjCodeWriter;
using kaleidoscope::ParseError;
using kaleidoscope::Parser;

namespace
{
    class DebugInfoHandler
    {
    private:
        template <typename T, typename F>
        void HandleParse(Parser &p,
                         T (Parser::*parseFunction)(),
                         F &&astHandler)
        {
            try
            {
                auto ast = (p.*parseFunction)();
                auto ir = codegen_(ast);

                astHandler(ast);
            }
            catch (Error const &e)
            {
                std::cerr << e.what() << std::endl;
                // Skip token for error recovery.
                p.getNextToken();
            }
        }

        template <typename T>
        void HandleParse(Parser &p, T (Parser::*parseFunction)())
        {
            HandleParse(p, parseFunction, [](auto &) {});
        }

    public:
        DebugInfoHandler(Parser &p)
            : codegen_(p, objWriter_.getDataLayout())
        {
        }

        void HandleDefinition(Parser &p)
        {
            HandleParse(p, &Parser::ParseDefinition);
        }

        void HandleExtern(Parser &p)
        {
            HandleParse(p, &Parser::ParseExtern, [this](auto &ast)
                        { codegen_.registerExtern(ast); });
        }

        void HandleTopLevelExpression(Parser &p)
        {
            HandleParse(p, &Parser::ParseTopLevelExpr);
        }

        void writeModuleToFile(std::string const &fileName, llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile)
        {
            std::error_code ec;
            llvm::raw_fd_ostream dest(fileName, ec, llvm::sys::fs::OF_None);

            if (ec)
            {
                throw std::runtime_error(ec.message());
            }

            objWriter_.writeModuleToStream(dest, codegen_.getModule(), fileType);
        }

    private:
        ObjCodeWriter objWriter_;
        CodeGenerator codegen_;
    };

    /// top ::= definition | external | expression | ';'
    static void MainLoop(Parser &p)
    {
        DebugInfoHandler handler(p);

        while (true)
        {
            switch (p.getCurrentToken().getType())
            {
            case kaleidoscope::tok_eof:
            {
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
