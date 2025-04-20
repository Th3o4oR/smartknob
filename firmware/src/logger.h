#pragma once

#include <cstdio>
#include <cstdarg>
#include <string>

#define LOG_COLOR_BLACK   "\033[1;30m"
#define LOG_COLOR_RED     "\033[1;31m"
#define LOG_COLOR_GREEN   "\033[1;32m"
#define LOG_COLOR_YELLOW  "\033[1;33m"
#define LOG_COLOR_BLUE    "\033[1;34m"
#define LOG_COLOR_MAGENTA "\033[1;35m"
#define LOG_COLOR_CYAN    "\033[1;36m"
#define LOG_COLOR_WHITE   "\033[1;37m"
#define LOG_COLOR_RESET   "\033[0m"

class Logger {
public:
    Logger() {}
    virtual ~Logger() {}
    virtual void log(const std::string& message) = 0;

    static std::string format_string(const char* format, ...) {
        char buffer[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return std::string(buffer);
    }
};

#define LOG_INFO(fmt, ...)    log(Logger::format_string(LOG_COLOR_BLUE    "[INFO] "    LOG_COLOR_RESET fmt, ##__VA_ARGS__))
#define LOG_SUCCESS(fmt, ...) log(Logger::format_string(LOG_COLOR_GREEN   "[SUCCESS] " LOG_COLOR_RESET fmt, ##__VA_ARGS__))
#define LOG_WARN(fmt, ...)    log(Logger::format_string(LOG_COLOR_YELLOW  "[WARN] "    LOG_COLOR_RESET fmt, ##__VA_ARGS__))
#define LOG_ERROR(fmt, ...)   log(Logger::format_string(LOG_COLOR_RED     "[ERROR] "   LOG_COLOR_RESET fmt, ##__VA_ARGS__))
#define LOG_DEBUG(fmt, ...)   log(Logger::format_string(LOG_COLOR_MAGENTA "[DEBUG] "   LOG_COLOR_RESET fmt, ##__VA_ARGS__))
