#ifndef INCLUDED_KALEIDOSCOPE_SYMBOLS_HPP
#define INCLUDED_KALEIDOSCOPE_SYMBOLS_HPP

#include <cassert>
#include <memory>
#include <unordered_map>

namespace kaleidoscope
{
    template <typename ValueType>
    class SymbolTable
    {
    public:
        SymbolTable(SymbolTable *surroundingScope = nullptr)
            : surroundingScope_(surroundingScope)
        {
        }

        ValueType *tryLookup(std::string const &name) const
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

        bool tryDeclare(std::string const &name, ValueType *value)
        {
            auto r = namedValues_.insert({name, value});
            return r.second;
        }

    protected:
        SymbolTable *surroundingScope_;

    private:
        std::unordered_map<std::string, ValueType *> namedValues_;
    };

    template <typename ValueType>
    class SymbolScope : public SymbolTable<ValueType>
    {
    public:
        SymbolScope(SymbolTable<ValueType> *&guardedPtr)
            : SymbolTable<ValueType>(guardedPtr),
              guardedPtr_(guardedPtr)
        {
            guardedPtr_ = this;
        }

        ~SymbolScope()
        {
            guardedPtr_ = SymbolTable<ValueType>::surroundingScope_;
        }

    private:
        SymbolTable<ValueType> *&guardedPtr_;
    };
}

#endif
