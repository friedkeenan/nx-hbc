#include <stdio.h>
#include <stdarg.h>

#include "log.h"

FILE *g_log_fp;

void logInitialize(const char *path) {
    g_log_fp = fopen(path, "w");
}

void logExit() {
    fclose(g_log_fp);
}

void logPrintf(const char *fmt, ...) {
    va_list argptr;

    va_start(argptr, fmt);
    vfprintf(g_log_fp, fmt, argptr);
    va_end(argptr);

    fflush(g_log_fp);
}