#include "ast.hpp"

#include <cassert>

namespace kaleidoscope
{
    NumberExprAST::NumberExprAST(double Val)
        : Val(Val) {}

    double NumberExprAST::getVal() const noexcept
    {
        return Val;
    }

    VariableExprAST::VariableExprAST(const std::string &Name)
        : Name(Name) {}

    std::string const &VariableExprAST::getName() const noexcept
    {
        return Name;
    }

    UnaryExprAST::UnaryExprAST(char op, ExprAST opd)
        : Op(op),
          Operand(std::make_unique<ExprAST>(std::move(opd)))
    {
    }

    char UnaryExprAST::getOp() const noexcept
    {
        return Op;
    }

    ExprAST const &UnaryExprAST::getOperand() const noexcept
    {
        return *Operand;
    }

    BinaryExprAST::BinaryExprAST(char op, ExprAST LHS, ExprAST RHS)
        : Op(op),
          LHS(std::make_unique<ExprAST>(std::move(LHS))),
          RHS(std::make_unique<ExprAST>(std::move(RHS))) {}

    char BinaryExprAST::getOp() const noexcept
    {
        return Op;
    }

    ExprAST const &BinaryExprAST::getLHS() const noexcept
    {
        return *LHS;
    }
    ExprAST const &BinaryExprAST::getRHS() const noexcept
    {
        return *RHS;
    }

    IfExprAST::IfExprAST(ExprAST Cond, ExprAST ThenBranch, ExprAST ElseBranch)
        : condition_(std::make_unique<ExprAST>(std::move(Cond))),
          thenBranch_(std::make_unique<ExprAST>(std::move(ThenBranch))),
          elseBranch_(std::make_unique<ExprAST>(std::move(ElseBranch)))
    {
    }

    ExprAST const &IfExprAST::getCondition() const noexcept
    {
        return *condition_;
    }
    ExprAST const &IfExprAST::getThenBranch() const noexcept
    {
        return *thenBranch_;
    }
    ExprAST const &IfExprAST::getElseBranch() const noexcept
    {
        return *elseBranch_;
    }

    ForExprAST::ForExprAST(std::string varName, ExprAST start, ExprAST end, std::unique_ptr<ExprAST> step, ExprAST body)
        : varName_(varName),
          start_(std::make_unique<ExprAST>(std::move(start))),
          end_(std::make_unique<ExprAST>(std::move(end))),
          step_(std::move(step)),
          body_(std::make_unique<ExprAST>(std::move(body)))
    {
    }

    std::string const &ForExprAST::getVarName() const noexcept
    {
        return varName_;
    }
    ExprAST const &ForExprAST::getStart() const noexcept
    {
        return *start_;
    }
    ExprAST const &ForExprAST::getEnd() const noexcept
    {
        return *end_;
    }
    ExprAST const *ForExprAST::getStep() const noexcept
    {
        return step_.get();
    }
    ExprAST const &ForExprAST::getBody() const noexcept
    {
        return *body_;
    }

    CallExprAST::CallExprAST(const std::string &Callee,
                             std::vector<ExprAST> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    std::string const &CallExprAST::getCallee() const noexcept { return Callee; }
    std::vector<ExprAST> const &CallExprAST::getArgs() const noexcept { return Args; }

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

    VariableDeclarationAST::VariableDeclarationAST(std::string const &name, ExprAST initVal)
        : name_(name),
          initVal_(std::make_unique<ExprAST>(std::move(initVal)))
    {
    }

    std::string const &VariableDeclarationAST::getName() const noexcept { return name_; }
    ExprAST const &VariableDeclarationAST::getInitVal() const noexcept { return *initVal_; }

    VarExprAST::VarExprAST(std::vector<VariableDeclarationAST> declarations, ExprAST Body)
        : declarations_(std::move(declarations)),
          body_(std::make_unique<ExprAST>(std::move(Body)))
    {
    }

    std::vector<VariableDeclarationAST> const &VarExprAST::getDeclarations() const noexcept { return declarations_; }
    ExprAST const &VarExprAST::getBody() const noexcept { return *body_; }

    const std::string &PrototypeAST::getName() const noexcept
    {
        return Name;
    }
    const std::vector<std::string> &PrototypeAST::getArgs() const noexcept
    {
        return Args;
    };

    bool PrototypeAST::isOperator() const noexcept
    {
        return isOperator_;
    }
    bool PrototypeAST::isUnaryOperator() const noexcept
    {
        return isOperator() && Args.size() == 1;
    }
    bool PrototypeAST::isBinaryOperator() const noexcept
    {
        return isOperator() && Args.size() == 2;
    }
    char PrototypeAST::getOperatorName() const noexcept
    {
        assert(isOperator());
        return *getName().rbegin();
    }
    int PrototypeAST::getBinaryPrecedence() const noexcept
    {
        return precedence_;
    }

    FunctionAST::FunctionAST(PrototypeAST Proto,
                             ExprAST Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    PrototypeAST const &FunctionAST::getProto() const noexcept { return Proto; }
    ExprAST const &FunctionAST::getBody() const noexcept { return Body; }
}
