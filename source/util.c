#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lvgl/lvgl.h>
#include <switch.h>

#include "util.h"
#include "log.h"

bool is_dir(char *path) {
    struct stat s;
    return (stat(path, &s) == 0) && (s.st_mode & S_IFDIR);
}

bool is_file(char *path) {
    struct stat s;
    return (stat(path, &s) == 0) && (s.st_mode & S_IFREG);
}

char *get_ext(char *str) {
    char *p;
    for (p = str + strlen(str); p > str && *p != '.' && *p != '/'; p--);
    return p + 1;
}

char *get_name(char *path) {
    char *p;
    for (p = path + strlen(path); p > path && *p != '/'; p--);
    return p + 1;
}

int mkdirs(char *path, mode_t mode) {
    char tmp_dir[PATH_MAX + 1];
    tmp_dir[0] = '\0';

    char path_dup[PATH_MAX + 1];
    strncpy(path_dup, path, PATH_MAX);

    for (char *tmp_str = strtok(path_dup, "/"); tmp_str != NULL; tmp_str = strtok(NULL, "/")) {
        strcat(tmp_dir, tmp_str);
        strcat(tmp_dir, "/");
        
        int res = mkdir(tmp_dir, mode);
        if (res != 0 && errno != EEXIST)
            return res;
    }

    return 0;
}

lv_res_t copy(char *dest, char *from) {
    FILE *frm = fopen(from, "rb");
    if (frm == NULL)
        return LV_RES_INV;

    FILE *dst = fopen(dest, "wb");
    if (dst == NULL) {
        fclose(frm);
        return LV_RES_INV;
    }

    u8 buf[0x1000];
    size_t read_len;

    while ((read_len = fread(buf, 1, sizeof(buf), frm)) > 0) {
        do {
            read_len -= fwrite(buf, 1, read_len, dst);
            if (ferror(dst)) {
                fclose(dst);
                fclose(frm);
                return LV_RES_INV;
            }
        } while(read_len);
    }

    fclose(dst);
    fclose(frm);

    return LV_RES_OK;
}