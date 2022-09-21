#include "lexer.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace kaleidoscope
{
    Lexer::Lexer(std::istream &in) : in_(in) {}

    Token Lexer::gettok()
    {
        if (std::isspace(LastChar))
        {
            if (!(in_ >> LastChar))
            {
                return tok_eof;
            }
        }

        if (std::isalpha(LastChar))
        {
            std::string identifier({LastChar});

            while (in_.get(LastChar) && std::isalnum(LastChar))
            {
                identifier += LastChar;
            }

            if (identifier == "def")
            {
                return tok_def;
            }
            else if (identifier == "extern")
            {
                return tok_extern;
            }

            return identifier;
        }
        else if (std::isdigit(LastChar) || LastChar == '.')
        {
            std::string NumStr;

            do
            {
                NumStr += LastChar;
            } while (in_.get(LastChar) && std::isdigit(LastChar) ||
                     LastChar == '.');

            std::istringstream parser(NumStr);
            double NumVal = 0.0;
            parser >> NumVal;

            return NumVal;
        }
        else if (LastChar == '#')
        {
            while (in_.get(LastChar) && LastChar != '\n' && LastChar != '\r')
            {
            }

            return gettok();
        }

        if (!in_)
        {
            return tok_eof;
        }

        char ThisChar = LastChar;
        in_.get(LastChar);

        return ThisChar;
    }
} // namespace kaleidoscope