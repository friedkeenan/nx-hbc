#include <lvgl/lvgl.h>
#include <switch.h>
#include <math.h>

#include "log.h"
#include "assets.h"

static Framebuffer g_framebuffer;
static lv_disp_buf_t g_disp_buf;
static lv_color_t g_buffer[LV_HOR_RES_MAX * LV_VER_RES_MAX];

static touchPosition g_touch_pos;

static lv_group_t *g_keypad_group;
// sixaxis handles
static u32 handles[4];
// we actually only need to keep track of a specific set of data so we dont need all sixaxis values
static HidVector g_gyro_center;
static lv_obj_t * g_pointer_canvas;
static lv_img_dsc_t g_pointer_img;
static lv_color_t g_pointer_buff[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(96, 96)];
static lv_obj_t * g_pointer_fake_canvas;
static float g_pointer_screen_magic = 0.7071f; // this is a repeating number that describes the top right of a square inside a unit circle whose sides are parallel to the x-y axis'
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

void centerGyro(SixAxisSensorValues sixaxis){
    // these track center, unknown is actually a float value that can keep track of full rotations, 1 unit = 1 full rotation in that direction
    g_gyro_center.x = sixaxis.unk.x; //rotation along y (which trannslates to left right)
    g_gyro_center.y = sixaxis.unk.z; //rotation along x (^^^^^^^^^^^^^^^^^^^^ up down) [Note: this is switched with Z in handheld mode]
    g_gyro_center.z = sixaxis.unk.y; //rotation on forward axis
    logPrintf("gyro centered at x: % .4f, y: % .4f \n", g_gyro_center.x, g_gyro_center.y);
}

static bool gyro_cb(lv_indev_drv_t *drv, lv_indev_data_t *data){
    if(hidGetHandheldMode()){
        data->point.x = -1; 
        data->point.y = -1; 
        data->state = LV_INDEV_STATE_REL;
        return false; 
    }
    // scan for input changes 
    SixAxisSensorValues sixaxis;
    hidSixAxisSensorValuesRead(&sixaxis, CONTROLLER_P1_AUTO, 1);
    // center gyro
    u64 pressed = hidKeysHeld(CONTROLLER_P1_AUTO);
    if (pressed & KEY_X)
        centerGyro(sixaxis);
    if (pressed & KEY_A)
    {
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    // center input according to g_gyro_center
    HidVector finalvector;
    finalvector.x = sixaxis.unk.x - g_gyro_center.x;
    finalvector.y = sixaxis.unk.z - g_gyro_center.y; 
    finalvector.z = sixaxis.unk.y - g_gyro_center.z;
    float XAngle = 360 * finalvector.x;
    float YAngle = 360 * finalvector.y;
    float ZAngle = 360 * finalvector.z;
    float XRadian = XAngle * M_PI / 180;
    float YRadian = YAngle * M_PI / 180;
    float ZRadian = ZAngle * M_PI / 180;
    // rotate 3d point at (0,0,1) along x y and z and then put it into (x,y) coordinates this return a point inside a circile of radius 1
    finalvector.x = sin(ZRadian) * sin(XRadian) - cos(ZRadian) * sin(YRadian) * cos(XRadian); 
    finalvector.y = cos(ZRadian) * sin(XRadian) + sin(ZRadian) * sin(YRadian) * cos(XRadian);
    // x and y need to be clamped to our boundries should the absolute value be above it
    if(fabs(finalvector.x) > g_pointer_screen_magic)
    {
        // the value can be negative or positive
        if(finalvector.x > 0.0f){
            finalvector.x = g_pointer_screen_magic;
        }
        else{
            finalvector.x = -g_pointer_screen_magic;
        }
    }
    if(fabs(finalvector.y) > g_pointer_screen_magic)
    {
        if(finalvector.y > 0.0f){
            finalvector.y = g_pointer_screen_magic;
        }
        else{
            finalvector.y = -g_pointer_screen_magic;
        }
    }
    // now we need to inevert y and convert them back to a 0 1 range
    finalvector.x = (finalvector.x + g_pointer_screen_magic) / 2.0f / g_pointer_screen_magic;
    finalvector.y = (-finalvector.y + g_pointer_screen_magic) / 2.0f  / g_pointer_screen_magic; 
    data->point.x = ((float) 1280 * finalvector.x); 
    data->point.y = ((float) 720 * finalvector.y); 
    // clear canvas and draw rotated pointer according to finalvector.z
    memset(g_pointer_buff, 0, sizeof(g_pointer_buff));
    lv_canvas_rotate(g_pointer_canvas, &g_pointer_img,  ZAngle, 0, 0, 96 / 2, 96 / 2);
    lv_obj_align(g_pointer_canvas, g_pointer_fake_canvas, LV_ALIGN_IN_TOP_LEFT, -48, -48);
    
    return false;
    
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
    
    lv_indev_drv_t pointer_drv;
    lv_indev_drv_init(&pointer_drv);
    pointer_drv.type = LV_INDEV_TYPE_POINTER;
    pointer_drv.read_cb = gyro_cb;
    lv_indev_t *gyro_indev = lv_indev_drv_register(&pointer_drv);
    g_pointer_canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_canvas_set_buffer(g_pointer_canvas, g_pointer_buff, 96,96, LV_IMG_CF_TRUE_COLOR_ALPHA);
    g_pointer_fake_canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_indev_set_cursor(gyro_indev, g_pointer_fake_canvas); // set cursor to fake canvas for centering click and rotating cursor
    lv_obj_set_parent(g_pointer_canvas, lv_layer_sys()); // set the real cursor to the system layer where the cursor should be drawn
    logPrintf("gyro_indev(%p)\n", gyro_indev);
    // cursor asset
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
    
    
    // get handles for sixaxis
    hidGetSixAxisSensorHandles(&handles[0], 2, CONTROLLER_PLAYER_1, TYPE_JOYCON_PAIR);
    hidGetSixAxisSensorHandles(&handles[2], 1, CONTROLLER_PLAYER_1, TYPE_PROCONTROLLER);
    hidGetSixAxisSensorHandles(&handles[3], 1, CONTROLLER_HANDHELD, TYPE_HANDHELD);
    hidStartSixAxisSensor(handles[0]);
    hidStartSixAxisSensor(handles[1]);
    hidStartSixAxisSensor(handles[2]);
    hidStartSixAxisSensor(handles[3]);
    
    // ideally scan for input here and set the zero
    g_gyro_center.x = 0; 
    g_gyro_center.y = 0;
    g_gyro_center.z = 1; 
}

void driversExit() {
    lv_group_del(g_keypad_group);

    framebufferClose(&g_framebuffer);
    
    hidStopSixAxisSensor(handles[0]);
    hidStopSixAxisSensor(handles[1]);
    hidStopSixAxisSensor(handles[2]);
    hidStopSixAxisSensor(handles[3]);
}

lv_group_t *keypad_group() {
    return g_keypad_group;
}