#pragma once

#include <lvgl/lvgl.h>

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

#define LOGO_W 381
#define LOGO_H 34

#define REMOTE_PROGRESS_W 600
#define REMOTE_PROGRESS_H 168
#define REMOTE_PROGRESS_INNER_W 480
#define REMOTE_PROGRESS_INNER_H 60

#define NET_ICON_W 48
#define NET_ICON_H 48

#define CURSOR_W 96
#define CURSOR_H 96

typedef struct {
    lv_img_dsc_t background_dsc;

    lv_img_dsc_t star_dscs[2]; // {small, big}

    lv_img_dsc_t list_btns_dscs[2]; // {normal, hover}

    lv_img_dsc_t dialog_bg_dsc;
    lv_img_dsc_t dialog_btns_dscs[2]; // {normal, hover}

    lv_img_dsc_t arrow_btns_dscs[4]; // {next_normal, next_hover, prev_normal, prev_hover}

    lv_img_dsc_t logo_dsc;

    lv_img_dsc_t net_icons_dscs[2]; // {inactive, active}

    lv_img_dsc_t remote_progress_dsc;

    lv_img_dsc_t cursor_dsc;

    lv_style_t no_apps_mbox_style;

    lv_style_t remote_bar_indic_style;
    lv_style_t remote_error_mbox_style;

    lv_style_t transp_style;
    lv_style_t dark_opa_64_style;

    lv_style_t normal_16_style;
    lv_style_t normal_22_style;
    lv_style_t normal_28_style;
    lv_style_t normal_48_style;

    lv_style_t warn_48_style;
} theme_t;

lv_res_t theme_init();
void theme_exit();

theme_t *curr_theme();