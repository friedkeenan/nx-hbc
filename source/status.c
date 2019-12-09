#include <lvgl/lvgl.h>
#include <switch.h>

#include "status.h"

lv_res_t status_init() {
    if (R_FAILED(nifmInitialize(NifmServiceType_User)))
        return LV_RES_INV;

    if (R_FAILED(tsInitialize())) {
        nifmExit();
        return LV_RES_INV;
    }

    return LV_RES_OK;
}

void status_exit() {
    tsExit();
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

lv_res_t get_thermal_status(s32 *temp) {
    if (R_FAILED(tsGetTemperatureMilliC(TsLocation_Internal, temp)))
        return LV_RES_INV;
    return LV_RES_OK;
}