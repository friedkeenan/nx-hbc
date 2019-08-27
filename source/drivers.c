#include <lvgl/lvgl.h>
#include <switch.h>

#include "log.h"

static Framebuffer g_framebuffer;
static lv_disp_buf_t g_disp_buf;
static lv_color_t g_buffer[LV_HOR_RES_MAX * LV_VER_RES_MAX];

static touchPosition g_touch_pos;

static lv_group_t *g_keypad_group;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    u32 stride;
    lv_color_t *fb = (lv_color_t *) framebufferBegin(&g_framebuffer, &stride);

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            fb[y * stride / sizeof(lv_color_t) + x] = *color_p;
            color_p++;
        }
    }

    framebufferEnd(&g_framebuffer);

    lv_disp_flush_ready(drv);
}

static bool touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {

    if (hidTouchCount()) {
        hidTouchRead(&g_touch_pos, 0);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->point.x = g_touch_pos.px;
    data->point.y = g_touch_pos.py;

    return false;
}

static bool keypad_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    u64 pressed = hidKeysHeld(CONTROLLER_P1_AUTO);

    data->state = LV_INDEV_STATE_PR;

    if (pressed & KEY_LEFT)
        data->key = LV_KEY_LEFT;
    else if (pressed & KEY_RIGHT)
        data->key = LV_KEY_RIGHT;
    else if (pressed & KEY_DOWN)
        data->key = LV_KEY_DOWN;
    else if (pressed & KEY_UP)
        data->key = LV_KEY_UP;
    else if (pressed & KEY_A)
        data->key = LV_KEY_ENTER;
    else if (pressed & KEY_B)
        data->key = LV_KEY_ESC;
    else
        data->state = LV_INDEV_STATE_REL;

    return false;
}

void driversInitialize() {
    NWindow *win = nwindowGetDefault();
    framebufferCreate(&g_framebuffer, win, LV_HOR_RES_MAX, LV_VER_RES_MAX, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&g_framebuffer);

    lv_disp_buf_init(&g_disp_buf, g_buffer, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.buffer = &g_disp_buf;
    disp_drv.flush_cb = flush_cb;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    logPrintf("disp(%p)\n", disp);

    lv_indev_drv_t touch_drv;
    lv_indev_drv_init(&touch_drv);
    touch_drv.type = LV_INDEV_TYPE_POINTER;
    touch_drv.read_cb = touch_cb;
    lv_indev_t *touch_indev = lv_indev_drv_register(&touch_drv);
    logPrintf("touch_indev(%p)\n", touch_indev);

    lv_indev_drv_t keypad_drv;
    lv_indev_drv_init(&keypad_drv);
    keypad_drv.type = LV_INDEV_TYPE_KEYPAD;
    keypad_drv.read_cb = keypad_cb;
    lv_indev_t *keypad_indev = lv_indev_drv_register(&keypad_drv);
    logPrintf("keypad_indev(%p)\n", keypad_indev);

    g_keypad_group = lv_group_create();
    lv_indev_set_group(keypad_indev, g_keypad_group);
}

void driversExit() {
    lv_group_del(g_keypad_group);

    framebufferClose(&g_framebuffer);
}

lv_group_t *keypad_group() {
    return g_keypad_group;
}