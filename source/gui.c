#include <lvgl/lvgl.h>

#include "log.h"
#include "assets.h"
#include "gui.h"
#include "drivers.h"

static lv_img_dsc_t g_background;
static lv_img_dsc_t g_apps_list;
static lv_img_dsc_t g_apps_list_hover;

void setup_screen() {
    u8 *bg_data;
    size_t bg_size;

    assetsGetData(AssetId_background, &bg_data, &bg_size);
    logPrintf("bg_data(%p), bg_size(%#lx)\n", bg_data, bg_size);

    g_background = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = LV_HOR_RES_MAX,
        .header.h = LV_VER_RES_MAX,
        .data_size = bg_size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = bg_data,
    };

    lv_obj_t *scr = lv_img_create(NULL, NULL);
    lv_img_set_src(scr, &g_background);
    lv_scr_load(scr);
}

void setup_buttons() {
    u8 *data;
    size_t size;

    assetsGetData(AssetId_apps_list, &data, &size);
    g_apps_list = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 864,
        .header.h = 128,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };

    assetsGetData(AssetId_apps_list_hover, &data, &size);
    g_apps_list_hover = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 864,
        .header.h = 128,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };

    lv_obj_t *btn = lv_imgbtn_create(lv_scr_act(), NULL);
    //lv_group_add_obj(keypad_group(), btn);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_REL, &g_apps_list);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_PR, &g_apps_list_hover);
    lv_obj_set_pos(btn, CENTER_X(g_apps_list.header.w), CENTER_Y(g_apps_list.header.h));
}