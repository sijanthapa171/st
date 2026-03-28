#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.h"
#include "editor.h"

#include "commands.h"
#include "ui.h"

char *explorerGetCurrentName(void) {
    if (E.numrows == 0 || E.cy < 4 || E.cy >= E.numrows) return NULL;
    
    char *row_text = E.row[E.cy].chars;
    while (*row_text == ' ') row_text++;

    if (strcmp(row_text, "../") == 0 || strcmp(row_text, "..") == 0) return strdup("..");
    if (strcmp(row_text, "./") == 0 || strcmp(row_text, ".") == 0) return strdup(".");

    char *name = strdup(row_text);
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '/') {
        name[len - 1] = '\0';
    }
    return name;
}

void editorSelectEntry(void) {
    char *name = explorerGetCurrentName();
    if (!name) return;

    char next_path[8192];
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
        } else {
            strcpy(next_path, ".");
        }
    } else if (strcmp(name, ".") == 0) {
        strcpy(next_path, E.filename);
    } else {
        snprintf(next_path, sizeof(next_path), "%s/%s", E.filename, name);
    }

    free(name);
    editorOpen(next_path);
}

void explorerRefresh(void) {
    editorOpen(E.filename);
}

void explorerToggleHidden(void) {
    E.explorer_show_hidden = !E.explorer_show_hidden;
    explorerRefresh();
}

void explorerCreateFile(void) {
    char *name = editorPrompt("Create file: %s");
    if (name == NULL) return;

    char path[8192];
    snprintf(path, sizeof(path), "%s/%s", E.filename, name);
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        close(fd);
        explorerRefresh();
    } else {
        editorSetStatusMessage("Error: Could not create file %s", name);
    }
    free(name);
}

void explorerCreateFolder(void) {
    char *name = editorPrompt("Create folder: %s");
    if (name == NULL) return;

    char path[8192];
    snprintf(path, sizeof(path), "%s/%s", E.filename, name);
    if (mkdir(path, 0755) == 0) {
        explorerRefresh();
    } else {
        editorSetStatusMessage("Error: Could not create folder %s", name);
    }
    free(name);
}

void explorerRename(void) {
    char *old_name = explorerGetCurrentName();
    if (!old_name || strcmp(old_name, "..") == 0 || strcmp(old_name, ".") == 0) {
        free(old_name);
        return;
    }

    char prompt[128];
    snprintf(prompt, sizeof(prompt), "Rename %s to: %%s", old_name);
    char *new_name = editorPrompt(prompt);
    if (new_name == NULL) {
        free(old_name);
        return;
    }

    char old_path[8192], new_path[8192];
    snprintf(old_path, sizeof(old_path), "%s/%s", E.filename, old_name);
    snprintf(new_path, sizeof(new_path), "%s/%s", E.filename, new_name);

    if (rename(old_path, new_path) == 0) {
        explorerRefresh();
    } else {
        editorSetStatusMessage("Error: Could not rename %s", old_name);
    }

    free(old_name);
    free(new_name);
}

void explorerDelete(void) {
    char *name = explorerGetCurrentName();
    if (!name || strcmp(name, "..") == 0 || strcmp(name, ".") == 0) {
        free(name);
        return;
    }

    char prompt[128];
    snprintf(prompt, sizeof(prompt), "Delete %s? (y/n): %%s", name);
    char *confirm = editorPrompt(prompt);
    if (confirm && (confirm[0] == 'y' || confirm[0] == 'Y')) {
        char path[8192];
        snprintf(path, sizeof(path), "%s/%s", E.filename, name);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char cmd[16384];
                snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
                if (system(cmd) == 0) explorerRefresh();
                else editorSetStatusMessage("Error: Failed to delete folder %s", name);
            } else {
                if (unlink(path) == 0) explorerRefresh();
                else editorSetStatusMessage("Error: Failed to delete file %s", name);
            }
        }
    }
    free(name);
    free(confirm);
}

void explorerCopy(void) {
    char *name = explorerGetCurrentName();
    if (!name || strcmp(name, "..") == 0 || strcmp(name, ".") == 0) {
        free(name);
        return;
    }

    snprintf(E.explorer_clip_path, sizeof(E.explorer_clip_path), "%s/%s", E.filename, name);
    E.explorer_clip_is_cut = 0;
    editorSetStatusMessage("Copied: %s", name);
    free(name);
}

void explorerCut(void) {
    char *name = explorerGetCurrentName();
    if (!name || strcmp(name, "..") == 0 || strcmp(name, ".") == 0) {
        free(name);
        return;
    }

    snprintf(E.explorer_clip_path, sizeof(E.explorer_clip_path), "%s/%s", E.filename, name);
    E.explorer_clip_is_cut = 1;
    editorSetStatusMessage("Cut: %s", name);
    free(name);
}

void explorerPaste(void) {
    if (E.explorer_clip_path[0] == '\0') return;

    char *src = E.explorer_clip_path;
    char *name = strrchr(src, '/');
    if (!name) name = src;
    else name++;

    char dest[8192];
    snprintf(dest, sizeof(dest), "%s/%s", E.filename, name);

    if (E.explorer_clip_is_cut) {
        if (rename(src, dest) == 0) {
            E.explorer_clip_path[0] = '\0';
            explorerRefresh();
        } else {
            editorSetStatusMessage("Error: Could not move %s", name);
        }
    } else {
        char cmd[16384];
        snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", src, dest);
        if (system(cmd) == 0) {
            explorerRefresh();
        } else {
            editorSetStatusMessage("Error: Could not copy %s", name);
        }
    }
}
