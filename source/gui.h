#pragma once

#include <lvgl/lvgl.h>

#define MAX_LIST_ROWS 5

#define PAGE_TIME 250
#define PAGE_WAIT 94

#define STAR_SMALL_W 25
#define STAR_SMALL_H 24
#define STAR_BIG_W 89
#define STAR_BIG_H 86

#define LIST_BTN_W 648
#define LIST_BTN_H 96

#define DIALOG_BG_W 780
#define DIALOG_BG_H 540
#define DIALOG_BTN_W 175
#define DIALOG_BTN_H 72

#define ARROW_BTN_W 96
#define ARROW_BTN_H 96
#define ARROW_OFF (20 + (ARROW_BTN_W + LIST_BTN_W) / 2)

#define LOGO_W 381
#define LOGO_H 34

void setup_screen();
void setup_menu();
void setup_misc();