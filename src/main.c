#include <unistd.h>
#include "editor.h"
#include "ui.h"
#include "input.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    
    if (argc >= 2) {
        editorOpen(argv[1]);
    } else {
        editorSetStatusMessage("HELP: :w = save | :q = quit | i = insert mode | h,j,k,l = move");
    }
    
    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}
