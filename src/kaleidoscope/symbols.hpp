#ifndef INCLUDED_KALEIDOSCOPE_SYMBOLS_HPP
#define INCLUDED_KALEIDOSCOPE_SYMBOLS_HPP

#include <llvm/IR/Instructions.h>

#include <memory>
#include <unordered_map>

namespace kaleidoscope
{
    class SymbolTable
    {
    public:
        SymbolTable(SymbolTable *surroundingScope = nullptr);

        llvm::AllocaInst *tryLookup(std::string const &name) const;
        bool tryDeclare(std::string const &name, llvm::AllocaInst *value);

    protected:
        SymbolTable *surroundingScope_;

    private:
        std::unordered_map<std::string, llvm::AllocaInst *> namedValues_;
    };

    class SymbolScope : public SymbolTable
    {
    public:
        SymbolScope(SymbolTable *&guardedPtr);
        ~SymbolScope();

    private:
        SymbolTable *&guardedPtr_;
    };
}

#endif
