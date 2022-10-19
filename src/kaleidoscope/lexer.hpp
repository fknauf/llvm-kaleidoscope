#ifndef INCLUDED_KALEIDOSCOPE_LEXER_HPP
#define INCLUDED_KALEIDOSCOPE_LEXER_HPP

#include "sourcelocation.hpp"
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
        SourceLocation const &getLocation() const { return srcLoc_; }

    private:
        bool advance();
        void discardLine();

        std::istream &in_;
        char LastChar = ' ';

        SourceLocation srcLoc_;

        std::map<std::string, TokenType> keywords_;
    };
} // namespace kaleidoscope

#endif
