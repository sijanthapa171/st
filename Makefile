CC = cc
CFLAGS = -D_GNU_SOURCE -std=c11 -Wall -Wextra -pedantic -O2
TARGET = notepad
SRC = src/app/main.c src/terminal/terminal.c \
      src/buffer/rows.c src/buffer/edit.c src/buffer/serialize.c src/buffer/selection.c \
      src/file/fileio.c src/screen/screen.c src/input/input.c src/ex/commands.c
INCLUDES = -Iinclude

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
