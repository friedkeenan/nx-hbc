#ifdef MUSIC

#include <malloc.h>
#include <threads.h>
#include <mpg123.h>
#include <switch.h>

#include "music.h"
#include "theme.h"
#include "log.h"

static bool g_should_loop_music = true;
static mtx_t g_music_mtx;

void stop_music_loop() {
    mtx_lock(&g_music_mtx);
    g_should_loop_music = false;
    mtx_unlock(&g_music_mtx);
}

static bool should_loop() {
    bool loop;

    mtx_lock(&g_music_mtx);
    loop = g_should_loop_music;
    mtx_unlock(&g_music_mtx);

    return loop;
}

static int decope_mp3(mpg123_handle *mh, void *in_buffer, size_t in_size, AudioOutBuffer *out_buffer) {
    int err = mpg123_open_feed(mh);
    if (err != MPG123_OK)
        return err;

    err = mpg123_feed(mh, in_buffer, in_size);
    if (err != MPG123_OK) {
        mpg123_close(mh);
        return err;
    }

    long rate;
    int channels, encoding;
    err = mpg123_getformat(mh, &rate, &channels, &encoding);
    if (err != MPG123_OK) {
        mpg123_close(mh);
        return err;
    }

    err = mpg123_format_none(mh);
    if (err != MPG123_OK) {
        mpg123_close(mh);
        return err;
    }

    err = mpg123_format(mh, rate, channels, encoding);
    if (err != MPG123_OK) {
        mpg123_close(mh);
        return err;
    }

    off_t length_samp = mpg123_length(mh);
    if (length_samp == MPG123_ERR) {
        mpg123_close(mh);
        return length_samp;
    }

    size_t data_size = length_samp * audoutGetChannelCount() * sizeof(s16);
    size_t buffer_size = (data_size + 0xfff) & ~0xfff; // Align to 0x1000 bytes
    void *buffer = memalign(0x1000, buffer_size);

    size_t done;
    err = mpg123_read(mh, buffer, data_size, &done);
    if (err != MPG123_OK) {
        free(buffer);
        mpg123_close(mh);
        return err;
    }

    mpg123_close(mh);

    out_buffer->next = NULL;
    out_buffer->buffer = buffer;
    out_buffer->buffer_size = buffer_size;
    out_buffer->data_size = data_size;
    out_buffer->data_offset = 0;

    return MPG123_OK;
}

static int music_init(mpg123_handle **mh) {
    g_should_loop_music = true;

    mtx_init(&g_music_mtx, mtx_plain);

    if (R_FAILED(audoutInitialize())) {
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    if (R_FAILED(audoutStartAudioOut())) {
        audoutExit();
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    if (mpg123_init() != MPG123_OK) {
        audoutStopAudioOut();
        audoutExit();
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    int err;
    mpg123_handle *mh_tmp = mpg123_new(NULL, &err);
    if (mh_tmp == NULL) {
        mpg123_exit();
        audoutStopAudioOut();
        audoutExit();
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    if (mpg123_param(mh_tmp, MPG123_FORCE_RATE, audoutGetSampleRate(), 0) != MPG123_OK) {
        mpg123_delete(mh_tmp);
        mpg123_exit();
        audoutStopAudioOut();
        audoutExit();
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    if (mpg123_param(mh_tmp, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO, 0) != MPG123_OK) {
        mpg123_delete(mh_tmp);
        mpg123_exit();
        audoutStopAudioOut();
        audoutExit();
        mtx_destroy(&g_music_mtx);
        return -1;
    }

    *mh = mh_tmp;

    return 0;
}

static void music_exit(mpg123_handle *mh) {
    mpg123_delete(mh);

    mpg123_exit();
    audoutStopAudioOut();
    audoutExit();

    mtx_destroy(&g_music_mtx);
}

int music_thread(void *arg) {
    logPrintf("music_thread start\n");

    mpg123_handle *mh;
    if (music_init(&mh) != 0)
        return -1;

    AudioOutBuffer intro_buf;
    if (decope_mp3(mh, curr_theme()->intro_music->buffer, curr_theme()->intro_music->size, &intro_buf) != MPG123_OK) {
        music_exit(mh);
        return -1;
    }

    AudioOutBuffer loop_buf;
    if (decope_mp3(mh, curr_theme()->loop_music->buffer, curr_theme()->loop_music->size, &loop_buf) != MPG123_OK) {
        free(intro_buf.buffer);
        music_exit(mh);
        return -1;
    }

    audoutAppendAudioOutBuffer(&intro_buf);
    audoutAppendAudioOutBuffer(&loop_buf);

    while (should_loop()) {
        AudioOutBuffer *released;
        u32 released_count;
        
        if (R_SUCCEEDED(audoutWaitPlayFinish(&released, &released_count, 1e+6L))) {
            audoutAppendAudioOutBuffer(&loop_buf);
        }
    }

    free(intro_buf.buffer);
    free(loop_buf.buffer);

    music_exit(mh);
    logPrintf("exit\n");

    return 0;
}

#endif