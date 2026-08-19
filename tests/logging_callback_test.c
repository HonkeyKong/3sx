#include "port/utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int captured_level = -1;
static char captured_message[128];

static void capture(int level, const char* fmt, ...) {
    va_list args;
    captured_level = level;
    va_start(args, fmt);
    vsnprintf(captured_message, sizeof(captured_message), fmt, args);
    va_end(args);
}

int main(void) {
    Log_SetCallback(capture);
    Log_Write(LOG_LEVEL_WARN, "MTS slot %d", 17);
    Log_SetCallback(NULL);

    if (captured_level != LOG_LEVEL_WARN || strcmp(captured_message, "MTS slot 17\n") != 0) {
        fprintf(stderr, "unexpected callback output: level=%d message=%s", captured_level, captured_message);
        return 1;
    }

    return 0;
}
