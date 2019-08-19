#include <lvgl/lvgl.h>

#include "log.h"
#include "assets.h"
#include "gui.h"
#include "drivers.h"

static lv_img_dsc_t g_background;
static lv_img_dsc_t g_apps_list;
static lv_img_dsc_t g_apps_list_hover;

void focus_cb(lv_group_t *group, lv_style_t *style) {
    lv_obj_t **obj;
    LV_LL_READ(group->obj_ll, obj) {
        lv_img_dsc_t *dsc;

        if (obj == group->obj_focus)
            dsc = &g_apps_list_hover;
        else
            dsc = &g_apps_list;
        
        lv_imgbtn_set_src(*obj, LV_BTN_STATE_REL, dsc);
    }
}

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

    lv_group_set_style_mod_cb(keypad_group(), focus_cb);

    lv_obj_t *btn = lv_imgbtn_create(lv_scr_act(), NULL);
    lv_group_add_obj(keypad_group(), btn);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_REL, &g_apps_list);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_PR, &g_apps_list_hover);
    lv_obj_align(btn, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn2 = lv_imgbtn_create(lv_scr_act(), btn);
    lv_obj_align(btn2, NULL, LV_ALIGN_CENTER, 0, 128);
}