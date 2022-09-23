#ifndef INCLUDED_KALEIDOSCOPE_OPTIMIZER_HPP
#define INCLUDED_KALEIDOSCOPE_OPTIMIZER_HPP

#include <llvm/IR/Module.h>

namespace kaleidoscope
{
    void optimizeModule(llvm::Module &module);
}

#endif
