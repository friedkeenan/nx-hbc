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
#include <limits.h>
#include <threads.h>
#include <zlib.h>
#include <sys/stat.h>
#include <lvgl/lvgl.h>

#include "remote.h"
#include "log.h"
#include "settings.h"
#include "util.h"

#define TMP_APP_PATH SETTINGS_DIR "/tmp_remote"

static bool remote_loader_end_recv(remote_loader_t *r);

static ssize_t recvall(remote_loader_t *r, void *buf, size_t len) {
    size_t curr_len = len;

    while (curr_len && !remote_loader_end_recv(r)) {
        ssize_t tmp_len = r->recv_cb(r, buf, curr_len);
        if (tmp_len < 0)
            return tmp_len;

        curr_len -= tmp_len;
        buf += tmp_len;
    }

    return len;
}

static ssize_t sendall(remote_loader_t *r, const void *buf, size_t len) {
    size_t curr_len = len;

    while (curr_len && !remote_loader_end_recv(r)) {
        ssize_t tmp_len = r->send_cb(r, buf, curr_len);
        if (tmp_len < 0)
            return tmp_len;

        curr_len -= tmp_len;
        buf += tmp_len;
    }

    return len;
}

static int decompress(remote_loader_t *r, FILE *fp) {
    z_stream strm;

    // TODO: Make these use lv_mem_alloc and lv_mem_free
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    int ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    do {
        u32 chunk_size = 0;
        if (remote_loader_end_recv(r)) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        ssize_t len = recvall(r, &chunk_size, sizeof(chunk_size));
        if (len != sizeof(chunk_size)) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        if (chunk_size > sizeof(r->in_buf)) {
            inflateEnd(&strm);
            return Z_DATA_ERROR;
        }

        strm.avail_in = recvall(r, r->in_buf, chunk_size);
        if (strm.avail_in != chunk_size) {
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
            if (fwrite(r->out_buf, read_len, 1, fp) != 1 || ferror(fp)) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }

            mtx_lock(&r->mtx);
            r->current += read_len;
            mtx_unlock(&r->mtx);
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return Z_OK;
}

static int recv_app(remote_loader_t *r) {
    logPrintf("recv_app start\n");
    int name_len, file_len;
    char file_name[PATH_MAX + 1];

    ssize_t len = recvall(r, &name_len, sizeof(name_len));
    logPrintf("len(%#lx), sizeof(name_len)(%#lx)\n", len, sizeof(name_len));
    if (len != sizeof(name_len))
        return -1;
    logPrintf("name_len(%d)\n", name_len);

    if (name_len >= sizeof(file_name) - 1)
        return -1;

    len = recvall(r, file_name, name_len);
    if (len != name_len)
        return -1;
    file_name[name_len] = '\0';
    logPrintf("file_name(%s)\n", file_name);

    len = recvall(r, &file_len, sizeof(file_len));
    if (len != sizeof(file_len))
        return -1;
    logPrintf("file_len(%d)\n", file_len);

    mtx_lock(&r->mtx);
    r->total = file_len;
    mtx_unlock(&r->mtx);

    // Does the path need to be sanitized?

    snprintf(r->entry.path, PATH_MAX + 1, APP_DIR "/%s", file_name);
    app_entry_init_base(&r->entry, r->entry.path);
    logPrintf("path: %s\n", r->entry.path);


    int response = 0;

    FILE *fp = fopen(TMP_APP_PATH, "wb");
    if (fp == NULL) {
        response = -1;
    } else {
        if (ftruncate(fileno(fp), file_len) < 0) {
            response = -2;
            fclose(fp);
        }
    }

    logPrintf("send response(%d)\n", response);
    sendall(r, &response, sizeof(response));
    logPrintf("sent response\n");

    if (response == 0) {
        if (decompress(r, fp) == Z_OK) {
            logPrintf("good decompress\n");
            len = sendall(r, &response, sizeof(response));
            if (len != sizeof(response))
                response = -1;

            int args_len;

            if (response == 0) {
                logPrintf("send successful\n");
                len = recvall(r, &args_len, sizeof(args_len));
                if (len != sizeof(args_len))
                    response = -1;
            }

            if (response == 0) {
                logPrintf("recv successful\n");
                if (args_len > APP_ARGS_LEN)
                    args_len = APP_ARGS_LEN;

                char args_buf[args_len];
                len = recvall(r, args_buf, args_len);
                if (len != args_len)
                    response = -1;

                if (response == 0) {
                    char *args_buf_tmp = args_buf;
                    char *args_buf_end = args_buf + args_len;

                    while (args_buf_tmp < args_buf_end) {
                        if (app_entry_add_arg(&r->entry, args_buf_tmp) != LV_RES_OK)
                            break;

                        args_buf_tmp += strlen(args_buf_tmp) + 1;
                    }

                    logPrintf("args 1: %s\n", r->entry.args);
                    if (r->add_args_cb != NULL)
                        r->add_args_cb(r); // For example the net loader would add the _NXLINK_ arg
                    logPrintf("args 2: %s\n", r->entry.args);
                }
            }
        } else {
            logPrintf("bad decompress\n");
            response = -1;
        }

        fflush(fp);
        fclose(fp);

        if (response < 0) {
            remove(TMP_APP_PATH);
        } else {
            switch (r->entry.type) {
                case AppEntryType_homebrew: {
                    char tmp_path[PATH_MAX + 1];
                    strncpy(tmp_path, r->entry.path, PATH_MAX);
                    tmp_path[PATH_MAX] = '\0';

                    *(get_name(tmp_path)) = '\0';
                    logPrintf("tmp_path(%s)\n", tmp_path);

                    mkdirs(tmp_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    remove(r->entry.path);
                    rename(TMP_APP_PATH, r->entry.path);
                } break;

                case AppEntryType_theme: {
                    strncpy(r->entry.path, TMP_APP_PATH, PATH_MAX);
                } break;

                default: {
                    remove(TMP_APP_PATH);
                    response = -1;
                } break;
            }
        }
    }

    return response;
}

bool remote_loader_get_activated(remote_loader_t *r) {
    bool flag;

    mtx_lock(&r->mtx);
    flag = r->flags & RemoteLoaderFlag_activated;
    mtx_unlock(&r->mtx);

    return flag;
}

static void remote_loader_set_activated(remote_loader_t *r, bool activated) {
    mtx_lock(&r->mtx);
    if (activated)
        r->flags |= RemoteLoaderFlag_activated;
    else
        r->flags &= ~(RemoteLoaderFlag_activated);
    mtx_unlock(&r->mtx);
}

bool remote_loader_get_exit(remote_loader_t *r) {
    bool flag;

    mtx_lock(&r->mtx);
    flag = r->flags & RemoteLoaderFlag_exit;
    mtx_unlock(&r->mtx);

    return flag;
}

void remote_loader_set_exit(remote_loader_t *r) {
    mtx_lock(&r->mtx);
    r->flags |= RemoteLoaderFlag_exit;
    mtx_unlock(&r->mtx);
}

bool remote_loader_get_error(remote_loader_t *r) {
    bool flag;

    mtx_lock(&r->mtx);
    flag = r->flags & RemoteLoaderFlag_error;
    mtx_unlock(&r->mtx);

    return flag;
}

void remote_loader_set_error(remote_loader_t *r, bool error) {
    mtx_lock(&r->mtx);
    if (error)
        r->flags |= RemoteLoaderFlag_error;
    else
        r->flags &= ~(RemoteLoaderFlag_error);
    mtx_unlock(&r->mtx);
}

static bool remote_loader_get_cancel(remote_loader_t *r) {
    bool flag;

    mtx_lock(&r->mtx);
    flag = r->flags & RemoteLoaderFlag_cancel;
    mtx_unlock(&r->mtx);

    return flag;
}

void remote_loader_set_cancel(remote_loader_t *r, bool cancel) {
    mtx_lock(&r->mtx);
    if (cancel)
        r->flags |= RemoteLoaderFlag_cancel;
    else
        r->flags &= ~(RemoteLoaderFlag_cancel);
    mtx_unlock(&r->mtx);
}

static bool remote_loader_end_recv(remote_loader_t *r) {
    bool flag;

    mtx_lock(&r->mtx);
    flag = r->flags & (RemoteLoaderFlag_exit | RemoteLoaderFlag_cancel);
    mtx_unlock(&r->mtx);

    return flag;
}

s16 remote_loader_get_progress(remote_loader_t *r) {
    size_t total, current;

    mtx_lock(&r->mtx);
    total = r->total;
    current = r->current;
    mtx_unlock(&r->mtx);

    if (total == 0)
        return 0;

    return 100 * ((float) current / (float) total);
}

static lv_res_t remote_loader_init(remote_loader_t *r) {
    if (mtx_init(&r->mtx, mtx_plain) != thrd_success)
        return LV_RES_INV;

    lv_res_t res = LV_RES_OK;

    if (r->init_cb != NULL) {
        while (!remote_loader_get_exit(r)) {
            res = r->init_cb(r);
            if (res == LV_RES_OK)
                break;

            struct timespec loop_sleep = {.tv_nsec = 100000000};
            thrd_sleep(&loop_sleep, NULL);
        }
    }

    if (res != LV_RES_OK)
        mtx_destroy(&r->mtx);

    return res;
}

static void remote_loader_exit(remote_loader_t *r) {
    mtx_destroy(&r->mtx);

    if (r->exit_cb != NULL)
        r->exit_cb(r);
}

int remote_loader_thread(void *arg) {
    logPrintf("Remote start\n");
    remote_loader_t *r = arg;

    if (remote_loader_init(r) != LV_RES_OK)
        return 0;

    logPrintf("init passed\n");

    while (true) {
        logPrintf("loop\n");
        struct timespec loop_sleep = {.tv_nsec = 100000000};

        while ((remote_loader_get_error(r) || (r->loop_cb(r) != LV_RES_OK)) && !remote_loader_get_exit(r))
            thrd_sleep(&loop_sleep, NULL);

        logPrintf("loop done\n");

        if (!remote_loader_get_exit(r)) {
            remote_loader_set_activated(r, true);
            if (recv_app(r) == 0) {
                app_entry_load(&r->entry);

                // If the app is a homebrew we want to exit as fast as possible
                if (r->entry.type != AppEntryType_homebrew) {
                    if (r->error_cb)
                        r->error_cb(r);

                    mtx_lock(&r->mtx);

                    r->flags = 0;
                    r->total = 0;
                    r->current = 0;

                    mtx_unlock(&r->mtx);
                }
            } else {
                remote_loader_set_activated(r, false);

                if (r->error_cb != NULL)
                    r->error_cb(r);

                mtx_lock(&r->mtx);
                r->total = 0;
                r->current = 0;
                mtx_unlock(&r->mtx);

                if (remote_loader_get_cancel(r))
                    remote_loader_set_cancel(r, false);
                else
                    remote_loader_set_error(r, true);
            }
        } else {
            break;
        }

        remove(TMP_APP_PATH);
    }

    remote_loader_exit(r);

    logPrintf("exit remote");
    return 0;
}