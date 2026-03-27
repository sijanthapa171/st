#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "editor.h"

void editorFreeUndoStep(UndoStep *step) {
    if (step->row) {
        for (int i = 0; i < step->numrows; i++) {
            free(step->row[i].chars);
            free(step->row[i].render);
        }
        free(step->row);
        step->row = NULL;
    }
    step->numrows = 0;
}

void editorSaveUndoState(void) {
    if (E.undo_stack_size == 50) {
        editorFreeUndoStep(&E.undo_stack[0]);
        memmove(&E.undo_stack[0], &E.undo_stack[1], sizeof(UndoStep) * 49);
        E.undo_stack_size--;
    }
    
    UndoStep *step = &E.undo_stack[E.undo_stack_size];
    
    if (E.numrows > 0) {
        step->row = malloc(sizeof(erow) * E.numrows);
        for (int i = 0; i < E.numrows; i++) {
            step->row[i].size = E.row[i].size;
            step->row[i].chars = malloc(E.row[i].size + 1);
            memcpy(step->row[i].chars, E.row[i].chars, E.row[i].size + 1);
            step->row[i].rsize = E.row[i].rsize;
            step->row[i].render = malloc(E.row[i].rsize + 1);
            memcpy(step->row[i].render, E.row[i].render, E.row[i].rsize + 1);
        }
    } else {
        step->row = NULL;
    }
    
    step->numrows = E.numrows;
    step->cx = E.cx;
    step->cy = E.cy;
    step->dirty = E.dirty;
    
    E.undo_stack_size++;
}

void editorUndo(void) {
    if (E.undo_stack_size == 0) {
        editorSetStatusMessage("Nothing to undo");
        return;
    }
    
    E.undo_stack_size--;
    UndoStep *step = &E.undo_stack[E.undo_stack_size];
    
    for (int i = 0; i < E.numrows; i++) {
        free(E.row[i].chars);
        free(E.row[i].render);
    }
    free(E.row);
    
    E.row = step->row;
    E.numrows = step->numrows;
    E.cx = step->cx;
    E.cy = step->cy;
    E.dirty = step->dirty;
    
    step->row = NULL;
    step->numrows = 0;
    
    editorSetStatusMessage("Undo performed (%d steps remaining)", E.undo_stack_size);
}
