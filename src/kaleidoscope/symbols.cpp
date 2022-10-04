#include "symbols.hpp"

#include <cassert>

namespace kaleidoscope
{
    SymbolTable::SymbolTable(SymbolTable *surroundingScope)
        : surroundingScope_(surroundingScope)
    {
    }

    llvm::AllocaInst *SymbolTable::tryLookup(std::string const &name) const
    {
        auto iter = namedValues_.find(name);

        if (iter != namedValues_.end())
        {
            return iter->second;
        }
        else if (surroundingScope_)
        {
            return surroundingScope_->tryLookup(name);
        }

        return nullptr;
    }

    bool SymbolTable::tryDeclare(std::string const &name, llvm::AllocaInst *value)
    {
        auto r = namedValues_.insert({name, value});
        return r.second;
    }

    SymbolScope::SymbolScope(SymbolTable *&guardedPtr)
        : SymbolTable(guardedPtr),
          guardedPtr_(guardedPtr)
    {
        guardedPtr_ = this;
    }

    SymbolScope::~SymbolScope()
    {
        guardedPtr_ = surroundingScope_;
    }
}
