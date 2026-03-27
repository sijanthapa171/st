#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "editor.h"

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }
    
    free(row->render);
    row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);
    
    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorInsertRow(int at, const char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    
    editorSaveUndoState();

    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);
    
    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorSaveUndoState();
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}

void editorClearBuffer(void) {
    for (int i = 0; i < E.numrows; i++) {
        editorFreeRow(&E.row[i]);
    }
    free(E.row);
    E.row = NULL;
    E.numrows = 0;
    E.dirty = 0;
}

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
            E.undo_row[i].render = malloc(E.row[i].rsize + 1);
            memcpy(E.undo_row[i].render, E.row[i].render, E.row[i].rsize + 1);
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
        editorSetStatusMessage("Nothing to undo");
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

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(erow *row, const char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline(void) {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar(void) {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    
    erow *row = &E.row[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}
