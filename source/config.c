#include <string.h>
#include <sys/stat.h>
#include <lvgl/lvgl.h>

#include "config.h"
#include "util.h"

lv_res_t config_init() {
    if (mkdirs(CONFIG_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        return LV_RES_INV;

    return LV_RES_OK;
}