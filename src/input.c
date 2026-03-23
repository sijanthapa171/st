#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "editor.h"

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      } else if (E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (row && E.cx == row->size) {
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) E.cy--;
      break;
    case ARROW_DOWN:
      if (E.cy < E.numrows) E.cy++;
      break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress(void) {
  static int quit_times = 3;
  static int pending_d = 0; // Normal-mode: first 'd' waits for second 'd'

  int c = editorReadKey();

  if (E.mode == MODE_NORMAL && pending_d && c != 'd') {
    pending_d = 0;
  }

  switch (c) {
    case CTRL_KEY('q'):
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {
        die("write");
      }
      if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {
        die("write");
      }
      exit(0);
      break;
    case CTRL_KEY('s'):
      editorSave();
      break;
    default:
      break;
  }

  // ESC switches out of Insert/Visual to Normal.
  if (c == '\x1b') {
    if (E.mode == MODE_INSERT || E.mode == MODE_VISUAL) {
      E.mode = MODE_NORMAL;
      pending_d = 0;
    }
    quit_times = 3;
    return;
  }

  switch (E.mode) {
    case MODE_INSERT: {
      switch (c) {
        case CTRL_KEY('h'):
        case 127:
        case DEL_KEY:
          if (c == DEL_KEY) {
            editorMoveCursor(ARROW_RIGHT);
          }
          editorDelChar();
          break;
        case '\r':
          editorInsertNewline();
          break;
        case CTRL_KEY('l'):
          break;
        default:
          if (isprint(c)) {
            editorInsertChar(c);
          }
          break;
      }
      break;
    }

    case MODE_NORMAL: {
      switch (c) {
        case 'i':
          E.mode = MODE_INSERT;
          editorSetStatusMessage("-- INSERT --");
          break;
        case 'a':
          editorMoveCursor(ARROW_RIGHT);
          E.mode = MODE_INSERT;
          editorSetStatusMessage("-- INSERT --");
          break;
        case 'o':
          if (E.cy == E.numrows) {
            editorInsertRow(E.numrows, "", 0);
            E.cy = E.numrows - 1;
            E.cx = 0;
          } else {
            if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
            editorInsertNewline();
          }
          E.mode = MODE_INSERT;
          editorSetStatusMessage("-- INSERT --");
          break;

        case 'v':
          E.mode = MODE_VISUAL;
          E.sel_sx = E.cx;
          E.sel_sy = E.cy;
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          editorSetStatusMessage("-- VISUAL --");
          break;

        case 'h':
          editorMoveCursor(ARROW_LEFT);
          break;
        case 'j':
          editorMoveCursor(ARROW_DOWN);
          break;
        case 'k':
          editorMoveCursor(ARROW_UP);
          break;
        case 'l':
          editorMoveCursor(ARROW_RIGHT);
          break;

        case HOME_KEY:
          E.cx = 0;
          break;
        case END_KEY:
          if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
          break;

        case PAGE_UP:
        case PAGE_DOWN:
          if (c == PAGE_UP) {
            E.cy = E.rowoff;
          } else {
            E.cy = E.rowoff + E.screenrows - 1;
            if (E.cy > E.numrows) E.cy = E.numrows;
          }

          for (int times = E.screenrows; times > 0; times--) {
            editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
          }
          break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
          editorMoveCursor(c);
          break;

        case 'x':
          if (E.cy < E.numrows) {
            editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
          }
          break;

        case 'd':
          if (pending_d) {
            pending_d = 0;
            if (E.cy >= 0 && E.cy < E.numrows) {
              editorDelRow(E.cy);
              if (E.cy >= E.numrows) E.cy = E.numrows - 1;
              if (E.cy < 0) E.cy = 0;
              E.cx = 0;
            }
          } else {
            pending_d = 1;
            editorSetStatusMessage("Press d again to delete line");
          }
          break;

        case CTRL_KEY('l'):
          break;
        default:
          break;
      }
      break;
    }

    case MODE_VISUAL: {
      switch (c) {
        case 'd':
          pending_d = 0;
          editorDelSelection();
          E.mode = MODE_NORMAL;
          editorSetStatusMessage("-- NORMAL --");
          break;

        case 'h':
          editorMoveCursor(ARROW_LEFT);
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;
        case 'j':
          editorMoveCursor(ARROW_DOWN);
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;
        case 'k':
          editorMoveCursor(ARROW_UP);
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;
        case 'l':
          editorMoveCursor(ARROW_RIGHT);
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;

        case HOME_KEY:
          E.cx = 0;
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;
        case END_KEY:
          if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;

        case PAGE_UP:
        case PAGE_DOWN:
          if (c == PAGE_UP) {
            E.cy = E.rowoff;
          } else {
            E.cy = E.rowoff + E.screenrows - 1;
            if (E.cy > E.numrows) E.cy = E.numrows;
          }

          for (int times = E.screenrows; times > 0; times--) {
            editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
          }
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
          editorMoveCursor(c);
          E.sel_ex = E.cx;
          E.sel_ey = E.cy;
          break;

        case CTRL_KEY('l'):
          break;
        default:
          break;
      }
      break;
    }
  }

  quit_times = 3;
}
