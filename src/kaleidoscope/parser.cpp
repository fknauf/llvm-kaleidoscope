#include "parser.hpp"

#include <iostream>
#include <sstream>

namespace kaleidoscope
{
    ParseError::ParseError(std::string const &errMsg)
        : Error("Parse error: " + errMsg)
    {
    }

    Parser::Parser(Lexer &lexer)
        : lexer_(lexer),
          binOpPrecedence{
              {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40}}
    {
    }

    int Parser::GetTokPrecedence() const
    {
        if (CurTok.getType() == tok_char)
        {
            auto iter = binOpPrecedence.find(CurTok.getCharValue());

            if (iter != binOpPrecedence.end())
            {
                return iter->second;
            }
        }

        return -1;
    }

    std::string Parser::expectIdentifier(std::string const &errMsg)
    {
        if (CurTok.getType() != tok_identifier)
        {
            throw ParseError(errMsg);
        }

        std::string result = CurTok.getIdentifierValue();
        getNextToken();

        return result;
    }

    void Parser::expectChar(char expected, std::string const &errMsg)
    {
        if (CurTok != expected)
        {
            throw ParseError(errMsg);
        }

        getNextToken();
    }

    void Parser::expectKeyword(TokenType expected, std::string const &errMsg)
    {
        if (CurTok.getType() != expected)
        {
            throw ParseError(errMsg);
        }

        getNextToken();
    }

    NumberExprAST Parser::ParseNumberExpr()
    {
        auto Result = CurTok.getNumValue();
        getNextToken(); // consume the number
        return Result;
    }

    ExprAST Parser::ParseParenExpr()
    {
        getNextToken(); // eat (.
        auto V = ParseExpression();

        expectChar(')', "expected ')'");

        return V;
    }

    ExprAST Parser::ParseIdentifierExpr()
    {
        std::string IdName = CurTok.getIdentifierValue();

        getNextToken(); // eat identifier.

        if (CurTok != '(') // Simple variable ref.
            return VariableExprAST(IdName);

        // Call.
        getNextToken(); // eat (
        std::vector<ExprAST> Args;
        if (CurTok != ')')
        {
            while (true)
            {
                Args.push_back(ParseExpression());

                if (CurTok == ')')
                    break;

                expectChar(',', "Expected ')' or ',' in argument list");
            }
        }

        // Eat the ')'.
        getNextToken();

        return CallExprAST(IdName, std::move(Args));
    }

    ExprAST Parser::ParsePrimary()
    {
        if (CurTok.getType() == tok_identifier)
        {
            return ParseIdentifierExpr();
        }
        else if (CurTok.getType() == tok_number)
        {
            return ParseNumberExpr();
        }
        else if (CurTok.getType() == tok_if)
        {
            return ParseIfExpr();
        }
        else if (CurTok.getType() == tok_for)
        {
            return ParseForExpr();
        }
        else if (CurTok == '(')
        {
            return ParseParenExpr();
        }

        throw ParseError("unknown token when expecting an expression");
    }

    ExprAST Parser::ParseExpression()
    {
        auto LHS = ParsePrimary();

        return ParseBinOpRHS(0, std::move(LHS));
    }

    ExprAST Parser::ParseBinOpRHS(int ExprPrec,
                                  ExprAST &&LHS)
    {
        // If this is a binop, find its precedence.
        while (1)
        {
            int TokPrec = GetTokPrecedence();

            // If this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done.
            if (TokPrec < ExprPrec)
                return std::move(LHS);
            // Okay, we know this is a binop.

            int BinOp = CurTok.getCharValue();
            getNextToken(); // eat binop

            // Parse the primary expression after the binary operator.
            auto RHS = ParsePrimary();
            int NextPrec = GetTokPrecedence();
            if (TokPrec < NextPrec)
            {
                RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            }

            // Merge LHS/RHS.
            LHS = BinaryExprAST(BinOp, std::move(LHS), std::move(RHS));
        } // loop around to the top of the while loop.
    }

    IfExprAST Parser::ParseIfExpr()
    {
        getNextToken();

        // condition.
        auto Cond = ParseExpression();

        expectKeyword(tok_then, "expected then");
        auto Then = ParseExpression();

        expectKeyword(tok_else, "expected else");
        auto Else = ParseExpression();

        return {std::move(Cond), std::move(Then), std::move(Else)};
    }

    ForExprAST Parser::ParseForExpr()
    {
        getNextToken(); // consume for

        std::string varName = expectIdentifier("expected identifier after for");
        expectChar('=', "expected = after for");

        auto start = ParseExpression();
        expectChar(',', "expected ',' after for start value");
        auto end = ParseExpression();

        std::optional<ExprAST> step;
        if (CurTok == ',')
        {
            getNextToken();
            step = ParseExpression();
        }

        expectKeyword(tok_in, "expected 'in' after for");
        auto body = ParseExpression();

        return ForExprAST(varName, std::move(start), std::move(end), std::move(step), std::move(body));
    }

    PrototypeAST Parser::ParsePrototype()
    {
        std::string FnName = expectIdentifier("Expected function name in prototype");
        expectChar('(', "Expected '(' in prototype");

        // Read the list of argument names.
        std::vector<std::string> ArgNames;
        while (CurTok.getType() == tok_identifier)
        {
            ArgNames.push_back(CurTok.getIdentifierValue());
            getNextToken();
        }
        expectChar(')', "Expected ')' in prototype");

        // success.

        return PrototypeAST(FnName, std::move(ArgNames));
    }

    FunctionAST Parser::ParseDefinition()
    {
        getNextToken(); // eat def.
        auto Proto = ParsePrototype();
        auto E = ParseExpression();

        return FunctionAST(std::move(Proto), std::move(E));
    }

    PrototypeAST Parser::ParseExtern()
    {
        getNextToken(); // eat extern.
        return ParsePrototype();
    }

    FunctionAST Parser::ParseTopLevelExpr()
    {
        auto E = ParseExpression();
        // Make an anonymous proto.
        PrototypeAST Proto{"__anon_expr", std::vector<std::string>()};
        return {std::move(Proto), std::move(E)};
    }
} // namespace kaleidoscope
