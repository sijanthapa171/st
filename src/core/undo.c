#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "editor.h"

void editorSaveUndoState(void) {
    if (E.undo_available) {
        for (int i = 0; i < E.undo_numrows; i++) {
            free(E.undo_row[i].chars);
            free(E.undo_row[i].render);
        }
        free(E.undo_row);
        E.undo_available = 0;
    }
    
    if (E.numrows > 0) {
        E.undo_row = malloc(sizeof(erow) * E.numrows);
        for (int i = 0; i < E.numrows; i++) {
            E.undo_row[i].size = E.row[i].size;
            E.undo_row[i].chars = malloc(E.row[i].size + 1);
            memcpy(E.undo_row[i].chars, E.row[i].chars, E.row[i].size + 1);
            E.undo_row[i].rsize = E.row[i].rsize;
            E.undo_row[i].render = malloc(E.undo_row[i].rsize + 1);
            memcpy(E.undo_row[i].render, E.row[i].render, E.undo_row[i].rsize + 1);
        }
    } else {
        E.undo_row = NULL;
    }
    
    E.undo_numrows = E.numrows;
    E.undo_cx = E.cx;
    E.undo_cy = E.cy;
    E.undo_dirty = E.dirty;
    E.undo_available = 1;
}

void editorUndo(void) {
    if (!E.undo_available) {
        return;
    }
    
    erow *tmp_row = E.row;
    int tmp_numrows = E.numrows;
    int tmp_cx = E.cx;
    int tmp_cy = E.cy;
    int tmp_dirty = E.dirty;
    
    E.row = E.undo_row;
    E.numrows = E.undo_numrows;
    E.cx = E.undo_cx;
    E.cy = E.undo_cy;
    E.dirty = E.undo_dirty;
    
    E.undo_row = tmp_row;
    E.undo_numrows = tmp_numrows;
    E.undo_cx = tmp_cx;
    E.undo_cy = tmp_cy;
    E.undo_dirty = tmp_dirty;
    
    editorSetStatusMessage("Undo performed");
}
