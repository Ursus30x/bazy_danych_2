#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdio>

namespace Logger
{
    extern bool verbose;

    void log(const char* fmt, ...);
    void log_verbose(const char* fmt, ...);
}

#endif
