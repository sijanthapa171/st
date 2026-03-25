#include <unistd.h>
#include <stdlib.h>
#include "editor.h"
#include "input.h"
#include "commands.h"
#include "core.h"

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    
    switch (key) {
        case ARROW_LEFT:
        case 'h':
            if (E.cx > 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
        case 'l':
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
        case 'k':
            if (E.cy > 0) E.cy--;
            break;
        case ARROW_DOWN:
        case 'j':
            if (E.cy < E.numrows) E.cy++;
            break;
    }
    
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

void normalModeProcessKey(int c) {
    switch (c) {
        case 'i':
            E.command_count = 0;
            E.mode = MODE_INSERT;
            editorSetStatusMessage("-- INSERT --");
            break;
            
        case ':':
            E.command_count = 0;
            E.mode = MODE_COMMAND;
            commandModeProcess();
            break;
            
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            E.command_count = E.command_count * 10 + (c - '0');
            break;
            
        case 'h':
        case 'j':
        case 'k':
        case 'l':
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            {
                int times = E.command_count ? E.command_count : 1;
                while (times--) {
                    editorMoveCursor(c);
                }
                E.command_count = 0;
            }
            break;
            
        case PAGE_UP:
        case PAGE_DOWN:
            {
                E.command_count = 0;
                int times = E.screenrows;
                while (times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
            
        case HOME_KEY:
        case '0':
            if (c == '0' && E.command_count > 0) {
                E.command_count = E.command_count * 10;
            } else {
                E.cx = 0;
                E.command_count = 0;
            }
            break;
            
        case END_KEY:
        case '$':
            E.command_count = 0;
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
            
        case 'd':
            E.command_count = 0;
            if (E.pending_key == 'd') {
                /* dd: delete current line */
                E.pending_key = 0;
                if (E.numrows == 0) break;
                editorDelRow(E.cy);
                if (E.cy >= E.numrows && E.cy > 0) E.cy--;
                E.cx = 0;
                editorSetStatusMessage("");
            } else {
                E.pending_key = 'd';
                editorSetStatusMessage("d");
            }
            break;

        case CTRL_KEY('q'):
            E.command_count = 0;
            E.pending_key = 0;
            if (E.dirty && E.quit_times > 0) {
                editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                                       "Press Ctrl-Q %d more times to quit.", E.quit_times);
                E.quit_times--;
                return;
            }
            if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {}
            if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {}
            exit(0);
            break;
            
        default:
            E.command_count = 0;
            E.pending_key = 0;
            break;
    }
}
