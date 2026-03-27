#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "editor.h"

void editorSelectEntry(void) {
    if (E.numrows == 0) return;
    
    struct stat statbuf;
    if (stat(E.filename, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
        return;
    }

    if (E.cy < 2 || E.cy >= E.numrows) return;

    char *row_text = E.row[E.cy].chars;
    while (*row_text == ' ') row_text++;

    char *name = strdup(row_text);
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '/') {
        name[len - 1] = '\0';
    }

    char next_path[1024];
    if (strcmp(name, "..") == 0) {
        char *last_slash = strrchr(E.filename, '/');
        if (last_slash) {
            if (last_slash == E.filename) {
                strcpy(next_path, "/");
            } else {
                size_t path_len = last_slash - E.filename;
                strncpy(next_path, E.filename, path_len);
                next_path[path_len] = '\0';
            }
        } else if (strcmp(E.filename, ".") != 0) {
            strcpy(next_path, "..");
        } else {
            strcpy(next_path, "..");
        }
    } else {
        snprintf(next_path, sizeof(next_path), "%s/%s", E.filename, name);
    }

    free(name);
    editorOpen(next_path);
}
