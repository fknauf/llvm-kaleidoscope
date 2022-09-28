#include <iostream>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((retain))
#endif

extern "C" double DLLEXPORT putchard(double X)
{
    std::cerr << static_cast<char>(X) << std::flush;
    return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" double DLLEXPORT printd(double X)
{
    std::cerr << X << std::endl;
    return 0;
}
