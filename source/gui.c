#include <lvgl/lvgl.h>
#include <math.h>

#include "gui.h"
#include "log.h"
#include "assets.h"
#include "decoder.h"
#include "drivers.h"
#include "apps.h"

static lv_img_dsc_t g_background;

static lv_ll_t g_apps_ll;
static int g_apps_ll_len;

static lv_img_dsc_t g_list_dscs[2] = {0}; // {normal, hover}
static lv_obj_t *g_list_buttons[MAX_LIST_ROWS] = {0};
static lv_obj_t *g_list_covers[MAX_LIST_ROWS] = {0}; // Needed because when objects are direct children of an imgbtn, they're always centered (don't know why)
static lv_obj_t *g_curr_focused = NULL;
static int g_list_index = 0; // -3 for right arrow, -2 for left

static lv_img_dsc_t g_arrow_dscs[4] = {0}; // {next_normal, next_hover, prev_normal, prev_hover}
static lv_obj_t *g_arrow_buttons[2] = {0}; // {next, prev}

static int g_curr_page = 0;

static lv_img_dsc_t g_logo;

static lv_style_t g_cover_style;
static lv_style_t g_name_style;
static lv_style_t g_auth_ver_style;

static int num_buttons() {
    return fmin(g_apps_ll_len - MAX_LIST_ROWS * g_curr_page, MAX_LIST_ROWS);
}

static app_entry_t *get_app_for_button(int btn_idx) {
    app_entry_t *entry;
    int i = 0;

    btn_idx = g_curr_page * MAX_LIST_ROWS + btn_idx;

    LV_LL_READ(g_apps_ll, entry) {
        if (i == btn_idx)
            return entry;
        i++;
    }

    return NULL;
}

/*
    Loop through all the buttons in the group;
    if it's focused, set its source to the hover
    source, else set its source to the normal source.
*/
static void focus_cb(lv_group_t *group, lv_style_t *style) {
    if (g_curr_focused == *group->obj_focus)
        return;

    g_curr_focused = *group->obj_focus;

    lv_img_dsc_t *dsc;

    for (int i=0; i<num_buttons(); i++) {
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
    switch (event) {
        case LV_EVENT_KEY: {
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

            if (g_list_index >= num_buttons())
                g_list_index = 0;
            else if (g_list_index < 0)
                g_list_index = num_buttons() - 1;

            lv_group_focus_obj(g_list_buttons[g_list_index]);
        } break;
    }
}

static void arrow_button_event(lv_obj_t *obj, lv_event_t event) {
    switch (event) {
        case LV_EVENT_KEY: {
            const u32 *key = lv_event_get_data();

            switch (*key) {
                case LV_KEY_LEFT:
                    if (obj == g_arrow_buttons[0]) 
                        lv_group_focus_obj(g_list_buttons[num_buttons() / 2]);

                    break;
                case LV_KEY_RIGHT:
                    if (obj == g_arrow_buttons[1])
                        lv_group_focus_obj(g_list_buttons[num_buttons() / 2]);

                    break;
            }
        } break;
    }
}

static void gen_apps_list() {
    app_entry_ll_init(&g_apps_ll);
    g_apps_ll_len = lv_ll_get_len(&g_apps_ll);
}

static void draw_entry_on_obj(lv_obj_t *obj, app_entry_t *entry) {
    u8 offset = (LIST_BTN_H - APP_ICON_SMALL_H) / 2;

    lv_obj_t *icon_small = lv_img_create(obj, NULL);
    lv_img_set_src(icon_small, &entry->icon_small);
    lv_obj_align(icon_small, NULL, LV_ALIGN_IN_LEFT_MID, offset, 0);

    lv_obj_t *name = lv_label_create(obj, NULL);
    lv_label_set_style(name, LV_LABEL_STYLE_MAIN, &g_name_style);
    lv_label_set_static_text(name, entry->name);
    lv_label_set_align(name, LV_LABEL_ALIGN_LEFT);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CROP);
    lv_obj_align(name, icon_small, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *author = lv_label_create(obj, NULL);
    lv_label_set_style(author, LV_LABEL_STYLE_MAIN, &g_auth_ver_style);
    lv_label_set_align(author, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_static_text(author, entry->author);
    lv_obj_align(author, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -offset, -offset);

    lv_obj_t *ver = lv_label_create(obj, author);
    lv_label_set_static_text(ver, entry->version);
    lv_obj_align(ver, NULL, LV_ALIGN_IN_TOP_RIGHT, -offset, offset);
}

static void draw_buttons() {
    if (num_buttons() > 0) {
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

        g_list_covers[0] = lv_cont_create(g_list_buttons[0], NULL);
        lv_cont_set_style(g_list_covers[0], LV_CONT_STYLE_MAIN, &g_cover_style);
        lv_obj_set_size(g_list_covers[0], lv_obj_get_width(g_list_buttons[0]), lv_obj_get_height(g_list_buttons[0]));

        app_entry_t *entry = get_app_for_button(0);
        app_entry_init_icon(entry);
        draw_entry_on_obj(g_list_covers[0], entry);

        for (int i = 1; i < num_buttons(); i++) {
            g_list_buttons[i] = lv_imgbtn_create(lv_scr_act(), g_list_buttons[i - 1]);
            g_list_covers[i] = lv_obj_create(g_list_buttons[i], g_list_covers[i - 1]);

            entry = lv_ll_get_next(&g_apps_ll, entry);
            app_entry_init_icon(entry);
            draw_entry_on_obj(g_list_covers[i], entry);

            lv_obj_align(g_list_buttons[i], g_list_buttons[i - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        }
        g_curr_focused = g_list_covers[0];

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

void setup_menu() {
    lv_style_copy(&g_cover_style, &lv_style_plain);
    g_cover_style.body.opa = LV_OPA_TRANSP;

    lv_style_copy(&g_name_style, &lv_style_plain);
    g_name_style.text.font = &lv_font_roboto_28;
    g_name_style.text.color = LV_COLOR_WHITE;

    lv_style_copy(&g_auth_ver_style, &lv_style_plain);
    g_auth_ver_style.text.color = LV_COLOR_WHITE;

    gen_apps_list();
    draw_buttons();
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
}