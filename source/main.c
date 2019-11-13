#include <lvgl/lvgl.h>
#include <switch.h>

#include "drivers.h"
#include "log.h"
#include "assets.h"
#include "decoder.h"
#include "gui.h"
#include "config.h"

static bool g_should_loop = true;

void stop_main_loop() {
    g_should_loop = false;
}

int main(int argc, char **argv) {
    appletLockExit();

    logInitialize("sdmc:/hbc.log");
    config_init();

    lv_init();

    Result rc = assetsInit();
    if (R_FAILED(rc)) return rc;

    driversInitialize();
    
    decoderInitialize();

    setup_screen();
    setup_menu();
    setup_misc();

    while (appletMainLoop() && g_should_loop) {
        hidScanInput();
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (kHeld & KEY_PLUS)
            break;

        lv_task_handler();
    }

    gui_exit();

    driversExit();
    assetsExit();
    logExit();

    appletUnlockExit();

    return 0;
}