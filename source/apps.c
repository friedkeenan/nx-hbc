#include <lvgl/lvgl.h>
#include <stdio.h>
#include <switch.h>

#include "apps.h"
#include "log.h"

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