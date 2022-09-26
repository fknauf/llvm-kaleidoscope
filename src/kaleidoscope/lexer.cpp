#include "lexer.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace kaleidoscope
{
    Lexer::Lexer(std::istream &in)
        : in_(in),
          keywords_({{"def", tok_def},
                     {"extern", tok_extern},
                     {"if", tok_if},
                     {"else", tok_else},
                     {"then", tok_then}})
    {
    }

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

            auto keyword_iter = keywords_.find(identifier);
            if (keyword_iter != keywords_.end())
            {
                return keyword_iter->second;
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