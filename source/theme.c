/*
 * Copyright 2017-2018 nx-hbmenu Authors
 *
 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby
 * granted, provided that the above copyright notice and
 * this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <minizip/unzip.h>
#include <libconfig.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "theme.h"
#include "settings.h"
#include "log.h"

#define DEFAULT_THEME_PATH "romfs:/theme.zip"

#define GEN_ASSET(x) {.file_name = x}

typedef enum {
    AssetId_background,
    AssetId_cursor,
    AssetId_apps_list,
    AssetId_apps_list_hover,
    AssetId_apps_next,
    AssetId_apps_next_hover,
    AssetId_apps_previous,
    AssetId_apps_previous_hover,
    AssetId_logo,
    AssetId_star_small,
    AssetId_star_big,
    AssetId_dialog_background,
    AssetId_button_tiny,
    AssetId_button_tiny_focus,
    AssetId_remote_progress,
    AssetId_network_inactive,
    AssetId_network_active,

    AssetId_max
} AssetId;

typedef struct {
    void *buffer;
    size_t size;
    const char *file_name;
} asset_t;

static asset_t g_assets_list[AssetId_max] = {
    GEN_ASSET("background.bin"),
    GEN_ASSET("cursor.bin"),
    GEN_ASSET("apps_list.bin"),
    GEN_ASSET("apps_list_hover.bin"),
    GEN_ASSET("apps_next.bin"),
    GEN_ASSET("apps_next_hover.bin"),
    GEN_ASSET("apps_previous.bin"),
    GEN_ASSET("apps_previous_hover.bin"),
    GEN_ASSET("logo.bin"),
    GEN_ASSET("star_small.bin"),
    GEN_ASSET("star_big.bin"),
    GEN_ASSET("dialog_background.bin"),
    GEN_ASSET("button_tiny.bin"),
    GEN_ASSET("button_tiny_focus.bin"),
    GEN_ASSET("remote_progress.bin"),
    GEN_ASSET("network_inactive.bin"),
    GEN_ASSET("network_active.bin"),
};

static theme_t g_curr_theme;

static int config_setting_lookup_color(config_setting_t *setting, const char *name, lv_color_t *value) {
    int tmp_col;

    int ret = config_setting_lookup_int(setting, name, &tmp_col);
    if (ret != CONFIG_TRUE)
        return ret;

    *value = lv_color_hex(tmp_col);

    return ret;
}

static int asset_load(asset_t *asset, unzFile zf) {
    int ret = unzLocateFile(zf, asset->file_name, 0);
    if (ret != UNZ_OK)
        return ret;

    ret = unzOpenCurrentFile(zf);
    if (ret != UNZ_OK)
        return ret;

    unz_file_info file_info;
    ret = unzGetCurrentFileInfo(zf, &file_info, NULL, 0, NULL, 0, NULL, 0);
    if (ret != UNZ_OK) {
        unzCloseCurrentFile(zf);
        return ret;
    }

    asset->size = file_info.uncompressed_size;

    asset->buffer = lv_mem_alloc(asset->size);
    ret = unzReadCurrentFile(zf, asset->buffer, asset->size);
    if (ret < asset->size) {
        lv_mem_free(asset->buffer);
        unzCloseCurrentFile(zf);
        return -12;
    }

    unzCloseCurrentFile(zf);

    return UNZ_OK;
}

static void asset_clean(asset_t *asset) {
    lv_mem_free(asset->buffer);
    asset->buffer = NULL;
    asset->size = 0;
}

static void asset_to_img_dsc(lv_img_dsc_t *dsc, asset_t *assets, AssetId id, u32 width, u32 height) {
    asset_t *asset = &assets[id];

    dsc->header.always_zero = 0;
    dsc->header.w = width;
    dsc->header.h = height;
    dsc->data_size = asset->size;
    dsc->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    dsc->data = asset->buffer;
}

static void theme_load_assets(theme_t *theme, asset_t *assets) {
    asset_to_img_dsc(&theme->background_dsc, assets, AssetId_background, LV_HOR_RES_MAX, LV_VER_RES_MAX);

    asset_to_img_dsc(&theme->star_dscs[0], assets, AssetId_star_small, STAR_SMALL_W, STAR_SMALL_H);
    asset_to_img_dsc(&theme->star_dscs[1], assets, AssetId_star_big, STAR_BIG_W, STAR_BIG_H);

    for (int i = 0; i < 2; i++)
        asset_to_img_dsc(&theme->list_btns_dscs[i], assets, AssetId_apps_list + i, LIST_BTN_W, LIST_BTN_H);

    asset_to_img_dsc(&theme->dialog_bg_dsc, assets, AssetId_dialog_background, DIALOG_BG_W, DIALOG_BG_H);

    for (int i = 0; i < 2; i++)
        asset_to_img_dsc(&theme->dialog_btns_dscs[i], assets, AssetId_button_tiny + i, DIALOG_BTN_W, DIALOG_BTN_H);

    for (int i = 0; i < 4; i++)
        asset_to_img_dsc(&theme->arrow_btns_dscs[i], assets, AssetId_apps_next + i, ARROW_BTN_W, ARROW_BTN_H);

    asset_to_img_dsc(&theme->logo_dsc, assets, AssetId_logo, LOGO_W, LOGO_H);

    for (int i = 0; i < 2; i++)
        asset_to_img_dsc(&theme->net_icons_dscs[i], assets, AssetId_network_inactive + i, NET_ICON_W, NET_ICON_H);

    asset_to_img_dsc(&theme->remote_progress_dsc, assets, AssetId_remote_progress, REMOTE_PROGRESS_W, REMOTE_PROGRESS_H);

    asset_to_img_dsc(&theme->cursor_dsc, assets, AssetId_cursor, CURSOR_W, CURSOR_H);
}

static void theme_init_styles(theme_t *theme) {
    lv_style_copy(&theme->no_apps_mbox_style, &lv_style_pretty);
    theme->no_apps_mbox_style.body.opa = 128;
    theme->no_apps_mbox_style.text.font = &lv_font_roboto_48;

    lv_style_copy(&theme->remote_bar_indic_style, &lv_style_plain);
    theme->remote_bar_indic_style.body.padding.left = 0;
    theme->remote_bar_indic_style.body.padding.right = 0;
    theme->remote_bar_indic_style.body.padding.top = 0;
    theme->remote_bar_indic_style.body.padding.bottom = 0;

    lv_style_copy(&theme->remote_error_mbox_style, &theme->no_apps_mbox_style);
    theme->remote_error_mbox_style.body.opa = LV_OPA_COVER;

    lv_style_copy(&theme->dark_opa_64_style, &lv_style_plain);
    theme->dark_opa_64_style.body.opa = 64;

    lv_style_copy(&theme->normal_16_style, &lv_style_plain);

    lv_style_copy(&theme->normal_22_style, &lv_style_plain);
    theme->normal_22_style.text.font = &lv_font_roboto_22;

    lv_style_copy(&theme->normal_28_style, &lv_style_plain);
    theme->normal_28_style.text.font = &lv_font_roboto_28;

    lv_style_copy(&theme->normal_48_style, &lv_style_plain);
    theme->normal_48_style.text.font = &lv_font_roboto_48;

    lv_style_copy(&theme->warn_48_style, &theme->normal_48_style);
}

static lv_res_t theme_load_styles(theme_t *theme, unzFile zf) {
    if (zf == NULL)
        return LV_RES_INV;

    if (unzLocateFile(zf, "styles.cfg", 0) != UNZ_OK)
        return LV_RES_INV;

    if (unzOpenCurrentFile(zf) != UNZ_OK)
        return LV_RES_INV;

    unz_file_info file_info;
    if (unzGetCurrentFileInfo(zf, &file_info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) {
        unzCloseCurrentFile(zf);
        return LV_RES_INV;
    }

    char cfg_str[file_info.uncompressed_size + 1];
    if (unzReadCurrentFile(zf, cfg_str, file_info.uncompressed_size) < file_info.uncompressed_size) {
        unzCloseCurrentFile(zf);
        return LV_RES_INV;
    }

    unzCloseCurrentFile(zf);

    cfg_str[file_info.uncompressed_size] = '\0';

    config_t cfg;

    config_init(&cfg);
    if (config_read_string(&cfg, cfg_str) != CONFIG_TRUE) {
        config_destroy(&cfg);
        return LV_RES_INV;
    }

    config_setting_t *styles = config_lookup(&cfg, "styles");
    if (styles == NULL) {
        config_destroy(&cfg);
        return LV_RES_INV;
    }

    lv_color_t tmp_col;

    if (config_setting_lookup_color(styles, "no_apps_mbox_bg_color", &tmp_col) == CONFIG_TRUE) {
        theme->no_apps_mbox_style.body.main_color = tmp_col;
        theme->no_apps_mbox_style.body.grad_color = tmp_col;
    }

    if (config_setting_lookup_color(styles, "remote_error_mbox_color", &tmp_col) == CONFIG_TRUE) {
        theme->remote_error_mbox_style.body.main_color = tmp_col;
        theme->remote_error_mbox_style.body.grad_color = tmp_col;
    }

    if (config_setting_lookup_color(styles, "remote_bar_main_color", &tmp_col) == CONFIG_TRUE)
        theme->remote_bar_indic_style.body.main_color = tmp_col;

    if (config_setting_lookup_color(styles, "remote_bar_grad_color", &tmp_col) == CONFIG_TRUE)
        theme->remote_bar_indic_style.body.grad_color = tmp_col;

    if (config_setting_lookup_color(styles, "dark_cover_color", &tmp_col) == CONFIG_TRUE) {
        theme->dark_opa_64_style.body.main_color = tmp_col;
        theme->dark_opa_64_style.body.grad_color = tmp_col;
    }

    if (config_setting_lookup_color(styles, "normal_text_color", &tmp_col) == CONFIG_TRUE) {
        theme->normal_16_style.text.color = tmp_col;
        theme->normal_22_style.text.color = tmp_col;
        theme->normal_28_style.text.color = tmp_col;
        theme->normal_48_style.text.color = tmp_col;
        theme->no_apps_mbox_style.text.color = tmp_col;
        theme->remote_error_mbox_style.text.color = tmp_col;
    }

    if (config_setting_lookup_color(styles, "warn_text_color", &tmp_col) == CONFIG_TRUE)
        theme->warn_48_style.text.color = tmp_col;

    config_destroy(&cfg);

    return LV_RES_OK;
}

lv_res_t theme_init() {
    logPrintf("romfsInit\n");
    Result rc = romfsInit();
    logPrintf("rc(%#x)\n", rc);
    if (R_FAILED(rc))
        return LV_RES_INV;
    logPrintf("after\n");

    unzFile zf_default = unzOpen(DEFAULT_THEME_PATH);
    unzFile zf = unzOpen(THEME_PATH);

    logPrintf("%p %p\n", zf, zf_default);

    int i_bad;
    int ret;
    for (int i = 0; i < AssetId_max; i++) {
        logPrintf("i(%d)\n", i);
        i_bad = i;
        ret = -1;

        if (zf != NULL)
            ret = asset_load(&g_assets_list[i], zf);

        if (ret != UNZ_OK && zf_default != NULL)
            ret = asset_load(&g_assets_list[i], zf_default);

        if (ret != UNZ_OK)
            break;
    }

    if (ret != UNZ_OK) {
        for (int i = 0; i < i_bad; i++)
            asset_clean(&g_assets_list[i]);

        if (zf != NULL)
            unzClose(zf);

        if (zf_default != NULL)
            unzClose(zf_default);

        romfsExit();

        return LV_RES_INV;
    }

    theme_init_styles(&g_curr_theme);
    theme_load_styles(&g_curr_theme, zf_default);
    theme_load_styles(&g_curr_theme, zf);

    if (zf != NULL)
        unzClose(zf);

    if (zf_default != NULL)
        unzClose(zf_default);

    romfsExit();

    theme_load_assets(&g_curr_theme, g_assets_list);

    return LV_RES_OK;
}

void theme_exit() {
    for (int i = 0; i < AssetId_max; i++)
        asset_clean(&g_assets_list[i]);
}

lv_res_t theme_reset() {
    theme_exit();

    lv_res_t res = theme_init();
    logPrintf("res(%u)\n");
    if (res != LV_RES_OK)
        return res;

    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);

    return res;
}

theme_t *curr_theme() {
    return &g_curr_theme;
}