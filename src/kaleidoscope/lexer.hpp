#ifndef INCLUDED_KALEIDOSCOPE_LEXER_HPP
#define INCLUDED_KALEIDOSCOPE_LEXER_HPP

#include "token.hpp"

#include <istream>
#include <map>

namespace kaleidoscope
{
    class Lexer
    {
    public:
        Lexer(std::istream &in);

        Token gettok();

    private:
        std::istream &in_;
        char LastChar = ' ';

        std::map<std::string, TokenType> keywords_;
    };
} // namespace kaleidoscope

#endif
