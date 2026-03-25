#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "editor.h"
#include "commands.h"
#include "input.h"
#include "utils.h"
#include "ui.h"

char *editorPrompt(char *prompt) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    
    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();
        
        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                return buf;
            }
        } else if (c >= 32 && c < 127) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

void commandModeProcess(void) {
    char *query = editorPrompt(":%s");
    if (query == NULL) {
        E.mode = MODE_NORMAL;
        return;
    }
    
    if (strcmp(query, "q") == 0) {
        if (E.dirty && E.quit_times > 0) {
            editorSetStatusMessage("No write since last change (add ! to override)");
            E.quit_times--;
            free(query);
            E.mode = MODE_NORMAL;
            return;
        }
        if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {}
        if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {}
        exit(0);
    } else if (strcmp(query, "q!") == 0) {
        if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {}
        if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {}
        exit(0);
    } else if (strcmp(query, "w") == 0) {
        editorSave();
    } else if (strncmp(query, "w ", 2) == 0) {
        char *filename = query + 2;
        free(E.filename);
        E.filename = strdup(filename);
        editorSave();
    } else if (strcmp(query, "wq") == 0) {
        editorSave();
        if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {}
        if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {}
        exit(0);
    } else {
        editorSetStatusMessage("Not an editor command: %s", query);
    }
    
    free(query);
    E.mode = MODE_NORMAL;
}
