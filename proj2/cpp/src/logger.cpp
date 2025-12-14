#include "logger.hpp"
#include <cstdarg>

namespace Logger
{
    bool verbose = false;

    void log(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    void log_verbose(const char* fmt, ...)
    {
        if (!verbose) return;
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}
