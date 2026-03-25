#include "input.h"
#include "editor.h"
#include "commands.h"

void editorProcessKeypress(void) {
    int c = editorReadKey();
    
    if (E.mode == MODE_NORMAL) {
        normalModeProcessKey(c);
    } else if (E.mode == MODE_INSERT) {
        insertModeProcessKey(c);
    }
}
