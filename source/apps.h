#pragma once

#include <lvgl/lvgl.h>
#include <limits.h>

#define APP_DIR "sdmc:/switch"

#define APP_NAME_LEN 0x200
#define APP_AUTHOR_LEN 0x200
#define APP_VER_LEN 0x10

#define APP_ICON_W 256
#define APP_ICON_H 256
#define APP_ICON_SMALL_W 72
#define APP_ICON_SMALL_H 72

typedef struct {
    char path[PATH_MAX];
    bool starred;

    char name[APP_NAME_LEN];
    char author[APP_AUTHOR_LEN];
    char version[APP_VER_LEN];

    lv_img_dsc_t icon;
    lv_img_dsc_t icon_small;
} app_entry_t;

lv_res_t app_entry_init_icon(app_entry_t *entry);
void app_entry_free_icon(app_entry_t *entry);

lv_res_t app_entry_init_info(app_entry_t *entry);

lv_res_t app_entry_ll_ins(lv_ll_t *ll, char *path);
lv_res_t app_entry_ll_init(lv_ll_t *ll);