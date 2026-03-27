#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include "editor.h"

void editorUpdateRow(erow *row);
void editorInsertRow(int at, const char *s, size_t len);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorClearBuffer(void);
void editorSaveUndoState(void);
void editorUndo(void);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, const char *s, size_t len);
void editorRowDelChar(erow *row, int at);
void editorInsertChar(int c);
void editorInsertNewline(void);
void editorDelChar(void);

#endif
