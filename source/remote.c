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

#include <stdio.h>
#include <zlib.h>
#include <lvgl/lvgl.h>

#include "remote.h"

static size_t recvall(remote_loader_t *r, void *buf, size_t len) {
    while (len) {
        size_t tmp_len = r->recv_cb(r, buf, len);

        len -= tmp_len;
        buf += tmp_len;
    }

    return len;
}

static size_t sendall(remote_loader_t *r, const void *buf, size_t len) {
    while (len) {
        size_t tmp_len = r->send_cb(r, buf, len);

        len -= tmp_len;
        buf += tmp_len;
    }

    return len;
}

static int decompress(remote_loader_t *r, FILE *fp, size_t file_size) {
    int ret;
    z_stream strm;
    u32 chunk_size = 0;

    // TODO: Make these use lv_mem_alloc and lv_mem_free
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    do {
        size_t len = recvall(r, &chunk_size, sizeof(chunk_size));
        if (len != sizeof(chunk_size)) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        if (chunk_size > sizeof(r->in_buf)) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        strm.avail_in = recvall(r, r->in_buf, chunk_size);
        if (strm.avail_in == 0) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        strm.next_in = r->in_buf;

        do {
            strm.avail_out = ZLIB_CHUNK;
            strm.next_out = r->out_buf;

            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                    inflateEnd(&strm);
                    return ret;
            }

            size_t read_len = ZLIB_CHUNK - strm.avail_out;
            if (fwrite(r->out_buf, read_len, 1, fp) != 1) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return Z_OK;
}