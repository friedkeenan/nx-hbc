#include <lvgl/lvgl.h>
#include <switch.h>

#include "drivers.h"
#include "log.h"
#include "assets.h"
#include "gui.h"

int main(int argc, char **argv) {
    appletLockExit();

    logInitialize("/hbc.log");

    lv_init();
    driversInitialize();

    Result rc = assetsInit();
    if (R_FAILED(rc)) return rc;

    setup_screen();
    setup_buttons();
    setup_misc();

    while (appletMainLoop()) {
        consoleUpdate(NULL);

        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break;

        lv_task_handler();
        svcSleepThread(1e+6L); // Sleep for 1 millisecond
        lv_tick_inc(1);
    }

    lv_obj_del(lv_scr_act());

    assetsExit();
    driversExit();
    logExit();

    appletUnlockExit();

    return 0;
}