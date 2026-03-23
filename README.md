# CLI base code editor like vim (its just make for learning purposes)

A minimal terminal text editor written in C.

## Features

- Open an existing file: `./st notes.txt`
- Move cursor with arrow keys, Home/End, Page Up/Down
- Delete with Backspace/Delete
- Save with `Ctrl+S` or `:w`
- Quit with `Ctrl+Q` or `:Q` (with unsaved-change warning)

## Build

```bash
make
```

## Run

```bash
./st
./st myfile.txt
```

## TODO:

