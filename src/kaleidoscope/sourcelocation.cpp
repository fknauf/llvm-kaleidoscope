#include "sourcelocation.hpp"

namespace kaleidoscope
{
    void SourceLocation::advance(char c)
    {
        if (c == '\n')
        {
            advanceLine();
        }
        else
        {
            ++column_;
        }
    }

    void SourceLocation::advanceLine()
    {
        column_ = 0;
        ++line_;
    }
}
