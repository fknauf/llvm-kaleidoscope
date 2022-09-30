#include "codegen.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>

#include <boost/format.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>

namespace kaleidoscope
{
    CodeGenerationError::CodeGenerationError(std::string const &errMsg)
        : Error("Code generation error: " + errMsg)
    {
    }

    CodeGenerator::CodeGenerator(Parser &p, llvm::DataLayout dataLayout)
        : TheParser(p),
          dataLayout(std::move(dataLayout)),
          TheContext(std::make_unique<llvm::LLVMContext>())
    {
        stealModule();
    }

    llvm::orc::ThreadSafeModule CodeGenerator::stealModule()
    {
        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto mod = std::make_unique<llvm::Module>("", *ctx);

        mod->setDataLayout(dataLayout);

        TheContext.swap(ctx);
        TheModule.swap(mod);
        Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

        return llvm::orc::ThreadSafeModule(std::move(mod), std::move(ctx));
    }

    llvm::Value *CodeGenerator::operator()(NumberExprAST const &expr)
    {
        return llvm::ConstantFP::get(*TheContext, llvm::APFloat(expr.getVal()));
    }

    llvm::Value *CodeGenerator::operator()(VariableExprAST const &expr)
    {
        auto iter = NamedValues.find(expr.getName());

        if (iter == NamedValues.end())
        {
            throw CodeGenerationError("Unknown variable " + expr.getName());
        }

        return iter->second;
    }

    llvm::Value *CodeGenerator::operator()(UnaryExprAST const &expr)
    {
        auto opd = (*this)(expr.getOperand());
        auto F = getFunction(std::string("unary") + expr.getOp(), "Unknown unary operator %1%");
        return Builder->CreateCall(F, opd, "unop");
    }

    llvm::Value *CodeGenerator::operator()(BinaryExprAST const &expr)
    {
        llvm::Value *L = (*this)(expr.getLHS());
        llvm::Value *R = (*this)(expr.getRHS());

        switch (expr.getOp())
        {
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return Builder->CreateFMul(L, R, "multmp");
        case '/':
            return Builder->CreateFDiv(L, R, "divtmp");
        case '<':
        {
            llvm::Value *C = Builder->CreateFCmpULT(L, R, "cmptmp");

            return Builder->CreateUIToFP(C, llvm::Type::getDoubleTy(*TheContext), "booltmp");
        }
        default:
            break;
        }

        auto F = getFunction(std::string("binary") + expr.getOp(), "binary operator %1% not found!");

        llvm::Value *Vals[] = {L, R};
        return Builder->CreateCall(F, Vals, "binop");
    }

    llvm::Value *CodeGenerator::operator()(CallExprAST const &expr)
    {
        // Look up the name in the global module table.
        llvm::Function *CalleeF = getFunction(expr.getCallee(), "Unknown function referenced: %1%");

        // If argument mismatch error.
        if (CalleeF->arg_size() != expr.getArgs().size())
            throw CodeGenerationError("Incorrect # arguments passed");

        std::vector<llvm::Value *> ArgsV;
        ArgsV.reserve(expr.getArgs().size());
        std::transform(begin(expr.getArgs()), end(expr.getArgs()), std::back_inserter(ArgsV), std::ref(*this));

        return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
    }

    llvm::Value *CodeGenerator::operator()(IfExprAST const &expr)
    {
        auto valCond = (*this)(expr.getCondition());
        valCond = Builder->CreateFCmpONE(valCond, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");

        auto parentFunction = Builder->GetInsertBlock()->getParent();

        auto ThenBB = llvm::BasicBlock::Create(*TheContext, "then", parentFunction);
        // else, merge werden fuer die condition gebraucht, aber sollen erst nach allen Bestandteilen
        // des then-Blocks an die Funktion angehangen werden. Darum hier ohne parentFunction-Parameter
        auto ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
        auto MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

        Builder->CreateCondBr(valCond, ThenBB, ElseBB);

        Builder->SetInsertPoint(ThenBB);
        auto valThen = (*this)(expr.getThenBranch());
        Builder->CreateBr(MergeBB);
        ThenBB = Builder->GetInsertBlock();

        // ElseBB an Parent-Funktion anhaengen, dann Inhalt generieren
        parentFunction->getBasicBlockList().push_back(ElseBB);
        Builder->SetInsertPoint(ElseBB);
        auto valElse = (*this)(expr.getElseBranch());
        Builder->CreateBr(MergeBB);
        ElseBB = Builder->GetInsertBlock();

        // Selbes Spiel fuer Merge-Block
        parentFunction->getBasicBlockList().push_back(MergeBB);
        Builder->SetInsertPoint(MergeBB);
        auto PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");
        PN->addIncoming(valThen, ThenBB);
        PN->addIncoming(valElse, ElseBB);

        return PN;
    }

    llvm::Value *CodeGenerator::operator()(ForExprAST const &expr)
    {
        auto startVal = (*this)(expr.getStart());

        auto TheFunction = Builder->GetInsertBlock()->getParent();
        auto PreheaderBB = Builder->GetInsertBlock();
        auto LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);

        // explicit fall-through from current to loop. Implicit is not allowed.
        Builder->CreateBr(LoopBB);
        Builder->SetInsertPoint(LoopBB);

        // Phi node for variable, first input is the start value. Second will be the loop counter
        auto Variable = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, expr.getVarName().c_str());
        Variable->addIncoming(startVal, PreheaderBB);

        llvm::Value *oldVal = NamedValues[expr.getVarName()];
        NamedValues[expr.getVarName()] = Variable;

        (*this)(expr.getBody());

        llvm::Value *StepVal = nullptr;
        if (expr.getStep())
        {
            StepVal = (*this)(*expr.getStep());
        }
        else
        {
            StepVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(1.0));
        }

        llvm::Value *nextVar = Builder->CreateFAdd(Variable, StepVal, "nextVar");
        llvm::Value *endVal = (*this)(expr.getEnd());
        llvm::Value *endCond = Builder->CreateFCmpONE(endVal, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "loopcond");

        auto LoopEndBB = Builder->GetInsertBlock();
        auto AfterBB = llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);
        Builder->CreateCondBr(endCond, LoopBB, AfterBB);
        Builder->SetInsertPoint(AfterBB);

        Variable->addIncoming(nextVar, LoopEndBB);

        if (oldVal)
        {
            NamedValues[expr.getVarName()] = oldVal;
        }
        else
        {
            NamedValues.erase(expr.getVarName());
        }

        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext));
    }

    llvm::Function *CodeGenerator::getFunction(std::string const &name, std::string const &errmsg_format)
    {
        llvm::Function *F = TheModule->getFunction(name);

        if (F)
        {
            return F;
        }

        auto FI = FunctionProtos.find(name);

        if (FI != FunctionProtos.end())
        {
            return (*this)(FI->second);
        }

        std::ostringstream formatter;
        formatter << boost::format(errmsg_format) % name;

        throw CodeGenerationError(formatter.str());
    }

    llvm::Function *CodeGenerator::operator()(PrototypeAST const &expr)
    {
        std::vector<llvm::Type *> Doubles(expr.getArgs().size(), llvm::Type::getDoubleTy(*TheContext));

        llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
        llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, expr.getName(), *TheModule);

        std::size_t ix = 0;
        for (auto &arg : F->args())
        {
            arg.setName(expr.getArgs()[ix]);
            ++ix;
        }

        return F;
    }

    void CodeGenerator::registerExtern(PrototypeAST ast)
    {
        FunctionProtos.insert_or_assign(ast.getName(), ast);
    }

    llvm::Function *CodeGenerator::operator()(FunctionAST const &expr)
    {
        llvm::Function *F = nullptr;

        try
        {
            registerExtern(expr.getProto());
            F = getFunction(expr.getProto().getName(), "Could not create function %1%");

            TheParser.registerOperator(expr.getProto());

            llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*TheContext, "entry", F);
            Builder->SetInsertPoint(entryBlock);

            NamedValues.clear();
            for (auto &arg : F->args())
            {
                NamedValues[std::string(arg.getName())] = &arg;
            }

            llvm::Value *bodyCode = (*this)(expr.getBody());
            Builder->CreateRet(bodyCode);

            llvm::verifyFunction(*F);

            return F;
        }
        catch (CodeGenerationError const &e)
        {
            std::cerr << e.what() << std::endl;

            TheParser.removeOperator(expr.getProto());

            if (F != nullptr)
            {
                F->eraseFromParent();
            }

            throw;
        }
    }
}
