#include <stdlib.h>
#include "editor.h"
#include "input.h"
#include "core.h"

void insertModeProcessKey(int c) {
    switch (c) {
        case '\r':
            editorInsertNewline();
            break;
            
        case '\x1b':
            E.mode = MODE_NORMAL;
            editorSetStatusMessage("");
            if (E.cx > 0) E.cx--;
            break;
            
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
            
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case HOME_KEY:
        case END_KEY:
        case PAGE_UP:
        case PAGE_DOWN:
            editorMoveCursor(c);
            break;
            
        default:
            if (c >= 32 && c < 127) {
                editorInsertChar(c);
            } else if (c == '\t') {
                editorInsertChar(c);
            }
            break;
    }
}
