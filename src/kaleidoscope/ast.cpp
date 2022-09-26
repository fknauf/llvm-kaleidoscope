#include "ast.hpp"

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

    CallExprAST::CallExprAST(const std::string &Callee,
                             std::vector<ExprAST> Args)
        : Callee(Callee), Args(std::move(Args)) {}

}
