#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/parser.hpp"

#include <fstream>
#include <iostream>

using kaleidoscope::Lexer;
using kaleidoscope::Parser;

namespace
{
    void HandleDefinition(Parser &p)
    {
        try
        {
            p.ParseDefinition();
            std::cerr << "Parsed a function definition\n";
        }
        catch (kaleidoscope::ParseError const &e)
        {
            std::cerr << e.what() << std::endl;
            // Skip token for error recovery.
            p.getNextToken();
        }
    }

    void HandleExtern(Parser &p)
    {
        try
        {
            p.ParseExtern();
            std::cerr << "Parsed an extern\n";
        }
        catch (kaleidoscope::ParseError const &e)
        {
            std::cerr << e.what() << std::endl;
            // Skip token for error recovery.
            p.getNextToken();
        }
    }

    void HandleTopLevelExpression(Parser &p)
    {
        // Evaluate a top-level expression into an anonymous function.
        try
        {
            p.ParseTopLevelExpr();
            std::cerr << "Parsed a top-level expr\n";
        }
        catch (kaleidoscope::ParseError const &e)
        {
            std::cerr << e.what() << std::endl;
            // Skip token for error recovery.
            p.getNextToken();
        }
    }

    /// top ::= definition | external | expression | ';'
    static void MainLoop(Parser &p)
    {
        while (true)
        {
            std::cerr << "ready> " << std::flush;

            switch (p.getCurrentToken().getType())
            {
            case kaleidoscope::tok_eof:
                return;
            case kaleidoscope::tok_def:
                HandleDefinition(p);
                break;
            case kaleidoscope::tok_extern:
                HandleExtern(p);
                break;
            case kaleidoscope::tok_char: // ignore top-level semicolons.
                if (p.getCurrentToken().getCharValue() == ';')
                {
                    p.getNextToken();
                    break;
                }
                // Fall-through
            default:
                HandleTopLevelExpression(p);
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

        std::cerr << "ready> " << std::flush;
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
