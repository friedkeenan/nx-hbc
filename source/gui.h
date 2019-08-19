#pragma once

#include <lvgl/lvgl.h>

#define CENTER_X(w) ((LV_HOR_RES_MAX - (w)) / 2)
#define CENTER_Y(h) ((LV_VER_RES_MAX - (h)) / 2)

void setup_screen();
void setup_buttons();