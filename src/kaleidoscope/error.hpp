#ifndef INCLUDED_KALEIDOSCOPE_ERROR_HPP
#define INCLUDED_KALEIDOSCOPE_ERROR_HPP

#include <stdexcept>

namespace kaleidoscope
{
    class Error : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };
}

#endif
