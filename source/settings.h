#pragma once

#include <switch.h>
#include <lvgl/lvgl.h>

#define SETTINGS_DIR "sdmc:/config/nx-hbc"

typedef enum {
    RemoteLoaderType_disabled,
    RemoteLoaderType_net,
} RemoteLoaderType;

typedef struct {
    bool use_gyro : 1;
    bool show_limit_warn : 1;
    bool use_fahrenheit : 1;

    #ifdef MUSIC

    bool play_bgm : 1;

    #endif

    RemoteLoaderType remote_type;

    u8 lang_id;
} settings_t;

lv_res_t settings_init();

settings_t *curr_settings();