#pragma once

#include <lvgl/lvgl.h>

#define MAX_LIST_ROWS 5

#define PAGE_TIME 200
#define PAGE_WAIT 75

#define STAR_W 25
#define STAR_H 24

#define LIST_BTN_W 648
#define LIST_BTN_H 96

#define ARROW_BTN_W 96
#define ARROW_BTN_H 96
#define ARROW_OFF (20 + (ARROW_BTN_W + LIST_BTN_W) / 2)

#define LOGO_W 381
#define LOGO_H 34

void setup_screen();
void setup_menu();
void setup_misc();