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
//sixaxis handle
static u32 handles[4];
//we actually only need to keep track of a specific set of data so we dont need all sixaxis values
static HidVector Gyro_Center;
//boundries for translating gyro directly to screen space
//smaller values for smaller tvs/smaller motions
static float YBoundry = 0.12f;
static float XBoundry = 0.12f;

lv_obj_t * pointer_canvas;
lv_img_dsc_t pointer_img;
lv_color_t pointer_buff[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(96, 96)];
lv_obj_t * cursor_fake_canvas;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    u32 stride;
    lv_color_t *fb = (lv_color_t *) framebufferBegin(&g_framebuffer, &stride);

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            u32 pos = y * stride / sizeof(lv_color_t) + x;
            //logPrintf("Flush: pos(%#x)\n", pos);
            fb[pos] = *color_p;
            color_p++;
        }
    }

    framebufferEnd(&g_framebuffer);

    lv_disp_flush_ready(drv);
}

void CenterGyro(SixAxisSensorValues sixaxis){
    //these track center, unknown is actually a float value that can keep track of full rotations, 1 unit = 1 full rotation in that direction
    Gyro_Center.x = sixaxis.unk.z; //rotation along y (which trannslates to left right)
    Gyro_Center.y = sixaxis.unk.x; //rotation along x (^^^^)
    Gyro_Center.z = sixaxis.unk.y; //rotation on forward axis
    logPrintf("gyro centered at x: % .4f, y: % .4f \n", Gyro_Center.x, Gyro_Center.y);
}

static bool gyro_cb(lv_indev_drv_t *drv, lv_indev_data_t *data){
    //scan for input changes 
    SixAxisSensorValues sixaxis;
    hidSixAxisSensorValuesRead(&sixaxis, CONTROLLER_P1_AUTO, 1);
    //center gyro
    u64 pressed = hidKeysDown(CONTROLLER_P1_AUTO) | hidKeysHeld(CONTROLLER_P1_AUTO);
    if (pressed & KEY_X)
        CenterGyro(sixaxis);
    //we need format data
    HidVector finalvector;
    // x and y need to be clamped to our boundries should the absolute value be above it
    finalvector.x = sixaxis.unk.z - Gyro_Center.x;
    finalvector.y = sixaxis.unk.x - Gyro_Center.y; 
    //this value can be left alone since its for pointer rotation
    finalvector.z = sixaxis.unk.y - Gyro_Center.z;
    
    //todo normalize finalvector so its 1 to 0 to fix overturn (full rotation)
    
    if(fabs(finalvector.x) > XBoundry)
    {
        //the value can be negative or positive
        if(finalvector.x > 0.0f){
            finalvector.x = XBoundry;
        }
        else{
            finalvector.x = -XBoundry;
        }
    }
    if(fabs(finalvector.y) > YBoundry)
    {
        if(finalvector.y > 0.0f){
            finalvector.y = YBoundry;
        }
        else{
            finalvector.y = -YBoundry;
        }
    }
    //now we need to inevert and convert them back to a 0 1 range
    finalvector.x = (-finalvector.x + XBoundry) / 2.0f / XBoundry;
    finalvector.y = (-finalvector.y + YBoundry) / 2.0f / YBoundry; 
    //convert input to screen space
    data->point.x = ((float) 1280 * finalvector.x); 
    data->point.y = ((float) 720 * finalvector.y); 
    //clear canvas and draw rotated pointer according to finalcetor.z
    memset(pointer_buff, 0, sizeof(pointer_buff));
    lv_canvas_rotate(pointer_canvas, &pointer_img, 360*finalvector.z, 0, 0, 96 / 2, 96 / 2);
    lv_obj_align(pointer_canvas, cursor_fake_canvas, LV_ALIGN_IN_TOP_LEFT, -48, -48);
    if (pressed & KEY_A)
    {
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
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
    u64 pressed = hidKeysDown(CONTROLLER_P1_AUTO) | hidKeysHeld(CONTROLLER_P1_AUTO);

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
    pointer_canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_canvas_set_buffer(pointer_canvas, pointer_buff, 96,96, LV_IMG_CF_TRUE_COLOR_ALPHA);
    cursor_fake_canvas = lv_canvas_create(lv_scr_act(), NULL);
    u8 *data;
    size_t size;
    assetsGetData(AssetId_cursor_pic, &data, &size); //get asset stuff
    pointer_img = (lv_img_dsc_t) {
        .header.always_zero = 0,
        .header.w = 96,
        .header.h = 96,
        .data_size = size,
        .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .data = data,
    };
    lv_indev_set_cursor(gyro_indev, cursor_fake_canvas); //set cursor
    lv_obj_align(pointer_canvas, cursor_fake_canvas, LV_ALIGN_IN_TOP_LEFT, -48, -48);
    lv_obj_set_parent(pointer_canvas, lv_layer_sys());
    logPrintf("gyro_indev(%p)\n", gyro_indev);

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
    
    
    //get handles for sixaxis
    hidGetSixAxisSensorHandles(&handles[0], 2, CONTROLLER_PLAYER_1, TYPE_JOYCON_PAIR);
    hidGetSixAxisSensorHandles(&handles[2], 1, CONTROLLER_PLAYER_1, TYPE_PROCONTROLLER);
    hidGetSixAxisSensorHandles(&handles[3], 1, CONTROLLER_HANDHELD, TYPE_HANDHELD);
    hidStartSixAxisSensor(handles[0]);
    hidStartSixAxisSensor(handles[1]);
    hidStartSixAxisSensor(handles[2]);
    hidStartSixAxisSensor(handles[3]);
    //ideally scan for input here and set the zero
    
    Gyro_Center.x = 0; //left right motions
    Gyro_Center.y = 0; //up down motions
    Gyro_Center.z = 1; //rotation along forward axis (pointer rotation)
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