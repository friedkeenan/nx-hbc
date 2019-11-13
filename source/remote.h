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

/*
 * Copyright 2017 libnx Authors
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

#include <threads.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "apps.h"

#define ZLIB_CHUNK (16 * 1024)

typedef enum {
    RemoteLoaderFlag_activated = BIT(0),
    RemoteLoaderFlag_exit = BIT(1),
    RemoteLoaderFlag_error = BIT(2),
    RemoteLoaderFlag_cancel = BIT(3),
} RemoteLoaderFlag;

typedef struct remote_loader {
    mtx_t mtx;

    u8 flags;

    app_entry_t entry;
    size_t total, current;

    u8 in_buf[ZLIB_CHUNK];
    u8 out_buf[ZLIB_CHUNK];

    lv_res_t (*init_cb)(struct remote_loader *r);
    void (*exit_cb)(struct remote_loader *r);

    void (*error_cb)(struct remote_loader *r);

    /*
     * Should return LV_RES_INV until it's ready to
     * receive the app, and then return LV_RES_OK.
     */
    lv_res_t (*loop_cb)(struct remote_loader *r);

    ssize_t (*recv_cb)(struct remote_loader *r, void *buf, size_t len);
    ssize_t (*send_cb)(struct remote_loader *r, const void *buf, size_t len);

    void (*add_args_cb)(struct remote_loader *r);

    void *custom_data;
} remote_loader_t;

bool remote_loader_get_activated(remote_loader_t *r);

bool remote_loader_get_exit(remote_loader_t *r);
void remote_loader_set_exit(remote_loader_t *r);

bool remote_loader_get_error(remote_loader_t *r);
void remote_loader_set_error(remote_loader_t *r, bool error);

void remote_loader_set_cancel(remote_loader_t *r, bool cancel);

s16 remote_loader_get_progress(remote_loader_t *r);

int remote_loader_thread(void *arg);