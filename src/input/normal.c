#include <unistd.h>
#include <stdlib.h>
#include "editor.h"
#include "input.h"
#include "commands.h"

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
            E.mode = MODE_INSERT;
            editorSetStatusMessage("-- INSERT --");
            break;
            
        case ':':
            E.mode = MODE_COMMAND;
            commandModeProcess();
            break;
            
        case 'h':
        case 'j':
        case 'k':
        case 'l':
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
            
        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while (times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
            
        case HOME_KEY:
        case '0':
            E.cx = 0;
            break;
            
        case END_KEY:
        case '$':
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
            
        case CTRL_KEY('q'):
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
            break;
    }
}
