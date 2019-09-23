#pragma once

#include <lvgl/lvgl.h>

#define CURSOR_SENSITIVITY 2

void driversInitialize();
void driversExit();

lv_group_t *keypad_group();