#include "codegen.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>

#include <boost/format.hpp>

#include <iostream>
#include <sstream>
#include <utility>

namespace kaleidoscope
{
    CodeGenerationError::CodeGenerationError(std::string const &errMsg)
        : Error("Code generation error: " + errMsg)
    {
    }

    CodeGenerator::CodeGenerator(llvm::DataLayout dataLayout)
        : dataLayout(std::move(dataLayout)),
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

    llvm::Value *CodeGenerator::operator()(BinaryExprAST const &expr)
    {
        llvm::Value *L = std::visit(*this, expr.getLHS());
        llvm::Value *R = std::visit(*this, expr.getRHS());

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
            throw CodeGenerationError("invalid binary operator");
        }
    }

    llvm::Value *CodeGenerator::operator()(CallExprAST const &expr)
    {
        // Look up the name in the global module table.
        llvm::Function *CalleeF = getFunction(expr.getCallee(), "Unknown function referenced: %1%");

        // If argument mismatch error.
        if (CalleeF->arg_size() != expr.getArgs().size())
            throw CodeGenerationError("Incorrect # arguments passed");

        std::vector<llvm::Value *> ArgsV;
        for (auto &arg : expr.getArgs())
        {
            ArgsV.push_back(std::visit(*this, arg));
        }

        return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
    }

    llvm::Value *CodeGenerator::operator()(IfExprAST const &expr)
    {
        auto valCond = std::visit(*this, expr.getCondition());

        auto irHead = Builder->CreateFCmpONE(valCond, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");

        auto parentFunction = Builder->GetInsertBlock()->getParent();

        auto ThenBB = llvm::BasicBlock::Create(*TheContext, "then", parentFunction);
        auto ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
        auto MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

        Builder->CreateCondBr(irHead, ThenBB, ElseBB);

        Builder->SetInsertPoint(ThenBB);
        auto valThen = std::visit(*this, expr.getThenBranch());
        Builder->CreateBr(MergeBB);
        auto resultThen = Builder->GetInsertBlock();

        parentFunction->getBasicBlockList().push_back(ElseBB);
        Builder->SetInsertPoint(ElseBB);
        auto valElse = std::visit(*this, expr.getElseBranch());
        Builder->CreateBr(MergeBB);
        auto resultElse = Builder->GetInsertBlock();

        parentFunction->getBasicBlockList().push_back(MergeBB);
        Builder->SetInsertPoint(MergeBB);
        auto PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");

        PN->addIncoming(valThen, ThenBB);
        PN->addIncoming(valElse, ElseBB);

        return PN;
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

            llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*TheContext, "entry", F);
            Builder->SetInsertPoint(entryBlock);

            NamedValues.clear();
            for (auto &arg : F->args())
            {
                NamedValues[std::string(arg.getName())] = &arg;
            }

            llvm::Value *bodyCode = std::visit(*this, expr.getBody());
            Builder->CreateRet(bodyCode);

            llvm::verifyFunction(*F);

            return F;
        }
        catch (CodeGenerationError const &e)
        {
            std::cerr << e.what() << std::endl;

            if (F != nullptr)
            {
                F->eraseFromParent();
            }

            throw;
        }
    }
}
