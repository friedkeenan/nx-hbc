#pragma once

#include <lvgl/lvgl.h>

#include "theme.h"

#define MAX_LIST_ROWS 5

#define PAGE_TIME 250
#define PAGE_WAIT 94

#define ARROW_OFF (20 + (ARROW_BTN_W + LIST_BTN_W) / 2)

void setup_screen();
void setup_menu();
void setup_misc();

void gui_exit();