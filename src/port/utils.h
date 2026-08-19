#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdarg.h>

#ifndef __dead2
#define __dead2 __attribute__((__noreturn__))
#endif

typedef enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} LogLevel;

typedef void (*LogCallback)(int level, const char* fmt, ...);

void Log_SetCallback(LogCallback callback);
void Log_Write(LogLevel level, const char* fmt, ...);
void Log_WriteV(LogLevel level, const char* fmt, va_list args);
__dead2 void fatal_error(const char* fmt, ...);
__dead2 void not_implemented(const char* func);
void debug_print(const char* fmt, ...);

#endif
