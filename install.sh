#!/bin/bash

INSTALL_DIR="/usr/local/bin"
BINARY_NAME="vedit"
SOURCE_BINARY="bin/vedit"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

if [ ! -f Makefile ]; then
    echo "Error: Makefile not found. Please run this script from the project root."
    exit 1
fi

echo "Building $BINARY_NAME..."

if command -v nix &> /dev/null && [ -f flake.nix ]; then
    echo "Nix detected, using 'nix develop' to build..."
    nix develop --command make clean
    nix develop --command make
else
    if ! command -v make &> /dev/null; then
        echo "Error: 'make' not found. Please install build-essential or use Nix."
        exit 1
    fi
    make clean
    make
fi

if [ $? -ne 0 ] || [ ! -f "$SOURCE_BINARY" ]; then
    echo "Error: Build failed or binary not found."
    exit 1
fi

echo "Installing $BINARY_NAME to $INSTALL_DIR..."

if [ ! -d "$INSTALL_DIR" ]; then
    echo "Directory $INSTALL_DIR does not exist. Creating it..."
    if [ -w "$(dirname "$INSTALL_DIR")" ]; then
        mkdir -p "$INSTALL_DIR"
    else
        sudo mkdir -p "$INSTALL_DIR"
    fi
fi

if [ -w "$INSTALL_DIR" ]; then
    cp "$SOURCE_BINARY" "$INSTALL_DIR/$BINARY_NAME"
else
    echo "Permission denied. Attempting to install with sudo..."
    sudo cp "$SOURCE_BINARY" "$INSTALL_DIR/$BINARY_NAME"
fi

if [ $? -eq 0 ]; then
    if [ -w "$INSTALL_DIR/$BINARY_NAME" ]; then
        chmod +x "$INSTALL_DIR/$BINARY_NAME"
    else
        sudo chmod +x "$INSTALL_DIR/$BINARY_NAME"
    fi
    echo "Successfully installed $BINARY_NAME!"
    
    if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
        echo -e "\nWARNING: $INSTALL_DIR is not in your PATH."
        if [ "$INSTALL_DIR" == "/usr/local/bin" ]; then
             echo "On NixOS, /usr/local/bin is often excluded from the default PATH."
             echo "RECOMMENDED for NixOS: Run 'nix profile install .' instead."
             echo "Alternatively, add this to your shell config (e.g., ~/.zshrc):"
             echo "  export PATH=\$PATH:$INSTALL_DIR"
        else
             echo "Add this to your shell config (e.g., ~/.zshrc):"
             echo "  export PATH=\$PATH:$INSTALL_DIR"
        fi
    else
        echo "You can now run it by typing '$BINARY_NAME' or '$BINARY_NAME .'"
    fi
else
    echo "Error: Installation failed."
    exit 1
fi
