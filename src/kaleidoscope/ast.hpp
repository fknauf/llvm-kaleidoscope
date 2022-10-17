#ifndef INCLUDED_KALEIDOSCOPE_AST_HPP
#define INCLUDED_KALEIDOSCOPE_AST_HPP

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace kaleidoscope
{
    class NumberExprAST;
    class VariableExprAST;
    class UnaryExprAST;
    class BinaryExprAST;
    class CallExprAST;
    class IfExprAST;
    class ForExprAST;
    class VarExprAST;

    using ExprAST = std::variant<NumberExprAST,
                                 VariableExprAST,
                                 UnaryExprAST,
                                 BinaryExprAST,
                                 CallExprAST,
                                 IfExprAST,
                                 ForExprAST,
                                 VarExprAST>;

    class NumberExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val);

        double getVal() const noexcept;
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name);

        std::string const &getName() const noexcept;
    };

    class UnaryExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> Operand;

    public:
        UnaryExprAST(char op, ExprAST opd);

        char getOp() const noexcept;
        ExprAST const &getOperand() const noexcept;
    };

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char op, ExprAST LHS, ExprAST RHS);

        char getOp() const noexcept;
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
        ForExprAST(std::string varName, ExprAST start, ExprAST end, std::unique_ptr<ExprAST> step, ExprAST body);

        std::string const &getVarName() const noexcept;
        ExprAST const &getStart() const noexcept;
        ExprAST const &getEnd() const noexcept;
        ExprAST const *getStep() const noexcept;
        ExprAST const &getBody() const noexcept;

    private:
        std::string varName_;
        std::unique_ptr<ExprAST> start_;
        std::unique_ptr<ExprAST> end_;
        std::unique_ptr<ExprAST> step_;
        std::unique_ptr<ExprAST> body_;
    };

    class VariableDeclarationAST
    {
    public:
        VariableDeclarationAST(std::string const &name, ExprAST initVal);

        std::string const &getName() const noexcept;
        ExprAST const &getInitVal() const noexcept;

    private:
        std::string name_;
        std::unique_ptr<ExprAST> initVal_;
    };

    class VarExprAST
    {
    public:
        VarExprAST(std::vector<VariableDeclarationAST> declarations, ExprAST Body);

        std::vector<VariableDeclarationAST> const &getDeclarations() const noexcept;
        ExprAST const &getBody() const noexcept;

    private:
        std::vector<VariableDeclarationAST> declarations_;
        std::unique_ptr<ExprAST> body_;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST
    {
        std::string Callee;
        std::vector<ExprAST> Args;

    public:
        CallExprAST(CallExprAST const &) = delete;
        CallExprAST(CallExprAST &&) = default;
        CallExprAST(const std::string &Callee,
                    std::vector<ExprAST> Args);

        CallExprAST &operator=(CallExprAST const &) = delete;
        CallExprAST &operator=(CallExprAST &&) = default;

        std::string const &getCallee() const noexcept;
        std::vector<ExprAST> const &getArgs() const noexcept;
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
        FunctionAST(PrototypeAST Proto, ExprAST Body);

        PrototypeAST const &getProto() const noexcept;
        ExprAST const &getBody() const noexcept;
    };
} // namespace kaleidoscope

#endif