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
        Parser(Lexer &lexer, std::string const &topLevelSymbolName = "__anon_expr");

        Token getNextToken() { return CurTok = lexer_.gettok(); }
        Token getCurrentToken() { return CurTok; }
        std::string const &getTopLevelSymbolName() const { return topLevelSymbolName_; }

        NumberExprAST ParseNumberExpr();
        ExprAST ParseParenExpr();
        ExprAST ParseIdentifierExpr();
        ExprAST ParsePrimary();
        ExprAST ParseExpression();
        ExprAST ParseUnary();
        ExprAST ParseBinOpRHS(int ExprPrec, ExprAST &&LHS);
        VarExprAST ParseVarExpr();
        IfExprAST ParseIfExpr();
        ForExprAST ParseForExpr();

        PrototypeAST ParsePrototype();
        FunctionAST ParseDefinition();
        PrototypeAST ParseExtern();
        FunctionAST ParseTopLevelExpr();

        void registerOperator(PrototypeAST const &operatorProto);
        void removeOperator(PrototypeAST const &operatorProto);

    private:
        int GetTokPrecedence() const;

        std::string expectIdentifier(std::string const &errMsg);
        char expectAscii(std::string const &errMsg);
        void expectChar(char expected, std::string const &errMsg);
        void expectKeyword(TokenType expected, std::string const &errMsg);

        bool tryConsumeChar(char expected);
        bool tryConsumeKeyword(TokenType expected);

        Lexer &lexer_;
        std::string topLevelSymbolName_;
        Token CurTok{tok_eof};

        std::unordered_map<char, int> binOpPrecedence;
    };
} // namespace kaleidoscope

#endif
