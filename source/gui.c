#include <lvgl/lvgl.h>

#include "gui.h"
#include "log.h"
#include "assets.h"
#include "decoder.h"
#include "drivers.h"
#include "apps.h"

static lv_img_dsc_t g_background;

static lv_img_dsc_t g_list_dscs[2] = {0}; // {normal, hover}
static lv_obj_t *g_list_buttons[MAX_LIST_ROWS] = {0};
static lv_obj_t *g_list_covers[MAX_LIST_ROWS] = {0}; // Needed because when objects are direct children of an imgbtn, they're always centered (don't know why)
static int g_list_index = 0; // -3 for right arrow, -2 for left

static lv_img_dsc_t g_arrow_dscs[4] = {0}; // {next_normal, next_hover, prev_normal, prev_hover}
static lv_obj_t *g_arrow_buttons[2] = {0}; // {next, prev}

static lv_img_dsc_t g_logo;
static app_entry_t g_app;

static lv_style_t g_cover_style;
static lv_style_t g_name_style;
static lv_style_t g_auth_ver_style;

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
            .header.w = LIST_BTN_W,
            .header.h = LIST_BTN_H,
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
    lv_obj_align(g_list_buttons[0], NULL, LV_ALIGN_IN_TOP_MID, 0, (LV_VER_RES_MAX - LIST_BTN_H * MAX_LIST_ROWS) / 2);

    lv_style_copy(&g_cover_style, &lv_style_plain);
    g_cover_style.body.opa = LV_OPA_TRANSP;

    g_list_covers[0] = lv_cont_create(g_list_buttons[0], NULL);
    lv_cont_set_style(g_list_covers[0], LV_CONT_STYLE_MAIN, &g_cover_style);
    lv_obj_set_size(g_list_covers[0], lv_obj_get_width(g_list_buttons[0]), lv_obj_get_height(g_list_buttons[0]));

    for (int i = 1; i < MAX_LIST_ROWS; i++) {
        g_list_buttons[i] = lv_imgbtn_create(lv_scr_act(), g_list_buttons[i - 1]);
        g_list_covers[i] = lv_cont_create(g_list_buttons[i], g_list_covers[i - 1]);
        lv_obj_align(g_list_buttons[i], g_list_buttons[i - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }

    // Page buttons
    for (int i=0; i<4; i++) {
        assetsGetData(AssetId_apps_next + i, &data, &size);
        g_arrow_dscs[i] = (lv_img_dsc_t) {
            .header.always_zero = 0,
            .header.w = ARROW_BTN_W,
            .header.h = ARROW_BTN_H,
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
    lv_obj_align(g_arrow_buttons[0], NULL, LV_ALIGN_CENTER, 20 + (ARROW_BTN_W + LIST_BTN_W) / 2, 0);

    g_arrow_buttons[1] = lv_imgbtn_create(lv_scr_act(), g_arrow_buttons[0]);
    lv_imgbtn_set_src(g_arrow_buttons[1], LV_BTN_STATE_REL, &g_arrow_dscs[2]);
    lv_imgbtn_set_src(g_arrow_buttons[1], LV_BTN_STATE_PR, &g_arrow_dscs[2]);
    lv_obj_align(g_arrow_buttons[1], NULL, LV_ALIGN_CENTER, -20 - (ARROW_BTN_W + LIST_BTN_W) / 2, 0);
}

void setup_misc() {
    u8 *data;
    size_t size;

    assetsGetData(AssetId_logo, &data, &size);
    g_logo = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = LOGO_W,
        .header.h = LOGO_H,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };

    lv_obj_t *logo = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(logo, &g_logo);
    lv_obj_align(logo, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 10, -32);

    strcpy(g_app.path, "sdmc:/switch/nx-hbc.nro");
    lv_res_t res = app_entry_init_info(&g_app);
    logPrintf("app_entry_init_info: %d\n", res);
    res = app_entry_init_icon(&g_app);
    logPrintf("app_entry_init_icon: %d\n", res);

    logPrintf("g_list_buttons[0](%d, %d)\n", lv_obj_get_width(g_list_buttons[0]), lv_obj_get_height(g_list_buttons[0]));

    u8 offset = (LIST_BTN_H - APP_ICON_SMALL_H) / 2;

    lv_obj_t *icon_small = lv_img_create(g_list_covers[0], NULL);
    lv_img_set_src(icon_small, &g_app.icon_small);
    lv_obj_align(icon_small, NULL, LV_ALIGN_IN_LEFT_MID, offset, 0);

    lv_style_copy(&g_name_style, &lv_style_plain);
    g_name_style.text.font = &lv_font_roboto_28;
    g_name_style.text.color = LV_COLOR_WHITE;

    lv_obj_t *name = lv_label_create(g_list_covers[0], NULL);
    lv_label_set_style(name, LV_LABEL_STYLE_MAIN, &g_name_style);
    lv_label_set_static_text(name, g_app.name);
    lv_label_set_align(name, LV_LABEL_ALIGN_LEFT);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CROP);
    lv_obj_align(name, icon_small, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_style_copy(&g_auth_ver_style, &lv_style_plain);
    g_auth_ver_style.text.color = LV_COLOR_WHITE;

    lv_obj_t *author = lv_label_create(g_list_covers[0], NULL);
    lv_label_set_style(author, LV_LABEL_STYLE_MAIN, &g_auth_ver_style);
    lv_label_set_align(author, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_static_text(author, g_app.author);
    lv_obj_align(author, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -offset, -offset);

    lv_obj_t *ver = lv_label_create(g_list_covers[0], author);
    lv_label_set_static_text(ver, g_app.version);
    lv_obj_align(ver, NULL, LV_ALIGN_IN_TOP_RIGHT, -offset, offset);
}