#ifndef INCLUDED_KALEIDOSCOPE_PARSER_HPP
#define INCLUDED_KALEIDOSCOPE_PARSER_HPP

#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace kaleidoscope
{
    class ParseError : public Error
    {
    public:
        ParseError(std::string const &errMsg);
    };

    class Parser
    {
    public:
        Parser(Lexer &lexer);

        Token getNextToken() { return CurTok = lexer_.gettok(); }
        Token getCurrentToken() { return CurTok; }

        NumberExprAST ParseNumberExpr();
        ExprAST ParseParenExpr();
        ExprAST ParseIdentifierExpr();
        ExprAST ParsePrimary();
        ExprAST ParseExpression();
        ExprAST ParseBinOpRHS(int ExprPrec, ExprAST &&LHS);
        IfExprAST ParseIfExpr();

        PrototypeAST ParsePrototype();
        FunctionAST ParseDefinition();
        PrototypeAST ParseExtern();
        FunctionAST ParseTopLevelExpr();

    private:
        int GetTokPrecedence() const;

        Lexer &lexer_;
        Token CurTok{tok_eof};

        std::unordered_map<char, int> binOpPrecedence;
    };
} // namespace kaleidoscope

#endif
