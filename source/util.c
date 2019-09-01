#include <sys/stat.h>
#include <string.h>
#include <switch.h>

#include "util.h"

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