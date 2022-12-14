#include "parser.hpp"

#include <iostream>
#include <sstream>
#include <cctype>

namespace kaleidoscope
{
    ParseError::ParseError(std::string const &errMsg)
        : Error("Parse error: " + errMsg)
    {
    }

    Parser::Parser(Lexer &lexer, std::string const &topLevelSymbolName)
        : lexer_(lexer),
          topLevelSymbolName_(topLevelSymbolName),
          binOpPrecedence{
              {'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40}}
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

    bool Parser::tryConsumeChar(char expected)
    {
        if (CurTok == expected)
        {
            getNextToken();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Parser::tryConsumeKeyword(TokenType expected)
    {
        if (CurTok.getType() == expected)
        {
            getNextToken();
            return true;
        }
        else
        {
            return false;
        }
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

    char Parser::expectAscii(std::string const &errMsg)
    {
        if (CurTok.getType() != tok_char || !std::isprint(CurTok.getCharValue()))
        {
            throw ParseError(errMsg);
        }

        char result = CurTok.getCharValue();
        getNextToken();

        return result;
    }

    void Parser::expectChar(char expected, std::string const &errMsg)
    {
        if (!tryConsumeChar(expected))
        {
            throw ParseError(errMsg);
        }
    }

    void Parser::expectKeyword(TokenType expected, std::string const &errMsg)
    {
        if (!tryConsumeKeyword(expected))
        {
            throw ParseError(errMsg);
        }
    }

    NumberExprAST Parser::ParseNumberExpr()
    {
        NumberExprAST Result(lexer_.getLocation(), CurTok.getNumValue());
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
        SourceLocation loc = lexer_.getLocation();
        std::string IdName = CurTok.getIdentifierValue();

        getNextToken(); // eat identifier.

        if (!tryConsumeChar('(')) // Simple variable ref.
        {
            return VariableExprAST(loc, IdName);
        }

        // Call.
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

        return CallExprAST(loc, IdName, std::move(Args));
    }

    ExprAST Parser::ParsePrimary()
    {
        switch (CurTok.getType())
        {
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case tok_if:
            return ParseIfExpr();
        case tok_for:
            return ParseForExpr();
        case tok_var:
            return ParseVarExpr();
        default:
            if (CurTok == '(')
            {
                return ParseParenExpr();
            }
            break;
        }

        throw ParseError("unknown token when expecting an expression");
    }

    ExprAST Parser::ParseExpression()
    {
        auto LHS = ParseUnary();

        return ParseBinOpRHS(0, std::move(LHS));
    }

    ExprAST Parser::ParseUnary()
    {
        SourceLocation loc = lexer_.getLocation();

        if (CurTok == '(' ||
            CurTok == ',' ||
            CurTok.getType() != tok_char)
        {
            return ParsePrimary();
        }

        char op = expectAscii("invalid unary operator %1%");
        auto opd = ParseUnary();

        return UnaryExprAST(loc, op, std::move(opd));
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
            {
                return std::move(LHS);
            }
            // Okay, we know this is a binop.

            int BinOp = CurTok.getCharValue();
            SourceLocation loc = lexer_.getLocation();

            getNextToken(); // eat binop

            // Parse the primary expression after the binary operator.
            auto RHS = ParseUnary();
            int NextPrec = GetTokPrecedence();
            if (TokPrec < NextPrec)
            {
                RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            }

            // Merge LHS/RHS.
            LHS = BinaryExprAST(loc, BinOp, std::move(LHS), std::move(RHS));
        } // loop around to the top of the while loop.
    }

    IfExprAST Parser::ParseIfExpr()
    {
        SourceLocation loc = lexer_.getLocation();

        getNextToken();

        // condition.
        auto Cond = ParseExpression();

        expectKeyword(tok_then, "expected then");
        auto Then = ParseExpression();

        expectKeyword(tok_else, "expected else");
        auto Else = ParseExpression();

        return {loc, std::move(Cond), std::move(Then), std::move(Else)};
    }

    ForExprAST Parser::ParseForExpr()
    {
        SourceLocation loc = lexer_.getLocation();
        getNextToken(); // consume for

        std::string varName = expectIdentifier("expected identifier after for");
        expectChar('=', "expected = after for");

        auto start = ParseExpression();
        expectChar(',', "expected ',' after for start value");
        auto end = ParseExpression();

        std::unique_ptr<ExprAST> step;
        if (tryConsumeChar(','))
        {
            step = std::make_unique<ExprAST>(ParseExpression());
        }

        expectKeyword(tok_in, "expected 'in' after for");
        auto body = ParseExpression();

        return {loc, varName, std::move(start), std::move(end), std::move(step), std::move(body)};
    }

    VarExprAST Parser::ParseVarExpr()
    {
        SourceLocation varLoc = lexer_.getLocation();
        getNextToken();

        std::vector<VariableDeclarationAST> varDecls;

        do
        {
            SourceLocation identLoc = lexer_.getLocation();
            std::string name = expectIdentifier("Expected identifier list after 'var'");
            ExprAST initVal = NumberExprAST(lexer_.getLocation(), 0.0);

            if (tryConsumeChar('='))
            {
                initVal = ParseExpression();
            }

            varDecls.emplace_back(identLoc, name, std::move(initVal));
        } while (tryConsumeChar(','));

        expectKeyword(tok_in, "expected 'in' keyword after 'var'");

        auto body = ParseExpression();

        return {varLoc, std::move(varDecls), std::move(body)};
    }

    PrototypeAST Parser::ParsePrototype()
    {
        SourceLocation loc = lexer_.getLocation();

        std::size_t opArgsCount;
        std::string FnName;
        int binprecedence = 30;

        switch (CurTok.getType())
        {
        case tok_identifier:
            FnName = CurTok.getIdentifierValue();
            getNextToken();
            opArgsCount = 0;
            break;
        case tok_unary:
            getNextToken();
            FnName = "unary";
            FnName += expectAscii("Expected unary operator");
            opArgsCount = 1;
            break;
        case tok_binary:
            getNextToken();
            FnName = "binary";
            FnName += expectAscii("Expected binary operator");
            opArgsCount = 2;

            if (CurTok.getType() == tok_number)
            {
                binprecedence = static_cast<int>(CurTok.getNumValue());
                getNextToken();
            }
            break;
        default:
            throw ParseError("Expected identifier, 'unary', or 'binary' in ParsePrototype");
        }

        expectChar('(', "Expected '(' in prototype");

        // Read the list of argument names.
        std::vector<std::string> ArgNames;
        while (CurTok.getType() == tok_identifier)
        {
            ArgNames.push_back(CurTok.getIdentifierValue());
            getNextToken();
        }
        expectChar(')', "Expected ')' in prototype");

        if (opArgsCount != 0 && ArgNames.size() != opArgsCount)
        {
            throw ParseError("Invalid number of operands for operator");
        }

        // success.

        return {loc, FnName, std::move(ArgNames), opArgsCount != 0, binprecedence};
    }

    FunctionAST Parser::ParseDefinition()
    {
        SourceLocation loc = lexer_.getLocation();

        getNextToken(); // eat def.
        auto Proto = ParsePrototype();
        auto E = ParseExpression();

        return {loc, std::move(Proto), std::move(E)};
    }

    PrototypeAST Parser::ParseExtern()
    {
        getNextToken(); // eat extern.
        return ParsePrototype();
    }

    FunctionAST Parser::ParseTopLevelExpr()
    {
        SourceLocation loc = lexer_.getLocation();
        auto E = ParseExpression();
        // Make an anonymous proto.
        PrototypeAST Proto{loc, topLevelSymbolName_, std::vector<std::string>()};
        return {loc, std::move(Proto), std::move(E)};
    }

    void Parser::registerOperator(PrototypeAST const &operatorProto)
    {
        if (operatorProto.isBinaryOperator())
        {
            binOpPrecedence[operatorProto.getOperatorName()] = operatorProto.getBinaryPrecedence();
        }
    }
    void Parser::removeOperator(PrototypeAST const &operatorProto)
    {
        if (operatorProto.isBinaryOperator())
        {
            binOpPrecedence.erase(operatorProto.getOperatorName());
        }
    }
} // namespace kaleidoscope
