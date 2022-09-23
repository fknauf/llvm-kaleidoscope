#include "parser.hpp"

#include <iostream>
#include <sstream>

namespace kaleidoscope
{
    ParseError::ParseError(std::string const &errMsg)
        : Error("Parse error: " + errMsg)
    {
    }

    Parser::Parser(Lexer &lexer) : lexer_(lexer)
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

        if (CurTok != ')')
            throw ParseError("expected ')'");
        getNextToken(); // eat ).

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

                if (CurTok != ',')
                    throw ParseError("Expected ')' or ',' in argument list");

                getNextToken();
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

    PrototypeAST Parser::ParsePrototype()
    {
        if (CurTok.getType() != tok_identifier)
            throw ParseError("Expected function name in prototype");

        std::string FnName = CurTok.getIdentifierValue();
        getNextToken();

        if (CurTok != '(')
            throw ParseError("Expected '(' in prototype");

        // Read the list of argument names.
        std::vector<std::string> ArgNames;
        while (getNextToken().getType() == tok_identifier)
            ArgNames.push_back(CurTok.getIdentifierValue());
        if (CurTok != ')')
            throw ParseError("Expected ')' in prototype");

        // success.
        getNextToken(); // eat ')'.

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
        PrototypeAST Proto{"", std::vector<std::string>()};
        return {std::move(Proto), std::move(E)};
    }
} // namespace kaleidoscope
