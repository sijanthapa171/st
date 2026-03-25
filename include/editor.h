#ifndef EDITOR_H
#define EDITOR_H

#include <termios.h>
#include <time.h>

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 8
#define KILO_QUIT_TIMES 1

enum EditorMode {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_HELP
};

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

struct editorConfig {
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int help_rowoff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct termios orig_termios;
    enum EditorMode mode;
    int quit_times;
    int command_count;
    int pending_key;
};

extern struct editorConfig E;

void initEditor(void);
void editorSetStatusMessage(const char *fmt, ...);

#endif
