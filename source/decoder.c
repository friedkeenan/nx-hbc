#include <lvgl/lvgl.h>
#include <turbojpeg.h>
#include <switch.h>

#include "decoder.h"

static lv_img_decoder_t *g_jpg_dec;

static int pos_from_coord(int x, int y, int w) {
    return (y * w + x) * sizeof(lv_color_t);
}

static void downscale_img(u8 *src, u8 *dst, u32 src_w, u32 src_h, u32 dst_w, u32 dst_h) {
    if (src_w == dst_w && src_h == dst_h) {
        memcpy(dst, src, src_w * src_h * sizeof(lv_color_t));
        return;
    }

    float x_scale = (float) src_w / (float) dst_w;
    float y_scale = (float) src_h / (float) dst_h;

    u8 b[4];
    u8 g[4];
    u8 r[4];
    u8 a[4];
    float f[4];
    int w[4];

    for (int x = 0; x < dst_w; x++) {
        for (int y = 0; y < dst_h; y++) {
            float src_x = x * x_scale;
            float src_y = y * y_scale;
            int pixel_x = src_x;
            int pixel_y = src_y;

            int pos = pos_from_coord(pixel_x, pixel_y, src_w);
            b[0] = src[pos];
            g[0] = src[pos + 1];
            r[0] = src[pos + 2];
            a[0] = src[pos + 3];

            pos = pos_from_coord(pixel_x + 1, pixel_y, src_w);
            b[1] = src[pos];
            g[1] = src[pos + 1];
            r[1] = src[pos + 2];
            a[1] = src[pos + 3];

            pos = pos_from_coord(pixel_x, pixel_y + 1, src_w);
            b[2] = src[pos];
            g[2] = src[pos + 1];
            r[2] = src[pos + 2];
            a[2] = src[pos + 3];

            pos = pos_from_coord(pixel_x + 1, pixel_y + 1, src_w);
            b[3] = src[pos];
            g[3] = src[pos + 1];
            r[3] = src[pos + 2];
            a[3] = src[pos + 3];

            f[0] = src_x - pixel_x;
            f[1] = src_y - pixel_y;
            f[2] = 1.0f - f[0];
            f[3] = 1.0f - f[1];

            w[0] = f[2] * f[3] * 256.0;
            w[1] = f[0] * f[3] * 256.0;
            w[2] = f[2] * f[1] * 256.0;
            w[3] = f[0] * f[1] * 256.0;

            pos = pos_from_coord(x, y, dst_w);
            dst[pos] = (b[0] * w[0] + b[1] * w[1] + b[2] * w[2] + b[3] * w[3]) >> 8;
            dst[pos + 1] = (g[0] * w[0] + g[1] * w[1] + g[2] * w[2] + g[3] * w[3]) >> 8;
            dst[pos + 2] = (r[0] * w[0] + r[1] * w[1] + r[2] * w[2] + r[3] * w[3]) >> 8;
            dst[pos + 3] = (a[0] * w[0] + a[1] * w[1] + a[2] * w[2] + a[3] * w[3]) >> 8;
        }
    }
}

static lv_res_t jpg_dec_info(lv_img_decoder_t *dec, const void *src, lv_img_header_t *header) {
    // Let's not deal with it if it's not an image descriptor
    if (lv_img_src_get_type(src) != LV_IMG_SRC_VARIABLE)
        return LV_RES_INV;

    const lv_img_dsc_t *dsc = src;

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

    if (img_dsc->header.w > w || img_dsc->header.h > h) {
        tjDestroy(decomp);
        return LV_RES_INV;
    }

    u8 *img_data = lv_mem_alloc(w * h * sizeof(lv_color_t));

    if (tjDecompress2(decomp, img_dsc->data, img_dsc->data_size, img_data, w, 0, h, TJPF_BGRA, TJFLAG_ACCURATEDCT)) {
        tjFree(img_data);
        tjDestroy(decomp);
        return LV_RES_INV;
    }

    u8 *resized_data = lv_mem_alloc(img_dsc->header.w * img_dsc->header.h * sizeof(lv_color_t));
    downscale_img(img_data, resized_data, w, h, img_dsc->header.w, img_dsc->header.h);

    dsc->img_data = resized_data;

    lv_mem_free(img_data);
    tjDestroy(decomp);
    return LV_RES_OK;
}

static void jpg_dec_close(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc) {
    lv_mem_free(dsc->img_data);
}


void decoderInitialize() {
    g_jpg_dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(g_jpg_dec, jpg_dec_info);
    lv_img_decoder_set_open_cb(g_jpg_dec, jpg_dec_open);
    lv_img_decoder_set_close_cb(g_jpg_dec, jpg_dec_close);
}