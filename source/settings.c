#include <string.h>
#include <sys/stat.h>
#include <libconfig.h>
#include <switch.h>
#include <lvgl/lvgl.h>

#include "settings.h"
#include "util.h"

#define SETTINGS_FILE SETTINGS_DIR "/settings.cfg"

static settings_t g_default_settings = {
    .use_gyro = false,
    .show_limit_warn = true,
    .remote_type = RemoteLoaderType_net,
    .lang_id = SetLanguage_ENUS,
};

static settings_t g_curr_settings;

static inline u64 str_to_lang_code(const char *str) {
    u64 code = 0;

    for (int i = 0; i < sizeof(code); i++) {
        if (str[i] == '\0')
            break;

        code |= str[i] << 8 * str[i];
    }

    return code;
}

lv_res_t settings_init() {
    if (mkdirs(SETTINGS_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        return LV_RES_INV;

    u64 lang_code = 0;

    config_t cfg;
    config_setting_t *settings;

    config_init(&cfg);

    if (config_read_file(&cfg, SETTINGS_FILE) == CONFIG_TRUE && (settings = config_lookup(&cfg, "settings")) != NULL) {
        int use_gyro;
        if (config_setting_lookup_bool(settings, "use_gyro", &use_gyro) != CONFIG_TRUE)
            use_gyro = g_default_settings.use_gyro;
        g_curr_settings.use_gyro = use_gyro;

        int show_limit_warn;
        if (config_setting_lookup_bool(settings, "show_limit_warn", &show_limit_warn) != CONFIG_TRUE)
            show_limit_warn = g_default_settings.show_limit_warn;
        g_curr_settings.show_limit_warn = show_limit_warn;

        int remote_type;
        if (config_setting_lookup_int(settings, "remote_type", &remote_type) != CONFIG_TRUE)
            remote_type = g_default_settings.remote_type;
        g_curr_settings.remote_type = remote_type;

        const char *lang;
        if (config_setting_lookup_string(settings, "language", &lang) == CONFIG_TRUE)
            lang_code = str_to_lang_code(lang);
    } else {
        g_curr_settings = g_default_settings;
    }

    config_destroy(&cfg);

    if (R_SUCCEEDED(setInitialize())) {
        if (lang_code == 0)
            setGetSystemLanguage(&lang_code);

        SetLanguage lang_id;
        if (R_FAILED(setMakeLanguage(lang_code, &lang_id)))
            lang_id = g_default_settings.lang_id;

        g_curr_settings.lang_id = lang_id;

        setExit();
    } else {
        g_curr_settings.lang_id = g_default_settings.lang_id;
    }

    return LV_RES_OK;
}

settings_t *curr_settings() {
    return &g_curr_settings;
}