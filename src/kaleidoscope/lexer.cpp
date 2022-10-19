#include "lexer.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace kaleidoscope
{
    Lexer::Lexer(std::istream &in)
        : in_(in),
          keywords_({
#define KEYWORD(kw) {#kw, tok_##kw},
#include "keywords.list"
          })
    {
    }

    bool Lexer::advance()
    {
        if (in_.get(LastChar))
        {
            srcLoc_.advance(LastChar);
            return true;
        }
        else
        {
            return false;
        }
    }

    void Lexer::discardLine()
    {
        while (in_.get(LastChar) && LastChar != '\n')
        {
        }

        srcLoc_.advanceLine();
    }

    Token Lexer::gettok()
    {
        while (std::isspace(LastChar) && advance())
        {
        }

        if (!in_)
        {
            return tok_eof;
        }
        else if (std::isalpha(LastChar))
        {
            std::string identifier({LastChar});

            while (advance() && std::isalnum(LastChar))
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
            } while (advance() && (std::isdigit(LastChar) || LastChar == '.'));

            std::istringstream parser(NumStr);
            double NumVal = 0.0;
            parser >> NumVal;

            return NumVal;
        }
        else if (LastChar == '#')
        {
            discardLine();
            return gettok();
        }

        char ThisChar = LastChar;
        advance();

        return ThisChar;
    }
} // namespace kaleidoscope