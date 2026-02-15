#ifndef FIND_IMGS_H
#define FIND_IMGS_H
// vibecoded
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>

static int has_image_extension(const char* name) {
    const char* dot = strrchr(name, '.');
    if (!dot) return 0;

    dot++;

    char ext[16];
    size_t len = strlen(dot);
    if (len >= sizeof(ext)) return 0;

    for (size_t i = 0; i < len; i++)
        ext[i] = (char)tolower((unsigned char)dot[i]);
    ext[len] = '\0';

    const char* allowed[] = {
        "png", "jpg", "jpeg", "bmp",
        "gif", "tiff", "webp"
    };

    for (size_t i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++) {
        if (strcmp(ext, allowed[i]) == 0)
            return 1;
    }

    return 0;
}

static void add_file(char*** files, size_t* count,
                     size_t* capacity, const char* path) {

    if (*count >= *capacity) {
        *capacity = *capacity ? *capacity * 2 : 16;
        char** tmp = (char**)realloc(*files, *capacity * sizeof(char*));
        if (!tmp) return;
        *files = tmp;
    }

    (*files)[*count] = strdup(path);
    if ((*files)[*count])
        (*count)++;
}

static void walk_directory(const char* dir_path,
                           char*** files,
                           size_t* count,
                           size_t* capacity) {

    DIR* dir = opendir(dir_path);
    if (!dir) return;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path),
                 "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode)) {
            walk_directory(full_path, files, count, capacity);
        }
        else if (S_ISREG(st.st_mode)) {
            if (has_image_extension(entry->d_name)) {
                add_file(files, count, capacity, full_path);
            }
        }
    }

    closedir(dir);
}

static char* expand_home(const char* path) {
    if (path[0] != '~')
        return strdup(path);

    const char* home = getenv("HOME");
    if (!home) return NULL;

    char full_path[PATH_MAX];

    if (path[1] == '/' || path[1] == '\0') {
        snprintf(full_path, sizeof(full_path),
                 "%s%s", home, path + 1);
        return strdup(full_path);
    }

    // We are not handling ~username (rare case)
    return NULL;
}

static inline char** get_all_image_files_recursive(const char* path,
                                     size_t* count) {

    *count = 0;
    size_t capacity = 0;
    char** files = NULL;

    char* expanded = expand_home(path);
    if (!expanded) return NULL;

    walk_directory(expanded, &files, count, &capacity);

    free(expanded);
    return files;
}

static inline void free_file_list(char** files, size_t count) {
    for (size_t i = 0; i < count; i++)
        free(files[i]);
    free(files);
}

#endif // FIND_IMGS_H
