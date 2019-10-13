#include <lvgl/lvgl.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <switch.h>

#include "apps.h"
#include "log.h"
#include "util.h"

lv_res_t app_entry_init_icon(app_entry_t *entry) {
    FILE *fp = fopen(entry->path, "rb");
    if (fp == NULL) {
        LV_LOG_WARN("Bad file");
        return LV_RES_INV;
    }

    NroHeader header;
    NroAssetHeader asset_header;

    fseek(fp, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        LV_LOG_WARN("Bad header read");
        fclose(fp);
        return LV_RES_INV;
    }

    fseek(fp, header.size, SEEK_SET);
    if (fread(&asset_header, sizeof(asset_header), 1, fp) != 1) {
        LV_LOG_WARN("Bad asset header read");
        fclose(fp);
        return LV_RES_INV;
    }

    entry->icon = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = APP_ICON_W,
        .header.h = APP_ICON_H,
        .data_size = asset_header.icon.size,
        .header.cf = LV_IMG_CF_RAW,
    };
    entry->icon.data = lv_mem_alloc(entry->icon.data_size);
    if (entry->icon.data == NULL) {
        LV_LOG_WARN("Bad icon alloc");
        fclose(fp);
        return LV_RES_INV;
    }

    fseek(fp, header.size + asset_header.icon.offset, SEEK_SET);
    if (fread((u8 *) entry->icon.data, entry->icon.data_size, 1, fp) != 1) {
        LV_LOG_WARN("Bad icon read");
        fclose(fp);
        return LV_RES_INV;
    }

    entry->icon_small = entry->icon;
    entry->icon_small.header.w = APP_ICON_SMALL_W;
    entry->icon_small.header.h = APP_ICON_SMALL_H;

    fclose(fp);
    return LV_RES_OK;
}

void app_entry_free_icon(app_entry_t *entry) {
    lv_mem_free((void *) entry->icon.data);
}

lv_res_t app_entry_init_info(app_entry_t *entry) {
    FILE *fp = fopen(entry->path, "rb");
    if (fp == NULL) {
        LV_LOG_WARN("Bad file\n");
        return LV_RES_INV;
    }

    NroHeader header;
    NroAssetHeader asset_header;

    fseek(fp, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        LV_LOG_WARN("Bad header read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    fseek(fp, header.size, SEEK_SET);
    if (fread(&asset_header, sizeof(asset_header), 1, fp) != 1) {
        LV_LOG_WARN("Bad asset header read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    NacpStruct nacp;

    fseek(fp, header.size + asset_header.nacp.offset, SEEK_SET);
    if (fread(&nacp, sizeof(nacp), 1, fp) != 1) {
        LV_LOG_WARN("Bad nacp read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    strcpy(entry->name, nacp.lang[0].name);
    strcpy(entry->author, nacp.lang[0].author);
    strcpy(entry->version, nacp.version);

    fclose(fp);
    return LV_RES_OK;
}

void app_entry_get_star_path(app_entry_t *entry, char *out_path) {
    strcpy(out_path, entry->path);
    strcpy(get_name(out_path), ".");
    strcpy(get_name(out_path) + 1, get_name(entry->path));
    strcat(out_path, ".star");
}

lv_res_t app_entry_set_star(app_entry_t *entry, bool star) {
    char star_path[PATH_MAX];
    app_entry_get_star_path(entry, star_path);

    if (star) {
        FILE *fp = fopen(star_path, "w");

        if (fp == NULL)
            return LV_RES_INV;

        fclose(fp);
    } else {
        if (remove(star_path) != 0)
            return LV_RES_INV;
    }

    entry->starred = star;

    return LV_RES_OK;
}

lv_res_t app_entry_delete(app_entry_t *entry) {
    app_entry_set_star(entry, false); // Try to remove star file

    if (get_name(entry->path) == entry->path + sizeof(APP_DIR)) { // Is just a file under the app directory
        if (remove(entry->path) != 0)
            return LV_RES_INV;
    } else {
        char del_path[PATH_MAX];
        strcpy(del_path, entry->path);

        *(get_name(del_path)) = '\0';

        if (R_FAILED(fsdevDeleteDirectoryRecursively(del_path))) // Maybe replace this with some stdio stuff?
            return LV_RES_INV;

        if (rmdir(del_path) != 0)
            return LV_RES_INV;
    }

    return LV_RES_OK;
}

lv_res_t app_entry_ll_ins(lv_ll_t *ll, char *path) {
    app_entry_t *entry = lv_ll_ins_tail(ll);
    strcpy(entry->path, path);
    
    char star_path[PATH_MAX];
    app_entry_get_star_path(entry, star_path);
    entry->starred = is_file(star_path);

    lv_res_t res = app_entry_init_info(entry);
    if (res != LV_RES_OK) {
        lv_ll_rem(ll, entry);
        return res;
    }

    app_entry_t *tmp_entry;
    LV_LL_READ(*ll, tmp_entry) {
        if (!entry->starred && tmp_entry->starred)
            continue;

        if ((entry->starred && !tmp_entry->starred) || strcasecmp(entry->name, tmp_entry->name) < 0) {
            lv_ll_move_before(ll, entry, tmp_entry);
            break;
        }
    }

    return LV_RES_OK;
}

lv_res_t app_entry_ll_init(lv_ll_t *ll) {
    DIR *app_dp = opendir(APP_DIR);
    if (app_dp == NULL)
        return LV_RES_INV;

    lv_ll_init(ll, sizeof(app_entry_t));

    struct dirent *ep;
    while ((ep = readdir(app_dp))) {
        char tmp_path[PATH_MAX];
        tmp_path[0] = '\0';
        snprintf(tmp_path, sizeof(tmp_path), "%s/%s", APP_DIR, ep->d_name);

        if (is_dir(tmp_path)) {
            DIR *dp = opendir(tmp_path);
            if (dp == NULL)
                continue;

            while ((ep = readdir(dp))) {
                char path[PATH_MAX];
                strcpy(path, tmp_path);
                strcat(path, "/");
                strcat(path, ep->d_name);

                if (strcasecmp(get_ext(ep->d_name), "nro") == 0) {
                    if (app_entry_ll_ins(ll, path) != LV_RES_OK)
                        continue;

                    break;
                }
            }

            closedir(dp);
        } else if (strcasecmp(get_ext(ep->d_name), "nro") == 0) {
            app_entry_ll_ins(ll, tmp_path);
        }
    }

    closedir(app_dp);
    return LV_RES_OK;
}