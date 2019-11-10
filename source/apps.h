#pragma once

#include <lvgl/lvgl.h>
#include <limits.h>

#define APP_DIR "sdmc:/switch"

#define APP_ARGS_LEN 0x800

#define APP_NAME_LEN 0x200
#define APP_AUTHOR_LEN 0x200
#define APP_VER_LEN 0x10

#define APP_ICON_W 256
#define APP_ICON_H 256
#define APP_ICON_SMALL_W 72
#define APP_ICON_SMALL_H 72

typedef struct {
    char path[PATH_MAX + 1];
    char args[APP_ARGS_LEN];
    bool starred;

    char name[APP_NAME_LEN];
    char author[APP_AUTHOR_LEN];
    char version[APP_VER_LEN];

    lv_img_dsc_t icon;
    lv_img_dsc_t icon_small;
} app_entry_t;

void app_entry_init_base(app_entry_t *entry, char *path);

lv_res_t app_entry_init_icon(app_entry_t *entry);
void app_entry_free_icon(app_entry_t *entry);

lv_res_t app_entry_init_info(app_entry_t *entry);

void app_entry_get_star_path(app_entry_t *entry, char *out_path);
lv_res_t app_entry_set_star(app_entry_t *entry, bool star);

lv_res_t app_entry_delete(app_entry_t *entry);

void app_entry_add_arg(app_entry_t *entry, char *arg);
lv_res_t app_entry_load(app_entry_t *entry);

lv_res_t app_entry_ll_ins(lv_ll_t *ll, char *path);
lv_res_t app_entry_ll_init(lv_ll_t *ll);