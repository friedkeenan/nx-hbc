#include <lvgl/lvgl.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <switch.h>

#include "apps.h"
#include "log.h"
#include "util.h"

lv_res_t app_entry_init(app_entry_t *entry) {
    FILE *fp = fopen(entry->path, "rb");
    if (fp == NULL) {
        logPrintf("Bad file\n");
        return LV_RES_INV;
    }

    NroHeader header;
    NroAssetHeader asset_header;

    fseek(fp, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        logPrintf("Bad header read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    fseek(fp, header.size, SEEK_SET);
    if (fread(&asset_header, sizeof(asset_header), 1, fp) != 1) {
        logPrintf("Bad asset header read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    logPrintf("asset_header(icon(%#lx, %#lx), nacp(%#lx, %#lx))\n", asset_header.icon.offset, asset_header.icon.size, asset_header.nacp.offset, asset_header.nacp.size);

    entry->icon = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = APP_ICON_W,
        .header.h = APP_ICON_H,
        .data_size = asset_header.icon.size,
        .header.cf = LV_IMG_CF_RAW,
    };
    entry->icon.data = lv_mem_alloc(entry->icon.data_size);
    if (entry->icon.data == NULL) {
        logPrintf("Bad icon alloc\n");
        fclose(fp);
        return LV_RES_INV;
    }

    fseek(fp, header.size + asset_header.icon.offset, SEEK_SET);
    if (fread((u8 *) entry->icon.data, entry->icon.data_size, 1, fp) != 1) {
        logPrintf("Bad icon read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    entry->icon_small = entry->icon;
    entry->icon_small.header.w = APP_ICON_SMALL_W;
    entry->icon_small.header.h = APP_ICON_SMALL_H;

    NacpStruct nacp;

    fseek(fp, header.size + asset_header.nacp.offset, SEEK_SET);
    if (fread(&nacp, sizeof(nacp), 1, fp) != 1) {
        logPrintf("Bad nacp read\n");
        fclose(fp);
        return LV_RES_INV;
    }

    strcpy(entry->name, nacp.lang[0].name);
    strcpy(entry->author, nacp.lang[0].author);
    strcpy(entry->version, nacp.version);

    fclose(fp);
    return LV_RES_OK;
}

void app_entry_free(app_entry_t *entry) {
    lv_mem_free(entry->icon.data);
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

        app_entry_t *entry;

        if (is_dir(tmp_path)) {
            DIR *dp = opendir(tmp_path);
            if (dp == NULL)
                continue;

            while ((ep = readdir(dp))) {
                strcat(tmp_path, "/");
                strcat(tmp_path, ep->d_name);

                if (strcasecmp(get_ext(ep->d_name), "nro") == 0) {
                    entry = lv_ll_ins_tail(ll);
                    strcpy(entry->path, tmp_path);
                    break;
                }
            }

            closedir(dp);
        } else if (strcasecmp(get_ext(ep->d_name), "nro") == 0) {
            entry = lv_ll_ins_tail(ll);
            strcpy(entry->path, tmp_path);
        }
    }

    closedir(app_dp);
    return LV_RES_OK;
}