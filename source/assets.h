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

#pragma once

#include <switch.h>

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

    AssetId_Max
} AssetId;

Result assetsInit(void);
void assetsExit(void);
void assetsGetData(AssetId id, u8 **buffer, size_t *size);
u8 *assetsGetDataBuffer(AssetId id);

