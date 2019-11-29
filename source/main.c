#include <threads.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "drivers.h"
#include "log.h"
#include "theme.h"
#include "decoder.h"
#include "gui.h"
#include "settings.h"

static bool g_should_loop = true;

void stop_main_loop() {
    g_should_loop = false;
}

int main(int argc, char **argv) {
    appletLockExit();

    logInitialize("sdmc:/hbc.log");

    lv_init();

    settings_init();
    theme_init();

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
    theme_exit();
    logExit();

    appletUnlockExit();

    return 0;
}