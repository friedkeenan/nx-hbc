#include <lvgl/lvgl.h>

#include "gui.h"
#include "log.h"
#include "assets.h"
#include "decoder.h"
#include "drivers.h"

static lv_img_dsc_t g_background;

static lv_img_dsc_t g_list_dscs[2] = {0}; // {normal, hover}
static lv_obj_t *g_list_buttons[MAX_LIST_ROWS] = {0};
static int g_list_index = 0; // -3 for right arrow, -2 for left

static lv_img_dsc_t g_arrow_dscs[4] = {0}; // {next_normal, next_hover, prev_normal, prev_hover}
static lv_obj_t *g_arrow_buttons[2] = {0}; // {next, prev}

static lv_img_dsc_t g_logo;
static lv_img_dsc_t g_icon;

/*
    Loop through all the buttons in the group;
    if it's focused, set its source to the hover
    source, else set its source to the normal source.
*/
static void focus_cb(lv_group_t *group, lv_style_t *style) {
    lv_img_dsc_t *dsc;

    for (int i=0; i<MAX_LIST_ROWS; i++) {
        lv_obj_t *obj = g_list_buttons[i];

        if (obj == NULL)
            continue;

        if (obj == *group->obj_focus) {
            g_list_index = i;
            dsc = &g_list_dscs[1];
        } else {
            dsc = &g_list_dscs[0];
        }

        lv_imgbtn_set_src(obj, LV_BTN_STATE_REL, dsc);
        lv_imgbtn_set_src(obj, LV_BTN_STATE_PR, dsc);
    }

    for (int i=0; i<2; i++) {
        lv_obj_t *obj = g_arrow_buttons[i];

        if (obj == NULL)
            continue;

        if (obj == *group->obj_focus) {
            g_list_index = i - 3;
            dsc = &g_arrow_dscs[i * 2 + 1];
        } else {
            dsc = &g_arrow_dscs[i * 2];
        }

        lv_imgbtn_set_src(obj, LV_BTN_STATE_REL, dsc);
        lv_imgbtn_set_src(obj, LV_BTN_STATE_PR, dsc);
    }
}

static void list_button_event(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_KEY) {
        const u32 *key = lv_event_get_data();

        switch (*key) {
            case LV_KEY_DOWN:
                g_list_index += 1;
                break;
            case LV_KEY_UP:
                g_list_index -= 1;
                break;
            case LV_KEY_RIGHT:
                lv_group_focus_obj(g_arrow_buttons[0]);
                return;
            case LV_KEY_LEFT:
                lv_group_focus_obj(g_arrow_buttons[1]);
                return;
        }

        if (g_list_index >= MAX_LIST_ROWS)
            g_list_index = 0;
        else if (g_list_index < 0)
            g_list_index = MAX_LIST_ROWS - 1;

        lv_group_focus_obj(g_list_buttons[g_list_index]);
    }
}

static void arrow_button_event(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_KEY) {
        const u32 *key = lv_event_get_data();

        switch (*key) {
            case LV_KEY_LEFT:
                if (obj == g_arrow_buttons[0]) 
                    lv_group_focus_obj(g_list_buttons[MAX_LIST_ROWS / 2]);

                break;
            case LV_KEY_RIGHT:
                if (obj == g_arrow_buttons[1])
                    lv_group_focus_obj(g_list_buttons[MAX_LIST_ROWS / 2]);

                break;
        }
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

    // List buttons
    for (int i=0; i<2; i++) {
        assetsGetData(AssetId_apps_list + i, &data, &size);
        g_list_dscs[i] = (lv_img_dsc_t) {
            .header.always_zero = 0,
            .header.w = 648,
            .header.h = 96,
            .data_size = size,
            .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
            .data = data,
        };
    }

    lv_group_set_style_mod_cb(keypad_group(), focus_cb);

    g_list_buttons[0] = lv_imgbtn_create(lv_scr_act(), NULL);
    lv_group_add_obj(keypad_group(), g_list_buttons[0]);
    lv_obj_set_event_cb(g_list_buttons[0], list_button_event);
    lv_imgbtn_set_src(g_list_buttons[0], LV_BTN_STATE_REL, &g_list_dscs[0]);
    lv_imgbtn_set_src(g_list_buttons[0], LV_BTN_STATE_PR, &g_list_dscs[0]);
    lv_obj_align(g_list_buttons[0], NULL, LV_ALIGN_IN_TOP_MID, 0, (LV_VER_RES_MAX - g_list_dscs[0].header.h * MAX_LIST_ROWS) / 2);

    for (int i = 1; i < MAX_LIST_ROWS; i++) {
        g_list_buttons[i] = lv_imgbtn_create(lv_scr_act(), g_list_buttons[i - 1]);
        lv_obj_align(g_list_buttons[i], g_list_buttons[i - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }

    // Page buttons
    for (int i=0; i<4; i++) {
        assetsGetData(AssetId_apps_next + i, &data, &size);
        g_arrow_dscs[i] = (lv_img_dsc_t) {
            .header.always_zero = 0,
            .header.w = 96,
            .header.h = 96,
            .data_size = size,
            .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
            .data = data,
        };
    }

    g_arrow_buttons[0] = lv_imgbtn_create(lv_scr_act(), NULL);
    lv_group_add_obj(keypad_group(), g_arrow_buttons[0]);
    lv_obj_set_event_cb(g_arrow_buttons[0], arrow_button_event);
    lv_imgbtn_set_src(g_arrow_buttons[0], LV_BTN_STATE_REL, &g_arrow_dscs[0]);
    lv_imgbtn_set_src(g_arrow_buttons[0], LV_BTN_STATE_PR, &g_arrow_dscs[0]);
    lv_obj_align(g_arrow_buttons[0], NULL, LV_ALIGN_CENTER, 20 + (g_arrow_dscs[0].header.w + g_list_dscs[0].header.w) / 2, 0);

    g_arrow_buttons[1] = lv_imgbtn_create(lv_scr_act(), g_arrow_buttons[0]);
    lv_imgbtn_set_src(g_arrow_buttons[1], LV_BTN_STATE_REL, &g_arrow_dscs[2]);
    lv_imgbtn_set_src(g_arrow_buttons[1], LV_BTN_STATE_PR, &g_arrow_dscs[2]);
    lv_obj_align(g_arrow_buttons[1], NULL, LV_ALIGN_CENTER, -20 - (g_arrow_dscs[0].header.w + g_list_dscs[0].header.w) / 2, 0);
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

    assetsGetData(AssetId_icon, &data, &size);
    g_icon = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 256,
        .header.h = 256,
        .data_size = size,
        .header.cf = LV_IMG_CF_RAW,
        .data = data,
    };

    lv_obj_t *icon = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(icon, &g_icon);
    lv_obj_align(icon, NULL, LV_ALIGN_CENTER, 0, 0);
}