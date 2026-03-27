#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "editor.h"
#include "ui.h"

static const char *help_text[] = {
    "Vedit Help",
    "----------",
    "",
    "NORMAL MODE:",
    "  h, j, k, l / Arrows  : Move cursor (Left, Down, Up, Right)",
    "  i                    : Enter Insert Mode",
    "  :                    : Enter Command Mode",
    "  PageUp / PageDown    : Scroll Page",
    "  Home / End           : Go to beginning / end of line",
    "  Ctrl-Q               : Quit editor",
    "",
    "INSERT MODE:",
    "  Esc                  : Exit Insert Mode (Return to Normal Mode)",
    "  Backspace / Del      : Delete character",
    "  Enter                : Insert newline",
    "",
    "COMMAND MODE:",
    "  :w                   : Save file",
    "  :w <file>            : Save as <file>",
    "  :q                   : Quit (fails if unsaved changes exist)",
    "  :q!                  : Quit and discard changes",
    "  :wq                  : Save and quit",
    "  :help                : Open this help screen",
    "",
    "HELP MODE:",
    "  j, k / Up, Down      : Scroll Help text",
    "  q, Esc, Enter        : Close Help screen",
    NULL
};

void editorScroll(void) {
    E.rx = 0;
    if (E.cy < E.numrows) {
        int j;
        for (j = 0; j < E.cx; j++) {
            if (E.row[E.cy].chars[j] == '\t')
                E.rx += (KILO_TAB_STOP - 1) - (E.rx % KILO_TAB_STOP);
            E.rx++;
        }
    }
    
    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }

    int lines = E.numrows;
    E.ln_width = 2; 
    if (lines > 0) {
        int digits = 0;
        while (lines > 0) {
            digits++;
            lines /= 10;
        }
        E.ln_width = digits + 1; 
    }
}

void editorDrawRows(struct abuf *ab) {
    if (E.mode == MODE_HELP) {
        int y;
        int help_lines = 0;
        while (help_text[help_lines] != NULL) help_lines++;
        
        if (E.help_rowoff >= help_lines) E.help_rowoff = help_lines - 1;
        if (E.help_rowoff < 0) E.help_rowoff = 0;
        
        for (y = 0; y < E.screenrows; y++) {
            int filerow = y + E.help_rowoff;
            if (filerow >= help_lines) {
                abAppend(ab, "~", 1);
            } else {
                int len = strlen(help_text[filerow]);
                if (len > E.screencols) len = E.screencols;
                abAppend(ab, help_text[filerow], len);
            }
            abAppend(ab, "\x1b[K", 3);
            abAppend(ab, "\r\n", 2);
        }
        return;
    }

    int y;
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Vedit editor -- version %s", KILO_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
                for (int i = 1; i < E.ln_width; i++) abAppend(ab, " ", 1);
            }
        } else {
            char ln[16];
            int ln_len = snprintf(ln, sizeof(ln), "%*d ", E.ln_width - 1, filerow + 1);
            
            if (filerow == E.cy) {
                abAppend(ab, "\x1b[33m", 5); 
                abAppend(ab, ln, ln_len);
                abAppend(ab, "\x1b[m", 3);
            } else {
                abAppend(ab, "\x1b[90m", 5); 
                abAppend(ab, ln, ln_len);
                abAppend(ab, "\x1b[m", 3);
            }

            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols - E.ln_width) len = E.screencols - E.ln_width;
            
            if (E.mode == MODE_VISUAL) {
                int sy = E.sel_sy, ey = E.cy;
                int sx = E.sel_sx, ex = E.cx;
                if (sy > ey || (sy == ey && sx > ex)) {
                    int ty = sy; sy = ey; ey = ty;
                    int tx = sx; sx = ex; ex = tx;
                }

                for (int i = 0; i < len; i++) {
                    int cur_rx = E.coloff + i;
                    int is_sel = 0;
                    if (filerow > sy && filerow < ey) is_sel = 1;
                    else if (sy == ey && filerow == sy) {
                        if (filerow == sy) {
                            if (cur_rx >= sx && cur_rx <= ex) is_sel = 1;
                        }
                    } else if (filerow == sy) {
                        if (cur_rx >= sx) is_sel = 1;
                    } else if (filerow == ey) {
                        if (cur_rx <= ex) is_sel = 1;
                    }

                    if (is_sel) abAppend(ab, "\x1b[7m", 4);
                    abAppend(ab, &E.row[filerow].render[cur_rx], 1);
                    if (is_sel) abAppend(ab, "\x1b[m", 3);
                }
            } else {
                if (len > 0) abAppend(ab, &E.row[filerow].render[E.coloff], len);
            }
        }
        
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       E.filename ? E.filename : "[No Name]", E.numrows, E.dirty ? "(modified)" : "");
    
    const char *modestr = "NORMAL";
    if (E.mode == MODE_INSERT) modestr = "INSERT";
    else if (E.mode == MODE_COMMAND) modestr = "COMMAND";
    else if (E.mode == MODE_HELP) modestr = "HELP";
    
    int rlen = snprintf(rstatus, sizeof(rstatus), "[%s] %d/%d", modestr, E.cy + 1, E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5) {
        abAppend(ab, E.statusmsg, msglen);
    }
}

void editorRefreshScreen(void) {
    editorScroll();
    
    struct abuf ab = ABUF_INIT;
    
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);
    
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);
    
    char buf[32];
    if (E.mode == MODE_HELP) {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 1, 1);
    } else {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1 + E.ln_width);
    }
    abAppend(&ab, buf, strlen(buf));
    
    abAppend(&ab, "\x1b[?25h", 6);
    
    if (write(STDOUT_FILENO, ab.b, ab.len) == -1) {}
    abFree(&ab);
}
