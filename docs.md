# vedit — Architecture & Developer Documentation

> A modular, terminal-based Vim-like text editor written in C.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Directory Structure](#2-directory-structure)
3. [Module Dependency Graph](#3-module-dependency-graph)
4. [Data Model](#4-data-model)
5. [Editor Modes & State Machine](#5-editor-modes--state-machine)
6. [Startup & Main Loop](#6-startup--main-loop)
7. [Input Pipeline](#7-input-pipeline)
8. [Normal Mode Key Handling](#8-normal-mode-key-handling)
9. [Insert Mode Key Handling](#9-insert-mode-key-handling)
10. [Command Mode Processing](#10-command-mode-processing)
11. [Buffer / Core Operations](#11-buffer--core-operations)
12. [UI Rendering Pipeline](#12-ui-rendering-pipeline)
13. [File I/O Flow](#13-file-io-flow)
14. [Build System](#14-build-system)
15. [Function Reference](#15-function-reference)

---

## 1. Project Overview

**vedit** is a lightweight, terminal-based text editor that mimics core Vim functionality. It is implemented in pure C (C99) and targets POSIX-compatible terminals via raw-mode terminal I/O and ANSI escape sequences.

Key characteristics:
- **Modal editing**: Normal, Insert, Command, and Help modes
- **Vim-like keybindings**: `h/j/k/l`, `dd`, `0/$`, `:w/:q/:wq`
- **Raw terminal I/O** via `termios`
- **Append-buffer rendering** for flicker-free redraws
- **Nix + Make build system** for reproducible builds

---

## 2. Directory Structure

```
vedit/
├── main.c                  # Entry point (delegates to src/)
├── Makefile                # Build rules
├── flake.nix               # Nix development environment
├── include/                # Public header files
│   ├── editor.h            # Global state & data types
│   ├── core.h              # Buffer manipulation API
│   ├── input.h             # Key reading & mode dispatch API
│   ├── ui.h                # Terminal & rendering API
│   ├── utils.h             # File I/O & utility API
│   └── commands.h          # Command mode API
├── src/
│   ├── main.c              # Actual main() implementation
│   ├── core/
│   │   ├── buffer.c        # Row-level buffer operations
│   │   └── state.c         # Editor state initialisation
│   ├── input/
│   │   ├── keyboard.c      # editorReadKey() — raw key reading
│   │   ├── modes.c         # editorProcessKeypress() — mode dispatch
│   │   ├── normal.c        # normalModeProcessKey() + editorMoveCursor()
│   │   ├── insert.c        # insertModeProcessKey()
│   │   └── help.c          # helpModeProcessKey()
│   ├── ui/
│   │   ├── render.c        # Screen drawing, scroll, status/message bars
│   │   └── terminal.c      # enableRawMode / disableRawMode / getWindowSize
│   ├── commands/
│   │   └── prompt.c        # editorPrompt() + commandModeProcess()
│   └── utils/
│       ├── file.c          # editorOpen() / editorSave() / editorRowsToString()
│       ├── utils.c         # abAppend() / abFree()
│       └── error.c         # die()
└── bin/
    └── vedit               # Compiled binary (after make)
```

---

## 3. Module Dependency Graph

```mermaid
graph TD
    main["main.c\n(entry point)"] --> editor_h["editor.h\n(global state)"]
    main --> ui_h["ui.h"]
    main --> input_h["input.h"]
    main --> utils_h["utils.h"]

    subgraph Input Layer
        keyboard["keyboard.c\neditorReadKey()"]
        modes["modes.c\neditorProcessKeypress()"]
        normal["normal.c\nnormalModeProcessKey()"]
        insert["insert.c\ninsertModeProcessKey()"]
        help["help.c\nhelpModeProcessKey()"]
    end

    subgraph Core Layer
        buffer["buffer.c\nRow buffer ops"]
        state["state.c\ninitEditor()"]
    end

    subgraph UI Layer
        render["render.c\neditorRefreshScreen()"]
        terminal["terminal.c\nenableRawMode()"]
    end

    subgraph Commands Layer
        prompt["prompt.c\ncommandModeProcess()"]
    end

    subgraph Utils Layer
        file["file.c\neditorOpen()/Save()"]
        utils["utils.c\nabAppend()/abFree()"]
        error["error.c\ndie()"]
    end

    modes --> keyboard
    modes --> normal
    modes --> insert
    modes --> help
    normal --> buffer
    normal --> prompt
    insert --> buffer
    prompt --> file
    render --> utils
    render --> editor_h
    buffer --> editor_h
    state --> editor_h
    terminal --> editor_h
```

---

## 4. Data Model

### Core structs and enums

```mermaid
classDiagram
    class EditorMode {
        <<enumeration>>
        MODE_NORMAL
        MODE_INSERT
        MODE_COMMAND
        MODE_HELP
    }

    class erow {
        +int size
        +int rsize
        +char* chars
        +char* render
    }

    class editorConfig {
        +int cx
        +int cy
        +int rx
        +int rowoff
        +int coloff
        +int help_rowoff
        +int screenrows
        +int screencols
        +int numrows
        +erow* row
        +int dirty
        +char* filename
        +char statusmsg[80]
        +time_t statusmsg_time
        +termios orig_termios
        +EditorMode mode
        +int quit_times
        +int command_count
        +int pending_key
    }

    class abuf {
        +char* b
        +int len
        +abAppend()
        +abFree()
    }

    editorConfig --> erow : "row[]"
    editorConfig --> EditorMode : mode
```

---

## 5. Editor Modes & State Machine

```mermaid
stateDiagram-v2
    [*] --> NORMAL : initEditor()

    NORMAL --> INSERT : Press 'i'
    NORMAL --> COMMAND : Press ':'
    NORMAL --> HELP : ':help' command
    NORMAL --> [*] : Ctrl-Q / ':q' / ':q!'

    INSERT --> NORMAL : Press Escape
    COMMAND --> NORMAL : Enter / Escape / command executed
    HELP --> NORMAL : Press 'q' / Escape / Enter

    NORMAL --> NORMAL : h j k l / arrows\n0 $ dd PageUp PageDown
```

### Mode Responsibilities

| Mode | File | Behaviour |
|---|---|---|
| `MODE_NORMAL` | `normal.c` | Navigation, `dd`, numeric prefix, trigger mode switches |
| `MODE_INSERT` | `insert.c` | Typing characters, Backspace, Enter, Escape to exit |
| `MODE_COMMAND` | `prompt.c` | Reads a `:cmd` string, dispatches `w`, `q`, `wq`, `q!`, `help` |
| `MODE_HELP` | `help.c` | Scrolls a built-in help text, any exit key returns to Normal |

---

## 6. Startup & Main Loop

```mermaid
flowchart TD
    A([Start: vedit argv]) --> B[enableRawMode]
    B --> C[initEditor\nset screenrows/cols/mode]
    C --> D{argc >= 2?}
    D -- Yes --> E[editorOpen\nload file into buffer]
    D -- No --> F[Empty buffer]
    E --> G[editorSetStatusMessage\nshow hint]
    F --> G
    G --> loop

    subgraph loop [Main Loop — forever]
        H[editorRefreshScreen] --> I[editorProcessKeypress]
        I --> H
    end
```

---

## 7. Input Pipeline

```mermaid
sequenceDiagram
    participant MainLoop
    participant modes as modes.c
    participant keyboard as keyboard.c
    participant normal as normal.c
    participant insert as insert.c
    participant help as help.c

    MainLoop->>modes: editorProcessKeypress()
    modes->>keyboard: editorReadKey()
    keyboard-->>modes: int keycode

    alt E.mode == MODE_NORMAL
        modes->>normal: normalModeProcessKey(c)
    else E.mode == MODE_INSERT
        modes->>insert: insertModeProcessKey(c)
    else E.mode == MODE_HELP
        modes->>help: helpModeProcessKey(c)
    end
```

### Key Encoding (`keyboard.c`)

```mermaid
flowchart LR
    raw[Read 1 byte from STDIN] --> esc{byte == ESC?}
    esc -- No --> ret_char[Return char as-is]
    esc -- Yes --> read2[Read 2 more bytes]
    read2 --> bracket{seq[0] == '[' ?}
    bracket -- Yes --> digit{seq[1] 0-9?}
    digit -- Yes --> read3[Read seq[2]]
    read3 --> tilde{seq[2] == '~'?}
    tilde -- Yes --> map_ext[Map to HOME/DEL/END\nPAGE_UP/PAGE_DOWN]
    digit -- No --> map_arrow[Map to ARROW_UP/DOWN\nLEFT/RIGHT HOME END]
    bracket -- No --> O{seq[0] == 'O'?}
    O -- Yes --> map_O[Map HOME/END]
    O -- No --> ret_esc[Return ESC]
```

---

## 8. Normal Mode Key Handling

```mermaid
flowchart TD
    key([Keypress in Normal Mode]) --> sw{Switch on key}

    sw --> i_key["'i'\n→ MODE_INSERT"]
    sw --> colon["':'\n→ MODE_COMMAND\n→ commandModeProcess()"]
    sw --> digit["'1'-'9'\n→ accumulate command_count"]
    sw --> motion["h/j/k/l / arrows\n→ editorMoveCursor()\n× command_count times"]
    sw --> page["PageUp/PageDown\n→ editorMoveCursor() × screenrows"]
    sw --> home["Home / '0'\n→ cx = 0"]
    sw --> end_key["End / '$'\n→ cx = row.size"]
    sw --> d_key["'d'\n→ pending_key logic"]
    sw --> ctrlq["Ctrl-Q\n→ check dirty → exit(0)"]
    sw --> default["default\n→ reset command_count\n   reset pending_key"]

    d_key --> pending{pending_key == 'd'?}
    pending -- Yes --> dd["dd: editorDelRow(cy)\nadjust cy if needed"]
    pending -- No --> set_pending["Set pending_key = 'd'\nShow 'd' in status bar"]
```

---

## 9. Insert Mode Key Handling

```mermaid
flowchart TD
    key([Keypress in Insert Mode]) --> sw{Switch on key}

    sw --> esc["Escape\n→ MODE_NORMAL"]
    sw --> enter["Enter / '\\r'\n→ editorInsertNewline()"]
    sw --> bs["Backspace / DEL / Ctrl-H\n→ editorDelChar()"]
    sw --> printable["Printable char (32–126)\n→ editorInsertChar(c)"]
    sw --> ctrlq_i["Ctrl-Q\n→ check dirty → exit(0)"]
```

---

## 10. Command Mode Processing

```mermaid
sequenceDiagram
    participant normal as normal.c
    participant prompt as prompt.c
    participant utils as utils/file.c

    normal->>prompt: commandModeProcess()
    prompt->>prompt: editorPrompt(":%s")\ncharacter-by-character loop

    loop Read chars until Enter/Escape
        prompt->>prompt: editorRefreshScreen on each char
        prompt->>prompt: handle Backspace, ESC, Enter, printable
    end

    prompt-->>normal: returns query string or NULL

    alt query == "q"
        prompt->>prompt: check dirty → exit or warn
    else query == "q!"
        prompt->>prompt: exit(0) unconditionally
    else query == "w"
        prompt->>utils: editorSave()
    else query == "w <file>"
        prompt->>utils: update filename + editorSave()
    else query == "wq"
        prompt->>utils: editorSave() then exit(0)
    else query == "help"
        prompt->>prompt: E.mode = MODE_HELP
    else unknown
        prompt->>prompt: setStatusMessage("Not an editor command")
    end

    prompt-->>normal: E.mode = MODE_NORMAL
```

---

## 11. Buffer / Core Operations

### Row-level call graph

```mermaid
graph LR
    subgraph High-Level
        editorInsertChar --> editorRowInsertChar
        editorInsertNewline --> editorInsertRow
        editorDelChar --> editorRowDelChar
        editorDelChar --> editorRowAppendString
        editorDelChar --> editorDelRow
    end

    subgraph Row-Level
        editorInsertRow --> editorUpdateRow
        editorRowInsertChar --> editorUpdateRow
        editorRowAppendString --> editorUpdateRow
        editorRowDelChar --> editorUpdateRow
        editorDelRow --> editorFreeRow
    end

    subgraph Memory
        editorUpdateRow["editorUpdateRow()\nExpands tabs → render buf"]
        editorFreeRow["editorFreeRow()\nfree(chars) + free(render)"]
    end
```

### Function Summary Table

| Function | File | Description |
|---|---|---|
| `editorUpdateRow(row)` | `buffer.c` | Rebuilds `row->render` expanding `\t` to spaces (tab stop = 8) |
| `editorInsertRow(at, s, len)` | `buffer.c` | Inserts a new row at position `at` in the global row array |
| `editorFreeRow(row)` | `buffer.c` | Frees `chars` and `render` of a row |
| `editorDelRow(at)` | `buffer.c` | Removes row at `at`, memmoves remaining rows up |
| `editorRowInsertChar(row, at, c)` | `buffer.c` | Inserts char at column `at` within a row |
| `editorRowAppendString(row, s, len)` | `buffer.c` | Appends a string to the end of a row (used when merging lines) |
| `editorRowDelChar(row, at)` | `buffer.c` | Deletes char at column `at` within a row |
| `editorInsertChar(c)` | `buffer.c` | High-level: insert char at cursor position |
| `editorInsertNewline()` | `buffer.c` | Splits current row at cursor or inserts blank row |
| `editorDelChar()` | `buffer.c` | High-level: delete char before cursor, merge lines if at col 0 |

---

## 12. UI Rendering Pipeline

```mermaid
flowchart TD
    refresh([editorRefreshScreen]) --> scroll[editorScroll\nrecalculate rowoff/coloff/rx]
    scroll --> hide[Hide cursor ESC 25l]
    hide --> home_cursor[Move cursor to home ESC H]
    home_cursor --> draw_rows[editorDrawRows\nWrite visible lines to abuf]
    draw_rows --> draw_status[editorDrawStatusBar\nFilename · line count · mode · position]
    draw_status --> draw_msg[editorDrawMessageBar\nStatusmsg if < 5 s old]
    draw_msg --> position[Reposition cursor ESC row;colH]
    position --> show[Show cursor ESC 25h]
    show --> flush["write(STDOUT, abuf)"]
    flush --> free_abuf[abFree]
```

### Scroll Calculation (`editorScroll`)

```mermaid
flowchart LR
    A[Compute rx\nfrom cx + tab stops] --> B{cy < rowoff?}
    B -- Yes --> C[rowoff = cy]
    B -- No --> D{cy >= rowoff + screenrows?}
    D -- Yes --> E[rowoff = cy - screenrows + 1]
    D -- No --> F{rx < coloff?}
    E --> F
    F -- Yes --> G[coloff = rx]
    F -- No --> H{rx >= coloff + screencols?}
    H -- Yes --> I[coloff = rx - screencols + 1]
    H -- No --> J([Done])
    G --> J
    I --> J
    C --> J
```

### Status Bar Layout

```
┌────────────────────────────────────────────────┐
│ filename.c - 42 lines (modified)  [INSERT] 10/42│
└────────────────────────────────────────────────┘
  ↑ left: filename + line count + dirty flag       ↑ right: [MODE] cur/total
```

---

## 13. File I/O Flow

### Open

```mermaid
sequenceDiagram
    participant main
    participant file as file.c
    participant buffer as buffer.c

    main->>file: editorOpen(filename)
    file->>file: fopen(filename, "r")
    loop For each line in file
        file->>file: getline(&line, &linecap, fp)
        file->>file: strip trailing \\r\\n
        file->>buffer: editorInsertRow(E.numrows, line, len)
    end
    file->>file: fclose(fp)
    file->>file: E.dirty = 0
```

### Save

```mermaid
sequenceDiagram
    participant prompt as prompt.c
    participant file as file.c
    participant buffer as buffer.c

    prompt->>file: editorSave()
    file->>buffer: editorRowsToString(&buflen)
    buffer-->>file: single string with \\n joiners
    file->>file: open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644)
    file->>file: write(fd, buf, buflen)
    file->>file: close(fd)
    file->>file: E.dirty = 0
    file->>file: editorSetStatusMessage("N bytes written")
```

---

## 14. Build System

```mermaid
flowchart TD
    all([make all]) --> target["bin/vedit"]
    target --> objs["build/**/*.o"]
    objs --> srcs["src/**/*.c\n(all source files)"]

    subgraph Compiler flags
        CF["-Wall -Wextra\n-pedantic -std=c99 -g"]
    end
    subgraph Include path
        INC["-Iinclude"]
    end

    srcs --> CC["gcc + CFLAGS + INC_FLAGS -c → .o"]
    CC --> Link["gcc .o files → bin/vedit"]

    debug([make debug]) --> debugflag["CFLAGS += -O0"]
    debugflag --> clean_first[make clean]
    clean_first --> all

    clean([make clean]) --> rm["rm -rf build/ bin/"]
```

### Makefile Source Discovery

```makefile
SRC_DIR = src src/core src/ui src/input src/commands src/utils
SRCS    = $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
OBJS    = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))
```

All `.c` files across every `SRC_DIR` subdirectory are automatically compiled; no manual source list needed.

---

## Key Bindings Summary

### Normal Mode

| Key | Action |
|---|---|
| `h / ←` | Move cursor left |
| `j / ↓` | Move cursor down |
| `k / ↑` | Move cursor up |
| `l / →` | Move cursor right |
| `0 / Home` | Beginning of line |
| `$ / End` | End of line |
| `PageUp` | Scroll up one screen |
| `PageDown` | Scroll down one screen |
| `[n]j` | Move `n` lines down (numeric prefix) |
| `dd` | Delete current line |
| `i` | Enter Insert mode |
| `:` | Enter Command mode |
| `Ctrl-Q` | Quit (with unsaved-changes check) |

### Insert Mode

| Key | Action |
|---|---|
| `Esc` | Return to Normal mode |
| `Enter` | Insert newline |
| `Backspace / Del` | Delete character before/at cursor |
| Any printable | Insert character at cursor |

### Command Mode (`:cmd`)

| Command | Action |
|---|---|
| `:w` | Save file |
| `:w <file>` | Save as named file |
| `:q` | Quit (blocked if unsaved) |
| `:q!` | Force quit |
| `:wq` | Save and quit |
| `:help` | Open Help screen |

### Help Mode

| Key | Action |
|---|---|
| `j / ↓` | Scroll help down |
| `k / ↑` | Scroll help up |
| `q / Esc / Enter` | Close help, return to Normal |
