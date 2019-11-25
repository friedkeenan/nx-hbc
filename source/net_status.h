#pragma once

#include <lvgl/lvgl.h>

typedef enum {
    NetStatus_disconnected,
    NetStatus_connecting,
    NetStatus_connected,    
} NetStatus;

lv_res_t net_status_init();
void net_status_exit();

NetStatus get_net_status();