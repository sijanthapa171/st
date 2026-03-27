#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"
#include "editor.h"
#include "core.h"

char *editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++) {
        totlen += E.row[j].size + 1;
    }
    *buflen = totlen;
    
    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorOpen(char *filename) {
    editorClearBuffer();
    free(E.filename);
    E.filename = strdup(filename);
    E.cy = 0;
    E.cx = 0;
    
    struct stat statbuf;
    if (stat(filename, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        DIR *d = opendir(filename);
        if (!d) {
            editorSetStatusMessage("Error: Could not open directory");
            return;
        }
        
        char header[256];
        snprintf(header, sizeof(header), "Directory: %s", filename);
        editorInsertRow(E.numrows, header, strlen(header));
        editorInsertRow(E.numrows, "----------", 10);
        
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0) continue;
            
            char entry[256];
            int is_dir = (dir->d_type == DT_DIR);
            
            // Handle filesystems where d_type is not supported
            if (dir->d_type == DT_UNKNOWN) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", filename, dir->d_name);
                struct stat entry_stat;
                if (stat(full_path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
                    is_dir = 1;
                }
            }
            
            if (is_dir) {
                snprintf(entry, sizeof(entry), "  %s/", dir->d_name);
            } else {
                snprintf(entry, sizeof(entry), "  %s", dir->d_name);
            }
            editorInsertRow(E.numrows, entry, strlen(entry));
        }
        closedir(d);
        E.dirty = 0;
        editorSetStatusMessage("Opened directory: %s (%d items)", filename, E.numrows - 2);
        return;
    }
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        editorSetStatusMessage("Error: Could not open file %s", filename);
        return;
    }
    
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

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

void editorSave(void) {
    if (E.filename == NULL) return;
    
    int len;
    char *buf = editorRowsToString(&len);
    
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    free(buf);
    editorSetStatusMessage("Can't save! I/O error");
}
