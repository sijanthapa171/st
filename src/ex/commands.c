#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "editor.h"
#include "ex.h"

static void trim_inplace(char *s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) {
    s[n - 1] = '\0';
    n--;
  }
  size_t i = 0;
  while (s[i] && isspace((unsigned char)s[i])) i++;
  if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
}

static int ieq(const char *a, const char *b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

int editorExecuteExCommand(const char *cmd_in) {
  if (!cmd_in) return 0;
  if (cmd_in[0] == '\0') return 1;

  char cmd[64];
  snprintf(cmd, sizeof(cmd), "%s", cmd_in);
  trim_inplace(cmd);

  if (cmd[0] == '\0') return 1;

  if (ieq(cmd, "w") || ieq(cmd, "write")) {
    editorSave();
    return 1;
  }

  if (ieq(cmd, "q") || ieq(cmd, "quit")) {
    if (E.dirty) {
      editorSetStatusMessage("E37: No write since last change");
      return 1;
    }
    // Match Ctrl-Q behavior: clear and exit.
    if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) die("write");
    if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) die("write");
    exit(0);
    return 1;
  }

  if (ieq(cmd, "wq") || ieq(cmd, "qw") || ieq(cmd, "x")) {
    editorSave();
    if (!E.dirty) {
      if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) die("write");
      if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) die("write");
      exit(0);
    }
    return 1;
  }

  editorSetStatusMessage("Unknown command: %s", cmd);
  return 1;
}

