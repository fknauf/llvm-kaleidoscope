#include "codegen.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

namespace kaleidoscope
{
    CodeGenerationError::CodeGenerationError(std::string const &errMsg)
        : runtime_error("Code generation error: " + errMsg)
    {
    }

    CodeGenerator::CodeGenerator()
        : TheContext(),
          Builder(TheContext),
          TheModule("", TheContext),
          NamedValues()
    {
    }

    llvm::Value *CodeGenerator::operator()(NumberExprAST const &expr)
    {
        return llvm::ConstantFP::get(TheContext, llvm::APFloat(expr.getVal()));
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
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFSub(L, R, "subtmp");
        case '*':
            return Builder.CreateFMul(L, R, "multmp");
        case '/':
            return Builder.CreateFDiv(L, R, "divtmp");
        case '<':
        {
            llvm::Value *C = Builder.CreateFCmpULT(L, R, "cmptmp");

            return Builder.CreateUIToFP(C, llvm::Type::getDoubleTy(TheContext), "booltmp");
        }
        default:
            throw CodeGenerationError("invalid binary operator");
        }
    }

    llvm::Value *CodeGenerator::operator()(CallExprAST const &expr)
    {
        // Look up the name in the global module table.
        llvm::Function *CalleeF = TheModule.getFunction(expr.getCallee());
        if (!CalleeF)
            throw CodeGenerationError("Unknown function referenced: " + expr.getCallee());

        // If argument mismatch error.
        if (CalleeF->arg_size() != expr.getArgs().size())
            throw CodeGenerationError("Incorrect # arguments passed");

        std::vector<llvm::Value *> ArgsV;
        for (auto &arg : expr.getArgs())
        {
            ArgsV.push_back(std::visit(*this, arg));
        }

        return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
    }

    llvm::Function *CodeGenerator::operator()(PrototypeAST const &expr)
    {
        std::vector<llvm::Type *> Doubles(expr.getArgs().size(), llvm::Type::getDoubleTy(TheContext));

        llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);
        llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, expr.getName(), TheModule);

        std::size_t ix = 0;
        for (auto &arg : F->args())
        {
            arg.setName(expr.getArgs()[ix]);
            ++ix;
        }

        return F;
    }

    llvm::Function *CodeGenerator::operator()(FunctionAST const &expr)
    {
        llvm::Function *F = nullptr;

        try
        {
            F = TheModule.getFunction(expr.getProto().getName());

            if (F == nullptr)
            {
                F = (*this)(expr.getProto());
            }

            if (F == nullptr)
            {
                throw CodeGenerationError("Could not create function " + expr.getProto().getName());
            }

            if (!F->empty())
            {
                throw CodeGenerationError("Function " + expr.getProto().getName() + " is already defined.");
            }

            llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(TheContext, "entry", F);
            Builder.SetInsertPoint(entryBlock);

            NamedValues.clear();
            for (auto &arg : F->args())
            {
                NamedValues[std::string(arg.getName())] = &arg;
            }

            llvm::Value *bodyCode = std::visit(*this, expr.getBody());
            Builder.CreateRet(bodyCode);

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

            return nullptr;
        }
    }
}
