#include <lvgl/lvgl.h>

#include "log.h"
#include "assets.h"
#include "gui.h"
#include "drivers.h"

static lv_img_dsc_t g_background;
static lv_img_dsc_t g_apps_list;
static lv_img_dsc_t g_apps_list_hover;

static lv_obj_t *g_buttons[NUM_BUTTONS] = {0};

static lv_img_dsc_t g_logo;

void focus_cb(lv_group_t *group, lv_style_t *style) {
    /*
        Loop through all the objects in the group;
        if the object is the focused one, set the
        hover source, else set the normal source.
    */
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
    u8 *data;
    size_t size;

    assetsGetData(AssetId_background, &data, &size);
    g_background = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = LV_HOR_RES_MAX,
        .header.h = LV_VER_RES_MAX,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
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
        .header.w = 648,
        .header.h = 96,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };

    assetsGetData(AssetId_apps_list_hover, &data, &size);
    g_apps_list_hover = g_apps_list;
    g_apps_list_hover.data_size = size;
    g_apps_list_hover.data = data;

    lv_group_set_style_mod_cb(keypad_group(), focus_cb);

    g_buttons[0] = lv_imgbtn_create(lv_scr_act(), NULL);
    lv_group_add_obj(keypad_group(), g_buttons[0]);
    lv_imgbtn_set_src(g_buttons[0], LV_BTN_STATE_REL, &g_apps_list);
    lv_imgbtn_set_src(g_buttons[0], LV_BTN_STATE_PR, &g_apps_list_hover);
    lv_obj_align(g_buttons[0], NULL, LV_ALIGN_IN_TOP_MID, 0, (LV_VER_RES_MAX - g_apps_list.header.h * NUM_BUTTONS) / 2);

    for (int i = 1; i < NUM_BUTTONS; i++) {
        g_buttons[i] = lv_imgbtn_create(lv_scr_act(), g_buttons[i - 1]);
        lv_obj_align(g_buttons[i], g_buttons[i - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }
}

void setup_misc() {
    u8 *data;
    size_t size;

    assetsGetData(AssetId_logo, &data, &size);
    g_logo = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 381,
        .header.h = 34,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };

    lv_obj_t *logo = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(logo, &g_logo);
    lv_obj_align(logo, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 10, -32);
}