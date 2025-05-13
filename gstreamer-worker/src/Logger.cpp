// Logger.cpp
#include "Logger.hpp"
#include <cstdarg>
#include <syslog.h>
#include "config.hpp"

void log_message(const std::string& level, const std::string& fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt.c_str(), args);
    va_end(args);

    if ((level == "debug" && !enable_debug_logs.load()) ||
        level == "info" || level == "warn" || level == "error")
    {
        syslog(LOG_INFO, "[%s] %s", level.c_str(), buffer);
    }

    // Optional: Forward logs over ZeroMQ
}