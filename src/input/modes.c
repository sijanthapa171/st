#include "input.h"
#include "editor.h"
#include "commands.h"

void editorProcessKeypress(void) {
    int c = editorReadKey();
    
    if (E.mode == MODE_NORMAL || E.mode == MODE_EXPLORER) {
        normalModeProcessKey(c);
    } else if (E.mode == MODE_INSERT) {
        insertModeProcessKey(c);
    } else if (E.mode == MODE_HELP) {
        helpModeProcessKey(c);
    } else if (E.mode == MODE_VISUAL) {
        visualModeProcessKey(c);
    }
}
