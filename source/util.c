#include <sys/stat.h>
#include <string.h>
#include <switch.h>

#include "util.h"

bool is_dir(char *path) {
    struct stat s;
    return (stat(path, &s) == 0) && (s.st_mode & S_IFDIR);
}

char *get_ext(char *str) {
    char *p;
    for (p = str + strlen(str); p > str && *p != '.' && *p != '/'; p--);
    return ++p;
}