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

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "apps.h"
#include "log.h"
#include "remote.h"
#include "remote_net.h"

#define PING_ENABLED 1

#if PING_ENABLED

#define PING_MSG "nxboot"
#define PONG_MSG "bootnx"

#endif

#define SERVER_PORT NXLINK_SERVER_PORT
#define CLIENT_PORT NXLINK_CLIENT_PORT

typedef struct {
    #if PING_ENABLED
    int udpfd;
    #endif

    int listenfd;
    int connfd;

    u32 host;
} net_data_t;

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return flags;

    flags |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags);
}

static lv_res_t net_init_cb(remote_loader_t *r) {
    if (R_FAILED(socketInitializeDefault())) {
        return LV_RES_INV;
    }

    if (R_FAILED(nifmInitialize())) {
        socketExit();
        return LV_RES_INV;
    }

    NifmInternetConnectionType conn_type;
    u32 conn_strength = 0;
    NifmInternetConnectionStatus conn_status;

    if (R_FAILED(nifmGetInternetConnectionStatus(&conn_type, &conn_strength, &conn_status))) {
        nifmExit();
        socketExit();
        return LV_RES_INV;
    }
    nifmExit();

    if (conn_strength == 0) {
        socketExit();
        return LV_RES_INV;
    }

    r->custom_data = malloc(sizeof(net_data_t));
    net_data_t *data = r->custom_data;

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);

    #if PING_ENABLED

    data->udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (data->udpfd < 0) {
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    if (bind(data->udpfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        close(data->connfd);
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    if (set_nonblocking(data->udpfd) == -1) {
        close(data->udpfd);
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    #endif

    data->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (data->listenfd < 0) {
        #if PING_ENABLED

        close(data->udpfd);

        #endif

        free(data);
        socketExit();
        return LV_RES_INV;
    }

    if (bind(data->listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        #if PING_ENABLED

        close(data->udpfd);

        #endif

        close(data->listenfd);
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    if (set_nonblocking(data->listenfd) == -1) {
        #if PING_ENABLED

        close(data->udpfd);

        #endif

        close(data->listenfd);
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    if (listen(data->listenfd, 10) < 0) {
        #if PING_ENABLED

        close(data->udpfd);

        #endif

        close(data->listenfd);
        free(data);
        socketExit();
        return LV_RES_INV;
    }

    data->connfd = -1;

    return LV_RES_OK;
}

static void net_exit_cb(remote_loader_t *r) {
    net_data_t *data = r->custom_data;

    if (data->listenfd >= 0)
        close(data->listenfd);

    if (data->connfd >= 0)
        close(data->connfd);

    #if PING_ENABLED

    if (data->udpfd >= 0)
        close(data->udpfd);

    #endif

    socketExit();

    free(data);
}

static void net_error_cb(remote_loader_t *r) {
    net_data_t *data = r->custom_data;

    if (data->connfd >= 0) {
        close(data->connfd);
        data->connfd = -1;
    }
}

static lv_res_t net_loop_cb(remote_loader_t *r) {
    net_data_t *data = r->custom_data;
    struct sockaddr_in remote;
    socklen_t remote_len = sizeof(remote);

    #if PING_ENABLED

    char recv_buf[0x100];

    ssize_t len = recvfrom(data->udpfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &remote, &remote_len);

    if (len >= 0) {
        if (strncmp(recv_buf, PING_MSG, strlen(PING_MSG)) == 0) {
            remote.sin_family = AF_INET;
            remote.sin_port = htons(CLIENT_PORT);

            sendto(data->udpfd, PONG_MSG, strlen(PONG_MSG), 0, (struct sockaddr *) &remote, remote_len);
        }
    }

    #endif

    if (data->listenfd >= 0 && data->connfd < 0) {
        data->connfd = accept(data->listenfd, (struct sockaddr *) &remote, &remote_len);
        if (data->connfd < 0) {
            data->connfd = -1;
            return LV_RES_INV;
        } else if (set_nonblocking(data->connfd) == -1) {
            close(data->connfd);
            data->connfd = -1;
            return LV_RES_INV;
        }

        data->host = remote.sin_addr.s_addr;

        return LV_RES_OK;
    }

    return LV_RES_INV;
}

static ssize_t net_recv_cb(remote_loader_t *r, void *buf, size_t len) {
    net_data_t *data = r->custom_data;

    ssize_t tmp_len = recv(data->connfd, buf, len, 0);
    if (tmp_len == 0)
        return -1;

    if (tmp_len < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return 0;
    }

    return tmp_len;
}

static ssize_t net_send_cb(remote_loader_t *r, const void *buf, size_t len) {
    net_data_t *data = r->custom_data;

    ssize_t tmp_len = send(data->connfd, buf, len, 0);
    if (tmp_len == 0)
        return -1;

    if (tmp_len < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return 0;
    }

    return tmp_len;
}

static void net_add_args_cb(remote_loader_t *r) {
    net_data_t *data = r->custom_data;

    char arg[17];
    arg[0] = '\0';

    sprintf(arg, "%08x_NXLINK_", data->host);
    app_entry_add_arg(&r->entry, arg);
}

static remote_loader_t g_net_loader = {
    .init_cb = net_init_cb,
    .exit_cb = net_exit_cb,

    .error_cb = net_error_cb,

    .loop_cb = net_loop_cb,

    .recv_cb = net_recv_cb,
    .send_cb = net_send_cb,

    .add_args_cb = net_add_args_cb,
};

remote_loader_t *net_loader() {
    return &g_net_loader;
}