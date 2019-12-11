#include <threads.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "drivers.h"
#include "log.h"
#include "theme.h"
#include "decoder.h"
#include "gui.h"
#include "settings.h"

#ifdef MUSIC

#include "music.h"

#endif

static bool g_should_loop = true;
static mtx_t g_loop_mtx;

void stop_main_loop() {
    mtx_lock(&g_loop_mtx);
    g_should_loop = false;
    mtx_unlock(&g_loop_mtx);
}

bool should_loop() {
    bool loop;

    mtx_lock(&g_loop_mtx);
    loop = g_should_loop;
    mtx_unlock(&g_loop_mtx);

    return loop;
}

int main(int argc, char **argv) {
    appletLockExit();
    appletSetScreenShotPermission(AppletScreenShotPermission_Enable);

    logInitialize("sdmc:/hbc.log");
    
    lv_init();
    
    settings_init();
    theme_init();

    driversInitialize();
    
    decoderInitialize();

    setup_screen();
    setup_menu();
    setup_misc();

    mtx_init(&g_loop_mtx, mtx_plain);

    while (appletMainLoop() && should_loop()) {
        hidScanInput();
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (kHeld & KEY_PLUS)
            break;

        lv_task_handler();
    }

    mtx_destroy(&g_loop_mtx);

    gui_exit();

    driversExit();
    theme_exit();
    logExit();

    appletUnlockExit();

    return 0;
}