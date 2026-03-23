#ifndef EDITOR_H
#define EDITOR_H

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stddef.h>
#include <termios.h>
#include <time.h>

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

typedef enum editorMode {
  MODE_NORMAL = 0,
  MODE_INSERT = 1,
  MODE_VISUAL = 2
} editorMode;

typedef struct erow {
  int size;
  char *chars;
} erow;

typedef struct abuf {
  char *b;
  int len;
} abuf;

#define ABUF_INIT {NULL, 0}

struct editorConfig {
  int cx, cy;
  int rx;
  editorMode mode;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios orig_termios;

  /* Visual-mode selection (charwise). sel_* are inclusive endpoints. */
  int sel_sx, sel_sy;
  int sel_ex, sel_ey;
};

extern struct editorConfig E;

void die(const char *s);

void disableRawMode(void);
void enableRawMode(void);
int editorReadKey(void);
int getWindowSize(int *rows, int *cols);

void abAppend(abuf *ab, const char *s, int len);
void abFree(abuf *ab);

void editorSetStatusMessage(const char *fmt, ...);

void editorInsertRow(int at, const char *s, size_t len);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);
void editorInsertChar(int c);
void editorInsertNewline(void);
void editorDelChar(void);
char *editorRowsToString(int *buflen);

void editorOpen(char *filename);
void editorSave(void);

void editorScroll(void);
void editorRefreshScreen(void);
void editorMoveCursor(int key);
void editorProcessKeypress(void);

void initEditor(void);

/* Deletes the currently selected Visual range and moves cursor to start. */
void editorDelSelection(void);

#endif
