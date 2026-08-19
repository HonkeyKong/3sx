#include "port/utils.h"

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dbghelp.h>
#define SYMBOL_NAME_MAX 256
#elif __APPLE__ || linux
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define BACKTRACE_MAX 100

static LogCallback log_callback;

void Log_SetCallback(LogCallback callback) {
    log_callback = callback;
}

void Log_WriteV(LogLevel level, const char* fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    const int length = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);

    if (length < 0) {
        return;
    }

    char stack_buffer[1024];
    char* message = stack_buffer;
    if ((size_t)length >= sizeof(stack_buffer)) {
        message = malloc((size_t)length + 1);
        if (!message) {
            return;
        }
    }

    vsnprintf(message, (size_t)length + 1, fmt, args);
    if (log_callback) {
        log_callback((int)level, "%s\n", message);
    } else {
        FILE* stream = level >= LOG_LEVEL_WARN ? stderr : stdout;
        fprintf(stream, "%s\n", message);
    }

    if (message != stack_buffer) {
        free(message);
    }
}

void Log_Write(LogLevel level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_WriteV(level, fmt, args);
    va_end(args);
}

void fatal_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    const int length = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    char* message = length >= 0 ? malloc((size_t)length + 1) : NULL;
    if (message) {
        vsnprintf(message, (size_t)length + 1, fmt, args);
        Log_Write(LOG_LEVEL_ERROR, "Fatal error: %s", message);
        free(message);
    } else {
        Log_Write(LOG_LEVEL_ERROR, "Fatal error (message formatting failed)");
    }
    va_end(args);

#if __APPLE__ || linux
    void* buffer[BACKTRACE_MAX];

    int nptrs = backtrace(buffer, BACKTRACE_MAX);
    Log_Write(LOG_LEVEL_ERROR, "Stack trace:");
    char** symbols = backtrace_symbols(buffer, nptrs);
    if (symbols) {
        for (int i = 0; i < nptrs; ++i) {
            Log_Write(LOG_LEVEL_ERROR, "%s", symbols[i]);
        }
        free(symbols);
    }
#elif _WIN32
    void* buffer[BACKTRACE_MAX];

    Log_Write(LOG_LEVEL_ERROR, "Stack trace:");
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    int nptrs = CaptureStackBackTrace(0, BACKTRACE_MAX, buffer, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(1, sizeof(SYMBOL_INFO) + SYMBOL_NAME_MAX);

    if (!symbol) {
        Log_Write(LOG_LEVEL_ERROR, "Calloc failed when allocating SYMBOL_INFO, bailing!");
        SymCleanup(process);
        abort();
    }

    symbol->MaxNameLen = SYMBOL_NAME_MAX;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (int i = 0; i < nptrs; i++) {
        SymFromAddr(process, (DWORD64)buffer[i], 0, symbol);
        Log_Write(LOG_LEVEL_ERROR, "%i: %s - 0x%0llX", nptrs - i - 1, symbol->Name, symbol->Address);
    }

    free(symbol);
    SymCleanup(process);
#endif

    abort();
}

void not_implemented(const char* func) {
    fatal_error("Function not implemented: %s\n", func);
}

void debug_print(const char* fmt, ...) {
#if DEBUG
    va_list args;
    va_start(args, fmt);
    Log_WriteV(LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
#endif
}
