#include <iostream>

extern "C" double putchard(double X)
{
    std::cerr << static_cast<char>(X) << std::flush;
    return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" double printd(double X)
{
    std::cerr << X << std::endl;
    return 0;
}
