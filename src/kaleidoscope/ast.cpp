#include "ast.hpp"

namespace kaleidoscope
{
    BinaryExprAST::BinaryExprAST(char op, ExprAST LHS, ExprAST RHS)
        : Op(op),
          LHS(std::make_unique<ExprAST>(std::move(LHS))),
          RHS(std::make_unique<ExprAST>(std::move(RHS))) {}

    ExprAST const &BinaryExprAST::getLHS() const noexcept { return *LHS; }
    ExprAST const &BinaryExprAST::getRHS() const noexcept { return *RHS; }
}
