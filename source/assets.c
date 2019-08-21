/*
    Copyright 2017-2018 nx-hbmenu Authors

    Permission to use, copy, modify, and/or distribute this
    software for any purpose with or without fee is hereby
    granted, provided that the above copyright notice and
    this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR
    DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
    INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
    FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
    OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
    NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
    CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "assets.h"

#include <minizip/unzip.h>
#include <string.h>

typedef struct {
    u8 *buffer;
    size_t size;
    const char *filename;
} assetsDataEntry;

#define GENASSET(x) {.filename = x}

static bool g_assetsInitialized = 0;
assetsDataEntry g_assetsDataList[AssetId_Max] = {
    GENASSET("background.bin"),
    GENASSET("apps_list.bin"),
    GENASSET("apps_list_hover.bin"),
    GENASSET("apps_next.bin"),
    GENASSET("apps_next_hover.bin"),
    GENASSET("apps_previous.bin"),
    GENASSET("apps_previous_hover.bin"),
    GENASSET("logo.bin"),
    GENASSET("icon.bin"),
};

static void assetsClearEntry(assetsDataEntry *entry) {
    free(entry->buffer);

    entry->size = 0;
    entry->buffer = NULL;
}

static int assetsLoadFile(unzFile zipf, assetsDataEntry *entry) {
    int ret;
    int filesize=0;
    unz_file_info file_info;
    u8* buffer = NULL;

    ret = unzLocateFile(zipf, entry->filename, 0);

    if (ret==UNZ_OK) ret = unzOpenCurrentFile(zipf);

    if (ret==UNZ_OK) {
        ret = unzGetCurrentFileInfo(zipf, &file_info, NULL, 0, NULL, 0, NULL, 0);

        filesize = file_info.uncompressed_size;
        if (filesize == 0) ret = -10;

        if (ret==UNZ_OK) {
            buffer = (u8*)malloc(filesize);
            if (buffer) {
                memset(buffer, 0, filesize);
            } else {
                ret = -11;
            }
        }

        if (ret==UNZ_OK) {
            ret = unzReadCurrentFile(zipf, buffer, filesize);
            if(ret < filesize) {
                ret = -12;
            } else {
                ret = UNZ_OK;
            }
        }

        if (ret!=UNZ_OK && buffer!=NULL) free(buffer);

        unzCloseCurrentFile(zipf);
    }

    if (ret==UNZ_OK) {
        entry->buffer = buffer;
        entry->size = filesize;
    }

    return ret;
}

Result assetsInit(void) {
    int ret=0;
    int i, stopi;
    unzFile zipf;
    assetsDataEntry *entry = NULL;
    char tmp_path[PATH_MAX+1];

    if (g_assetsInitialized) return 0;

    Result rc = romfsInit();
    if (R_FAILED(rc)) return rc;

    memset(tmp_path, 0, sizeof(tmp_path));

    strncpy(tmp_path, "romfs:/assets.zip", sizeof(tmp_path)-1);

    zipf = unzOpen(tmp_path);
    if(zipf==NULL) {
        romfsExit();

        return 0x80;
    }

    for (i=0; i<AssetId_Max; i++) {
        stopi = i;
        entry = &g_assetsDataList[i];
        ret = assetsLoadFile(zipf, entry);
        if (ret!=UNZ_OK) break;
    }

    if (ret!=UNZ_OK) {
        for (i=0; i<stopi; i++) {
            assetsClearEntry(&g_assetsDataList[i]);
        }
    }

    if (ret==UNZ_OK) g_assetsInitialized = 1;

    unzClose(zipf);

    romfsExit();

    return ret;
}

void assetsExit(void) {
    int i;

    if (!g_assetsInitialized) return;
    g_assetsInitialized = 0;

    for (i=0; i<AssetId_Max; i++) {
        assetsClearEntry(&g_assetsDataList[i]);
    }
}

void assetsGetData(AssetId id, u8 **buffer, size_t *size) {
    if (buffer) *buffer = NULL;
    if (size) *size = 0;
    if (id < 0 || id >= AssetId_Max) return;

    assetsDataEntry *entry = &g_assetsDataList[id];

    if (buffer) *buffer = entry->buffer;
    if (size) *size = entry->size;
}

u8 *assetsGetDataBuffer(AssetId id) {
    u8 *buffer = NULL;

    assetsGetData(id, &buffer, NULL);
    return buffer;
}

