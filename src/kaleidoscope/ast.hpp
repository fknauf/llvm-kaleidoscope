#ifndef INCLUDED_KALEIDOSCOPE_AST_HPP
#define INCLUDED_KALEIDOSCOPE_AST_HPP

#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <optional>

namespace kaleidoscope
{
    class NumberExprAST;
    class VariableExprAST;
    class BinaryExprAST;
    class CallExprAST;
    class IfExprAST;
    class ForExprAST;

    using ExprAST = std::variant<NumberExprAST,
                                 VariableExprAST,
                                 BinaryExprAST,
                                 CallExprAST,
                                 IfExprAST,
                                 ForExprAST>;

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
        ExprAST const &getLHS() const noexcept;
        ExprAST const &getRHS() const noexcept;
    };

    class IfExprAST
    {
    public:
        IfExprAST(ExprAST Cond, ExprAST ThenBranch, ExprAST ElseBranch);

        ExprAST const &getCondition() const noexcept;
        ExprAST const &getThenBranch() const noexcept;
        ExprAST const &getElseBranch() const noexcept;

    private:
        std::unique_ptr<ExprAST> condition_, thenBranch_, elseBranch_;
    };

    class ForExprAST
    {
    public:
        ForExprAST(std::string varName, ExprAST start, ExprAST end, std::optional<ExprAST> step, ExprAST body);

        std::string const &getVarName() const noexcept;
        ExprAST const &getStart() const noexcept;
        ExprAST const &getEnd() const noexcept;
        std::optional<ExprAST> const &getStep() const noexcept;
        ExprAST const &getBody() const noexcept;

    private:
        std::string varName_;
        std::unique_ptr<ExprAST> start_, end_, body_;
        std::unique_ptr<std::optional<ExprAST>> step_;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST
    {
        std::string Callee;
        std::vector<ExprAST> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<ExprAST> Args);

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
        bool isOperator_;
        int precedence_;

    public:
        PrototypeAST(const std::string &name, std::vector<std::string> Args, bool isOperator = false, int precedence = 0);

        const std::string &getName() const noexcept;
        const std::vector<std::string> &getArgs() const noexcept;

        bool isOperator() const noexcept;
        bool isUnaryOperator() const noexcept;
        bool isBinaryOperator() const noexcept;
        char getOperatorName() const noexcept;
        int getBinaryPrecedence() const noexcept;
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

        PrototypeAST const &getProto() const noexcept { return Proto; }
        ExprAST const &getBody() const noexcept { return Body; }
    };
} // namespace kaleidoscope

#endif