#include "ast.hpp"

#include <cassert>

namespace kaleidoscope
{
    BinaryExprAST::BinaryExprAST(char op, ExprAST LHS, ExprAST RHS)
        : Op(op),
          LHS(std::make_unique<ExprAST>(std::move(LHS))),
          RHS(std::make_unique<ExprAST>(std::move(RHS))) {}

    ExprAST const &BinaryExprAST::getLHS() const noexcept { return *LHS; }
    ExprAST const &BinaryExprAST::getRHS() const noexcept { return *RHS; }

    IfExprAST::IfExprAST(ExprAST Cond, ExprAST ThenBranch, ExprAST ElseBranch)
        : condition_(std::make_unique<ExprAST>(std::move(Cond))),
          thenBranch_(std::make_unique<ExprAST>(std::move(ThenBranch))),
          elseBranch_(std::make_unique<ExprAST>(std::move(ElseBranch)))
    {
    }

    ExprAST const &IfExprAST::getCondition() const noexcept { return *condition_; }
    ExprAST const &IfExprAST::getThenBranch() const noexcept { return *thenBranch_; }
    ExprAST const &IfExprAST::getElseBranch() const noexcept { return *elseBranch_; }

    ForExprAST::ForExprAST(std::string varName, ExprAST start, ExprAST end, std::optional<ExprAST> step, ExprAST body)
        : varName_(varName),
          start_(std::make_unique<ExprAST>(std::move(start))),
          end_(std::make_unique<ExprAST>(std::move(end))),
          step_(std::make_unique<std::optional<ExprAST>>(std::move(step))),
          body_(std::make_unique<ExprAST>(std::move(body)))
    {
    }

    std::string const &ForExprAST::getVarName() const noexcept
    {
        return varName_;
    }
    ExprAST const &ForExprAST::getStart() const noexcept { return *start_; }
    ExprAST const &ForExprAST::getEnd() const noexcept { return *end_; }
    std::optional<ExprAST> const &ForExprAST::getStep() const noexcept { return *step_; }
    ExprAST const &ForExprAST::getBody() const noexcept { return *body_; }

    CallExprAST::CallExprAST(const std::string &Callee,
                             std::vector<ExprAST> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    PrototypeAST::PrototypeAST(const std::string &name,
                               std::vector<std::string> Args,
                               bool isOperator,
                               int precedence)
        : Name(name),
          Args(std::move(Args)),
          isOperator_(isOperator),
          precedence_(precedence)
    {
    }

    const std::string &PrototypeAST::getName() const noexcept { return Name; }
    const std::vector<std::string> &PrototypeAST::getArgs() const noexcept { return Args; };

    bool PrototypeAST::isOperator() const noexcept { return isOperator_; }
    bool PrototypeAST::isUnaryOperator() const noexcept { return isOperator() && Args.size() == 1; }
    bool PrototypeAST::isBinaryOperator() const noexcept { return isOperator() && Args.size() == 2; }
    char PrototypeAST::getOperatorName() const noexcept
    {
        assert(isOperator());
        return *getName().rbegin();
    }
    int PrototypeAST::getBinaryPrecedence() const noexcept { return precedence_; }
}
