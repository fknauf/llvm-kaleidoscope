#ifndef INCLUDED_KALEIDOSCOPE_AST_HPP
#define INCLUDED_KALEIDOSCOPE_AST_HPP

#include <string>
#include <memory>
#include <vector>
#include <variant>

namespace kaleidoscope
{
    class NumberExprAST;
    class VariableExprAST;
    class BinaryExprAST;
    class CallExprAST;

    using ExprAST = std::variant<NumberExprAST, VariableExprAST, BinaryExprAST, CallExprAST>;

    class NumberExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val) : Val(Val) {}

        double getVal() const noexcept { return Val; }
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name(Name) {}

        std::string const &getName() const noexcept { return Name; }
    };

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char op, ExprAST LHS, ExprAST RHS);

        char getOp() const noexcept { return Op; }
        ExprAST const *getLHS() const noexcept;
        ExprAST const *getRHS() const noexcept;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST
    {
        std::string Callee;
        std::vector<ExprAST> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<ExprAST> Args)
            : Callee(Callee), Args(std::move(Args)) {}

        auto const &getCallee() const noexcept { return Callee; }
        auto const &getArgs() const noexcept { return Args; }
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the
    /// number of arguments the function takes).
    class PrototypeAST
    {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &name, std::vector<std::string> Args)
            : Name(name), Args(std::move(Args)) {}

        const std::string &getName() const { return Name; }
        const std::vector<std::string> &getArgs() const { return Args; };
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST
    {
        PrototypeAST Proto;
        ExprAST Body;

    public:
        FunctionAST(PrototypeAST Proto,
                    ExprAST Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
    };
} // namespace kaleidoscope

#endif