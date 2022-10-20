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

    CodeGenerator::CodeGenerator(Parser &p, llvm::DataLayout dataLayout, std::string const &moduleName)
        : TheParser(p),
          dataLayout(std::move(dataLayout)),
          TheContext(std::make_unique<llvm::LLVMContext>()),
          globalSymbols_(nullptr),
          activeScope_(&globalSymbols_)
    {
        finalizeModule(moduleName);
    }

    llvm::orc::ThreadSafeModule CodeGenerator::finalizeModule(std::string const &newModuleName)
    {
        if (debugInfo_)
        {
            debugInfo_->finalize();
        }

        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto mod = std::make_unique<llvm::Module>(newModuleName, *ctx);
        mod->setDataLayout(dataLayout);

        TheContext.swap(ctx);
        TheModule.swap(mod);
        TheBuilder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

        TheModule->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
        // TheModule->addModuleFlag(llvm::Module::Warning, "CodeView", 1);

        debugInfo_ = std::make_unique<DebugInfo>(*TheModule);

        return llvm::orc::ThreadSafeModule(std::move(mod), std::move(ctx));
    }

    llvm::Value *CodeGenerator::getConstant(double value) const
    {
        return llvm::ConstantFP::get(*TheContext, llvm::APFloat(value));
    }

    llvm::Value *CodeGenerator::generateOptional(ExprAST const *ast, double defaultValue)
    {
        return ast ? (*this)(*ast) : getConstant(defaultValue);
    }

    llvm::Value *CodeGenerator::getBoolCondition(llvm::Value *condValue, llvm::Twine const &name)
    {
        return TheBuilder->CreateFCmpONE(condValue, getConstant(0.0), name);
    }

    llvm::AllocaInst *CodeGenerator::createScopedVariable(llvm::Function *F, std::string const &varName)
    {
        llvm::IRBuilder<> tempBuilder(&F->getEntryBlock(), F->getEntryBlock().begin());
        return tempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*TheContext), 0, varName.c_str());
    }

    llvm::Value *CodeGenerator::operator()(ExprAST const &expr)
    {
        return std::visit(*this, expr);
    }

    llvm::Value *CodeGenerator::operator()(NumberExprAST const &expr)
    {
        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());
        return getConstant(expr.getVal());
    }

    llvm::Value *CodeGenerator::operator()(VariableExprAST const &expr)
    {
        auto value = activeScope_->tryLookup(expr.getName());

        if (value == nullptr)
        {
            throw CodeGenerationError("Unknown variable " + expr.getName());
        }

        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());
        return TheBuilder->CreateLoad(llvm::Type::getDoubleTy(*TheContext), value, expr.getName().c_str());
    }

    llvm::Value *CodeGenerator::operator()(UnaryExprAST const &expr)
    {
        auto opd = (*this)(expr.getOperand());
        auto F = getFunction(std::string("unary") + expr.getOp(), "Unknown unary operator %1%");

        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());
        return TheBuilder->CreateCall(F, opd, "unop");
    }

    llvm::Value *CodeGenerator::operator()(BinaryExprAST const &expr)
    {
        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());

        if (expr.getOp() == '=')
        {
            if (auto destVarAST = std::get_if<VariableExprAST>(&expr.getLHS()))
            {
                auto destVarSpace = activeScope_->tryLookup(destVarAST->getName());
                auto assignedValue = (*this)(expr.getRHS());

                TheBuilder->CreateStore(assignedValue, destVarSpace);

                return assignedValue;
            }
            else
            {
                throw CodeGenerationError("destination of '=' must be a variable");
            }
        }
        else
        {
            llvm::Value *L = (*this)(expr.getLHS());
            llvm::Value *R = (*this)(expr.getRHS());

            switch (expr.getOp())
            {
            case '+':
                return TheBuilder->CreateFAdd(L, R, "addtmp");
            case '-':
                return TheBuilder->CreateFSub(L, R, "subtmp");
            case '*':
                return TheBuilder->CreateFMul(L, R, "multmp");
            case '/':
                return TheBuilder->CreateFDiv(L, R, "divtmp");
            case '<':
            {
                llvm::Value *C = TheBuilder->CreateFCmpULT(L, R, "cmptmp");

                return TheBuilder->CreateUIToFP(C, llvm::Type::getDoubleTy(*TheContext), "booltmp");
            }
            default:
                break;
            }

            auto F = getFunction(std::string("binary") + expr.getOp(), "binary operator %1% not found!");

            llvm::Value *Vals[] = {L, R};
            return TheBuilder->CreateCall(F, Vals, "binop");
        }
    }

    llvm::Value *CodeGenerator::operator()(CallExprAST const &expr)
    {
        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());

        // Look up the name in the global module table.
        llvm::Function *CalleeF = getFunction(expr.getCallee(), "Unknown function referenced: %1%");

        // If argument mismatch error.
        if (CalleeF->arg_size() != expr.getArgs().size())
            throw CodeGenerationError("Incorrect # arguments passed");

        std::vector<llvm::Value *> ArgsV;
        ArgsV.reserve(expr.getArgs().size());
        std::transform(begin(expr.getArgs()), end(expr.getArgs()), std::back_inserter(ArgsV), std::ref(*this));

        return TheBuilder->CreateCall(CalleeF, ArgsV, "calltmp");
    }

    llvm::Value *CodeGenerator::operator()(IfExprAST const &expr)
    {
        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());

        auto conditionValue = (*this)(expr.getCondition());
        auto condition = getBoolCondition(conditionValue, "ifcond");

        auto parentFunction = TheBuilder->GetInsertBlock()->getParent();

        auto ThenBBStart = llvm::BasicBlock::Create(*TheContext, "then", parentFunction);
        auto ElseBBStart = llvm::BasicBlock::Create(*TheContext, "else", parentFunction);
        auto MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont", parentFunction);

        TheBuilder->CreateCondBr(condition, ThenBBStart, ElseBBStart);

        TheBuilder->SetInsertPoint(ThenBBStart);
        auto valThen = (*this)(expr.getThenBranch());
        TheBuilder->CreateBr(MergeBB);
        auto ThenBBEnd = TheBuilder->GetInsertBlock();

        TheBuilder->SetInsertPoint(ElseBBStart);
        auto valElse = (*this)(expr.getElseBranch());
        TheBuilder->CreateBr(MergeBB);
        auto ElseBBEnd = TheBuilder->GetInsertBlock();

        TheBuilder->SetInsertPoint(MergeBB);

        auto PN = TheBuilder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");
        PN->addIncoming(valThen, ThenBBEnd);
        PN->addIncoming(valElse, ElseBBEnd);

        return PN;
    }

    llvm::Value *CodeGenerator::operator()(ForExprAST const &expr)
    {
        auto TheFunction = TheBuilder->GetInsertBlock()->getParent();
        auto loopVarSpace = createScopedVariable(TheFunction, expr.getVarName());

        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());

        auto startVal = (*this)(expr.getStart());
        TheBuilder->CreateStore(startVal, loopVarSpace);

        auto LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
        // explicit fall-through from current to loop. Implicit is not allowed.
        TheBuilder->CreateBr(LoopBB);
        TheBuilder->SetInsertPoint(LoopBB);

        {
            SymbolScope loopScope(activeScope_);
            loopScope.tryDeclare(expr.getVarName(), loopVarSpace);

            (*this)(expr.getBody());

            llvm::Value *StepVal = generateOptional(expr.getStep(), 1.0);

            llvm::Value *curLoopVarValue = TheBuilder->CreateLoad(llvm::Type::getDoubleTy(*TheContext), loopVarSpace, expr.getVarName());
            llvm::Value *nextLoopVarValue = TheBuilder->CreateFAdd(curLoopVarValue, StepVal, "nextVar");
            TheBuilder->CreateStore(nextLoopVarValue, loopVarSpace);

            llvm::Value *endVal = (*this)(expr.getEnd());
            llvm::Value *endCond = getBoolCondition(endVal, "loopcond");

            auto AfterBB = llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);
            TheBuilder->CreateCondBr(endCond, LoopBB, AfterBB);
            TheBuilder->SetInsertPoint(AfterBB);
        }

        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext));
    }

    llvm::Value *CodeGenerator::operator()(VarExprAST const &expr)
    {
        SymbolScope varScope(activeScope_);
        auto F = TheBuilder->GetInsertBlock()->getParent();

        for (auto &decl : expr.getDeclarations())
        {
            llvm::Value *initVal = (*this)(decl.getInitVal());

            auto space = createScopedVariable(F, decl.getName());
            TheBuilder->CreateStore(initVal, space);

            if (!varScope.tryDeclare(decl.getName(), space))
            {
                throw CodeGenerationError("redefined variable '" + decl.getName() + "' in var block");
            }
        }

        debugInfo_->emitLocation(*TheBuilder, expr.getLocation());
        return (*this)(expr.getBody());
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
            TheBuilder->SetInsertPoint(entryBlock);

            {
                DebugScope debugScope(*debugInfo_, *TheBuilder, F, expr.getProto());
                SymbolScope functionScope(activeScope_);

                int argIdx = 0;
                for (auto &arg : F->args())
                {
                    auto varSpace = createScopedVariable(F, arg.getName().str());
                    debugInfo_->declareParameter(*TheBuilder, varSpace, arg.getName().str(), argIdx, expr.getProto().getLocation());
                    TheBuilder->CreateStore(&arg, varSpace);
                    functionScope.tryDeclare(std::string(arg.getName()), varSpace);

                    ++argIdx;
                }

                debugInfo_->emitLocation(*TheBuilder, std::visit([](ASTBase const &ast)
                                                                 { return ast.getLocation(); },
                                                                 expr.getBody()));
                llvm::Value *bodyCode = (*this)(expr.getBody());
                TheBuilder->CreateRet(bodyCode);
            }

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
