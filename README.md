# Vedit - A Modular Vim-like C Terminal Text Editor [WIP]

Vedit is a lightweight, terminal-based text editor written entirely in C. It features a modular architecture, modal editing, and directory navigation, all powered by VT100 escape sequences without any external UI libraries.

## New Features

- **Interactive Directory Exploration**:
    - Open any directory with `vedit <path>` (e.g., `vedit .`).
    - Navigate folders and open files directly using **Enter**.
    - Supports parent directory (`..`) navigation.
- **Improved Buffer Management**:
    - **Multi-Step Undo**: Press `u` in Normal Mode to revert up to **50 previous changes**.
    - **Line Numbers**: Highlighting the current line for better focus.
    - **Modular Codebase**: Split into logical components (rows, editing, undo, explorer, etc.) for easier maintenance.
- **Enhanced Editing**:
    - `x`: Delete character under the cursor.
    - `dd`: Delete current line.
    - Full support for `Home`, `End`, `PageUp`, and `PageDown`.
- **Global Installation**:
    - **Nix Support**: Install globally using `nix profile install .`.
    - **Manual Option**: Use `bash install.sh` to install to `/usr/local/bin`.

## Modal Editing

- `NORMAL`: Navigation (`h, j, k, l`), undo (`u`), and character deletion (`x`).
- `INSERT`: Standard text entry (`i` to enter).
- `COMMAND`: File operations and help (`:` to enter).
- `HELP`: Interactive help screen (`:help`).

## Setup and Installation

### Recommended (NixOS/Nix)

```bash
nix profile install .
```

### Manual Installation

```bash
bash install.sh
```

### Local Development

1. Run `nix develop` for a complete dev shell.
2. Build with `make`.
3. Run with `./bin/vedit [filename_or_directory]`.

## Commands

- `:w` - Save changes.
- `:q` - Quit (fails if unsaved changes exist).
- `:wq` - Save and quit.
- `:q!` - Force quit.
- `:help` - Open interactive help.
