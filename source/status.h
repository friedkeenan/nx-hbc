#pragma once

#include <lvgl/lvgl.h>

typedef enum {
    NetStatus_disconnected,
    NetStatus_connecting,
    NetStatus_connected,    
} NetStatus;

lv_res_t status_init();
void status_exit();

NetStatus get_net_status();

lv_res_t get_thermal_status(s32 *temp);