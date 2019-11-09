#include <lvgl/lvgl.h>
#include <switch.h>
#include <math.h>

#include "drivers.h"
#include "log.h"
#include "assets.h"

static Framebuffer g_framebuffer;
static lv_disp_buf_t g_disp_buf;
static lv_color_t g_buffer[LV_HOR_RES_MAX * LV_VER_RES_MAX];

static touchPosition g_touch_pos;

static lv_indev_t *g_keypad_indev;
static lv_group_t *g_keypad_group;

static u32 g_sixaxis_handles[4]; // Sixaxis handles
// We actually only need to keep track of a specific set of data so we dont need all sixaxis values
static HidVector g_gyro_center;
static lv_obj_t * g_pointer_canvas;
static lv_img_dsc_t g_pointer_img;
static lv_color_t g_pointer_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(96, 96)];
static lv_obj_t * g_pointer_fake_canvas;
static float g_pointer_screen_magic = 0.7071f; // This is a repeating number that describes the top right of a square inside a unit circle whose sides are parallel to the x-y axis'
static lv_indev_t *g_gyro_indev;
static bool g_clear_pointer_canvas = true;

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
    hidScanInput();

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
    hidScanInput();

    u64 pressed = hidKeysHeld(CONTROLLER_P1_AUTO);

    data->state = LV_INDEV_STATE_PR;

    if (pressed & KEY_A)
        data->key = LV_KEY_ENTER;
    else if (pressed & KEY_B)
        data->key = LV_KEY_ESC;
    else if (pressed & KEY_LEFT)
        data->key = LV_KEY_LEFT;
    else if (pressed & KEY_RIGHT)
        data->key = LV_KEY_RIGHT;
    else if (pressed & KEY_DOWN)
        data->key = LV_KEY_DOWN;
    else if (pressed & KEY_UP)
        data->key = LV_KEY_UP;
    else
        data->state = LV_INDEV_STATE_REL;

    return false;
}

static void center_gyro(SixAxisSensorValues sixaxis) {
    // These track center, unknown is actually a float value that can keep track of full rotations, 1 unit = 1 full rotation in that direction
    g_gyro_center.x = sixaxis.unk.x; // Rotation along y (which trannslates to left right)
    g_gyro_center.y = sixaxis.unk.z; // Rotation along x (^^^^^^^^^^^^^^^^^^^^ up down) [Note: this is switched with Z in handheld mode]
    g_gyro_center.z = sixaxis.unk.y; // Rotation on forward axis
}

static bool gyro_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    hidScanInput();

    // Scan for input changes 
    SixAxisSensorValues sixaxis;
    hidSixAxisSensorValuesRead(&sixaxis, CONTROLLER_P1_AUTO, 1);

    // Center gyro
    u64 pressed = hidKeysHeld(CONTROLLER_P1_AUTO);
    if (pressed & KEY_X)
        center_gyro(sixaxis);

    if (pressed & KEY_A)
        data->state = LV_INDEV_STATE_PR;
    else
        data->state = LV_INDEV_STATE_REL;

    // Center input according to g_gyro_center
    HidVector finalvector;
    finalvector.x = CURSOR_SENSITIVITY * (sixaxis.unk.x - g_gyro_center.x);
    finalvector.y = CURSOR_SENSITIVITY * (sixaxis.unk.z - g_gyro_center.y);
    finalvector.z = sixaxis.unk.y - g_gyro_center.z;

    float x_rad = 2 * M_PI * finalvector.x;
    float y_rad = 2 * M_PI * finalvector.y;
    float z_rad = 2 * M_PI * finalvector.z;

    // Rotate 3d point at (0,0,1) along x y and z and then put it into (x,y) coordinates this return a point inside a circile of radius 1
    finalvector.x = sin(z_rad) * sin(x_rad) - cos(z_rad) * sin(y_rad) * cos(x_rad);
    finalvector.y = cos(z_rad) * sin(x_rad) + sin(z_rad) * sin(y_rad) * cos(x_rad);

    // x and y need to be clamped to our boundries should the absolute value be above it
    if (fabs(finalvector.x) > g_pointer_screen_magic) {
        // The value can be negative or positive
        if (finalvector.x > 0.0f)
            finalvector.x = g_pointer_screen_magic;
        else
            finalvector.x = -g_pointer_screen_magic;
    }

    if (fabs(finalvector.y) > g_pointer_screen_magic) {
        if (finalvector.y > 0.0f)
            finalvector.y = g_pointer_screen_magic;
        else
            finalvector.y = -g_pointer_screen_magic;
    }

    // Now we need to invert y and convert them back to a 0 1 range
    finalvector.x = (finalvector.x + g_pointer_screen_magic) / 2.0f / g_pointer_screen_magic;
    finalvector.y = (-finalvector.y + g_pointer_screen_magic) / 2.0f  / g_pointer_screen_magic;
    data->point.x = ((float) 1280 * finalvector.x);
    data->point.y = ((float) 720 * finalvector.y);

    // Clear canvas and draw rotated pointer according to finalvector.z
    memset(g_pointer_buf, 0, sizeof(g_pointer_buf));
    lv_canvas_rotate(g_pointer_canvas, &g_pointer_img,  z_rad * 180 / M_PI, 0, 0, 96 / 2, 96 / 2);
    lv_obj_align(g_pointer_canvas, g_pointer_fake_canvas, LV_ALIGN_IN_TOP_LEFT, -48, -48);
    
    return false;
}

static void handheld_changed_task(lv_task_t * t) {
    if (hidGetHandheldMode()) {
        g_gyro_indev->proc.disabled = true;
        g_keypad_indev->proc.disabled = false;

        if (g_clear_pointer_canvas) {
            g_clear_pointer_canvas = false;
            lv_obj_set_opa_scale(g_pointer_canvas, LV_OPA_TRANSP);
        }
    } else {
        g_gyro_indev->proc.disabled = false;
        g_keypad_indev->proc.disabled = true;

        if (!g_clear_pointer_canvas) {
            g_clear_pointer_canvas = true;
            lv_obj_set_opa_scale(g_pointer_canvas, LV_OPA_100);
        }
    }
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
    g_keypad_indev = lv_indev_drv_register(&keypad_drv);
    logPrintf("g_keypad_indev(%p)\n", g_keypad_indev);
    g_keypad_indev->proc.disabled = true;

    g_keypad_group = lv_group_create();
    lv_indev_set_group(g_keypad_indev, g_keypad_group);
    
    lv_indev_drv_t pointer_drv;
    lv_indev_drv_init(&pointer_drv);
    pointer_drv.type = LV_INDEV_TYPE_POINTER;
    pointer_drv.read_cb = gyro_cb;
    g_gyro_indev = lv_indev_drv_register(&pointer_drv);
    logPrintf("g_gyro_indev(%p)\n", g_gyro_indev);

    g_pointer_canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_canvas_set_buffer(g_pointer_canvas, g_pointer_buf, 96,96, LV_IMG_CF_TRUE_COLOR_ALPHA);
    g_pointer_fake_canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_indev_set_cursor(g_gyro_indev, g_pointer_fake_canvas); // Set cursor to fake canvas for centering click and rotating cursor
    lv_obj_set_parent(g_pointer_canvas, lv_layer_sys()); // Set the real cursor to the system layer where the cursor should be drawn
    lv_obj_set_opa_scale_enable(g_pointer_canvas, true);

    if (hidGetHandheldMode()) {
        g_gyro_indev->proc.disabled = true;
        g_keypad_indev->proc.disabled = false;

        g_clear_pointer_canvas = false;
        lv_obj_set_opa_scale(g_pointer_canvas, LV_OPA_TRANSP);
    }

    lv_task_t * handheld_check = lv_task_create(handheld_changed_task, 500, LV_TASK_PRIO_MID, NULL);
    lv_task_ready(handheld_check);
    
    // Cursor asset
    u8 *data;
    size_t size;
    assetsGetData(AssetId_cursor, &data, &size);
    g_pointer_img = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 96,
        .header.h = 96,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };
    
    // Get handles for sixaxis
    hidGetSixAxisSensorHandles(&g_sixaxis_handles[0], 2, CONTROLLER_PLAYER_1, TYPE_JOYCON_PAIR);
    hidGetSixAxisSensorHandles(&g_sixaxis_handles[2], 1, CONTROLLER_PLAYER_1, TYPE_PROCONTROLLER);
    hidGetSixAxisSensorHandles(&g_sixaxis_handles[3], 1, CONTROLLER_HANDHELD, TYPE_HANDHELD);
    hidStartSixAxisSensor(g_sixaxis_handles[0]);
    hidStartSixAxisSensor(g_sixaxis_handles[1]);
    hidStartSixAxisSensor(g_sixaxis_handles[2]);
    hidStartSixAxisSensor(g_sixaxis_handles[3]);
    
    // Ideally scan for input here and set the zero
    g_gyro_center.x = 0;
    g_gyro_center.y = 0;
    g_gyro_center.z = 1;
}

void driversExit() {
    lv_group_del(g_keypad_group);

    framebufferClose(&g_framebuffer);
    
    hidStopSixAxisSensor(g_sixaxis_handles[0]);
    hidStopSixAxisSensor(g_sixaxis_handles[1]);
    hidStopSixAxisSensor(g_sixaxis_handles[2]);
    hidStopSixAxisSensor(g_sixaxis_handles[3]);
}

lv_group_t *keypad_group() {
    return g_keypad_group;
}