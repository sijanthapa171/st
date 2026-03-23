#include <stdlib.h>
#include <string.h>

#include "editor.h"

static void editorRowDeleteRange(erow *row, int from, int to) {
  if (row == NULL) return;
  if (row->size <= 0) return;

  if (from < 0) from = 0;
  if (to < 0) return;
  if (from >= row->size) return;

  if (to >= row->size) to = row->size - 1;
  if (from > to) return;

  int len = to - from + 1;
  memmove(&row->chars[from], &row->chars[to + 1], (size_t)(row->size - to));
  row->size -= len;
  row->chars[row->size] = '\0';

  char *newchars = realloc(row->chars, (size_t)row->size + 1);
  if (newchars) row->chars = newchars;

  E.dirty++;
}

void editorDelSelection(void) {
  if (E.numrows <= 0) return;

  int sy = E.sel_sy;
  int sx = E.sel_sx;
  int ey = E.sel_ey;
  int ex = E.sel_ex;

  // Normalize: start <= end in (y, x) lexicographic order.
  if (sy > ey || (sy == ey && sx > ex)) {
    int tmpy = sy;
    int tmpx = sx;
    sy = ey;
    sx = ex;
    ey = tmpy;
    ex = tmpx;
  }

  if (sy < 0 || sy >= E.numrows) return;
  if (ey < 0) return;
  if (ey >= E.numrows) {
    ey = E.numrows - 1;
    ex = E.row[ey].size;
  }

  erow *start = &E.row[sy];
  erow *end = &E.row[ey];

  if (sx < 0) sx = 0;
  if (sx > start->size) sx = start->size;
  if (ex < 0) ex = 0;
  if (ex > end->size) ex = end->size;

  if (sy == ey) {
    editorRowDeleteRange(start, sx, ex);
    E.cy = sy;
    E.cx = sx;
    return;
  }

  int start_trunc = sx;
  int end_cut = ex;

  // Save suffix after the inclusive end character.
  int suffix_start = end_cut + 1;
  if (suffix_start > end->size) suffix_start = end->size;
  int suffix_len = end->size - suffix_start;

  char *suffix = NULL;
  if (suffix_len > 0) {
    suffix = malloc((size_t)suffix_len);
    if (suffix) memcpy(suffix, &end->chars[suffix_start], (size_t)suffix_len);
    else suffix_len = 0;
  }

  // Truncate start line to selection start.
  start->size = start_trunc;
  start->chars[start->size] = '\0';
  char *newchars = realloc(start->chars, (size_t)start->size + 1);
  if (newchars) start->chars = newchars;

  // Delete rows sy+1 .. ey (inclusive). Repeat deleting at sy+1.
  int del_count = ey - sy;
  for (int i = 0; i < del_count; i++) {
    editorDelRow(sy + 1);
  }

  // Append saved suffix back to start line.
  if (suffix_len > 0 && suffix) {
    editorRowAppendString(&E.row[sy], suffix, (size_t)suffix_len);
  }
  free(suffix);

  E.cy = sy;
  E.cx = start_trunc;
  if (E.cy < E.numrows && E.cx > E.row[E.cy].size) E.cx = E.row[E.cy].size;
}

