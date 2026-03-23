#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "editor.h"

void abAppend(abuf *ab, const char *s, int len) {
  char *newb = realloc(ab->b, ab->len + len);

  if (newb == NULL) return;
  memcpy(&newb[ab->len], s, (size_t)len);
  ab->b = newb;
  ab->len += len;
}

void abFree(abuf *ab) {
  free(ab->b);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

void editorScroll(void) {
  E.rx = E.cx;

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

static void editorDrawRows(abuf *ab) {
  int y;

  /* Visual selection endpoints are charwise, inclusive.
   * We normalize once per draw for performance. */
  int has_visual = (E.mode == MODE_VISUAL);
  int vs_sy = 0, vs_sx = 0, vs_ey = 0, vs_ex = 0;
  if (has_visual) {
    vs_sy = E.sel_sy;
    vs_sx = E.sel_sx;
    vs_ey = E.sel_ey;
    vs_ex = E.sel_ex;

    if (vs_sy > vs_ey || (vs_sy == vs_ey && vs_sx > vs_ex)) {
      int tmpy = vs_sy;
      int tmpx = vs_sx;
      vs_sy = vs_ey;
      vs_sx = vs_ex;
      vs_ey = tmpy;
      vs_ex = tmpx;
    }

    if (vs_sy < 0 || vs_sy >= E.numrows) {
      has_visual = 0;
    } else if (vs_ey >= E.numrows) {
      vs_ey = E.numrows - 1;
      vs_ex = E.row[vs_ey].size;
    }
  }

  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome), "Terminal Notepad -- Ctrl+S Save | Ctrl+Q Quit");
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].size - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      if (!has_visual) {
        abAppend(ab, &E.row[filerow].chars[E.coloff], len);
      } else {
        int in_sel = 0;
        for (int j = 0; j < len; j++) {
          int idx = E.coloff + j;
          int sel = 0;

          if (filerow >= vs_sy && filerow <= vs_ey) {
            if (vs_sy == vs_ey) {
              sel = (idx >= vs_sx && idx <= vs_ex);
            } else if (filerow == vs_sy) {
              sel = (idx >= vs_sx);
            } else if (filerow == vs_ey) {
              sel = (idx <= vs_ex);
            } else {
              sel = 1;
            }
          }

          if (sel && !in_sel) abAppend(ab, "\x1b[7m", 4);
          if (!sel && in_sel) abAppend(ab, "\x1b[m", 3);
          char ch = E.row[filerow].chars[idx];
          abAppend(ab, &ch, 1);
          in_sel = sel;
        }
        if (in_sel) abAppend(ab, "\x1b[m", 3);
      }
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

static void editorDrawStatusBar(abuf *ab) {
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
      E.filename ? E.filename : "[No Name]",
      E.numrows,
      E.dirty ? "(modified)" : "");
  const char *mode_str = (E.mode == MODE_NORMAL)
                            ? "NORMAL"
                            : (E.mode == MODE_INSERT ? "INSERT" : "VISUAL");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s Ln %d, Col %d", mode_str, E.cy + 1, E.cx + 1);
  if (len > E.screencols) len = E.screencols;
  abAppend(ab, status, len);

  int cur = len;
  if (rlen > 0 && cur < E.screencols) {
    if (rlen > E.screencols - cur) rlen = E.screencols - cur;
    abAppend(ab, rstatus, rlen);
    cur += rlen;
  }
  while (cur < E.screencols) {
    abAppend(ab, " ", 1);
    cur++;
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

static void editorDrawMessageBar(abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = (int)strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5) {
    abAppend(ab, E.statusmsg, msglen);
  }
}

void editorRefreshScreen(void) {
  editorScroll();

  abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, (int)strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  ssize_t out = write(STDOUT_FILENO, ab.b, (size_t)ab.len);
  if (out == -1) {
    die("write");
  }
  abFree(&ab);
}
