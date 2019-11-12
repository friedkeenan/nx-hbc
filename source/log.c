#include <stdio.h>
#include <stdarg.h>
#include <threads.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "log.h"

static FILE *g_log_fp;
static mtx_t g_log_mtx;

static void log_cb(lv_log_level_t level, const char *file, u32 line, const char *dsc) {
    switch (level) {
        case LV_LOG_LEVEL_ERROR:
            logPrintf("ERROR: ");
            break;
        case LV_LOG_LEVEL_WARN:
            logPrintf("WARN: ");
            break;
        case LV_LOG_LEVEL_INFO:
            logPrintf("INFO: ");
            break;
        case LV_LOG_LEVEL_TRACE:
            logPrintf("TRACE: ");
            break;
    }

    logPrintf("File: %s:%d : %s\n", file, line, dsc);
}

void logInitialize(const char *path) {
    mtx_init(&g_log_mtx, mtx_plain);

    g_log_fp = fopen(path, "w");

    lv_log_register_print_cb(log_cb);
}

void logExit() {
    fclose(g_log_fp);
    mtx_destroy(&g_log_mtx);
}

void logPrintf(const char *fmt, ...) {
    mtx_lock(&g_log_mtx);
    va_list argptr;

    va_start(argptr, fmt);
    vfprintf(g_log_fp, fmt, argptr);
    va_end(argptr);

    fflush(g_log_fp);
    mtx_unlock(&g_log_mtx);
}