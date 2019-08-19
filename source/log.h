#pragma once

void logInitialize(const char *path);
void logExit();

void logPrintf(const char *fmt, ...);