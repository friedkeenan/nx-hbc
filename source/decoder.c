#include <lvgl/lvgl.h>
#include <turbojpeg.h>
#include <switch.h>

#include "decoder.h"

static lv_img_decoder_t *g_jpg_dec;

static lv_res_t jpg_dec_info(lv_img_decoder_t *dec, const void *src, lv_img_header_t *header) {
    // Let's not deal with it if it's not an image descriptor
    if (lv_img_src_get_type(src) != LV_IMG_SRC_VARIABLE)
        return LV_RES_INV;

    const lv_img_dsc_t *dsc = src;

    /*tjhandle decomp = tjInitDecompress();
    if (decomp == NULL)
        return LV_RES_INV;

    int w, h,samp, color_space;

    if (tjDecompressHeader3(decomp, dsc->data, dsc->data_size, &w, &h, &samp, &color_space))
        return LV_RES_INV;*/

    header->always_zero = 0;
    header->w = dsc->header.w;
    header->h = dsc->header.h;
    header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;

    return LV_RES_OK;
}

static lv_res_t jpg_dec_open(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc) {
    // Let's not deal with it if it's not an image descriptor
    if (dsc->src_type != LV_IMG_SRC_VARIABLE)
        return LV_RES_INV;

    const lv_img_dsc_t *img_dsc = dsc->src;

    tjhandle decomp = tjInitDecompress();
    if (decomp == NULL)
        return LV_RES_INV;

    int w, h, samp, color_space;

    if (tjDecompressHeader3(decomp, img_dsc->data, img_dsc->data_size, &w, &h, &samp, &color_space)) {
        tjDestroy(decomp);
        return LV_RES_INV;
    }

    if (w != img_dsc->header.w || h != img_dsc->header.h) {
        tjDestroy(decomp);
        return LV_RES_INV;
    }

    u8 *img_data = tjAlloc(w * h * sizeof(lv_color_t));

    if (tjDecompress2(decomp, img_dsc->data, img_dsc->data_size, img_data, w, 0, h, TJPF_RGBA, TJFLAG_ACCURATEDCT)) {
        tjFree(img_data);
        tjDestroy(decomp);
        return LV_RES_INV;
    }

    dsc->img_data = img_data;

    tjDestroy(decomp);
    return LV_RES_OK;
}

static void jpg_dec_close(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc) {
    tjFree((unsigned char *) dsc->img_data);
}

void decoderInitialize() {
    g_jpg_dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(g_jpg_dec, jpg_dec_info);
    lv_img_decoder_set_open_cb(g_jpg_dec, jpg_dec_open);
    lv_img_decoder_set_close_cb(g_jpg_dec, jpg_dec_close);
}