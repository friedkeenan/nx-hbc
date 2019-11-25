#include <lvgl/lvgl.h>
#include <switch.h>

#include "net_status.h"

lv_res_t net_status_init() {
    if (R_FAILED(nifmInitialize(NifmServiceType_User)))
        return LV_RES_INV;
    return LV_RES_OK;
}

void net_status_exit() {
    nifmExit();
}

NetStatus get_net_status() {
    NifmInternetConnectionType conn_type;
    u32 conn_strength;
    NifmInternetConnectionStatus conn_status;

    if (R_FAILED(nifmGetInternetConnectionStatus(&conn_type, &conn_strength, &conn_status)))
        return NetStatus_disconnected;

    if (conn_status == NifmInternetConnectionStatus_Connected)
        return NetStatus_connected;

    return NetStatus_connecting;
}