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

typedef struct {
    char *name;
    int is_dir;
} DirEntry;

int compareDirEntries(const void *a, const void *b) {
    DirEntry *entryA = (DirEntry *)a;
    DirEntry *entryB = (DirEntry *)b;

    if (strcmp(entryA->name, "..") == 0) return -1;
    if (strcmp(entryB->name, "..") == 0) return 1;

    if (strcmp(entryA->name, ".") == 0) return -1;
    if (strcmp(entryB->name, ".") == 0) return 1;

    if (entryA->is_dir && !entryB->is_dir) return -1;
    if (!entryA->is_dir && entryB->is_dir) return 1;

    return strcasecmp(entryA->name, entryB->name);
}

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
    char *new_filename = strdup(filename);
    editorClearBuffer();
    free(E.filename);
    E.filename = new_filename;
    E.cy = 0;
    E.cx = 0;
    
    struct stat statbuf;
    if (stat(new_filename, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        DIR *d = opendir(new_filename);
        if (!d) {
            editorSetStatusMessage("Error: Could not open directory");
            return;
        }
        
        DirEntry *entries = NULL;
        int count = 0;
        int capacity = 0;
        
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (!E.explorer_show_hidden && dir->d_name[0] == '.' && 
                strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                continue;
            }

            if (count >= capacity) {
                capacity = capacity == 0 ? 10 : capacity * 2;
                entries = realloc(entries, capacity * sizeof(DirEntry));
            }
            
            int is_dir = (dir->d_type == DT_DIR);
            if (dir->d_type == DT_UNKNOWN) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", new_filename, dir->d_name);
                struct stat entry_stat;
                if (stat(full_path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
                    is_dir = 1;
                }
            }
            
            entries[count].name = strdup(dir->d_name);
            entries[count].is_dir = is_dir;
            count++;
        }
        closedir(d);
        
        qsort(entries, count, sizeof(DirEntry), compareDirEntries);
        
        char header[1024];
        editorInsertRow(E.numrows, "\" ============================================================================", 78);
        editorInsertRow(E.numrows, "\" Netrw Directory Listing", 25);
        snprintf(header, sizeof(header), "\"   %s", new_filename);
        editorInsertRow(E.numrows, header, strlen(header));
        editorInsertRow(E.numrows, "\" ============================================================================", 78);

        for (int i = 0; i < count; i++) {
            char line[512];
            if (entries[i].is_dir) {
                snprintf(line, sizeof(line), "%s/", entries[i].name);
            } else {
                snprintf(line, sizeof(line), "%s", entries[i].name);
            }
            editorInsertRow(E.numrows, line, strlen(line));
            free(entries[i].name);
        }
        free(entries);
        
        E.mode = MODE_EXPLORER;
        E.dirty = 0;
        E.cy = 4; 
        editorSetStatusMessage("Opened directory: %s (%d items)", new_filename, count);
        return;
    }

    E.mode = MODE_NORMAL;
    
    FILE *fp = fopen(new_filename, "r");
    if (!fp) {
        editorSetStatusMessage("Error: Could not open file %s", new_filename);
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
