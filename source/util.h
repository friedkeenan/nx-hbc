#pragma once

#include <sys/types.h>

bool is_dir(char *path);
bool is_file(char *path);

char *get_ext(char *str);
char *get_name(char *path);

int mkdirs(char *path, mode_t mode);